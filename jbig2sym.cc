// Copyright 2006 Google Inc. All Rights Reserved.
// Author: agl@imperialviolet.org (Adam Langley)
//
// Copyright (C) 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "jbmap.h"

#include "jbig2arith.h"

#ifdef _MSC_VER
#define restrict __restrict
#else
#define restrict __restrict__
#endif

#include <stdio.h>

#include <allheaders.h>
#include <pix.h>

#include <math.h>

#define S(i) symbols->pix[i]


// -----------------------------------------------------------------------------
// iota isn't part of the STL standard, and it can be a pain to include even on
// gcc based systems. Thus we define it here and save the issues
// -----------------------------------------------------------------------------
template <class _ForwardIterator, class _Tp>
void
myiota(_ForwardIterator __first, _ForwardIterator __last, _Tp __val) {
  while (__first != __last) *__first++ = __val++;
}

// -----------------------------------------------------------------------------
// Sorts a vector of indexes into the symbols PIXA by height. This is needed
// because symbols are placed into the JBIG2 table in height order
// -----------------------------------------------------------------------------

class HeightSorter {  // concept: stl/StrictWeakOrdering
 public:
  HeightSorter(const PIXA *isymbols)
      : symbols(isymbols) {}

  bool operator() (int x, int y) {
    return S(x)->h < S(y)->h;
  }

 private:
  const PIXA *const symbols;
};

// -----------------------------------------------------------------------------
// Sorts a vector of indexes into the symbols PIXA by width. This is needed
// because symbols are placed into the JBIG2 table in width order (for a given
// height class)
// -----------------------------------------------------------------------------
class WidthSorter {  // concept: stl/StrictWeakOrdering
 public:
  WidthSorter(const PIXA *isymbols)
      : symbols(isymbols) {}

  bool operator() (int x, int y) {
    return S(x)->w < S(y)->w;
  }

 private:
  const PIXA *const symbols;
};

static const int kBorderSize = 6;

template <typename T, typename IsLess>
static void mergesort_low(T *a, size_t n, T *b, IsLess is_less, bool f) {
  if (n == 1) {  // n == 0 is not supported.
    if (f) *b = *a;
    return;
  }
  size_t an = n >> 1;
  size_t bn = n - an;
  f = !f;
  mergesort_low(a, an, b, is_less, f);
  mergesort_low(a + an, bn, b + an, is_less, f);
  T *c;  
  if (f) {
    c = a;
    a = b;
    b += an;
  } else {
    c = b;
    b = a + an;
  }
  while (an > 0 && bn > 0) {  // Now merge a[:an] and b[:bn] to c[:an+bn].
    if (is_less(*b, *a)) {
      *c++ = *b++;
      --bn;
    } else {
      *c++ = *a++;
      --an;
    }
  }
  for (; an > 0; --an) {
    *c++ = *a++;
  }
  for (; bn > 0; --bn) {
    *c++ = *b++;
  }
}

// Using a stable sort to get deterministic output for lists with equal values. 
template <typename T, typename IsLess>
static void jbstablesort(std::vector<T> *v, IsLess is_less) {
  const size_t n = v->size();
  if (n < 2) return;
  v->resize(n << 1);  // Allocate temporary space.
  mergesort_low(v->data(), n, v->data() + n, is_less, false);
  v->resize(n);
}

