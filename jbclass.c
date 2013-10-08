/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/


/*
 * jbclass.c
 *
 *     These are functions for unsupervised classification of
 *     collections of connected components -- either characters or
 *     words -- in binary images.  They can be used as image
 *     processing steps in jbig2 compression.
 *
 *     Initialization
 *
 *         JBCLASSER         *jbRankHausInit()      [rank hausdorff encoder]
 *         JBCLASSER         *jbCorrelationInit()   [correlation encoder]
 *         JBCLASSER         *jbCorrelationInitWithoutComponents()  [ditto]
 *         static JBCLASSER  *jbCorrelationInitInternal()
 *
 *     Classify the pages
 *
 *         l_int32     jbAddPages()
 *         l_int32     jbAddPage()
 *         l_int32     jbAddPageComponents()
 *
 *     Rank hausdorff classifier
 *
 *         l_int32     jbClassifyRankHaus()
 *         l_int32     pixHaustest()
 *         l_int32     pixRankHaustest()
 *
 *     Binary correlation classifier
 *
 *         l_int32     jbClassifyCorrelation()
 *
 *     Determine the image components we start with
 *
 *         l_int32     jbGetComponents()
 *         PIX        *pixWordMaskByDilation()
 *
 *     Build grayscale composites (templates)
 *
 *         PIXA       *jbAccumulateComposites
 *         PIXA       *jbTemplatesFromComposites
 *
 *     Utility functions for Classer
 *
 *         JBCLASSER  *jbClasserCreate()
 *         void        jbClasserDestroy()
 *
 *     Utility functions for Data
 *
 *         JBDATA     *jbDataSave()
 *         void        jbDataDestroy()
 *         l_int32     jbDataWrite()
 *         JBDATA     *jbDataRead()
 *         PIXA       *jbDataRender()
 *         l_int32     jbGetULCorners()
 *         l_int32     jbGetLLCorners()
 *
 *     Static helpers
 *
 *         static JBFINDCTX *findSimilarSizedTemplatesInit()
 *         static l_int32    findSimilarSizedTemplatesNext()
 *         static void       findSimilarSizedTemplatesDestroy()
 *         static l_int32    finalPositioningForAlignment()
 *
 *     Note: this is NOT an implementation of the JPEG jbig2
 *     proposed standard encoder, the specifications for which
 *     can be found at http://www.jpeg.org/jbigpt2.html.
 *     (See below for a full implementation.)
 *     It is an implementation of the lower-level part of an encoder that:
 *
 *        (1) identifies connected components that are going to be used
 *        (2) puts them in similarity classes (this is an unsupervised
 *            classifier), and
 *        (3) stores the result in a simple file format (2 files,
 *            one for templates and one for page/coordinate/template-index
 *            quartets).
 *
 *     An actual implementation of the official jbig2 encoder could
 *     start with parts (1) and (2), and would then compress the quartets
 *     according to the standards requirements (e.g., Huffman or
 *     arithmetic coding of coordinate differences and image templates).
 *
 *     The low-level part of the encoder provided here has the
 *     following useful features:
 *
 *         - It is accurate in the identification of templates
 *           and classes because it uses a windowed hausdorff
 *           distance metric.
 *         - It is accurate in the placement of the connected
 *           components, doing a two step process of first aligning
 *           the the centroids of the template with those of each instance,
 *           and then making a further correction of up to +- 1 pixel
 *           in each direction to best align the templates.
 *         - It is fast because it uses a morphologically based
 *           matching algorithm to implement the hausdorff criterion,
 *           and it selects the patterns that are possible matches
 *           based on their size.
 *
 *     We provide two different matching functions, one using Hausdorff
 *     distance and one using a simple image correlation.
 *     The Hausdorff method sometimes produces better results for the
 *     same number of classes, because it gives a relatively small
 *     effective weight to foreground pixels near the boundary,
 *     and a relatively  large weight to foreground pixels that are
 *     not near the boundary.  By effectively ignoring these boundary
 *     pixels, Hausdorff weighting corresponds better to the expected
 *     probabilities of the pixel values in a scanned image, where the
 *     variations in instances of the same printed character are much
 *     more likely to be in pixels near the boundary.  By contrast,
 *     the correlation method gives equal weight to all foreground pixels.
 *
 *     For best results, use the correlation method.  Correlation takes
 *     the number of fg pixels in the AND of instance and template,
 *     divided by the product of the number of fg pixels in instance
 *     and template.  It compares this with a threshold that, in
 *     general, depends on the fractional coverage of the template.
 *     For heavy text, the threshold is raised above that for light
 *     text,  By using both these parameters (basic threshold and
 *     adjustment factor for text weight), one has more flexibility
 *     and can arrive at the fewest substitution errors, although
 *     this comes at the price of more templates.
 *
 *     The strict Hausdorff scoring is not a rank weighting, because a
 *     single pixel beyond the given distance will cause a match
 *     failure.  A rank Hausdorff is more robust to non-boundary noise,
 *     but it is also more susceptible to confusing components that
 *     should be in different classes.  For implementing a jbig2
 *     application for visually lossless binary image compression,
 *     you have two choices:
 *
 *        (1) use a 3x3 structuring element (size = 3) and a strict
 *            Hausdorff comparison (rank = 1.0 in the rank Hausdorff
 *            function).  This will result in a minimal number of classes,
 *            but confusion of small characters, such as italic and
 *            non-italic lower-case 'o', can still occur.
 *        (2) use the correlation method with a threshold of 0.85
 *            and a weighting factor of about 0.7.  This will result in
 *            a larger number of classes, but should not be confused
 *            either by similar small characters or by extremely
 *            thick sans serif characters, such as in prog/cootoots.png.
 *
 *     As mentioned above, if visual substitution errors must be
 *     avoided, you should use the correlation method.
 *
 *     We provide executables that show how to do the encoding:
 *         prog/jbrankhaus.c
 *         prog/jbcorrelation.c
 *
 *     The basic flow for correlation classification goes as follows,
 *     where specific choices have been made for parameters (Hausdorff
 *     is the same except for initialization):
 *
 *             // Initialize and save data in the classer
 *         JBCLASSER *classer =
 *             jbCorrelationInit(JB_CONN_COMPS, 0, 0, 0.8, 0.7);
 *         SARRAY *safiles = getSortedPathnamesInDirectory(directory,
 *                                                         NULL, 0, 0);
 *         jbAddPages(classer, safiles);
 *
 *             // Save the data in a data structure for serialization,
 *             // and write it into two files.
 *         JBDATA *data = jbDataSave(classer);
 *         jbDataWrite(rootname, data);
 *
 *             // Reconstruct (render) the pages from the encoded data.
 *         PIXA *pixa = jbDataRender(data, FALSE);
 *
 *     Adam Langley has recently built a jbig2 standards-compliant
 *     encoder, the first one to appear in open source.  You can get
 *     this encoder at:
 *
 *          http://www.imperialviolet.org/jbig2.html
 *
 *     It uses arithmetic encoding throughout.  It encodes binary images
 *     losslessly with a single arithmetic coding over the full image.
 *     It also does both lossy and lossless encoding from connected
 *     components, using leptonica to generate the templates representing
 *     each cluster.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

    /* MSVC can't handle arrays dimensioned by static const integers */
