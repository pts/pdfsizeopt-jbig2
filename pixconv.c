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
 *  pixconv.c
 *
 *      These functions convert between images of different types
 *      without scaling.
 *
 *      Conversion from 8 bpp grayscale to 1, 2, 4 and 8 bpp
 *           PIX        *pixThreshold8()
 *
 *      Conversion from colormap to full color or grayscale
 *           PIX        *pixRemoveColormap()
 *
 *      Add colormap losslessly (8 to 8)
 *           l_int32     pixAddGrayColormap8()
 *           PIX        *pixAddMinimalGrayColormap8()
 *
 *      Conversion from RGB color to grayscale
 *           PIX        *pixConvertRGBToLuminance()
 *           PIX        *pixConvertRGBToGray()
 *           PIX        *pixConvertRGBToGrayFast()
 *           PIX        *pixConvertRGBToGrayMinMax()
 *
 *      Conversion from grayscale to colormap
 *           PIX        *pixConvertGrayToColormap()  -- 2, 4, 8 bpp
 *           PIX        *pixConvertGrayToColormap8()  -- 8 bpp only
 *
 *      Colorizing conversion from grayscale to color
 *           PIX        *pixColorizeGray()  -- 8 bpp or cmapped
 *
 *      Conversion from RGB color to colormap
 *           PIX        *pixConvertRGBToColormap()
 *
 *      Quantization for relatively small number of colors in source
 *           l_int32     pixQuantizeIfFewColors()
 *
 *      Conversion from 16 bpp to 8 bpp
 *           PIX        *pixConvert16To8()
 *
 *      Conversion from grayscale to false color
 *           PIX        *pixConvertGrayToFalseColor()
 *
 *      Unpacking conversion from 1 bpp to 2, 4, 8, 16 and 32 bpp
 *           PIX        *pixUnpackBinary()
 *           PIX        *pixConvert1To16()
 *           PIX        *pixConvert1To32()
 *
 *      Unpacking conversion from 1 bpp to 2 bpp
 *           PIX        *pixConvert1To2Cmap()
 *           PIX        *pixConvert1To2()
 *
 *      Unpacking conversion from 1 bpp to 4 bpp
 *           PIX        *pixConvert1To4Cmap()
 *           PIX        *pixConvert1To4()
 *
 *      Unpacking conversion from 1, 2 and 4 bpp to 8 bpp
 *           PIX        *pixConvert1To8()
 *           PIX        *pixConvert2To8()
 *           PIX        *pixConvert4To8()
 *
 *      Unpacking conversion from 8 bpp to 16 bpp
 *           PIX        *pixConvert8To16()
 *
 *      Top-level conversion to 1 bpp
 *           PIX        *pixConvertTo1()
 *           PIX        *pixConvertTo1BySampling()
 *
 *      Top-level conversion to 8 bpp
 *           PIX        *pixConvertTo8()
 *           PIX        *pixConvertTo8BySampling()
 *
 *      Top-level conversion to 16 bpp
 *           PIX        *pixConvertTo16()
 *
 *      Top-level conversion to 32 bpp (RGB)
 *           PIX        *pixConvertTo32()   ***
 *           PIX        *pixConvertTo32BySampling()   ***
 *           PIX        *pixConvert8To32()  ***
 *
 *      Top-level conversion to 8 or 32 bpp, without colormap
 *           PIX        *pixConvertTo8Or32
 *
 *      Conversion between 24 bpp and 32 bpp rgb
 *           PIX        *pixConvert24To32()
 *           PIX        *pixConvert32To24()
 *
 *      Lossless depth conversion (unpacking)
 *           PIX        *pixConvertLossless()
 *
 *      Conversion for printing in PostScript
 *           PIX        *pixConvertForPSWrap()
 *
 *      Scaling conversion to subpixel RGB
 *           PIX        *pixConvertToSubpixelRGB()
 *           PIX        *pixConvertGrayToSubpixelRGB()
 *           PIX        *pixConvertColorToSubpixelRGB()
 *
 *      *** indicates implicit assumption about RGB component ordering
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define DEBUG_CONVERT_TO_COLORMAP  0
#define DEBUG_UNROLLING 0
#endif   /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *               Conversion from colormapped pix               *
 *-------------------------------------------------------------*/
