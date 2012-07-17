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
 *  scale.c
 *
 *         Top-level scaling
 *               PIX    *pixScale()     ***
 *               PIX    *pixScaleToSize()     ***
 *               PIX    *pixScaleGeneral()     ***
 *
 *         Linearly interpreted (usually up-) scaling
 *               PIX    *pixScaleLI()     ***
 *               PIX    *pixScaleColorLI()
 *               PIX    *pixScaleColor2xLI()   ***
 *               PIX    *pixScaleColor4xLI()   ***
 *               PIX    *pixScaleGrayLI()
 *               PIX    *pixScaleGray2xLI()
 *               PIX    *pixScaleGray4xLI()
 *
 *         Scaling by closest pixel sampling
 *               PIX    *pixScaleBySampling()
 *               PIX    *pixScaleByIntSubsampling()
 *
 *         Fast integer factor subsampling RGB to gray and to binary
 *               PIX    *pixScaleRGBToGrayFast()
 *               PIX    *pixScaleRGBToBinaryFast()
 *               PIX    *pixScaleGrayToBinaryFast()
 *
 *         Downscaling with (antialias) smoothing
 *               PIX    *pixScaleSmooth() ***
 *               PIX    *pixScaleRGBToGray2()   [special 2x reduction to gray]
 *
 *         Downscaling with (antialias) area mapping
 *               PIX    *pixScaleAreaMap()     ***
 *               PIX    *pixScaleAreaMap2()
 *
 *         Binary scaling by closest pixel sampling
 *               PIX    *pixScaleBinary()
 *
 *         Scale-to-gray (1 bpp --> 8 bpp; arbitrary downscaling)
 *               PIX    *pixScaleToGray()
 *               PIX    *pixScaleToGrayFast()
 *
 *         Scale-to-gray (1 bpp --> 8 bpp; integer downscaling)
 *               PIX    *pixScaleToGray2()
 *               PIX    *pixScaleToGray3()
 *               PIX    *pixScaleToGray4()
 *               PIX    *pixScaleToGray6()
 *               PIX    *pixScaleToGray8()
 *               PIX    *pixScaleToGray16()
 *
 *         Scale-to-gray by mipmap(1 bpp --> 8 bpp, arbitrary reduction)
 *               PIX    *pixScaleToGrayMipmap()
 *
 *         Grayscale scaling using mipmap
 *               PIX    *pixScaleMipmap()
 *
 *         Replicated (integer) expansion (all depths)
 *               PIX    *pixExpandReplicate()
 *
 *         Upscale 2x followed by binarization
 *               PIX    *pixScaleGray2xLIThresh()
 *               PIX    *pixScaleGray2xLIDither()
 *
 *         Upscale 4x followed by binarization
 *               PIX    *pixScaleGray4xLIThresh()
 *               PIX    *pixScaleGray4xLIDither()
 *
 *         Grayscale downscaling using min and max
 *               PIX    *pixScaleGrayMinMax()
 *               PIX    *pixScaleGrayMinMax2()
 *
 *         Grayscale downscaling using rank value
 *               PIX    *pixScaleGrayRankCascade()
 *               PIX    *pixScaleGrayRank2()
 *
 *         RGB scaling including alpha (blend) component and gamma transform
 *               PIX    *pixScaleWithAlpha()   ***
 *               PIX    *pixScaleGammaXform()  ***
 *
 *  *** Note: these functions make an implicit assumption about RGB
 *            component ordering.
 */

#include <string.h>
#include "allheaders.h"

extern l_float32  AlphaMaskBorderVals[2];

/*------------------------------------------------------------------*
 *               Binary scaling by closest pixel sampling           *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleBinary()
 *
 *      Input:  pixs (1 bpp)
 *              scalex, scaley
 *      Return: pixd, or null on error
 * 
 *  Notes:
 *      (1) This function samples from the source without
 *          filtering.  As a result, aliasing will occur for
 *          subsampling (scalex and scaley < 1.0).
 */