#define  L_BUF_SIZE  512

    /* For jbClassifyRankHaus(): size of border added around
     * pix of each c.c., to allow further processing.  This
     * should be at least the sum of the MAX_DIFF_HEIGHT
     * (or MAX_DIFF_WIDTH) and one-half the size of the Sel  */
static const l_int32  JB_ADDED_PIXELS = 6;

    /* For pixHaustest(), pixRankHaustest() and pixCorrelationScore():
     * choose these to be 2 or greater */
static const l_int32  MAX_DIFF_WIDTH = 2;  /* use at least 2 */
static const l_int32  MAX_DIFF_HEIGHT = 2;  /* use at least 2 */

    /* In initialization, you have the option to discard components
     * (cc, characters or words) that have either width or height larger
     * than a given size.  This is convenient for jbDataSave(), because
     * the components are placed onto a regular lattice with cell
     * dimension equal to the maximum component size.  The default
     * values are given here.  If you want to save all components,
     * use a sufficiently large set of dimensions. */
static const l_int32  MAX_CONN_COMP_WIDTH = 350;  /* default max cc width */
static const l_int32  MAX_CHAR_COMP_WIDTH = 350;  /* default max char width */
static const l_int32  MAX_WORD_COMP_WIDTH = 1000;  /* default max word width */
static const l_int32  MAX_COMP_HEIGHT = 120;  /* default max component height */

    /* Max allowed dilation to merge characters into words */
