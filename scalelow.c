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