LEPTONICA_EXPORT PIX *
pixScaleBinary(PIX       *pixs,
               l_float32  scalex,
               l_float32  scaley)
{
l_int32    ws, hs, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleBinary");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyColormap(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleBinaryLow(datad, wd, hd, wpld, datas, ws, hs, wpls);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Replicated (integer) expansion                  *
 *------------------------------------------------------------------*/
/*!
 *  pixExpandReplicate()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              factor (integer scale factor for replicative expansion)
 *      Return: pixd (scaled up), or null on error.
 */
LEPTONICA_EXPORT PIX *
pixExpandReplicate(PIX     *pixs,
                   l_int32  factor)
{
l_int32    w, h, d, wd, hd, wpls, wpld, bpld, start, i, j, k;
l_uint8    sval;
l_uint16   sval16;
l_uint32   sval32;
l_uint32  *lines, *datas, *lined, *datad;
PIX       *pixd;

    PROCNAME("pixExpandReplicate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not in {1,2,4,8,16,32}", procName, NULL);
    if (factor <= 0)
        return (PIX *)ERROR_PTR("factor <= 0; invalid", procName, NULL);
    if (factor == 1)
        return pixCopy(NULL, pixs);

    if (d == 1)
        return pixExpandBinaryReplicate(pixs, factor);

    wd = factor * w;
    hd = factor * h;
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyColormap(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, (l_float32)factor, (l_float32)factor);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    switch (d) {
    case 2:
        bpld = (wd + 3) / 4;  /* occupied bytes only */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_DIBIT(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_DIBIT(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 4:
        bpld = (wd + 1) / 2;  /* occupied bytes only */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_QBIT(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_QBIT(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 8:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_BYTE(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_BYTE(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 16:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval16 = GET_DATA_TWO_BYTES(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_TWO_BYTES(lined, start + k, sval16);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 32:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval32 = *(lines + j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    *(lined + start + k) = sval32;
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    default:
        fprintf(stderr, "invalid depth\n");
    }

    return pixd;
}


/*------------------------------------------------------------------*
 *                Scale 2x followed by binarization                 *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleGray2xLIThresh()
 *
 *      Input:  pixs (8 bpp)
 *              thresh  (between 0 and 256)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 2x upscale on pixs, using linear interpolation,
 *          followed by thresholding to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 */
LEPTONICA_REAL_EXPORT PIX *
pixScaleGray2xLIThresh(PIX     *pixs,
                       l_int32  thresh)
{
l_int32    i, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad, *lines, *lined, *lineb;
PIX       *pixd;

    PROCNAME("pixScaleGray2xLIThresh");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (thresh < 0 || thresh > 256)
        return (PIX *)ERROR_PTR("thresh must be in [0, ... 256]",
            procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pixs has colormap", procName);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 2 * ws;
    hd = 2 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffer for 2 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(2 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Do all but last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 2 * i * wpld;  /* do 2 dest lines at a time */
        scaleGray2xLILineLow(lineb, wplb, lines, ws, wpls, 0);
        thresholdToBinaryLineLow(lined, wd, lineb, 8, thresh);
        thresholdToBinaryLineLow(lined + wpld, wd, lineb + wplb, 8, thresh);
    }

        /* Do last src line */
    lines = datas + hsm * wpls;
    lined = datad + 2 * hsm * wpld;
    scaleGray2xLILineLow(lineb, wplb, lines, ws, wpls, 1);
    thresholdToBinaryLineLow(lined, wd, lineb, 8, thresh);
    thresholdToBinaryLineLow(lined + wpld, wd, lineb + wplb, 8, thresh);

    FREE(lineb);
    return pixd;
}


/*------------------------------------------------------------------*
 *                Scale 4x followed by binarization                 *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleGray4xLIThresh()
 *
 *      Input:  pixs (8 bpp)
 *              thresh  (between 0 and 256)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 4x upscale on pixs, using linear interpolation,
 *          followed by thresholding to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *      (3) If a full 4x expanded grayscale image can be kept in memory,
 *          this function is only about 10% faster than separately doing
 *          a linear interpolation to a large grayscale image, followed
 *          by thresholding to binary.
 */
LEPTONICA_REAL_EXPORT PIX *
pixScaleGray4xLIThresh(PIX     *pixs,
                       l_int32  thresh)
{
l_int32    i, j, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad, *lines, *lined, *lineb;
PIX       *pixd;

    PROCNAME("pixScaleGray4xLIThresh");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (thresh < 0 || thresh > 256)
        return (PIX *)ERROR_PTR("thresh must be in [0, ... 256]",
            procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pixs has colormap", procName);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 4 * ws;
    hd = 4 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffer for 4 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(4 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Do all but last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 4 * i * wpld;  /* do 4 dest lines at a time */
        scaleGray4xLILineLow(lineb, wplb, lines, ws, wpls, 0);
        for (j = 0; j < 4; j++) {
            thresholdToBinaryLineLow(lined + j * wpld, wd,
                                     lineb + j * wplb, 8, thresh);
        }
    }

        /* Do last src line */
    lines = datas + hsm * wpls;
    lined = datad + 4 * hsm * wpld;
    scaleGray4xLILineLow(lineb, wplb, lines, ws, wpls, 1);
    for (j = 0; j < 4; j++) {
        thresholdToBinaryLineLow(lined + j * wpld, wd,
                                 lineb + j * wplb, 8, thresh);
    }

    FREE(lineb);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *                    Downscaling using min or max                       *
 *-----------------------------------------------------------------------*/


/*!
 *  pixScaleGrayMinMax2()
 *
 *      Input:  pixs (8 bpp)
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX, L_CHOOSE_MAX_MIN_DIFF)
 *      Return: pixd (8 bpp downscaled by 2x)
 *
 *  Notes:
 *      (1) Special version for 2x reduction.  The downscaled pixels
 *          in pixd are the min, max or (max - min) of the corresponding
 *          set of 4 pixels in pixs.
 *      (2) The max and min operations are a special case (for levels 1
 *          and 4) of grayscale analog to the binary rank scaling operation
 *          pixReduceRankBinary2().  Note, however, that because of
 *          the photometric definition that higher gray values are
 *          lighter, the erosion-like L_CHOOSE_MIN will darken
 *          the resulting image, corresponding to a threshold level 1
 *          in the binary case.  Likewise, L_CHOOSE_MAX will lighten
 *          the pixd, corresponding to a threshold level of 4.
 *      (3) To choose any of the four rank levels in a 2x grayscale
 *          reduction, use pixScaleGrayRank2().
 *      (4) This runs at about 70 MPix/sec/GHz of source data for
 *          erosion and dilation.
 */
LEPTONICA_EXPORT PIX *
pixScaleGrayMinMax2(PIX     *pixs,
                    l_int32  type)
{
l_int32    ws, hs, d, wd, hd, wpls, wpld, i, j, k;
l_int32    minval, maxval;
l_int32    val[4];
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayMinMax2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (ws < 2 || hs < 2)
        return (PIX *)ERROR_PTR("too small: ws < 2 or hs < 2", procName, NULL);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX &&
        type != L_CHOOSE_MAX_MIN_DIFF)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    wd = ws / 2;
    hd = hs / 2;
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hd; i++) {
        lines = datas + 2 * i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            val[0] = GET_DATA_BYTE(lines, 2 * j);
            val[1] = GET_DATA_BYTE(lines, 2 * j + 1);
            val[2] = GET_DATA_BYTE(lines + wpls, 2 * j);
            val[3] = GET_DATA_BYTE(lines + wpls, 2 * j + 1);
            if (type == L_CHOOSE_MIN || type == L_CHOOSE_MAX_MIN_DIFF) {
                minval = 255;
                for (k = 0; k < 4; k++) {
                    if (val[k] < minval)
                        minval = val[k];
                }
            }
            if (type == L_CHOOSE_MAX || type == L_CHOOSE_MAX_MIN_DIFF) {
                maxval = 0;
                for (k = 0; k < 4; k++) {
                    if (val[k] > maxval)
                        maxval = val[k];
                }
            }
            if (type == L_CHOOSE_MIN)
                SET_DATA_BYTE(lined, j, minval);
            else if (type == L_CHOOSE_MAX)
                SET_DATA_BYTE(lined, j, maxval);
            else  /* type == L_CHOOSE_MAX_MIN_DIFF */
                SET_DATA_BYTE(lined, j, maxval - minval);
        }
    }
            
    return pixd;
}
