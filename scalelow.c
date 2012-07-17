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
 *  scalelow.c
 *
 *         Color (interpolated) scaling: general case
 *                  void       scaleColorLILow()
 *
 *         Grayscale (interpolated) scaling: general case
 *                  void       scaleGrayLILow()
 *
 *         Color (interpolated) scaling: 2x upscaling
 *                  void       scaleColor2xLILow()
 *                  void       scaleColor2xLILineLow()
 *
 *         Grayscale (interpolated) scaling: 2x upscaling
 *                  void       scaleGray2xLILow()
 *                  void       scaleGray2xLILineLow()
 *
 *         Grayscale (interpolated) scaling: 4x upscaling
 *                  void       scaleGray4xLILow()
 *                  void       scaleGray4xLILineLow()
 *
 *         Grayscale and color scaling by closest pixel sampling
 *                  l_int32    scaleBySamplingLow()
 *
 *         Color and grayscale downsampling with (antialias) lowpass filter
 *                  l_int32    scaleSmoothLow()
 *                  void       scaleRGBToGray2Low()
 *
 *         Color and grayscale downsampling with (antialias) area mapping
 *                  l_int32    scaleColorAreaMapLow()
 *                  l_int32    scaleGrayAreaMapLow()
 *                  l_int32    scaleAreaMapLow2()
 *
 *         Binary scaling by closest pixel sampling
 *                  l_int32    scaleBinaryLow()
 *
 *         Scale-to-gray 2x
 *                  void       scaleToGray2Low()
 *                  l_uint32  *makeSumTabSG2()
 *                  l_uint8   *makeValTabSG2()
 *
 *         Scale-to-gray 3x
 *                  void       scaleToGray3Low()
 *                  l_uint32  *makeSumTabSG3()
 *                  l_uint8   *makeValTabSG3()
 *
 *         Scale-to-gray 4x
 *                  void       scaleToGray4Low()
 *                  l_uint32  *makeSumTabSG4()
 *                  l_uint8   *makeValTabSG4()
 *
 *         Scale-to-gray 6x
 *                  void       scaleToGray6Low()
 *                  l_uint8   *makeValTabSG6()
 *
 *         Scale-to-gray 8x
 *                  void       scaleToGray8Low()
 *                  l_uint8   *makeValTabSG8()
 *
 *         Scale-to-gray 16x
 *                  void       scaleToGray16Low()
 *
 *         Grayscale mipmap
 *                  l_int32    scaleMipmapLow()
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define  DEBUG_OVERFLOW   0
#define  DEBUG_UNROLLING  0
#endif  /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *                2x linear interpolated color scaling              *
 *------------------------------------------------------------------*/