#define  MAX_ALLOWED_DILATION  14

    /* This stores the state of a state machine which fetches
     * similar sized templates */
struct JbFindTemplatesState
{
    JBCLASSER       *classer;    /* classer                               */
    l_int32          w;          /* desired width                         */
    l_int32          h;          /* desired height                        */
    l_int32          i;          /* index into two_by_two step array      */
    NUMA            *numa;       /* current number array                  */
    l_int32          n;          /* current element of numa               */
};
typedef struct JbFindTemplatesState JBFINDCTX;


    /* Static initialization function */
static JBCLASSER * jbCorrelationInitInternal(l_int32 components,
                       l_int32 maxwidth, l_int32 maxheight, l_float32 thresh,
                       l_float32 weightfactor, l_int32 keep_components);

    /* Static helper functions */
static JBFINDCTX * findSimilarSizedTemplatesInit(JBCLASSER *classer, PIX *pixs);
static l_int32 findSimilarSizedTemplatesNext(JBFINDCTX *context);
static void findSimilarSizedTemplatesDestroy(JBFINDCTX **pcontext);
static l_int32 finalPositioningForAlignment(PIX *pixs, l_int32 x, l_int32 y,
                             l_int32 idelx, l_int32 idely, PIX *pixt,
                             l_int32 *sumtab, l_int32 *pdx, l_int32 *pdy);

#ifndef NO_CONSOLE_IO
#define  DEBUG_PLOT_CC             0
#define  DEBUG_CORRELATION_SCORE   0
#endif  /* ~NO_CONSOLE_IO */

static JBCLASSER *
jbCorrelationInitInternal(l_int32    components,
                          l_int32    maxwidth,
                          l_int32    maxheight,
                          l_float32  thresh,
                          l_float32  weightfactor,
                          l_int32    keep_components)
{
JBCLASSER  *classer;

    PROCNAME("jbCorrelationInitInternal");

    if (components != JB_CONN_COMPS)
        return (JBCLASSER *)ERROR_PTR("expected JB_CONN_COMPS", procName, NULL);
    if (thresh < 0.4 || thresh > 0.98)
        return (JBCLASSER *)ERROR_PTR("thresh not in range [0.4 - 0.98]",
                procName, NULL);
    if (weightfactor < 0.0 || weightfactor > 1.0)
        return (JBCLASSER *)ERROR_PTR("weightfactor not in range [0.0 - 1.0]",
                procName, NULL);
    if (maxwidth == 0) {
        maxwidth = MAX_CONN_COMP_WIDTH;
    }
    if (maxheight == 0)
        maxheight = MAX_COMP_HEIGHT;


    if ((classer = jbClasserCreate(JB_CORRELATION, components)) == NULL)
        return (JBCLASSER *)ERROR_PTR("classer not made", procName, NULL);
    classer->maxwidth = maxwidth;
    classer->maxheight = maxheight;
    classer->thresh = thresh;
    classer->weightfactor = weightfactor;
    classer->nahash = numaHashCreate(5507, 4);  /* 5507 is prime */
    classer->keep_pixaa = keep_components;
    return classer;
}

/*----------------------------------------------------------------------*
 *             Determine the image components we start with             *
 *----------------------------------------------------------------------*/