/*!
 *  pixRemoveColormap()
 *
 *      Input:  pixs (see restrictions below)
 *              type (REMOVE_CMAP_TO_BINARY,
 *                    REMOVE_CMAP_TO_GRAYSCALE,
 *                    REMOVE_CMAP_TO_FULL_COLOR,
 *                    REMOVE_CMAP_BASED_ON_SRC)
 *      Return: new pix, or null on error
 *
 *  Notes:
 *      (1) If there is no colormap, a clone is returned.
 *      (2) Otherwise, the input pixs is restricted to 1, 2, 4 or 8 bpp.
 *      (3) Use REMOVE_CMAP_TO_BINARY only on 1 bpp pix.
 *      (4) For grayscale conversion from RGB, use a weighted average
 *          of RGB values, and always return an 8 bpp pix, regardless
 *          of whether the input pixs depth is 2, 4 or 8 bpp.
 */
LEPTONICA_REAL_EXPORT PIX *
pixRemoveColormap(PIX     *pixs,
                  l_int32  type)
{
l_int32    sval, rval, gval, bval;
l_int32    i, j, k, w, h, d, wpls, wpld, ncolors, count;
l_int32    colorfound;
l_int32   *rmap, *gmap, *bmap, *graymap;
l_uint32  *datas, *lines, *datad, *lined, *lut;
l_uint32   sword, dword;
PIXCMAP   *cmap;
PIX       *pixd;

    PROCNAME("pixRemoveColormap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if ((cmap = pixGetColormap(pixs)) == NULL)
        return pixClone(pixs);

    if (type != REMOVE_CMAP_TO_BINARY &&
        type != REMOVE_CMAP_TO_GRAYSCALE &&
        type != REMOVE_CMAP_TO_FULL_COLOR &&
        type != REMOVE_CMAP_BASED_ON_SRC) {
        L_WARNING("Invalid type; converting based on src", procName);
        type = REMOVE_CMAP_BASED_ON_SRC;
    }

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("pixs must be {1,2,4,8} bpp", procName, NULL);

    if (pixcmapToArrays(cmap, &rmap, &gmap, &bmap))
        return (PIX *)ERROR_PTR("colormap arrays not made", procName, NULL);

    if (d != 1 && type == REMOVE_CMAP_TO_BINARY) {
        L_WARNING("not 1 bpp; can't remove cmap to binary", procName);
        type = REMOVE_CMAP_BASED_ON_SRC;
    }

    if (type == REMOVE_CMAP_BASED_ON_SRC) {
            /* select output type depending on colormap */
        pixcmapHasColor(cmap, &colorfound);
        if (!colorfound) {
            if (d == 1)
                type = REMOVE_CMAP_TO_BINARY;
            else
                type = REMOVE_CMAP_TO_GRAYSCALE;
        }
        else
            type = REMOVE_CMAP_TO_FULL_COLOR;
    }

    ncolors = pixcmapGetCount(cmap);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (type == REMOVE_CMAP_TO_BINARY) {
        if ((pixd = pixCopy(NULL, pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixcmapGetColor(cmap, 0, &rval, &gval, &bval);
        if (rval == 0)  /* photometrically inverted from standard */
            pixInvert(pixd, pixd);
        pixDestroyColormap(pixd);
    }
    else if (type == REMOVE_CMAP_TO_GRAYSCALE) {
        if ((pixd = pixCreate(w, h, 8)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixCopyResolution(pixd, pixs);
        datad = pixGetData(pixd);
        wpld = pixGetWpl(pixd);
        if ((graymap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL)
            return (PIX *)ERROR_PTR("calloc fail for graymap", procName, NULL);
        for (i = 0; i < pixcmapGetCount(cmap); i++) {
            graymap[i] = (rmap[i] + 2 * gmap[i] + bmap[i]) / 4;
        }
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            switch (d)   /* depth test above; no default permitted */
            {
                case 8:
                        /* Unrolled 4x */
                    for (j = 0, count = 0; j + 3 < w; j += 4, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 24) & 0xff] << 24) |
                            (graymap[(sword >> 16) & 0xff] << 16) |
                            (graymap[(sword >> 8) & 0xff] << 8) |
                            graymap[sword & 0xff];
                        lined[count] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_BYTE(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
#define CHECK_VALUE(a, b, c) if (GET_DATA_BYTE(a, b) != c) { \
    fprintf(stderr, "Error: mismatch at %d, %d vs %d\n", \
            j, GET_DATA_BYTE(a, b), c); }
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_BYTE(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 4:
                        /* Unrolled 8x */
                    for (j = 0, count = 0; j + 7 < w; j += 8, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 28) & 0xf] << 24) |
                            (graymap[(sword >> 24) & 0xf] << 16) |
                            (graymap[(sword >> 20) & 0xf] << 8) |
                            graymap[(sword >> 16) & 0xf];
                        lined[2 * count] = dword;
                        dword = (graymap[(sword >> 12) & 0xf] << 24) |
                            (graymap[(sword >> 8) & 0xf] << 16) |
                            (graymap[(sword >> 4) & 0xf] << 8) |
                            graymap[sword & 0xf];
                        lined[2 * count + 1] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_QBIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_QBIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 2:
                        /* Unrolled 16x */
                    for (j = 0, count = 0; j + 15 < w; j += 16, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 30) & 0x3] << 24) |
                            (graymap[(sword >> 28) & 0x3] << 16) |
                            (graymap[(sword >> 26) & 0x3] << 8) |
                            graymap[(sword >> 24) & 0x3];
                        lined[4 * count] = dword;
                        dword = (graymap[(sword >> 22) & 0x3] << 24) |
                            (graymap[(sword >> 20) & 0x3] << 16) |
                            (graymap[(sword >> 18) & 0x3] << 8) |
                            graymap[(sword >> 16) & 0x3];
                        lined[4 * count + 1] = dword;
                        dword = (graymap[(sword >> 14) & 0x3] << 24) |
                            (graymap[(sword >> 12) & 0x3] << 16) |
                            (graymap[(sword >> 10) & 0x3] << 8) |
                            graymap[(sword >> 8) & 0x3];
                        lined[4 * count + 2] = dword;
                        dword = (graymap[(sword >> 6) & 0x3] << 24) |
                            (graymap[(sword >> 4) & 0x3] << 16) |
                            (graymap[(sword >> 2) & 0x3] << 8) |
                            graymap[sword & 0x3];
                        lined[4 * count + 3] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_DIBIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_DIBIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 1:
                        /* Unrolled 8x */
                    for (j = 0, count = 0; j + 31 < w; j += 32, count++) {
                        sword = lines[count];
                        for (k = 0; k < 4; k++) {
                                /* The top byte is always the relevant one */
                            dword = (graymap[(sword >> 31) & 0x1] << 24) |
                                (graymap[(sword >> 30) & 0x1] << 16) |
                                (graymap[(sword >> 29) & 0x1] << 8) |
                                graymap[(sword >> 28) & 0x1];
                            lined[8 * count + 2 * k] = dword;
                            dword = (graymap[(sword >> 27) & 0x1] << 24) |
                                (graymap[(sword >> 26) & 0x1] << 16) |
                                (graymap[(sword >> 25) & 0x1] << 8) |
                                graymap[(sword >> 24) & 0x1];
                            lined[8 * count + 2 * k + 1] = dword;
                            sword <<= 8;  /* Move up the next byte */
                        }
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_BIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_BIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#undef CHECK_VALUE
#endif
                    break;
                default:
                    return NULL;
            }
        }
        if (graymap)
            FREE(graymap);
    }
    else {  /* type == REMOVE_CMAP_TO_FULL_COLOR */
        if ((pixd = pixCreate(w, h, 32)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixCopyResolution(pixd, pixs);
        datad = pixGetData(pixd);
        wpld = pixGetWpl(pixd);
        if ((lut = (l_uint32 *)CALLOC(ncolors, sizeof(l_uint32))) == NULL)
            return (PIX *)ERROR_PTR("calloc fail for lut", procName, NULL);
        for (i = 0; i < ncolors; i++)
            composeRGBPixel(rmap[i], gmap[i], bmap[i], lut + i);

        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                if (d == 8)
                    sval = GET_DATA_BYTE(lines, j);
                else if (d == 4)
                    sval = GET_DATA_QBIT(lines, j);
                else if (d == 2)
                    sval = GET_DATA_DIBIT(lines, j);
                else if (d == 1)
                    sval = GET_DATA_BIT(lines, j);
                else
                    return NULL;
                if (sval >= ncolors)
                    L_WARNING("pixel value out of bounds", procName);
                else
                    lined[j] = lut[sval];
            }
        }
        FREE(lut);
    }

    FREE(rmap);
    FREE(gmap);
    FREE(bmap);
    return pixd;
}


/*-------------------------------------------------------------*
 *              Add colormap losslessly (8 to 8)               *
 *-------------------------------------------------------------*/
/*!
 *  pixAddGrayColormap8()
 *
 *      Input:  pixs (8 bpp)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If pixs has a colormap, this is a no-op.
 */
LEPTONICA_EXPORT l_int32
pixAddGrayColormap8(PIX  *pixs)
{
PIXCMAP  *cmap;

    PROCNAME("pixAddGrayColormap8");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return 0;

    cmap = pixcmapCreateLinear(8, 256);
    pixSetColormap(pixs, cmap);
    return 0;
}


/*-------------------------------------------------------------*
 *            Conversion from RGB color to grayscale           *
 *-------------------------------------------------------------*/
/*!
 *  pixConvertRGBToLuminance()
 *
 *      Input:  pix (32 bpp RGB)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) Use a standard luminance conversion.
 */
LEPTONICA_EXPORT PIX *
pixConvertRGBToLuminance(PIX *pixs)
{
  return pixConvertRGBToGray(pixs, 0.0, 0.0, 0.0);
}


/*!
 *  pixConvertRGBToGray()
 *
 *      Input:  pix (32 bpp RGB)
 *              rwt, gwt, bwt  (non-negative; these should add to 1.0,
 *                              or use 0.0 for default)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) Use a weighted average of the RGB values.
 */
LEPTONICA_EXPORT PIX *
pixConvertRGBToGray(PIX       *pixs,
                    l_float32  rwt,
                    l_float32  gwt,
                    l_float32  bwt)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32   word;
l_uint32  *datas, *lines, *datad, *lined;
l_float32  sum;
PIX       *pixd;

    PROCNAME("pixConvertRGBToGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (rwt < 0.0 || gwt < 0.0 || bwt < 0.0)
        return (PIX *)ERROR_PTR("weights not all >= 0.0", procName, NULL);

        /* Make sure the sum of weights is 1.0; otherwise, you can get
         * overflow in the gray value. */
    if (rwt == 0.0 && gwt == 0.0 && bwt == 0.0) {
        rwt = L_RED_WEIGHT;
        gwt = L_GREEN_WEIGHT;
        bwt = L_BLUE_WEIGHT;
    }
    sum = rwt + gwt + bwt;
    if (L_ABS(sum - 1.0) > 0.0001) {  /* maintain ratios with sum == 1.0 */
        L_WARNING("weights don't sum to 1; maintaining ratios", procName);
        rwt = rwt / sum;
        gwt = gwt / sum;
        bwt = bwt / sum;
    }

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0, lines = datas, lined = datad; i < h; i++) {
        for (j = 0; j < w; j++) {
            word = *(lines + j);
            val = (l_int32)(rwt * ((word >> L_RED_SHIFT) & 0xff) +
                            gwt * ((word >> L_GREEN_SHIFT) & 0xff) +
                            bwt * ((word >> L_BLUE_SHIFT) & 0xff) + 0.5);
            SET_DATA_BYTE(lined, j, val);
        }
        lines += wpls;
        lined += wpld;
    }

    return pixd;
}


/*!
 *  pixConvertRGBToGrayFast()
 *
 *      Input:  pix (32 bpp RGB)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) This function should be used if speed of conversion
 *          is paramount, and the green channel can be used as
 *          a fair representative of the RGB intensity.  It is
 *          several times faster than pixConvertRGBToGray().
 *      (2) To combine RGB to gray conversion with subsampling,
 *          use pixScaleRGBToGrayFast() instead.
 */
LEPTONICA_REAL_EXPORT PIX *
pixConvertRGBToGrayFast(PIX  *pixs)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *datas, *lines, *datad, *lined;
PIX       *pixd;

    PROCNAME("pixConvertRGBToGrayFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++, lines++) {
            val = ((*lines) >> L_GREEN_SHIFT) & 0xff;
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}

/*---------------------------------------------------------------------------*
 *                Colorizing conversion from grayscale to color              *
 *---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*
 *                    Conversion from 16 bpp to 8 bpp                        *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert16To8()
 *     
 *      Input:  pixs (16 bpp)
 *              whichbyte (1 for MSB, 0 for LSB)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) For each dest pixel, use either the MSB or LSB of each src pixel.
 */
LEPTONICA_EXPORT PIX *
pixConvert16To8(PIX     *pixs,
                l_int32  whichbyte)
{
l_uint16   dsword;
l_int32    w, h, wpls, wpld, i, j;
l_uint32   sword;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixConvert16To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 16)
        return (PIX *)ERROR_PTR("pixs not 16 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Convert 2 pixels at a time */
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (whichbyte == 0) {  /* LSB */
            for (j = 0; j < wpls; j++) {
                sword = *(lines + j);
                dsword = ((sword >> 8) & 0xff00) | (sword & 0xff);
                SET_DATA_TWO_BYTES(lined, j, dsword);
            }
        }
        else {  /* MSB */
            for (j = 0; j < wpls; j++) {
                sword = *(lines + j);
                dsword = ((sword >> 16) & 0xff00) | ((sword >> 8) & 0xff);
                SET_DATA_TWO_BYTES(lined, j, dsword);
            }
        }
    }

    return pixd;
}
    


