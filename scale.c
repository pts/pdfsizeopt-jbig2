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