/*!
 *  scaleColor2xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
LEPTONICA_EXPORT void
scaleColor2xLILineLow(l_uint32  *lined,
                      l_int32    wpld,
                      l_uint32  *lines,
                      l_int32    ws,
                      l_int32    wpls,
                      l_int32    lastlineflag)
{
l_int32    j, jd, wsm;
l_int32    rval1, rval2, rval3, rval4, gval1, gval2, gval3, gval4;
l_int32    bval1, bval2, bval3, bval4;
l_uint32   pixels1, pixels2, pixels3, pixels4, pixel;
l_uint32  *linesp, *linedp;

    wsm = ws - 1;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp = lined + wpld;
        pixels1 = *lines;
        pixels3 = *linesp;

            /* initialize with v(2) and v(4) */
        rval2 = pixels1 >> 24;
        gval2 = (pixels1 >> 16) & 0xff;
        bval2 = (pixels1 >> 8) & 0xff;
        rval4 = pixels3 >> 24;
        gval4 = (pixels3 >> 16) & 0xff;
        bval4 = (pixels3 >> 8) & 0xff;

        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
                /* shift in previous src values */
            rval1 = rval2;
            gval1 = gval2;
            bval1 = bval2;
            rval3 = rval4;
            gval3 = gval4;
            bval3 = bval4;
                /* get new src values */
            pixels2 = *(lines + j + 1);
            pixels4 = *(linesp + j + 1);
            rval2 = pixels2 >> 24;
            gval2 = (pixels2 >> 16) & 0xff;
            bval2 = (pixels2 >> 8) & 0xff;
            rval4 = pixels4 >> 24;
            gval4 = (pixels4 >> 16) & 0xff;
            bval4 = (pixels4 >> 8) & 0xff;
                /* save dest values */
            pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
            *(lined + jd) = pixel;                               /* pix 1 */
            pixel = ((((rval1 + rval2) << 23) & 0xff000000) |
                     (((gval1 + gval2) << 15) & 0x00ff0000) |
                     (((bval1 + bval2) << 7) & 0x0000ff00));
            *(lined + jd + 1) = pixel;                           /* pix 2 */
            pixel = ((((rval1 + rval3) << 23) & 0xff000000) |
                     (((gval1 + gval3) << 15) & 0x00ff0000) |
                     (((bval1 + bval3) << 7) & 0x0000ff00));
            *(linedp + jd) = pixel;                              /* pix 3 */
            pixel = ((((rval1 + rval2 + rval3 + rval4) << 22) & 0xff000000) | 
                     (((gval1 + gval2 + gval3 + gval4) << 14) & 0x00ff0000) |
                     (((bval1 + bval2 + bval3 + bval4) << 6) & 0x0000ff00));
            *(linedp + jd + 1) = pixel;                          /* pix 4 */
        }  
            /* last src pixel on line */
        rval1 = rval2;
        gval1 = gval2;
        bval1 = bval2;
        rval3 = rval4;
        gval3 = gval4;
        bval3 = bval4;
        pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
        *(lined + 2 * wsm) = pixel;                        /* pix 1 */
        *(lined + 2 * wsm + 1) = pixel;                    /* pix 2 */
        pixel = ((((rval1 + rval3) << 23) & 0xff000000) |
                 (((gval1 + gval3) << 15) & 0x00ff0000) |
                 (((bval1 + bval3) << 7) & 0x0000ff00));
        *(linedp + 2 * wsm) = pixel;                       /* pix 3 */
        *(linedp + 2 * wsm + 1) = pixel;                   /* pix 4 */
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp = lined + wpld;
        pixels2 = *lines;
        rval2 = pixels2 >> 24;
        gval2 = (pixels2 >> 16) & 0xff;
        bval2 = (pixels2 >> 8) & 0xff;
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            rval1 = rval2;
            gval1 = gval2;
            bval1 = bval2;
            pixels2 = *(lines + j + 1);
            rval2 = pixels2 >> 24;
            gval2 = (pixels2 >> 16) & 0xff;
            bval2 = (pixels2 >> 8) & 0xff;
            pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
            *(lined + jd) = pixel;                            /* pix 1 */
            *(linedp + jd) = pixel;                           /* pix 2 */
            pixel = ((((rval1 + rval2) << 23) & 0xff000000) |
                     (((gval1 + gval2) << 15) & 0x00ff0000) |
                     (((bval1 + bval2) << 7) & 0x0000ff00));
            *(lined + jd + 1) = pixel;                        /* pix 3 */
            *(linedp + jd + 1) = pixel;                       /* pix 4 */
        }  
        rval1 = rval2;
        gval1 = gval2;
        bval1 = bval2;
        pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
        *(lined + 2 * wsm) = pixel;                           /* pix 1 */
        *(lined + 2 * wsm + 1) = pixel;                       /* pix 2 */
        *(linedp + 2 * wsm) = pixel;                          /* pix 3 */
        *(linedp + 2 * wsm + 1) = pixel;                      /* pix 4 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *                2x linear interpolated gray scaling               *
 *------------------------------------------------------------------*/

/*!
 *  scaleGray2xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 */