/*!
 *  jbGetComponents()
 *
 *      Input:  pixs (1 bpp)
 *              components (JB_CONN_COMPS)
 *              maxwidth, maxheight (of saved components; larger are discarded)
 *              &pboxa (<return> b.b. of component items)
 *              &ppixa (<return> component items)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
jbGetComponents(PIX     *pixs,
                l_int32  components,
                l_int32  maxwidth,
                l_int32  maxheight,
                BOXA   **pboxad,
                PIXA   **ppixad)
{
l_int32    empty;
BOXA      *boxa;
PIXA      *pixa;

    PROCNAME("jbGetComponents");

    if (!pboxad)
        return ERROR_INT("&boxad not defined", procName, 1);
    *pboxad = NULL;
    if (!ppixad)
        return ERROR_INT("&pixad not defined", procName, 1);
    *ppixad = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (components != JB_CONN_COMPS)
        return ERROR_INT("invalid components", procName, 1);

    pixZero(pixs, &empty);
    if (empty) {
        *pboxad = boxaCreate(0);
        *ppixad = pixaCreate(0);
        return 0;
    }

        /* If required, preprocess input pixs.  The method for both
         * characters and words is to generate a connected component
         * mask over the units that we want to aggregrate, which are,
         * in general, sets of related connected components in pixs.
         * For characters, we want to include the dots with
         * 'i', 'j' and '!', so we do a small vertical closing to
         * generate the mask.  For words, we make a mask over all
         * characters in each word.  This is a bit more tricky, because
         * the spacing between words is difficult to predict a priori,
         * and words can be typeset with variable spacing that can
         * in some cases be barely larger than the space between
         * characters.  The first step is to generate the mask and
         * identify each of its connected components.  */
    boxa = pixConnComp(pixs, &pixa, 8);

        /* Remove large components, and save the results.  */
    *ppixad = pixaSelectBySize(pixa, maxwidth, maxheight, L_SELECT_IF_BOTH,
                               L_SELECT_IF_LTE, NULL);
    *pboxad = boxaSelectBySize(boxa, maxwidth, maxheight, L_SELECT_IF_BOTH,
                               L_SELECT_IF_LTE, NULL);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);

    return 0;
}


/*----------------------------------------------------------------------*
 *                       jbig2 utility routines                         *
 *----------------------------------------------------------------------*/
/*!
 *  jbClasserCreate()
 *
 *      Input:  method (JB_CORRELATION) 
 *              components (JB_CONN_COMPS)
 *      Return: jbclasser, or null on error
 */
LEPTONICA_EXPORT JBCLASSER *
jbClasserCreate(l_int32  method,
                l_int32  components)
{
JBCLASSER  *classer;

    PROCNAME("jbClasserCreate");

    if ((classer = (JBCLASSER *)CALLOC(1, sizeof(JBCLASSER))) == NULL)
        return (JBCLASSER *)ERROR_PTR("classer not made", procName, NULL);
    if (method != JB_CORRELATION)
        return (JBCLASSER *)ERROR_PTR("invalid method", procName, NULL);
    if (components != JB_CONN_COMPS)
        return (JBCLASSER *)ERROR_PTR("invalid type", procName, NULL);

    classer->method = method;
    classer->components = components;
    classer->nacomps = numaCreate(0);
    classer->pixaa = pixaaCreate(0);
    classer->pixat = pixaCreate(0);
    classer->pixatd = pixaCreate(0);
    classer->nafgt = numaCreate(0);
    classer->naarea = numaCreate(0);
    classer->ptac = ptaCreate(0);
    classer->ptact = ptaCreate(0);
    classer->naclass = numaCreate(0);
    classer->napage = numaCreate(0);
    classer->ptaul = ptaCreate(0);

    return classer;
}


/*!
 *  jbGetULCorners()
 *
 *      Input:  jbclasser
 *              pixs (full res image)
 *              boxa (of c.c. bounding rectangles for this page)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes the ptaul field, which has the global UL corners,
 *          adjusted for each specific component, so that each component
 *          can be replaced by the template for its class and have the
 *          centroid in the template in the same position as the
 *          centroid of the original connected component.  It is important
 *          that this be done properly to avoid a wavy baseline in the
 *          result.
 *      (2) The array fields ptac and ptact give the centroids of
 *          those components relative to the UL corner of each component.
 *          Here, we compute the difference in each component, round to
 *          nearest integer, and correct the box->x and box->y by
 *          the appropriate integral difference.
 *      (3) The templates and stored instances are all bordered.
 */