/*---------------------------------------------------------------------------*
 *         Unpacking conversion from 1 bpp to 2, 4, 8, 16 and 32 bpp         *
 *---------------------------------------------------------------------------*/

/*!
 *  pixConvert1To32()
 *
 *      Input:  pixd (<optional> 32 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (32 bit value to be used for 0s in pixs)
 *              val1 (32 bit value to be used for 1s in pixs)
 *      Return: pixd (32 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 */     
LEPTONICA_EXPORT PIX *
pixConvert1To32(PIX      *pixd,
                PIX      *pixs,
                l_uint32  val0,
                l_uint32  val1)
{
l_int32    w, h, i, j, wpls, wpld, bit;
l_uint32   val[2];
l_uint32  *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 32)
            return (PIX *)ERROR_PTR("pixd not 32 bpp", procName, pixd);
    }
    else {
        if ((pixd = pixCreate(w, h, 32)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

    val[0] = val0;
    val[1] = val1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j <w; j++) {
            bit = GET_DATA_BIT(lines, j);
            lined[j] = val[bit];
        }
    }

    return pixd;
}
 

/*---------------------------------------------------------------------------*
 *               Conversion from 1, 2 and 4 bpp to 8 bpp                     *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert1To8()
 *
 *      Input:  pixd (<optional> 8 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (8 bit value to be used for 0s in pixs)
 *              val1 (8 bit value to be used for 1s in pixs)
 *      Return: pixd (8 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 *      (3) A simple unpacking might use val0 = 0 and val1 = 255, or v.v.
 *      (4) In a typical application where one wants to use a colormap
 *          with the dest, you can use val0 = 0, val1 = 1 to make a
 *          non-cmapped 8 bpp pix, and then make a colormap and set 0
 *          and 1 to the desired colors.  Here is an example:
 *             pixd = pixConvert1To8(NULL, pixs, 0, 1);
 *             cmap = pixCreate(8);
 *             pixcmapAddColor(cmap, 255, 255, 255);
 *             pixcmapAddColor(cmap, 0, 0, 0);
 *             pixSetColormap(pixd, cmap);
 */     