// see comment in .h file
void
jbig2enc_symboltable(struct jbig2enc_ctx *restrict ctx,
                     PIXA *restrict const symbols,
                     jbvector<unsigned> *__restrict__ symbol_list,
                     jbmap<int, int> *symmap, const bool unborder_symbols) {
  const unsigned n = symbol_list->size();
  int number = 0;

#ifdef JBIG2_DEBUGGING
  fprintf(stderr, "  symbols: %d\n", n);
#endif

  // this is a vector of indexes into symbols
  jbvector<unsigned> syms(*symbol_list);
  // now sort that vector by height
  jbstablesort(&syms, HeightSorter(symbols));  // on unsigned

  // this is used for each height class to sort into increasing width
  WidthSorter sorter(symbols);

  // this stores the indexes of the symbols for a given height class
  jbvector<int> hc;
  // this keeps the value of the height of the current class
  unsigned hcheight = 0;
  for (unsigned i = 0; i < n;) {
    // height is the height of this class of symbols
    const unsigned height = S(syms[i])->h - (unborder_symbols ? 2*kBorderSize : 0);
#ifdef JBIG2_DEBUGGING
    fprintf(stderr, "height is %d\n", height);
#endif
    unsigned j;
    hc.clear();
    hc.push_back(syms[i]);  // this is the first member of the new class
    // walk the vector until we find a symbol with a different height
    for (j = i + 1; j < n; ++j) {
      if (S(syms[j])->h - (unborder_symbols ? 2*kBorderSize : 0) != height) break;
      hc.push_back(syms[j]);  // add each symbol of the same height to the class
    }
#ifdef JBIG2_DEBUGGING
    fprintf(stderr, "  hc (height: %d, members: %d)\n", height, hc.size());
#endif
    // all the symbols from i to j-1 are a height class
    // now sort them into increasing width
    jbstablesort(&hc, sorter);  // on int
    // encode the delta height
    const int deltaheight = height - hcheight;
    jbig2enc_int(ctx, JBIG2_IADH, deltaheight);
    hcheight = height;
    int symwidth = 0;
    // encode each symbol
    for (jbvector<int>::const_iterator k = hc.begin(); k != hc.end(); ++k) {
      const int sym = *k;
      const int thissymwidth = S(sym)->w - (unborder_symbols ? 2*kBorderSize : 0);
      const int deltawidth = thissymwidth - symwidth;
#ifdef JBIG2_DEBUGGING
      fprintf(stderr, "    h: %d\n", S(sym)->w);
#endif
      symwidth += deltawidth;
      //fprintf(stderr, "width is %d\n", S(sym)->w);
      jbig2enc_int(ctx, JBIG2_IADW, deltawidth);

      PIX *unbordered;
      if (unborder_symbols) {
        // the exemplars are stored with a border
        unbordered = pixRemoveBorder(S(sym), kBorderSize);
        // encoding the bitmap requires that the pad bits be zero
      } else {
        unbordered = pixClone(S(sym));
      }
      pixSetPadBits(unbordered, 0);
      jbig2enc_bitimage(ctx, (uint8_t *) unbordered->data, thissymwidth, height,
                        false);
      // add this symbol to the map
      (*symmap)[sym] = number++;
      pixDestroy(&unbordered);
    }
    // OOB marks the end of the height class
    //fprintf(stderr, "OOB\n");
    jbig2enc_oob(ctx, JBIG2_IADW);
    i = j;
  }

  // now we have the list of exported symbols (which is all of them)
  // it's run length encoded and we have a run length of 0 (for all the symbols
  // which aren't set) followed by a run length of the number of symbols

  jbig2enc_int(ctx, JBIG2_IAEX, 0);
  jbig2enc_int(ctx, JBIG2_IAEX, n);

  jbig2enc_final(ctx);
}

// sort by the bottom-left corner of the box
class YSorter {  // concept: stl/StrictWeakOrdering
 public:
  YSorter(const PTA *ill)
    : ll(ill) {}

  bool operator() (int x, int y) {
    return ll->y[x] < ll->y[y];
  }

 private:
  const PTA *const ll;
};

// sort by the bottom-left corner of the box
class XSorter {  // concept: stl/StrictWeakOrdering
 public:
  XSorter(const PTA *ill)
    : ll(ill) {}

  bool operator() (int x, int y) {
    return ll->x[x] < ll->x[y];
  }

 private:
  const PTA *const ll;
};

#if (__GNUC__ <= 2) || defined(sun)
#define lrint(x) static_cast<int>(x)
#endif

#define BY(x) (lrint(ll->y[x]))