LEPTONICA_EXPORT l_int32
jbGetULCorners(JBCLASSER  *classer,
               PIX        *pixs,
               BOXA       *boxa)
{
l_int32    i, baseindex, index, n, iclass, idelx, idely, x, y, dx, dy;
l_int32   *sumtab;
l_float32  x1, x2, y1, y2, delx, dely;
BOX       *box;
NUMA      *naclass;
PIX       *pixt;
PTA       *ptac, *ptact, *ptaul;

    PROCNAME("jbGetULCorners");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    ptaul = classer->ptaul;
    naclass = classer->naclass;
    ptac = classer->ptac;
    ptact = classer->ptact;
    baseindex = classer->baseindex;  /* num components before this page */
    sumtab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        index = baseindex + i;
        ptaGetPt(ptac, index, &x1, &y1);
        numaGetIValue(naclass, index, &iclass);
        ptaGetPt(ptact, iclass, &x2, &y2);
        delx = x2 - x1;
        dely = y2 - y1;
        if (delx >= 0)
            idelx = (l_int32)(delx + 0.5);
        else
            idelx = (l_int32)(delx - 0.5);
        if (dely >= 0)
            idely = (l_int32)(dely + 0.5);
        else
            idely = (l_int32)(dely - 0.5);
        if ((box = boxaGetBox(boxa, i, L_CLONE)) == NULL)
            return ERROR_INT("box not found", procName, 1);
        boxGetGeometry(box, &x, &y, NULL, NULL);

            /* Get final increments dx and dy for best alignment */
        pixt = pixaGetPix(classer->pixat, iclass, L_CLONE);
        finalPositioningForAlignment(pixs, x, y, idelx, idely,
                                     pixt, sumtab, &dx, &dy);
/*        if (i % 20 == 0)
            fprintf(stderr, "dx = %d, dy = %d\n", dx, dy); */
        ptaAddPt(ptaul, x - idelx + dx, y - idely + dy);
        boxDestroy(&box);
        pixDestroy(&pixt);
    }

    FREE(sumtab);
    return 0;
}


/*----------------------------------------------------------------------*
 *                              Static helpers                          *
 *----------------------------------------------------------------------*/
/* When looking for similar matches we check templates whose size is +/- 2 in
 * each direction. This involves 25 possible sizes. This array contains the
 * offsets for each of those positions in a spiral pattern. There are 25 pairs
 * of numbers in this array: even positions are x values. */
static int two_by_two_walk[50] = {
  0, 0,
  0, 1,
  -1, 0,
  0, -1,
  1, 0,
  -1, 1,
  1, 1,
  -1, -1,
  1, -1,
  0, -2,
  2, 0,
  0, 2,
  -2, 0,
  -1, -2,
  1, -2,
  2, -1,
  2, 1,
  1, 2,
  -1, 2,
  -2, 1,
  -2, -1,
  -2, -2,
  2, -2,
  2, 2,
  -2, 2};


/*!
 *  findSimilarSizedTemplatesInit()
 *
 *      Input:  classer
 *              pixs (instance to be matched)
 *      Return: Allocated context to be used with findSimilar*
 */
static JBFINDCTX *
findSimilarSizedTemplatesInit(JBCLASSER  *classer,
                              PIX        *pixs)
{
JBFINDCTX  *state;

    state = (JBFINDCTX *)CALLOC(1, sizeof(JBFINDCTX));
    state->w = pixGetWidth(pixs) - 2 * JB_ADDED_PIXELS;
    state->h = pixGetHeight(pixs) - 2 * JB_ADDED_PIXELS;
    state->classer = classer;

    return state;
}


static void
findSimilarSizedTemplatesDestroy(JBFINDCTX  **pstate)
{
JBFINDCTX  *state;

    PROCNAME("findSimilarSizedTemplatesDestroy");

    if (pstate == NULL) {
        L_WARNING("ptr address is null", procName);
        return;
    }
    if ((state = *pstate) == NULL)
        return;

    numaDestroy(&state->numa);
    FREE(state);
    *pstate = NULL;
    return;
}


/*!
 *  findSimilarSizedTemplatesNext()
 *
 *      Input:  state (from findSimilarSizedTemplatesInit)
 *      Return: Next template number, or -1 when finished
 *
 *  We have a hash table mapping template area to a list of template
 *  numbers with that area.  We wish to find similar sized templates,
 *  so we first look for templates with the same width and height, and
 *  then with width + 1, etc.  This walk is guided by the
 *  two_by_two_walk array, above.
 *
 *  We don't want to have to collect the whole list of templates first because
 *  (we hope) to find it quickly.  So we keep the context for this walk in an
 *  explictit state structure and this function acts like a generator.
 */