LEPTONICA_EXPORT void
scaleGray2xLILineLow(l_uint32  *lined,
                     l_int32    wpld,
                     l_uint32  *lines,
                     l_int32    ws,
                     l_int32    wpls,
                     l_int32    lastlineflag)
{
l_int32    j, jd, wsm, w;
l_int32    sval1, sval2, sval3, sval4;
l_uint32  *linesp, *linedp;
l_uint32   words, wordsp, wordd, worddp;

    wsm = ws - 1;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp = lined + wpld;

            /* Unroll the loop 4x and work on full words */
        words = lines[0];
        wordsp = linesp[0];
        sval2 = (words >> 24) & 0xff;
        sval4 = (wordsp >> 24) & 0xff;
        for (j = 0, jd = 0, w = 0; j + 3 < wsm; j += 4, jd += 8, w++) {
                /* At the top of the loop,
                 * words == lines[w], wordsp == linesp[w]
                 * and the top bytes of those have been loaded into
                 * sval2 and sval4. */
            sval1 = sval2;
            sval2 = (words >> 16) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 16) & 0xff;
            wordd = (sval1 << 24) | (((sval1 + sval2) >> 1) << 16);
            worddp = (((sval1 + sval3) >> 1) << 24) |
                (((sval1 + sval2 + sval3 + sval4) >> 2) << 16);

            sval1 = sval2;
            sval2 = (words >> 8) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 8) & 0xff;
            wordd |= (sval1 << 8) | ((sval1 + sval2) >> 1);
            worddp |= (((sval1 + sval3) >> 1) << 8) |
                ((sval1 + sval2 + sval3 + sval4) >> 2);
            lined[w * 2] = wordd;
            linedp[w * 2] = worddp;

            sval1 = sval2;
            sval2 = words & 0xff;
            sval3 = sval4;
            sval4 = wordsp & 0xff;
            wordd = (sval1 << 24) |                              /* pix 1 */
                (((sval1 + sval2) >> 1) << 16);                  /* pix 2 */
            worddp = (((sval1 + sval3) >> 1) << 24) |            /* pix 3 */
                (((sval1 + sval2 + sval3 + sval4) >> 2) << 16);  /* pix 4 */

                /* Load the next word as we need its first byte */
            words = lines[w + 1];
            wordsp = linesp[w + 1];
            sval1 = sval2;
            sval2 = (words >> 24) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 24) & 0xff;
            wordd |= (sval1 << 8) |                              /* pix 1 */
                ((sval1 + sval2) >> 1);                          /* pix 2 */
            worddp |= (((sval1 + sval3) >> 1) << 8) |            /* pix 3 */
                ((sval1 + sval2 + sval3 + sval4) >> 2);          /* pix 4 */
            lined[w * 2 + 1] = wordd;
            linedp[w * 2 + 1] = worddp;
        }

            /* Finish up the last word */
        for (; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval3 = sval4;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            sval4 = GET_DATA_BYTE(linesp, j + 1);
            SET_DATA_BYTE(lined, jd, sval1);                     /* pix 1 */
            SET_DATA_BYTE(lined, jd + 1, (sval1 + sval2) / 2);   /* pix 2 */
            SET_DATA_BYTE(linedp, jd, (sval1 + sval3) / 2);      /* pix 3 */
            SET_DATA_BYTE(linedp, jd + 1,
                          (sval1 + sval2 + sval3 + sval4) / 4);  /* pix 4 */
        }
        sval1 = sval2;
        sval3 = sval4;
        SET_DATA_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        SET_DATA_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        SET_DATA_BYTE(linedp, 2 * wsm, (sval1 + sval3) / 2);      /* pix 3 */
        SET_DATA_BYTE(linedp, 2 * wsm + 1, (sval1 + sval3) / 2);  /* pix 4 */

