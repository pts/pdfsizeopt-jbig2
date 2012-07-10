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


/*!
 *  pixScaleColor2xLI()
 * 
 *      Input:  pixs  (32 bpp, representing rgb)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of linear interpolated scaling,
 *          for 2x upscaling.  It is about 8x faster than using
 *          the generic pixScaleColorLI(), and about 4x faster than
 *          using the special 2x scale function pixScaleGray2xLI()
 *          on each of the three components separately.
 *      (2) The speed on intel hardware is about
 *          80 * 10^6 dest-pixels/sec/GHz (!!)
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
LEPTONICA_EXPORT PIX *
pixScaleColor2xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleColor2xLI");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    
    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(2 * ws, 2 * hs, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleColor2xLILow(datad, wpld, datas, ws, hs, wpls);
    return pixd;
}


/*!
 *  pixScaleColor4xLI()
 *
 *      Input:  pixs  (32 bpp, representing rgb)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of color linear interpolated scaling,
 *          for 4x upscaling.  It is about 3x faster than using
 *          the generic pixScaleColorLI().
 *      (2) The speed on intel hardware is about
 *          30 * 10^6 dest-pixels/sec/GHz 
 *      (3) This scales each component separately, using pixScaleGray4xLI().
 *          It would be about 4x faster to inline the color code properly,
 *          in analogy to scaleColor4xLILow(), and I leave this as
 *          an exercise for someone who really needs it.
 */
