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
 *            Conversion from RGB color to grayscale           *
 *-------------------------------------------------------------*/

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
