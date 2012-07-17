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
 *  grayquantlow.c
 *                     
 *      Thresholding from 8 bpp to 1 bpp
 *
 *          Floyd-Steinberg dithering to binary
 *              void       ditherToBinaryLow()
 *              void       ditherToBinaryLineLow()
 *
 *          Simple (pixelwise) binarization
 *              void       thresholdToBinaryLow()
 *              void       thresholdToBinaryLineLow()
 *
 *          A slower version of Floyd-Steinberg dithering that uses LUTs
 *              void       ditherToBinaryLUTLow()
 *              void       ditherToBinaryLineLUTLow()
 *              l_int32    make8To1DitherTables()
 *
 *      Thresholding from 8 bpp to 2 bpp
 *
 *          Floyd-Steinberg-like dithering to 2 bpp
 *              void       ditherTo2bppLow()
 *              void       ditherTo2bppLineLow()
 *              l_int32    make8To2DitherTables()
 *
 *          Simple thresholding to 2 bpp
 *              void       thresholdTo2bppLow()
 *
 *      Thresholding from 8 bpp to 4 bpp
 *
 *          Simple thresholding to 4 bpp
 *              void       thresholdTo4bppLow()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define DEBUG_UNROLLING 0
#endif   /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *             Simple binarization with fixed threshold             *
 *------------------------------------------------------------------*/
/*
 *  thresholdToBinaryLow()
 *
 *  If the source pixel is less than thresh,
 *  the dest will be 1; otherwise, it will be 0
 */
LEPTONICA_EXPORT void
thresholdToBinaryLow(l_uint32  *datad,
                     l_int32    w,
                     l_int32    h,
                     l_int32    wpld,
                     l_uint32  *datas,
                     l_int32    d,
                     l_int32    wpls,
                     l_int32    thresh)
{
l_int32    i;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        thresholdToBinaryLineLow(lined, w, lines, d, thresh);
    }
    return;
}


/*
 *  thresholdToBinaryLineLow()
 *
 */
LEPTONICA_EXPORT void
thresholdToBinaryLineLow(l_uint32  *lined,
                         l_int32    w,
                         l_uint32  *lines,
                         l_int32    d,
                         l_int32    thresh)
{
l_int32  j, k, gval, scount, dcount;
l_uint32 sword, dword;

    PROCNAME("thresholdToBinaryLineLow");

    switch (d)
    {
    case 4:
            /* Unrolled as 4 source words, 1 dest word */
        for (j = 0, scount = 0, dcount = 0; j + 31 < w; j += 32) {
            dword = 0;
            for (k = 0; k < 4; k++) {
                sword = lines[scount++];
                dword <<= 8;
                gval = (sword >> 28) & 0xf;
                    /* Trick used here and below: if gval < thresh then
                     * gval - thresh < 0, so its high-order bit is 1, and
                     * ((gval - thresh) >> 31) & 1 == 1; likewise, if
                     * gval >= thresh, then ((gval - thresh) >> 31) & 1 == 0
                     * Doing it this way avoids a random (and thus easily
                     * mispredicted) branch on each pixel. */
                dword |= ((gval - thresh) >> 24) & 128;
                gval = (sword >> 24) & 0xf;
                dword |= ((gval - thresh) >> 25) & 64;
                gval = (sword >> 20) & 0xf;
                dword |= ((gval - thresh) >> 26) & 32;
                gval = (sword >> 16) & 0xf;
                dword |= ((gval - thresh) >> 27) & 16;
                gval = (sword >> 12) & 0xf;
                dword |= ((gval - thresh) >> 28) & 8;
                gval = (sword >> 8) & 0xf;
                dword |= ((gval - thresh) >> 29) & 4;
                gval = (sword >> 4) & 0xf;
                dword |= ((gval - thresh) >> 30) & 2;
                gval = sword & 0xf;
                dword |= ((gval - thresh) >> 31) & 1;
            }
            lined[dcount++] = dword;
        }

        if (j < w) {
          dword = 0;
          for (; j < w; j++) {
              if ((j & 7) == 0) {
                  sword = lines[scount++];
              }
              gval = (sword >> 28) & 0xf;
              sword <<= 4;
              dword |= (((gval - thresh) >> 31) & 1) << (31 - (j & 31));
          }
          lined[dcount] = dword;
        }
#if DEBUG_UNROLLING
#define CHECK_BIT(a, b, c) if (GET_DATA_BIT(a, b) != c) { \
    fprintf(stderr, "Error: mismatch at %d/%d(%d), %d vs %d\n", \
            j, w, d, GET_DATA_BIT(a, b), c); }
        for (j = 0; j < w; j++) {
            gval = GET_DATA_QBIT(lines, j);
            CHECK_BIT(lined, j, gval < thresh ? 1 : 0);
        }
#endif
        break;
    case 8:
            /* Unrolled as 8 source words, 1 dest word */
        for (j = 0, scount = 0, dcount = 0; j + 31 < w; j += 32) {
            dword = 0;
            for (k = 0; k < 8; k++) {
                sword = lines[scount++];
                dword <<= 4;
                gval = (sword >> 24) & 0xff;
                dword |= ((gval - thresh) >> 28) & 8;
                gval = (sword >> 16) & 0xff;
                dword |= ((gval - thresh) >> 29) & 4;
                gval = (sword >> 8) & 0xff;
                dword |= ((gval - thresh) >> 30) & 2;
                gval = sword & 0xff;
                dword |= ((gval - thresh) >> 31) & 1;
            }
            lined[dcount++] = dword;
        }

        if (j < w) {
            dword = 0;
            for (; j < w; j++) {
                if ((j & 3) == 0) {
                    sword = lines[scount++];
                }
                gval = (sword >> 24) & 0xff;
                sword <<= 8;
                dword |= (((gval - thresh) >> 31) & 1) << (31 - (j & 31));
            }
            lined[dcount] = dword;
        }
#if DEBUG_UNROLLING
        for (j = 0; j < w; j++) {
            gval = GET_DATA_BYTE(lines, j);
            CHECK_BIT(lined, j, gval < thresh ? 1 : 0);
        }
#undef CHECK_BIT
#endif
        break;
    default:
        L_ERROR("src depth not 4 or 8 bpp", procName);
        break;
    }
    return;
}