static l_int32
findSimilarSizedTemplatesNext(JBFINDCTX  *state)
{
l_int32  desiredh, desiredw, size, templ;
PIX     *pixt;

    while(1) {  /* Continue the walk over step 'i' */
        if (state->i >= 25) {  /* all done */
            return -1;
        }

        desiredw = state->w + two_by_two_walk[2 * state->i];
        desiredh = state->h + two_by_two_walk[2 * state->i + 1];
        if (desiredh < 1 || desiredw < 1) {  /* invalid size */
            state->i++;
            continue;
        }

        if (!state->numa) {
                /* We have yet to start walking the array for the step 'i' */
            state->numa = numaHashGetNuma(state->classer->nahash,
                                          desiredh * desiredw);
            if (!state->numa) {  /* nothing there */
                state->i++;
                continue;
            }

            state->n = 0;  /* OK, we got a numa. */
        }

            /* Continue working on this numa */
        size = numaGetCount(state->numa);
        for ( ; state->n < size; ) {
            templ = (l_int32)(state->numa->array[state->n++] + 0.5);
            pixt = pixaGetPix(state->classer->pixat, templ, L_CLONE);
            if (pixGetWidth(pixt) - 2 * JB_ADDED_PIXELS == desiredw &&
                pixGetHeight(pixt) - 2 * JB_ADDED_PIXELS == desiredh) {
                pixDestroy(&pixt);
                return templ;
            }
            pixDestroy(&pixt);
        }

            /* Exhausted the numa; take another step and try again */
        state->i++;
        numaDestroy(&state->numa);
        continue;
    }
}


/*!
 *  finalPositioningForAlignment()
 *
 *      Input:  pixs (input page image)
 *              x, y (location of UL corner of bb of component in pixs)
 *              idelx, idely (compensation to match centroids of component
 *                            and template)
 *              pixt (template, with JB_ADDED_PIXELS of padding on all sides)
 *              sumtab (for summing fg pixels in an image)
 *              &dx, &dy (return delta on position for best match; each
 *                        one is in the set {-1, 0, 1})
 *      Return: 0 if OK, 1 on error
 *
 */
static l_int32
finalPositioningForAlignment(PIX      *pixs,
                             l_int32   x,
                             l_int32   y,
                             l_int32   idelx,
                             l_int32   idely,
                             PIX      *pixt,
                             l_int32  *sumtab,
                             l_int32  *pdx,
                             l_int32  *pdy)
{
l_int32  w, h, i, j, minx, miny, count, mincount;
PIX     *pixi;  /* clipped from source pixs */
PIX     *pixr;  /* temporary storage */
BOX     *box;

    PROCNAME("finalPositioningForAlignment");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixt)
        return ERROR_INT("pixt not defined", procName, 1);
    if (!pdx || !pdy)
        return ERROR_INT("&dx and &dy not both defined", procName, 1);
    if (!sumtab)
        return ERROR_INT("sumtab not defined", procName, 1);
    *pdx = *pdy = 0;

        /* Use JB_ADDED_PIXELS pixels padding on each side */
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
    box = boxCreate(x - idelx - JB_ADDED_PIXELS,
                    y - idely - JB_ADDED_PIXELS, w, h);
    pixi = pixClipRectangle(pixs, box, NULL);
    boxDestroy(&box);
    if (!pixi)
        return ERROR_INT("pixi not made", procName, 1);

    pixr = pixCreate(pixGetWidth(pixi), pixGetHeight(pixi), 1);
    mincount = 0x7fffffff;
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            pixCopy(pixr, pixi);
            pixRasterop(pixr, j, i, w, h, PIX_SRC ^ PIX_DST, pixt, 0, 0);
            pixCountPixels(pixr, &count, sumtab);
            if (count < mincount) {
                minx = j;
                miny = i;
                mincount = count;
            }
        }
    }
    pixDestroy(&pixi);
    pixDestroy(&pixr);

    *pdx = minx;
    *pdy = miny;
    return 0;
}