LEPTONICA_EXPORT PIX *
pixConvert1To8(PIX     *pixd,
               PIX     *pixs,
               l_uint8  val0,
               l_uint8  val1)
{
l_int32    w, h, i, j, qbit, nqbits, wpls, wpld;
l_uint8    val[2];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 8)
            return (PIX *)ERROR_PTR("pixd not 8 bpp", procName, pixd);
    }
    else {
        if ((pixd = pixCreate(w, h, 8)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

        /* Use a table to convert 4 src bits at a time */
    if ((tab = (l_uint32 *)CALLOC(16, sizeof(l_uint32))) == NULL) 
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    for (index = 0; index < 16; index++) {
        tab[index] = (val[(index >> 3) & 1] << 24) |
                     (val[(index >> 2) & 1] << 16) |
                     (val[(index >> 1) & 1] << 8) | val[index & 1];
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    nqbits = (w + 3) / 4;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nqbits; j++) {
            qbit = GET_DATA_QBIT(lines, j);
            lined[j] = tab[qbit];
        }
    }

    FREE(tab);
    return pixd;
}
 

/*!
 *  pixConvert2To8()
 *
 *      Input:  pixs (2 bpp)
 *              val0 (8 bit value to be used for 00 in pixs)
 *              val1 (8 bit value to be used for 01 in pixs)
 *              val2 (8 bit value to be used for 10 in pixs)
 *              val3 (8 bit value to be used for 11 in pixs)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      - A simple unpacking might use val0 = 0,
 *        val1 = 85 (0x55), val2 = 170 (0xaa), val3 = 255.
 *      - If cmapflag is TRUE:
 *          - The 8 bpp image is made with a colormap.
 *          - If pixs has a colormap, the input values are ignored and
 *            the 8 bpp image is made using the colormap
 *          - If pixs does not have a colormap, the input values are
 *            used to build the colormap.
 *      - If cmapflag is FALSE:
 *          - The 8 bpp image is made without a colormap.
 *          - If pixs has a colormap, the input values are ignored,
 *            the colormap is removed, and the values stored in the 8 bpp
 *            image are from the colormap.
 *          - If pixs does not have a colormap, the input values are
 *            used to populate the 8 bpp image.
 */     
LEPTONICA_EXPORT PIX *
pixConvert2To8(PIX     *pixs,
               l_uint8  val0,
               l_uint8  val1,
               l_uint8  val2,
               l_uint8  val3,
               l_int32  cmapflag)
{
l_int32    w, h, i, j, nbytes, wpls, wpld, dibit, ncolor;
l_int32    rval, gval, bval, byte;
l_uint8    val[4];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;
PIX       *pixd;
PIXCMAP   *cmaps, *cmapd;

    PROCNAME("pixConvert2To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 2)
        return (PIX *)ERROR_PTR("pixs not 2 bpp", procName, NULL);

    cmaps = pixGetColormap(pixs);
    if (cmaps && cmapflag == FALSE)
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    if (cmapflag == TRUE) {  /* pixd will have a colormap */
        cmapd = pixcmapCreate(8);  /* 8 bpp standard cmap */
        if (cmaps) {  /* use the existing colormap from pixs */
            ncolor = pixcmapGetCount(cmaps);
            for (i = 0; i < ncolor; i++) {
                pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
                pixcmapAddColor(cmapd, rval, gval, bval);
            }
        }
        else {  /* make a colormap from the input values */
            pixcmapAddColor(cmapd, val0, val0, val0);
            pixcmapAddColor(cmapd, val1, val1, val1);
            pixcmapAddColor(cmapd, val2, val2, val2);
            pixcmapAddColor(cmapd, val3, val3, val3);
        }
        pixSetColormap(pixd, cmapd);
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                dibit = GET_DATA_DIBIT(lines, j);
                SET_DATA_BYTE(lined, j, dibit);
            }
        }
        return pixd;
    }

        /* Last case: no colormap in either pixs or pixd.
         * Use input values and build a table to convert 1 src byte
         * (4 src pixels) at a time */
    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL) 
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    val[2] = val2;
    val[3] = val3;
    for (index = 0; index < 256; index++) {
        tab[index] = (val[(index >> 6) & 3] << 24) |
                     (val[(index >> 4) & 3] << 16) |
                     (val[(index >> 2) & 3] << 8) | val[index & 3];
    }

    nbytes = (w + 3) / 4;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nbytes; j++) {
            byte = GET_DATA_BYTE(lines, j);
            lined[j] = tab[byte];
        }
    }

    FREE(tab);
    return pixd;
}
 