/*------------------------------------------------------------------*
 *                   Simple thresholding to 2 bpp                   *
 *------------------------------------------------------------------*/
/*
 *  thresholdTo2bppLow()
 *
 *  Low-level function for thresholding from 8 bpp (datas) to
 *  2 bpp (datad), using thresholds implicitly defined through @tab,
 *  a 256-entry lookup table that gives a 2-bit output value
 *  for each possible input.
 *
 *  For each line, unroll the loop so that for each 32 bit src word,
 *  representing four consecutive 8-bit pixels, we compose one byte
 *  of output consisiting of four 2-bit pixels.
 */
LEPTONICA_EXPORT void
thresholdTo2bppLow(l_uint32  *datad,
                   l_int32    h,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_int32   *tab)
{
l_uint8    sval1, sval2, sval3, sval4, dval;
l_int32    i, j, k;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wpls; j++) {
            k = 4 * j;
            sval1 = GET_DATA_BYTE(lines, k);
            sval2 = GET_DATA_BYTE(lines, k + 1);
            sval3 = GET_DATA_BYTE(lines, k + 2);
            sval4 = GET_DATA_BYTE(lines, k + 3);
            dval = (tab[sval1] << 6) | (tab[sval2] << 4) |
                   (tab[sval3] << 2) | tab[sval4];
            SET_DATA_BYTE(lined, j, dval);
        }
    }
    return;
}


/*------------------------------------------------------------------*
 *                   Simple thresholding to 4 bpp                   *
 *------------------------------------------------------------------*/
/*
 *  thresholdTo4bppLow()
 *
 *  Low-level function for thresholding from 8 bpp (datas) to
 *  4 bpp (datad), using thresholds implicitly defined through @tab,
 *  a 256-entry lookup table that gives a 4-bit output value
 *  for each possible input.
 *  
 *  For each line, unroll the loop so that for each 32 bit src word,
 *  representing four consecutive 8-bit pixels, we compose two bytes
 *  of output consisiting of four 4-bit pixels.
 */
LEPTONICA_EXPORT void
thresholdTo4bppLow(l_uint32  *datad,
                   l_int32    h,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_int32   *tab)
{
l_uint8    sval1, sval2, sval3, sval4;
l_uint16   dval;
l_int32    i, j, k;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wpls; j++) {
            k = 4 * j;
            sval1 = GET_DATA_BYTE(lines, k);
            sval2 = GET_DATA_BYTE(lines, k + 1);
            sval3 = GET_DATA_BYTE(lines, k + 2);
            sval4 = GET_DATA_BYTE(lines, k + 3);
            dval = (tab[sval1] << 12) | (tab[sval2] << 8) |
                   (tab[sval3] << 4) | tab[sval4];
            SET_DATA_TWO_BYTES(lined, j, dval);
        }
    }
    return;
}