#if DEBUG_UNROLLING
#define CHECK_BYTE(a, b, c) if (GET_DATA_BYTE(a, b) != c) {\
     fprintf(stderr, "Error: mismatch at %d, %d vs %d\n", \
             j, GET_DATA_BYTE(a, b), c); }

        sval2 = GET_DATA_BYTE(lines, 0);
        sval4 = GET_DATA_BYTE(linesp, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval3 = sval4;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            sval4 = GET_DATA_BYTE(linesp, j + 1);
            CHECK_BYTE(lined, jd, sval1);                     /* pix 1 */
            CHECK_BYTE(lined, jd + 1, (sval1 + sval2) / 2);   /* pix 2 */
            CHECK_BYTE(linedp, jd, (sval1 + sval3) / 2);      /* pix 3 */
            CHECK_BYTE(linedp, jd + 1,
                          (sval1 + sval2 + sval3 + sval4) / 4);  /* pix 4 */
        }
        sval1 = sval2;
        sval3 = sval4;
        CHECK_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        CHECK_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        CHECK_BYTE(linedp, 2 * wsm, (sval1 + sval3) / 2);      /* pix 3 */
        CHECK_BYTE(linedp, 2 * wsm + 1, (sval1 + sval3) / 2);  /* pix 4 */
#undef CHECK_BYTE
#endif
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp = lined + wpld;
        sval2 = GET_DATA_BYTE(lines, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            SET_DATA_BYTE(lined, jd, sval1);                       /* pix 1 */
            SET_DATA_BYTE(linedp, jd, sval1);                      /* pix 3 */
            SET_DATA_BYTE(lined, jd + 1, (sval1 + sval2) / 2);     /* pix 2 */
            SET_DATA_BYTE(linedp, jd + 1, (sval1 + sval2) / 2);    /* pix 4 */
        }  
        sval1 = sval2;
        SET_DATA_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        SET_DATA_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        SET_DATA_BYTE(linedp, 2 * wsm, sval1);                    /* pix 3 */
        SET_DATA_BYTE(linedp, 2 * wsm + 1, sval1);                /* pix 4 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *               4x linear interpolated gray scaling                *
 *------------------------------------------------------------------*/
/*!
 *  scaleGray4xLILow()
 *
 *  This is a special case of 4x expansion by linear
 *  interpolation.  Each src pixel contains 16 dest pixels.
 *  The 16 dest pixels in src pixel 1 are numbered at
 *  their UL corners.  The 16 dest pixels in src pixel 1
 *  are related to that src pixel and its 3 neighboring
 *  src pixels as follows:
 *
 *             1---2---3---4---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             5---6---7---8---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *  src 1 -->  9---a---b---c---|---|---|---|---|  <-- src 2
 *             |   |   |   |   |   |   |   |   |
 *             d---e---f---g---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             |===|===|===|===|===|===|===|===|
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *  src 3 -->  |---|---|---|---|---|---|---|---|  <-- src 4
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *
 *           dest      src
 *           ----      ---
 *           dp1    =  sp1
 *           dp2    =  (3 * sp1 + sp2) / 4
 *           dp3    =  (sp1 + sp2) / 2
 *           dp4    =  (sp1 + 3 * sp2) / 4
 *           dp5    =  (3 * sp1 + sp3) / 4
 *           dp6    =  (9 * sp1 + 3 * sp2 + 3 * sp3 + sp4) / 16 
 *           dp7    =  (3 * sp1 + 3 * sp2 + sp3 + sp4) / 8
 *           dp8    =  (3 * sp1 + 9 * sp2 + 1 * sp3 + 3 * sp4) / 16 
 *           dp9    =  (sp1 + sp3) / 2
 *           dp10   =  (3 * sp1 + sp2 + 3 * sp3 + sp4) / 8
 *           dp11   =  (sp1 + sp2 + sp3 + sp4) / 4 
 *           dp12   =  (sp1 + 3 * sp2 + sp3 + 3 * sp4) / 8
 *           dp13   =  (sp1 + 3 * sp3) / 4
 *           dp14   =  (3 * sp1 + sp2 + 9 * sp3 + 3 * sp4) / 16 
 *           dp15   =  (sp1 + sp2 + 3 * sp3 + 3 * sp4) / 8
 *           dp16   =  (sp1 + 3 * sp2 + 3 * sp3 + 9 * sp4) / 16 
 *
 *  We iterate over the src pixels, and unroll the calculation
 *  for each set of 16 dest pixels corresponding to that src
 *  pixel, caching pixels for the next src pixel whenever possible.
 */