/*!
 *  pixConvert4To8()
 *
 *      Input:  pixs (4 bpp)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      - If cmapflag is TRUE:
 *          - pixd is made with a colormap.
 *          - If pixs has a colormap, it is copied and the colormap
 *            index values are placed in pixd.
 *          - If pixs does not have a colormap, a colormap with linear
 *            trc is built and the pixel values in pixs are placed in
 *            pixd as colormap index values.
 *      - If cmapflag is FALSE:
 *          - pixd is made without a colormap.
 *          - If pixs has a colormap, it is removed and the values stored
 *            in pixd are from the colormap (converted to gray).
 *          - If pixs does not have a colormap, the pixel values in pixs
 *            are used, with shift replication, to populate pixd.
 */     
LEPTONICA_EXPORT PIX *
pixConvert4To8(PIX     *pixs,
               l_int32  cmapflag)
{
l_int32    w, h, i, j, wpls, wpld, ncolor;
l_int32    rval, gval, bval, byte, qbit;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;
PIXCMAP   *cmaps, *cmapd;

    PROCNAME("pixConvert4To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 4)
        return (PIX *)ERROR_PTR("pixs not 4 bpp", procName, NULL);

    cmaps = pixGetColormap(pixs);
    if (cmaps && cmapflag == FALSE)
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    if (cmapflag == TRUE) {  /* pixd will have a colormap */
        cmapd = pixcmapCreate(8);
        if (cmaps) {  /* use the existing colormap from pixs */
            ncolor = pixcmapGetCount(cmaps);
            for (i = 0; i < ncolor; i++) {
                pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
                pixcmapAddColor(cmapd, rval, gval, bval);
            }
        }
        else {  /* make a colormap with a linear trc */
            for (i = 0; i < 16; i++) 
                pixcmapAddColor(cmapd, 17 * i, 17 * i, 17 * i);
        }
        pixSetColormap(pixd, cmapd);
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                qbit = GET_DATA_QBIT(lines, j);
                SET_DATA_BYTE(lined, j, qbit);
            }
        }
        return pixd;
    }

        /* Last case: no colormap in either pixs or pixd.
         * Replicate the qbit value into 8 bits. */
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            qbit = GET_DATA_QBIT(lines, j);
            byte = (qbit << 4) | qbit;
            SET_DATA_BYTE(lined, j, byte);
        }
    }
    return pixd;
}