LEPTONICA_EXPORT PIX *
pixScaleColor4xLI(PIX  *pixs)
{
PIX  *pixr, *pixg, *pixb;
PIX  *pixrs, *pixgs, *pixbs;
PIX  *pixd;

    PROCNAME("pixScaleColor4xLI");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    pixr = pixGetRGBComponent(pixs, COLOR_RED);
    pixrs = pixScaleGray4xLI(pixr);
    pixDestroy(&pixr);
    pixg = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixgs = pixScaleGray4xLI(pixg);
    pixDestroy(&pixg);
    pixb = pixGetRGBComponent(pixs, COLOR_BLUE);
    pixbs = pixScaleGray4xLI(pixb);
    pixDestroy(&pixb);

    if ((pixd = pixCreateRGBImage(pixrs, pixgs, pixbs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    pixDestroy(&pixrs);
    pixDestroy(&pixgs);
    pixDestroy(&pixbs);
    return pixd;
}


/*!
 *  pixScaleGray2xLI()
 * 
 *      Input:  pixs (8 bpp grayscale)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of gray linear interpolated scaling,
 *          for 2x upscaling.  It is about 6x faster than using
 *          the generic pixScaleGrayLI().
 *      (2) The speed on intel hardware is about
 *          100 * 10^6 dest-pixels/sec/GHz
 */
LEPTONICA_EXPORT PIX *
pixScaleGray2xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleGray2xLI");

    if (!pixs || (pixGetDepth(pixs) != 8))
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pix has colormap", procName);
    
    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(2 * ws, 2 * hs, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleGray2xLILow(datad, wpld, datas, ws, hs, wpls);
    return pixd;
}


/*!
 *  pixScaleGray4xLI()
 * 
 *      Input:  pixs (8 bpp grayscale)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of gray linear interpolated scaling,
 *          for 4x upscaling.  It is about 12x faster than using
 *          the generic pixScaleGrayLI().
 *      (2) The speed on intel hardware is about
 *          160 * 10^6 dest-pixels/sec/GHz  (!!)
 */
LEPTONICA_EXPORT PIX *
pixScaleGray4xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleGray4xLI");

    if (!pixs || (pixGetDepth(pixs) != 8))
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pix has colormap", procName);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(4 * ws, 4 * hs, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleGray4xLILow(datad, wpld, datas, ws, hs, wpls);
    return pixd;
}



/*------------------------------------------------------------------*
 *                  Scaling by closest pixel sampling               *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleBySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              scalex, scaley
 *      Return: pixd, or null on error
 * 
 *  Notes:
 *      (1) This function samples from the source without
 *          filtering.  As a result, aliasing will occur for
 *          subsampling (@scalex and/or @scaley < 1.0).
 *      (2) If @scalex == 1.0 and @scaley == 1.0, returns a copy.
 */
LEPTONICA_EXPORT PIX *
pixScaleBySampling(PIX       *pixs,
                   l_float32  scalex,
                   l_float32  scaley)
{
l_int32    ws, hs, d, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleBySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);
    if ((d = pixGetDepth(pixs)) == 1)
        return pixScaleBinary(pixs, scalex, scaley);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    pixCopyColormap(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleBySamplingLow(datad, wd, hd, wpld, datas, ws, hs, d, wpls);
    return pixd;
}


/*!
 *  pixScaleByIntSubsampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              factor (integer subsampling)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Simple interface to pixScaleBySampling(), for
 *          isotropic integer reduction.
 *      (2) If @factor == 1, returns a copy.
 */
LEPTONICA_EXPORT PIX *
pixScaleByIntSubsampling(PIX     *pixs,
                         l_int32  factor)
{
l_float32  scale;

    PROCNAME("pixScaleByIntSubsampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor <= 1) {
        if (factor < 1)
            L_ERROR("factor must be >= 1; returning a copy", procName);
        return pixCopy(NULL, pixs);
    }

    scale = 1. / (l_float32)factor;
    return pixScaleBySampling(pixs, scale, scale);
}


/*------------------------------------------------------------------*
 *            Fast integer factor subsampling RGB to gray           *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleRGBToGrayFast()
 *
 *      Input:  pixs (32 bpp rgb)
 *              factor (integer reduction factor >= 1)
 *              color (one of COLOR_RED, COLOR_GREEN, COLOR_BLUE)
 *      Return: pixd (8 bpp), or null on error
 * 
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          extraction of the color from the RGB pix.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized grayscale image from a higher resolution
 *          RGB image.  This would typically be used for image analysis.
 *      (3) The standard color byte order (RGBA) is assumed.
 */
LEPTONICA_EXPORT PIX *
pixScaleRGBToGrayFast(PIX     *pixs,
                      l_int32  factor,
                      l_int32  color)
{
l_int32    byteval, shift;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld;
l_uint32  *datas, *words, *datad, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleRGBToGrayFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("depth not 32 bpp", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);

    if (color == COLOR_RED)
        shift = L_RED_SHIFT;
    else if (color == COLOR_GREEN)
        shift = L_GREEN_SHIFT;
    else if (color == COLOR_BLUE)
        shift = L_BLUE_SHIFT;
    else
        return (PIX *)ERROR_PTR("invalid color", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    
    for (i = 0; i < hd; i++) {
        words = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++, words += factor) {
            byteval = ((*words) >> shift) & 0xff;
            SET_DATA_BYTE(lined, j, byteval);
        }
    }

    return pixd;
}


/*!
 *  pixScaleRGBToBinaryFast()
 *
 *      Input:  pixs (32 bpp RGB)
 *              factor (integer reduction factor >= 1)
 *              thresh (binarization threshold)
 *      Return: pixd (1 bpp), or null on error
 * 
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          conversion from RGB to gray to binary.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized binary image from a higher resolution
 *          RGB image.  This would typically be used for image analysis.
 *      (3) It uses the green channel to represent the RGB pixel intensity.
 */
LEPTONICA_EXPORT PIX *
pixScaleRGBToBinaryFast(PIX     *pixs,
                        l_int32  factor,
                        l_int32  thresh)
{
l_int32    byteval;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld;
l_uint32  *datas, *words, *datad, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleRGBToBinaryFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("depth not 32 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < hd; i++) {
        words = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++, words += factor) {
            byteval = ((*words) >> L_GREEN_SHIFT) & 0xff;
            if (byteval < thresh)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*!
 *  pixScaleGrayToBinaryFast()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              factor (integer reduction factor >= 1)
 *              thresh (binarization threshold)
 *      Return: pixd (1 bpp), or null on error
 * 
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          thresholding from gray to binary.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized binary image from a higher resolution
 *          gray image.  This would typically be used for image analysis.
 */
LEPTONICA_EXPORT PIX *
pixScaleGrayToBinaryFast(PIX     *pixs,
                         l_int32  factor,
                         l_int32  thresh)
{
l_int32    byteval;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld, sj;
l_uint32  *datas, *datad, *lines, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleGrayToBinaryFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("depth not 8 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < hd; i++) {
        lines = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0, sj = 0; j < wd; j++, sj += factor) {
            byteval = GET_DATA_BYTE(lines, sj);
            if (byteval < thresh)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*!
 *  pixScaleRGBToGray2()
 *
 *      Input:  pixs (32 bpp rgb)
 *              rwt, gwt, bwt (must sum to 1.0)
 *      Return: pixd, (8 bpp, 2x reduced), or null on error
 */
LEPTONICA_EXPORT PIX *
pixScaleRGBToGray2(PIX       *pixs,
                   l_float32  rwt,
                   l_float32  gwt,
                   l_float32  bwt)
{
l_int32    wd, hd, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleRGBToGray2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (rwt + gwt + bwt < 0.98 || rwt + gwt + bwt > 1.02)
        return (PIX *)ERROR_PTR("sum of wts should be 1.0", procName, NULL);

    wd = pixGetWidth(pixs) / 2;
    hd = pixGetHeight(pixs) / 2;
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    scaleRGBToGray2Low(datad, wd, hd, wpld, datas, wpls, rwt, gwt, bwt);
    return pixd;
}


/*!
 *  pixScaleAreaMap2()
 *
 *      Input:  pixs (2, 4, 8 or 32 bpp; and 2, 4, 8 bpp with colormap)
 *      Return: pixd, or null on error
 * 
 *  Notes:
 *      (1) This function does an area mapping (average) for 2x
 *          reduction.
 *      (2) This works only on 2, 4, 8 and 32 bpp images.  If there is
 *          a colormap, it is removed by converting to RGB.
 *      (3) Speed on 3 GHz processor:
 *             Color: 160 Mpix/sec
 *             Gray: 700 Mpix/sec
 *          This contrasts with the speed of the general pixScaleAreaMap():
 *             Color: 35 Mpix/sec
 *             Gray: 50 Mpix/sec
 *      (4) From (3), we see that this special function is about 4.5x
 *          faster for color and 14x faster for grayscale
 *      (5) Consequently, pixScaleAreaMap2() is incorporated into the
 *          general area map scaling function, for the special cases
 *          of 2x, 4x, 8x and 16x reduction.
 */
LEPTONICA_EXPORT PIX *
pixScaleAreaMap2(PIX  *pix)
{
l_int32    wd, hd, d, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixs, *pixd;

    PROCNAME("pixScaleAreaMap2");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    d = pixGetDepth(pix);
    if (d != 2 && d != 4 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 2, 4, 8 or 32 bpp", procName, NULL);

        /* Remove colormap if necessary.
         * If 2 bpp or 4 bpp gray, convert to 8 bpp */
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    }
    else if (d == 2 || d == 4) {
        pixs = pixConvertTo8(pix, FALSE);
        d = 8;
    }
    else
        pixs = pixClone(pix);

    wd = pixGetWidth(pixs) / 2;
    hd = pixGetHeight(pixs) / 2;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreate(wd, hd, d);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    scaleAreaMapLow2(datad, wd, hd, wpld, datas, d, wpls);
    pixDestroy(&pixs);
    return pixd;
}


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


/*-----------------------------------------------------------------------*
 *          Scale-to-gray (1 bpp --> 8 bpp; integer downscaling)         *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleToGray2()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 2x in each direction,
 *              or null on error.
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray2(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 2;
    hd = hs / 2;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG2()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG2()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray2Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray3()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 3x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) Speed is about 100 x 10^6 src-pixels/sec/GHz.
 *          Another way to express this is it processes 1 src pixel
 *          in about 10 cycles.
 *      (2) The width of pixd is truncated is truncated to a factor of 8.
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray3(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 3) & 0xfffffff8;    /* truncate to factor of 8 */
    hd = hs / 3;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.33333, 0.33333);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG3()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG3()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray3Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray4()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 4x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) The width of pixd is truncated is truncated to a factor of 2.
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray4(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray4");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 4) & 0xfffffffe;    /* truncate to factor of 2 */
    hd = hs / 4;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.25, 0.25);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG4()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG4()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray4Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}



/*!
 *  pixScaleToGray6()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 6x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) The width of pixd is truncated is truncated to a factor of 8.
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray6(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd, wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray6");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 6) & 0xfffffff8;    /* truncate to factor of 8 */
    hd = hs / 6;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.16667, 0.16667);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);
    if ((valtab = makeValTabSG6()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray6Low(datad, wd, hd, wpld, datas, wpls, tab8, valtab);

    FREE(tab8);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray8()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 8x in each direction,
 *              or null on error
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray8(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 8;  /* truncate to nearest dest byte */
    hd = hs / 8;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.125, 0.125);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);
    if ((valtab = makeValTabSG8()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray8Low(datad, wd, hd, wpld, datas, wpls, tab8, valtab);

    FREE(tab8);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray16()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 16x in each direction,
 *              or null on error.
 */
LEPTONICA_EXPORT PIX *
pixScaleToGray16(PIX  *pixs)
{
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray16");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 16;
    hd = hs / 16;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.0625, 0.0625);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);

    scaleToGray16Low(datad, wd, hd, wpld, datas, wpls, tab8);

    FREE(tab8);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Grayscale scaling using mipmap                  *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleMipmap()
 *
 *      Input:  pixs1 (high res 8 bpp)
 *              pixs2 (low res -- 2x reduced -- 8 bpp)
 *              scale (reduction with respect to high res image, > 0.5)
 *      Return: 8 bpp pix, scaled down by reduction in each direction,
 *              or NULL on error.
 *
 *  Notes:
 *      (1) See notes in pixScaleToGrayMipmap().
 *      (2) This function suffers from aliasing effects that are
 *          easily seen in document images.
 */
LEPTONICA_EXPORT PIX *
pixScaleMipmap(PIX       *pixs1,
               PIX       *pixs2,
               l_float32  scale)
{
l_int32    ws1, hs1, ds1, ws2, hs2, ds2, wd, hd, wpls1, wpls2, wpld;
l_uint32  *datas1, *datas2, *datad;
PIX       *pixd;

    PROCNAME("pixScaleMipmap");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, NULL);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, NULL);
    pixGetDimensions(pixs1, &ws1, &hs1, &ds1);
    pixGetDimensions(pixs2, &ws2, &hs2, &ds2);
    if (ds1 != 8 || ds2 != 8)
        return (PIX *)ERROR_PTR("pixs1, pixs2 not both 8 bpp", procName, NULL);
    if (scale > 1.0 || scale < 0.5)
        return (PIX *)ERROR_PTR("scale not in [0.5, 1.0]", procName, NULL);
    if (pixGetColormap(pixs1) || pixGetColormap(pixs2))
        L_WARNING("pixs1 or pixs2 has colormap", procName);
    if (ws1 < 2 * ws2)
        return (PIX *)ERROR_PTR("invalid width ratio", procName, NULL);
    if (hs1 < 2 * hs2)
        return (PIX *)ERROR_PTR("invalid height ratio", procName, NULL);

        /* Generate wd and hd from the lower resolution dimensions,
         * to guarantee staying within both src images */
    datas1 = pixGetData(pixs1);
    wpls1 = pixGetWpl(pixs1);
    datas2 = pixGetData(pixs2);
    wpls2 = pixGetWpl(pixs2);
    wd = (l_int32)(2. * scale * pixGetWidth(pixs2));
    hd = (l_int32)(2. * scale * pixGetHeight(pixs2));
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs1);
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    scaleMipmapLow(datad, wd, hd, wpld, datas1, wpls1, datas2, wpls2, scale);
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


/*!
 *  pixScaleGray2xLIDither()
 *
 *      Input:  pixs (8 bpp)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 2x upscale on pixs, using linear interpolation,
 *          followed by Floyd-Steinberg dithering to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *          - Two line buffers are used for the src, required for the 2x
 *            LI upscale.
 *          - Three line buffers are used for the intermediate image.
 *            Two are filled with each 2xLI row operation; the third is
 *            needed because the upscale and dithering ops are out of sync.
 */
LEPTONICA_EXPORT PIX *
pixScaleGray2xLIDither(PIX  *pixs)
{
l_int32    i, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad;
l_uint32  *lined;
l_uint32  *lineb;   /* 2 intermediate buffer lines */
l_uint32  *linebp;  /* 1 intermediate buffer line */
l_uint32  *bufs;    /* 2 source buffer lines */
PIX       *pixd;

    PROCNAME("pixScaleGray2xLIDither");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pixs has colormap", procName);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 2 * ws;
    hd = 2 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffers for 2 lines of src image */
    if ((bufs = (l_uint32 *)CALLOC(2 * wpls, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("bufs not made", procName, NULL);

        /* Make line buffer for 2 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(2 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make line buffer for 1 line of virtual intermediate image */
    if ((linebp = (l_uint32 *)CALLOC(wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("linebp not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Start with the first src and the first dest line */
    memcpy(bufs, datas, 4 * wpls);   /* first src line */
    memcpy(bufs + wpls, datas + wpls, 4 * wpls);  /* 2nd src line */
    scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 2 i lines */
    lined = datad;
    ditherToBinaryLineLow(lined, wd, lineb, lineb + wplb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                    /* 1st d line */

        /* Do all but last src line */
    for (i = 1; i < hsm; i++) {
        memcpy(bufs, datas + i * wpls, 4 * wpls);  /* i-th src line */
        memcpy(bufs + wpls, datas + (i + 1) * wpls, 4 * wpls);
        memcpy(linebp, lineb + wplb, 4 * wplb);
        scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 2 i lines */
        lined = datad + 2 * i * wpld;
        ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* odd dest line */
        ditherToBinaryLineLow(lined, wd, lineb, lineb + wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* even dest line */
    }

        /* Do the last src line and the last 3 dest lines */
    memcpy(bufs, datas + hsm * wpls, 4 * wpls);  /* hsm-th src line */
    memcpy(linebp, lineb + wplb, 4 * wplb);   /* 1 i line */
    scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 1);  /* 2 i lines */
    ditherToBinaryLineLow(lined + wpld, wd, linebp, lineb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* odd dest line */
    ditherToBinaryLineLow(lined + 2 * wpld, wd, lineb, lineb + wplb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* even dest line */
    ditherToBinaryLineLow(lined + 3 * wpld, wd, lineb + wplb, NULL,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 1);
                                                   /* last dest line */

    FREE(bufs);
    FREE(lineb);
    FREE(linebp);
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


/*!
 *  pixScaleGray4xLIDither()
 *
 *      Input:  pixs (8 bpp)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 4x upscale on pixs, using linear interpolation,
 *          followed by Floyd-Steinberg dithering to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *          - Two line buffers are used for the src, required for the
 *            4xLI upscale.
 *          - Five line buffers are used for the intermediate image.
 *            Four are filled with each 4xLI row operation; the fifth
 *            is needed because the upscale and dithering ops are
 *            out of sync.
 *      (3) If a full 4x expanded grayscale image can be kept in memory,
 *          this function is only about 5% faster than separately doing
 *          a linear interpolation to a large grayscale image, followed
 *          by error-diffusion dithering to binary.
 */
LEPTONICA_EXPORT PIX *
pixScaleGray4xLIDither(PIX  *pixs)
{
l_int32    i, j, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad;
l_uint32  *lined;
l_uint32  *lineb;   /* 4 intermediate buffer lines */
l_uint32  *linebp;  /* 1 intermediate buffer line */
l_uint32  *bufs;    /* 2 source buffer lines */
PIX       *pixd;

    PROCNAME("pixScaleGray4xLIDither");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        L_WARNING("pixs has colormap", procName);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 4 * ws;
    hd = 4 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffers for 2 lines of src image */
    if ((bufs = (l_uint32 *)CALLOC(2 * wpls, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("bufs not made", procName, NULL);

        /* Make line buffer for 4 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(4 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make line buffer for 1 line of virtual intermediate image */
    if ((linebp = (l_uint32 *)CALLOC(wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("linebp not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Start with the first src and the first 3 dest lines */
    memcpy(bufs, datas, 4 * wpls);   /* first src line */
    memcpy(bufs + wpls, datas + wpls, 4 * wpls);  /* 2nd src line */
    scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 4 b lines */
    lined = datad;
    for (j = 0; j < 3; j++) {  /* first 3 d lines of Q */
        ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                              lineb + (j + 1) * wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
    }

        /* Do all but last src line */
    for (i = 1; i < hsm; i++) {
        memcpy(bufs, datas + i * wpls, 4 * wpls);  /* i-th src line */
        memcpy(bufs + wpls, datas + (i + 1) * wpls, 4 * wpls);
        memcpy(linebp, lineb + 3 * wplb, 4 * wplb);
        scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 4 b lines */
        lined = datad + 4 * i * wpld;
        ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                     /* 4th dest line of Q */
        for (j = 0; j < 3; j++) {  /* next 3 d lines of Quad */
            ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                                  lineb + (j + 1) * wplb,
                                 DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
        }                             
    }

        /* Do the last src line and the last 5 dest lines */
    memcpy(bufs, datas + hsm * wpls, 4 * wpls);  /* hsm-th src line */
    memcpy(linebp, lineb + 3 * wplb, 4 * wplb);   /* 1 b line */
    scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 1);  /* 4 b lines */
    lined = datad + 4 * hsm * wpld;
    ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* 4th dest line of Q */
    for (j = 0; j < 3; j++) {  /* next 3 d lines of Quad */
        ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                              lineb + (j + 1) * wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
    }
        /* And finally, the last dest line */
    ditherToBinaryLineLow(lined + 3 * wpld, wd, lineb + 3 * wplb, NULL,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 1);

    FREE(bufs);
    FREE(lineb);
    FREE(linebp);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *                    Downscaling using min or max                       *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleGrayMinMax()
 *
 *      Input:  pixs (8 bpp)
 *              xfact (x downscaling factor; integer)
 *              yfact (y downscaling factor; integer)
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX, L_CHOOSE_MAX_MIN_DIFF)
 *      Return: pixd (8 bpp)
 *
 *  Notes:
 *      (1) The downscaled pixels in pixd are the min, max or (max - min)
 *          of the corresponding set of xfact * yfact pixels in pixs.
 *      (2) Using L_CHOOSE_MIN is equivalent to a grayscale erosion,
 *          using a brick Sel of size (xfact * yfact), followed by
 *          subsampling within each (xfact * yfact) cell.  Using
 *          L_CHOOSE_MAX is equivalent to the corresponding dilation.
 *      (3) Using L_CHOOSE_MAX_MIN_DIFF finds the difference between max
 *          and min values in each cell.
 *      (4) For the special case of downscaling by 2x in both directions,
 *          pixScaleGrayMinMax2() is about 2x more efficient.
 */
LEPTONICA_EXPORT PIX *
pixScaleGrayMinMax(PIX     *pixs,
                   l_int32  xfact,
                   l_int32  yfact,
                   l_int32  type)
{
l_int32    ws, hs, d, wd, hd, wpls, wpld, i, j, k, m;
l_int32    minval, maxval, val;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayMinMax");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX &&
        type != L_CHOOSE_MAX_MIN_DIFF)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (xfact < 1 || yfact < 1)
        return (PIX *)ERROR_PTR("xfact and yfact must be > 0", procName, NULL);

    if (xfact == 2 && yfact == 2)
        return pixScaleGrayMinMax2(pixs, type);

    wd = ws / xfact;
    if (wd == 0) {  /* single tile */
        wd = 1;
        xfact = ws;
    }
    hd = hs / yfact;
    if (hd == 0) {  /* single tile */
        hd = 1;
        yfact = hs;
    }
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hd; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            if (type == L_CHOOSE_MIN || type == L_CHOOSE_MAX_MIN_DIFF) {
                minval = 255;
                for (k = 0; k < yfact; k++) {
                    lines = datas + (yfact * i + k) * wpls;
                    for (m = 0; m < xfact; m++) {
                        val = GET_DATA_BYTE(lines, xfact * j + m);
                        if (val < minval)
                            minval = val;
                    }
                }
            }
            if (type == L_CHOOSE_MAX || type == L_CHOOSE_MAX_MIN_DIFF) {
                maxval = 0;
                for (k = 0; k < yfact; k++) {
                    lines = datas + (yfact * i + k) * wpls;
                    for (m = 0; m < xfact; m++) {
                        val = GET_DATA_BYTE(lines, xfact * j + m);
                        if (val > maxval)
                            maxval = val;
                    }
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


/*-----------------------------------------------------------------------*
 *                  Grayscale downscaling using rank value               *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleGrayRankCascade()
 *
 *      Input:  pixs (8 bpp)
 *              level1, ... level4 (rank thresholds, in set {0, 1, 2, 3, 4})
 *      Return: pixd (8 bpp, downscaled by up to 16x)
 *
 *  Notes:
 *      (1) This performs up to four cascaded 2x rank reductions.
 *      (2) Use level = 0 to truncate the cascade.
 */
LEPTONICA_EXPORT PIX *
pixScaleGrayRankCascade(PIX     *pixs,
                        l_int32  level1,
                        l_int32  level2,
                        l_int32  level3,
                        l_int32  level4)
{
PIX  *pixt1, *pixt2, *pixt3, *pixt4;

    PROCNAME("pixScaleGrayRankCascade");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (level1 > 4 || level2 > 4 || level3 > 4 || level4 > 4)
        return (PIX *)ERROR_PTR("levels must not exceed 4", procName, NULL);

    if (level1 <= 0) {
        L_WARNING("no reduction because level1 not > 0", procName);
        return pixCopy(NULL, pixs);
    }

    pixt1 = pixScaleGrayRank2(pixs, level1);
    if (level2 <= 0)
        return pixt1;

    pixt2 = pixScaleGrayRank2(pixt1, level2);
    pixDestroy(&pixt1);
    if (level3 <= 0)
        return pixt2;

    pixt3 = pixScaleGrayRank2(pixt2, level3);
    pixDestroy(&pixt2);
    if (level4 <= 0)
        return pixt3;

    pixt4 = pixScaleGrayRank2(pixt3, level4);
    pixDestroy(&pixt3);
    return pixt4;
}


/*!
 *  pixScaleGrayRank2()
 *
 *      Input:  pixs (8 bpp)
 *              rank (1 (darkest), 2, 3, 4 (lightest))
 *      Return: pixd (8 bpp, downscaled by 2x)
 *
 *  Notes:
 *      (1) Rank 2x reduction.  If rank == 1(4), the downscaled pixels
 *          in pixd are the min(max) of the corresponding set of
 *          4 pixels in pixs.  Values 2 and 3 are intermediate.
 *      (2) This is the grayscale analog to the binary rank scaling operation
 *          pixReduceRankBinary2().  Here, because of the photometric
 *          definition that higher gray values are lighter, rank 1 gives
 *          the darkest pixel, whereas rank 4 gives the lightest pixel.
 *          This is opposite to the binary rank operation.
 *      (3) For rank = 1 and 4, this calls pixScaleGrayMinMax2(),
 *          which runs at about 70 MPix/sec/GHz of source data.
 *          For rank 2 and 3, this runs 3x slower, at about 25 MPix/sec/GHz.
 */
LEPTONICA_EXPORT PIX *
pixScaleGrayRank2(PIX     *pixs,
                  l_int32  rank)
{
l_int32    d, ws, hs, wd, hd, wpls, wpld, i, j, k, m;
l_int32    minval, maxval, rankval, minindex, maxindex;
l_int32    val[4];
l_int32    midval[4];  /* should only use 2 of these */
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayRank2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (rank < 1 || rank > 4)
        return (PIX *)ERROR_PTR("invalid rank", procName, NULL);

    if (rank == 1)
        return pixScaleGrayMinMax2(pixs, L_CHOOSE_MIN);
    if (rank == 4)
        return pixScaleGrayMinMax2(pixs, L_CHOOSE_MAX);

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
            minval = maxval = val[0];
            minindex = maxindex = 0;
            for (k = 1; k < 4; k++) {
                if (val[k] < minval) {
                    minval = val[k];
                    minindex = k;
                    continue;
                }
                if (val[k] > maxval) {
                    maxval = val[k];
                    maxindex = k;
                }
            }
            for (k = 0, m = 0; k < 4; k++) {
                if (k == minindex || k == maxindex)
                    continue;
                midval[m++] = val[k];
            }
            if (m > 2)  /* minval == maxval; all val[k] are the same */
                rankval = minval;
            else if (rank == 2)
                rankval = L_MIN(midval[0], midval[1]);
            else  /* rank == 3 */
                rankval = L_MAX(midval[0], midval[1]);
            SET_DATA_BYTE(lined, j, rankval);
        }
    }
            
    return pixd;
}