LEPTONICA_EXPORT void
scaleGray4xLILow(l_uint32  *datad,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    ws,
                 l_int32    hs,
                 l_int32    wpls)
{
l_int32    i, hsm;
l_uint32  *lines, *lined;

    hsm = hs - 1;

        /* We're taking 2 src and 4 dest lines at a time,
         * and for each src line, we're computing 4 dest lines.
         * Call these 4 dest lines:  destline1 - destline4.
         * The first src line is used for destline 1.
         * Two src lines are used for all other dest lines,
         * except for the last 4 dest lines, which are computed
         * using only the last src line. */

        /* iterate over all but the last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 4 * i * wpld;
        scaleGray4xLILineLow(lined, wpld, lines, ws, wpls, 0);
    }
    
        /* last src line */
    lines = datas + hsm * wpls;
    lined = datad + 4 * hsm * wpld;
    scaleGray4xLILineLow(lined, wpld, lines, ws, wpls, 1);

    return;
}


/*!
 *  scaleGray4xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 */
LEPTONICA_EXPORT void
scaleGray4xLILineLow(l_uint32  *lined,
                     l_int32    wpld,
                     l_uint32  *lines,
                     l_int32    ws,
                     l_int32    wpls,
                     l_int32    lastlineflag)
{
l_int32    j, jd, wsm, wsm4;
l_int32    s1, s2, s3, s4, s1t, s2t, s3t, s4t;
l_uint32  *linesp, *linedp1, *linedp2, *linedp3;

    wsm = ws - 1;
    wsm4 = 4 * wsm;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp1 = lined + wpld;
        linedp2 = lined + 2 * wpld;
        linedp3 = lined + 3 * wpld;
        s2 = GET_DATA_BYTE(lines, 0);
        s4 = GET_DATA_BYTE(linesp, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 4) {
            s1 = s2;
            s3 = s4;
            s2 = GET_DATA_BYTE(lines, j + 1);
            s4 = GET_DATA_BYTE(linesp, j + 1);
            s1t = 3 * s1;
            s2t = 3 * s2;
            s3t = 3 * s3;
            s4t = 3 * s4;
            SET_DATA_BYTE(lined, jd, s1);                             /* d1 */
            SET_DATA_BYTE(lined, jd + 1, (s1t + s2) / 4);             /* d2 */
            SET_DATA_BYTE(lined, jd + 2, (s1 + s2) / 2);              /* d3 */
            SET_DATA_BYTE(lined, jd + 3, (s1 + s2t) / 4);             /* d4 */
            SET_DATA_BYTE(linedp1, jd, (s1t + s3) / 4);                /* d5 */
            SET_DATA_BYTE(linedp1, jd + 1, (9*s1 + s2t + s3t + s4) / 16); /*d6*/
            SET_DATA_BYTE(linedp1, jd + 2, (s1t + s2t + s3 + s4) / 8); /* d7 */
            SET_DATA_BYTE(linedp1, jd + 3, (s1t + 9*s2 + s3 + s4t) / 16);/*d8*/
            SET_DATA_BYTE(linedp2, jd, (s1 + s3) / 2);                /* d9 */
            SET_DATA_BYTE(linedp2, jd + 1, (s1t + s2 + s3t + s4) / 8);/* d10 */
            SET_DATA_BYTE(linedp2, jd + 2, (s1 + s2 + s3 + s4) / 4);  /* d11 */
            SET_DATA_BYTE(linedp2, jd + 3, (s1 + s2t + s3 + s4t) / 8);/* d12 */
            SET_DATA_BYTE(linedp3, jd, (s1 + s3t) / 4);               /* d13 */
            SET_DATA_BYTE(linedp3, jd + 1, (s1t + s2 + 9*s3 + s4t) / 16);/*d14*/
            SET_DATA_BYTE(linedp3, jd + 2, (s1 + s2 + s3t + s4t) / 8); /* d15 */
            SET_DATA_BYTE(linedp3, jd + 3, (s1 + s2t + s3t + 9*s4) / 16);/*d16*/
        }  
        s1 = s2;
        s3 = s4;
        s1t = 3 * s1;
        s3t = 3 * s3;
        SET_DATA_BYTE(lined, wsm4, s1);                               /* d1 */
        SET_DATA_BYTE(lined, wsm4 + 1, s1);                           /* d2 */
        SET_DATA_BYTE(lined, wsm4 + 2, s1);                           /* d3 */
        SET_DATA_BYTE(lined, wsm4 + 3, s1);                           /* d4 */
        SET_DATA_BYTE(linedp1, wsm4, (s1t + s3) / 4);                 /* d5 */
        SET_DATA_BYTE(linedp1, wsm4 + 1, (s1t + s3) / 4);             /* d6 */
        SET_DATA_BYTE(linedp1, wsm4 + 2, (s1t + s3) / 4);             /* d7 */
        SET_DATA_BYTE(linedp1, wsm4 + 3, (s1t + s3) / 4);             /* d8 */
        SET_DATA_BYTE(linedp2, wsm4, (s1 + s3) / 2);                  /* d9 */
        SET_DATA_BYTE(linedp2, wsm4 + 1, (s1 + s3) / 2);              /* d10 */
        SET_DATA_BYTE(linedp2, wsm4 + 2, (s1 + s3) / 2);              /* d11 */
        SET_DATA_BYTE(linedp2, wsm4 + 3, (s1 + s3) / 2);              /* d12 */
        SET_DATA_BYTE(linedp3, wsm4, (s1 + s3t) / 4);                 /* d13 */
        SET_DATA_BYTE(linedp3, wsm4 + 1, (s1 + s3t) / 4);             /* d14 */
        SET_DATA_BYTE(linedp3, wsm4 + 2, (s1 + s3t) / 4);             /* d15 */
        SET_DATA_BYTE(linedp3, wsm4 + 3, (s1 + s3t) / 4);             /* d16 */
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp1 = lined + wpld;
        linedp2 = lined + 2 * wpld;
        linedp3 = lined + 3 * wpld;
        s2 = GET_DATA_BYTE(lines, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 4) {
            s1 = s2;
            s2 = GET_DATA_BYTE(lines, j + 1);
            s1t = 3 * s1;
            s2t = 3 * s2;
            SET_DATA_BYTE(lined, jd, s1);                            /* d1 */
            SET_DATA_BYTE(lined, jd + 1, (s1t + s2) / 4 );           /* d2 */
            SET_DATA_BYTE(lined, jd + 2, (s1 + s2) / 2 );            /* d3 */
            SET_DATA_BYTE(lined, jd + 3, (s1 + s2t) / 4 );           /* d4 */
            SET_DATA_BYTE(linedp1, jd, s1);                          /* d5 */
            SET_DATA_BYTE(linedp1, jd + 1, (s1t + s2) / 4 );         /* d6 */
            SET_DATA_BYTE(linedp1, jd + 2, (s1 + s2) / 2 );          /* d7 */
            SET_DATA_BYTE(linedp1, jd + 3, (s1 + s2t) / 4 );         /* d8 */
            SET_DATA_BYTE(linedp2, jd, s1);                          /* d9 */
            SET_DATA_BYTE(linedp2, jd + 1, (s1t + s2) / 4 );         /* d10 */
            SET_DATA_BYTE(linedp2, jd + 2, (s1 + s2) / 2 );          /* d11 */
            SET_DATA_BYTE(linedp2, jd + 3, (s1 + s2t) / 4 );         /* d12 */
            SET_DATA_BYTE(linedp3, jd, s1);                          /* d13 */
            SET_DATA_BYTE(linedp3, jd + 1, (s1t + s2) / 4 );         /* d14 */
            SET_DATA_BYTE(linedp3, jd + 2, (s1 + s2) / 2 );          /* d15 */
            SET_DATA_BYTE(linedp3, jd + 3, (s1 + s2t) / 4 );         /* d16 */
        }  
        s1 = s2;
        SET_DATA_BYTE(lined, wsm4, s1);                              /* d1 */
        SET_DATA_BYTE(lined, wsm4 + 1, s1);                          /* d2 */
        SET_DATA_BYTE(lined, wsm4 + 2, s1);                          /* d3 */
        SET_DATA_BYTE(lined, wsm4 + 3, s1);                          /* d4 */
        SET_DATA_BYTE(linedp1, wsm4, s1);                            /* d5 */
        SET_DATA_BYTE(linedp1, wsm4 + 1, s1);                        /* d6 */
        SET_DATA_BYTE(linedp1, wsm4 + 2, s1);                        /* d7 */
        SET_DATA_BYTE(linedp1, wsm4 + 3, s1);                        /* d8 */
        SET_DATA_BYTE(linedp2, wsm4, s1);                            /* d9 */
        SET_DATA_BYTE(linedp2, wsm4 + 1, s1);                        /* d10 */
        SET_DATA_BYTE(linedp2, wsm4 + 2, s1);                        /* d11 */
        SET_DATA_BYTE(linedp2, wsm4 + 3, s1);                        /* d12 */
        SET_DATA_BYTE(linedp3, wsm4, s1);                            /* d13 */
        SET_DATA_BYTE(linedp3, wsm4 + 1, s1);                        /* d14 */
        SET_DATA_BYTE(linedp3, wsm4 + 2, s1);                        /* d15 */
        SET_DATA_BYTE(linedp3, wsm4 + 3, s1);                        /* d16 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *       Grayscale and color scaling by closest pixel sampling      *
 *------------------------------------------------------------------*/
/*!
 *  scaleBySamplingLow()
 *
 *  Notes:
 *      (1) The dest must be cleared prior to this operation,
 *          and we clear it here in the low-level code.
 *      (2) We reuse dest pixels and dest pixel rows whenever
 *          possible.  This speeds the upscaling; downscaling
 *          is done by strict subsampling and is unaffected.
 *      (3) Because we are sampling and not interpolating, this
 *          routine works directly, without conversion to full
 *          RGB color, for 2, 4 or 8 bpp palette color images.
 */
LEPTONICA_EXPORT l_int32
scaleBySamplingLow(l_uint32  *datad,
                   l_int32    wd,
                   l_int32    hd,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    ws,
                   l_int32    hs,
                   l_int32    d,
                   l_int32    wpls)
{
l_int32    i, j, bpld;
l_int32    xs, prevxs, sval;
l_int32   *srow, *scol;
l_uint32   csval;
l_uint32  *lines, *prevlines, *lined, *prevlined;
l_float32  wratio, hratio;

    PROCNAME("scaleBySamplingLow");

        /* clear dest */
    bpld = 4 * wpld;
    memset((char *)datad, 0, hd * bpld);
    
        /* the source row corresponding to dest row i ==> srow[i]
         * the source col corresponding to dest col j ==> scol[j]  */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);

    wratio = (l_float32)ws / (l_float32)wd;
    hratio = (l_float32)hs / (l_float32)hd;
    for (i = 0; i < hd; i++)
        srow[i] = L_MIN((l_int32)(hratio * i + 0.5), hs - 1);
    for (j = 0; j < wd; j++)
        scol[j] = L_MIN((l_int32)(wratio * j + 0.5), ws - 1);

    prevlines = NULL;
    for (i = 0; i < hd; i++) {
        lines = datas + srow[i] * wpls;
        lined = datad + i * wpld;
        if (lines != prevlines) {  /* make dest from new source row */
            prevxs = -1;
            sval = 0;
            csval = 0;
            switch (d)
            {
            case 2:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_DIBIT(lines, xs);
                        SET_DATA_DIBIT(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_DIBIT(lined, j, sval);
                }
                break;
            case 4:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_QBIT(lines, xs);
                        SET_DATA_QBIT(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_QBIT(lined, j, sval);
                }
                break;
            case 8:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_BYTE(lines, xs);
                        SET_DATA_BYTE(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_BYTE(lined, j, sval);
                }
                break;
            case 16:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_TWO_BYTES(lines, xs);
                        SET_DATA_TWO_BYTES(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_TWO_BYTES(lined, j, sval);
                }
                break;
            case 32:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        csval = lines[xs];
                        lined[j] = csval;
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        lined[j] = csval;
                }
                break;
            default:
                return ERROR_INT("pixel depth not supported", procName, 1);
                break;
            }
        }
        else {  /* lines == prevlines; copy prev dest row */
            prevlined = lined - wpld;
            memcpy((char *)lined, (char *)prevlined, bpld);
        }
        prevlines = lines;
    }

    FREE(srow);
    FREE(scol);

    return 0;
}