/*---------------------------------------------------------------------------*
 *                     Top-level conversion to 8 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo8()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a top-level function, with simple default values
 *          for unpacking.
 *      (2) The result, pixd, is made with a colormap if specified.
 *      (3) If d == 8, and cmapflag matches the existence of a cmap
 *          in pixs, the operation is lossless and it returns a copy.
 *      (4) The default values used are:
 *          - 1 bpp: val0 = 255, val1 = 0
 *          - 2 bpp: 4 bpp:  even increments over dynamic range
 *          - 8 bpp: lossless if cmap matches cmapflag
 *          - 16 bpp: use most significant byte
 *      (5) If 32 bpp RGB, this is converted to gray.  If you want
 *          to do color quantization, you must specify the type
 *          explicitly, using the color quantization code.
 */     
LEPTONICA_EXPORT PIX *
pixConvertTo8(PIX     *pixs,
              l_int32  cmapflag)
{
l_int32   d;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvertTo8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not {1,2,4,8,16,32}", procName, NULL);

    if (d == 1) {
        if (!cmapflag)
            return pixConvert1To8(NULL, pixs, 255, 0);
        else {
            pixd = pixConvert1To8(NULL, pixs, 0, 1);
            cmap = pixcmapCreate(8);
            pixcmapAddColor(cmap, 255, 255, 255);
            pixcmapAddColor(cmap, 0, 0, 0);
            pixSetColormap(pixd, cmap);
            return pixd;
        }
    }
    else if (d == 2)
        return pixConvert2To8(pixs, 0, 85, 170, 255, cmapflag);
    else if (d == 4)
        return pixConvert4To8(pixs, cmapflag);
    else if (d == 8) {
        cmap = pixGetColormap(pixs);
        if ((cmap && cmapflag) || (!cmap && !cmapflag))
            return pixCopy(NULL, pixs);
        else if (cmap)  /* !cmapflag */
            return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
        else {  /* !cmap && cmapflag; add colormap to pixd */
            pixd = pixCopy(NULL, pixs);
            pixAddGrayColormap8(pixd);
            return pixd;
        }
    }
    else if (d == 16) {
        pixd = pixConvert16To8(pixs, 1);
        if (cmapflag)
            pixAddGrayColormap8(pixd);
        return pixd;
    }
    else { /* d == 32 */
        pixd = pixConvertRGBToLuminance(pixs);
        if (cmapflag)
            pixAddGrayColormap8(pixd);
        return pixd;
    }
}