// see comment in .h file
void
jbig2enc_textregion(struct jbig2enc_ctx *restrict ctx,
                    const jbmap<int, int> &symmap,
                    const jbmap<int, int> &symmap2,
                    const jbvector<int> &comps,
                    PTA *const in_ll,
                    PIXA *const symbols,
                    NUMA *assignments, int stripwidth, int symbits,
                    PIXA *const source, BOXA *boxes, int baseindex,
                    int refine_level, bool unborder_symbols) {
  // these are the only valid values for stripwidth
  if (stripwidth != 1 && stripwidth != 2 && stripwidth != 4 &&
      stripwidth != 8) {
    abort();
  }

  PTA *ll;

  // In the case of refinement, we have to put the symbols where the original
  // boxes were. So we make up an array of lower-left (ll) points from the
  // boxes. Otherwise we take the points from the in_ll array we were given.
  // However, the in_ll array is absolutely indexed and the boxes array is
  // relative to this page so watch out below.
  if (source) {
    ll = ptaCreate(0);
    for (int i = 0; i < boxes->n; ++i) {
      ptaAddPt(ll, boxes->box[i]->x,
               boxes->box[i]->y + boxes->box[i]->h - 1);
    }
  } else {
    // if we aren't doing refinement - we just put the symbols where they
    // matched best
    ll = in_ll;
  }

  const int n = comps.size();

  // sort each box by distance from the top of the page
  // syms (a copy of comps) is a list of indexes into symmap and ll
  // elements which are indexes into symmap and ll are labeled I
  // indexes into the syms array are labeled II
  jbvector<int> syms(n);
  if (source) {
    // refining: fill syms with the numbers 0..n because ll is relative to this
    // page in this case
    myiota(syms.begin(), syms.end(), 0);
  } else {
    // fill syms with the component numbers from the comps array because ll is
    // absolutly indexed in this case (absolute: over the whole multi-page
    // document)
    syms = comps;
  }
  // sort into height order
  jbstablesort(&syms, YSorter(ll));  // on int

  XSorter sorter(ll);

  int stript = 0;
  int firsts = 0;
  int wibble = 0;
  // this is the initial stript value. I don't see why encoding this as zero,
  // then encoding the first stript value as the real start is any worst than
  // encoding this value correctly and then having a 0 value for the first
  // deltat
  jbig2enc_int(ctx, JBIG2_IADT, 0);

  // for each symbol we group it into a strip, which is stripwidth px high
  // for each strip we sort into left-right order
  jbvector<int> strip; // elements of strip: I
  for (int i = 0; i < n;) {   // i: II
    const int height = (BY(syms[i]) / stripwidth) * stripwidth;
    int j;
    strip.clear();
    strip.push_back(syms[i]);

    // now walk until we hit the first symbol which isn't in this strip
    for (j = i + 1; j < n; ++j) {  // j: II
      if (BY(syms[j]) < height) abort();
      if (BY(syms[j]) >= height + stripwidth) {
        // outside strip
        break;
      }
      strip.push_back(syms[j]);
    }

    // now sort the strip into left-right order
    jbstablesort(&strip, sorter);  // int
    const int deltat = height - stript;
#ifdef SYM_DEBUGGING
    fprintf(stderr, "deltat is %d\n", deltat);
#endif
    jbig2enc_int(ctx, JBIG2_IADT, deltat / stripwidth);
    stript = height;
#ifdef SYM_DEBUGGING
    fprintf(stderr, "t now: %d\n", stript);
#endif

    bool firstsymbol = true;
    int curs = 0;
    // k: iterator(I)
    for (jbvector<int>::const_iterator k = strip.begin(); k != strip.end(); ++k) {
      const int sym = *k;  // sym: I
      if (firstsymbol) {
        firstsymbol = false;
        const int deltafs = lrint(ll->x[sym]) - firsts;
        jbig2enc_int(ctx, JBIG2_IAFS, deltafs);
        firsts += deltafs;
        curs = firsts;
      } else {
        const int deltas = lrint(ll->x[sym]) - curs;
        jbig2enc_int(ctx, JBIG2_IADS, deltas);
        curs += deltas;
      }

      // if stripwidth is 1, all the t values must be the same so they aren't
      // even encoded
      if (stripwidth > 1) {
        const int deltat = BY(sym) - stript;
        jbig2enc_int(ctx, JBIG2_IAIT, deltat);
      }

      // The assignments array is absolutely indexed, but in the case that we
      // are doing refinement (source != NULL) then the symbol number is
      // relative to this page, so we have to add the baseindex to get an
      // absolute index.
      const int assigned = (int)assignments->array
        [sym + (source ? baseindex : 0)];

      // the symmap maps the number of the symbol from the classifier to the
      // order in while it was written in the symbol dict

      // We have two symbol dictionaries. A global one and a per-page one.
      int symid;
      jbmap<int, int>::const_iterator symit = symmap.find(assigned);
      if (symit != symmap.end()) {
        symid = symit->second;
      } else {
        symit = symmap2.find(assigned);
        if (symit != symmap2.end()) {
          symid = symit->second + symmap.size();
        } else {
          for (symit = symmap.begin(); symit != symmap.end(); ++symit) {
            fprintf(stderr, "%d ", symit->first);
          }
          for (symit = symmap2.begin(); symit != symmap2.end(); ++symit) {
            fprintf(stderr, "%d ", symit->first);
          }
          fprintf(stderr, "\n%d\n", assigned);
          abort();
        }
      }
#ifdef SYM_DEBUGGING
      fprintf(stderr, "sym: %d\n", symid);
#endif
      jbig2enc_iaid(ctx, symbits, symid);

      // refinement is enabled if the original source components are given
      if (source) {
        // the boxes array is indexed by the number of the symbol on this page.
        // So we subtract the number of the first symbol to get this relative
        // number.
        const int abssym = baseindex + sym;

        PIX *symbol;
        if (unborder_symbols) {
          // the symbol has a 6 px border around it, which we need to remove
          symbol = pixRemoveBorder(S(assigned), kBorderSize);
        } else {
          symbol = pixClone(S(assigned));
        }
        pixSetPadBits(symbol, 0);

        const int targetw = boxes->box[sym]->w;
        const int targeth = boxes->box[sym]->h;
        const int targetx = boxes->box[sym]->x;
        const int targety = boxes->box[sym]->y;

        const int symboly = (int) (in_ll->y[abssym] - symbol->h) + 1;
        const int symbolx = (int) in_ll->x[abssym];

        const int deltaw = targetw - symbol->w;
        const int deltah = targeth - symbol->h;
        const int deltax = targetx - symbolx;
        const int deltay = targety - symboly;

        pixSetPadBits(source->pix[sym], 0);
        // now see how well the symbol matches
        PIX *targetcopy = pixCopy(NULL, source->pix[sym]);
        pixRasterop(targetcopy, deltax, deltay, symbol->w, symbol->h,
                    PIX_SRC ^ PIX_DST,
                    symbol, 0, 0);
        int deltacount;
        pixCountPixels(targetcopy, &deltacount, NULL);
#ifdef SYMBOL_COMPRESSION_DEBUGGING
        fprintf(stderr, "delta count: %d\n", deltacount);
#endif
        pixDestroy(&targetcopy);

#ifdef SYMBOL_COMPRESSION_DEBUGGING
          fprintf(stderr, "refinement: dw:%d dh:%d dx:%d dy:%d w:%d h:%d\n",
                  deltaw, deltah, deltax, deltay, targetw, targeth);
          fprintf(stderr, "  box: %d %d symbol: %d %d h:%d ll:%f %f\n",
                  targetx, targety, symbolx, symboly, symbol->h,
                  in_ll->x[abssym], in_ll->y[abssym]);
#endif

        // Note that the refinement encoding function can only cope with x
        // offsets in [-1, 0, 1] so refinement is disabled if the offset is
        // outside this range. This should be *very* rare.
        if (deltacount <= refine_level || deltax < -1 || deltax > 1) {
        //if (deltaw > 1 || deltaw < -1 || deltax || deltah || deltay) {
          // refinement disabled.
          jbig2enc_int(ctx, JBIG2_IARI, 0);
          // update curs given the width of the bitmap
          curs += (S(assigned)->w - (unborder_symbols ? 2*kBorderSize : 0)) - 1;
        } else {
          wibble++;
          jbig2enc_int(ctx, JBIG2_IARI, 1);

          jbig2enc_int(ctx, JBIG2_IARDW, deltaw);
          jbig2enc_int(ctx, JBIG2_IARDH, deltah);
          jbig2enc_int(ctx, JBIG2_IARDX, deltax - (deltaw >> 1));
          jbig2enc_int(ctx, JBIG2_IARDY, deltay - (deltah >> 1));

          jbig2enc_refine
            (ctx, (uint8_t *) symbol->data, symbol->w, symbol->h,
             (uint8_t *) source->pix[sym]->data, targetw, targeth,
             deltax, -deltay);

          pixDestroy(&symbol);
          curs += targetw - 1;
        }
      } else {
        // update curs given the width of the bitmap
        curs += (S(assigned)->w - (unborder_symbols ? 2*kBorderSize : 0)) - 1;
      }
    }
    // terminate the strip
    jbig2enc_oob(ctx, JBIG2_IADS);
    i = j;
  }

  jbig2enc_final(ctx);
  if (ll != in_ll) ptaDestroy(&ll);
}