/*------------------------------------------------------------------*
 *              Binary scaling by closest pixel sampling            *
 *------------------------------------------------------------------*/
/*
 *  scaleBinaryLow()
 *
 *  Notes:
 *      (1) The dest must be cleared prior to this operation,
 *          and we clear it here in the low-level code.
 *      (2) We reuse dest pixels and dest pixel rows whenever
 *          possible for upscaling; downscaling is done by
 *          strict subsampling.
 */
LEPTONICA_EXPORT l_int32
scaleBinaryLow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas,
               l_int32    ws,
               l_int32    hs,
               l_int32    wpls)
{
l_int32    i, j, bpld;
l_int32    xs, prevxs, sval;
l_int32   *srow, *scol;
l_uint32  *lines, *prevlines, *lined, *prevlined;
l_float32  wratio, hratio;

    PROCNAME("scaleBinaryLow");

        /* clear dest */
    bpld = 4 * wpld;
    memset((char *)datad, 0, hd * bpld);
    
        /* The source row corresponding to dest row i ==> srow[i]
         * The source col corresponding to dest col j ==> scol[j]  */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);

    wratio = (l_float32)ws / (l_float32)wd;
    hratio = (l_float32)hs / (l_float32)hd;
    for (i = 0; i < hd; i++)
        srow[i] = L_MIN((l_int32)(hratio * i + 0.5), hs - 1);
    for (j = 0; j < wd; j++)
        scol[j] = L_MIN((l_int32)(wratio * j + 0.5), ws - 1);

    prevlines = NULL;
    prevxs = -1;
    sval = 0;
    for (i = 0; i < hd; i++) {
        lines = datas + srow[i] * wpls;
        lined = datad + i * wpld;
        if (lines != prevlines) {  /* make dest from new source row */
            for (j = 0; j < wd; j++) {
                xs = scol[j];
                if (xs != prevxs) {  /* get dest pix from source col */
                    if ((sval = GET_DATA_BIT(lines, xs)))
                        SET_DATA_BIT(lined, j);
                    prevxs = xs;
                }
                else {  /* copy prev dest pix, if set */
                    if (sval)
                        SET_DATA_BIT(lined, j);
                }
            }
        }
        else {  /* lines == prevlines; copy prev dest row */
            prevlined = lined - wpld;
            memcpy((char *)lined, (char *)prevlined, bpld);
        }
        prevlines = lines;
    }

    FREE(srow);
    FREE(scol);

    return 0;
}