/*---------------------------------------------------------------------------*
 *                    Top-level conversion to 32 bpp                         *
 *---------------------------------------------------------------------------*/

/*!
 *  pixConvert8To32()
 *
 *      Input:  pix (8 bpp)
 *      Return: 32 bpp rgb pix, or null on error
 *
 *  Notes:
 *      (1) If there is no colormap, replicates the gray value
 *          into the 3 MSB of the dest pixel.
 *      (2) Implicit assumption about RGB component ordering.
 */
LEPTONICA_EXPORT PIX *
pixConvert8To32(PIX  *pixs)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *datas, *datad, *lines, *lined;
l_uint32  *tab;
PIX       *pixd;

    PROCNAME("pixConvert8To32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    if (pixGetColormap(pixs))
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);

        /* Replication table */
    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    for (i = 0; i < 256; i++)
      tab[i] = (i << 24) | (i << 16) | (i << 8);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            lined[j] = tab[val];
        }
    }

    FREE(tab);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                 Conversion between 24 bpp and 32 bpp rgb                  *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert24To32()
 *
 *      Input:  pixs (24 bpp rgb)
 *      Return: pixd (32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) 24 bpp rgb pix are not supported in leptonica.
 *          The data is a byte array, with pixels in order r,g,b, and
 *          padded to 32 bit boundaries in each line.
 *      (2) Because they are conveniently generated by programs
 *          such as xpdf, we need to provide the ability to write them
 *          in png, jpeg and tiff, as well as to convert between 24 and
 *          32 bpp in memory.
 */
LEPTONICA_EXPORT PIX *
pixConvert24To32(PIX  *pixs)
{
l_uint8   *lines;
l_int32    w, h, d, i, j, wpls, wpld, rval, gval, bval;
l_uint32   pixel;
l_uint32  *datas, *datad, *lined;
PIX       *pixd;

    PROCNAME("pixConvert24to32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 24)
        return (PIX *)ERROR_PTR("pixs not 24 bpp", procName, NULL);

    pixd = pixCreateNoInit(w, h, 32);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = (l_uint8 *)(datas + i * wpls);
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            rval = *lines++;
            gval = *lines++;
            bval = *lines++;
            composeRGBPixel(rval, gval, bval, &pixel);
            lined[j] = pixel;
        }
    }
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    return pixd;
}
