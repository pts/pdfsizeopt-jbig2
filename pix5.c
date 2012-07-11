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
 *  pix5.c
 *
 *    This file has these operations:
 *
 *      (1) Measurement of 1 bpp image properties
 *      (2) Extract rectangular region
 *      (3) Clip to foreground
 *      (4) Extract pixel averages and reversals along lines
 *      (5) Rank row and column transforms
 *
 *    Measurement of properties
 *           l_int32     pixaFindDimensions()
 *           NUMA       *pixaFindAreaPerimRatio()
 *           l_int32     pixFindAreaPerimRatio()
 *           NUMA       *pixaFindPerimSizeRatio()
 *           l_int32     pixFindPerimSizeRatio()
 *           NUMA       *pixaFindAreaFraction()
 *           l_int32     pixFindAreaFraction()
 *           NUMA       *pixaFindWidthHeightRatio()
 *           NUMA       *pixaFindWidthHeightProduct()
 *           l_int32     pixFindOverlapFraction()
 *           BOXA       *pixFindRectangleComps()
 *           l_int32     pixConformsToRectangle()
 *
 *    Extract rectangular region
 *           PIX        *pixClipRectangle()
 *           PIX        *pixClipMasked()
 *           PIX        *pixResizeToMatch()
 *
 *    Clip to foreground
 *           PIX        *pixClipToForeground()
 *           l_int32     pixClipBoxToForeground()
 *           l_int32     pixScanForForeground()
 *           l_int32     pixClipBoxToEdges()
 *           l_int32     pixScanForEdge()
 *
 *    Extract pixel averages and reversals along lines
 *           NUMA       *pixExtractOnLine()
 *           l_float32   pixAverageOnLine();
 *           NUMA       *pixAverageIntensityProfile()
 *           NUMA       *pixReversalProfile()
 *
 *    Rank row and column transforms
 *           PIX        *pixRankRowTransform()
 *           PIX        *pixRankColumnTransform()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

#if !RMASK32_DEFINED
#define RMASK32_DEFINED 1
static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
#endif

#ifndef  NO_CONSOLE_IO
#define  DEBUG_EDGES         0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                 Measurement of properties                   *
 *-------------------------------------------------------------*/
/*!
 *  pixaFindDimensions()
 *
 *      Input:  pixa
 *              &naw (<optional return> numa of pix widths)
 *              &nah (<optional return> numa of pix heights)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaFindDimensions(PIXA   *pixa,
                   NUMA  **pnaw,
                   NUMA  **pnah)
{
l_int32  i, n, w, h;
PIX     *pixt;

    PROCNAME("pixaFindDimensions");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pnaw && !pnah)
        return 0;

    n = pixaGetCount(pixa);
    if (pnaw) *pnaw = numaCreate(n);
    if (pnah) *pnah = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        if (pnaw)
            numaAddNumber(*pnaw, w);
        if (pnah)
            numaAddNumber(*pnah, h);
        pixDestroy(&pixt);
    }
    return 0;
}


/*!
 *  pixaFindAreaPerimRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of area/perimeter ratio for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT NUMA *
pixaFindAreaPerimRatio(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  fract;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindAreaPerimRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindAreaPerimRatio(pixt, tab, &fract);
        numaAddNumber(na, fract);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindAreaPerimRatio()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> area/perimeter ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The area is the number of fg pixels that are not on the
 *          boundary (i.e., not 8-connected to a bg pixel), and the
 *          perimeter is the number of boundary fg pixels.
 *      (2) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT l_int32
pixFindAreaPerimRatio(PIX        *pixs,
                      l_int32    *tab,
                      l_float32  *pfract)
{
l_int32  *tab8;
l_int32   nin, nbound;
PIX      *pixt;

    PROCNAME("pixFindAreaPerimRatio");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixt = pixErodeBrick(NULL, pixs, 3, 3);
    pixCountPixels(pixt, &nin, tab8);
    pixXor(pixt, pixt, pixs);
    pixCountPixels(pixt, &nbound, tab8);
    *pfract = (l_float32)nin / (l_float32)nbound;

    if (!tab)
        FREE(tab8);
    pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixaFindPerimSizeRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of fg perimeter/(w*h) ratio for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT NUMA *
pixaFindPerimSizeRatio(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  ratio;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindPerimSizeRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindPerimSizeRatio(pixt, tab, &ratio);
        numaAddNumber(na, ratio);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindPerimSizeRatio()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &ratio (<return> perimeter/size ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The size is the sum of the width and height of the pix,
 *          and the perimeter is the number of boundary fg pixels.
 *      (2) This has a large value for dendritic, fractal-like components
 *          with highly irregular boundaries.
 *      (3) This is typically used for a single connected component.
 */
LEPTONICA_EXPORT l_int32
pixFindPerimSizeRatio(PIX        *pixs,
                      l_int32    *tab,
                      l_float32  *pratio)
{
l_int32  *tab8;
l_int32   w, h, nbound;
PIX      *pixt;

    PROCNAME("pixFindPerimSizeRatio");

    if (!pratio)
        return ERROR_INT("&ratio not defined", procName, 1);
    *pratio = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixt = pixErodeBrick(NULL, pixs, 3, 3);
    pixXor(pixt, pixt, pixs);
    pixCountPixels(pixt, &nbound, tab8);
    pixGetDimensions(pixs, &w, &h, NULL);
    *pratio = (l_float32)nbound / (l_float32)(w + h);

    if (!tab)
        FREE(tab8);
    pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixaFindAreaFraction()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of area fractions for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT NUMA *
pixaFindAreaFraction(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  fract;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindAreaFraction");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindAreaFraction(pixt, tab, &fract);
        numaAddNumber(na, fract);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindAreaFraction()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> fg area/size ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This finds the ratio of the number of fg pixels to the
 *          size of the pix (w * h).  It is typically used for a
 *          single connected component.
 */
LEPTONICA_EXPORT l_int32
pixFindAreaFraction(PIX        *pixs,
                    l_int32    *tab,
                    l_float32  *pfract)
{
l_int32   w, h, d, sum;
l_int32  *tab8;

    PROCNAME("pixFindAreaFraction");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    pixGetDimensions(pixs, &w, &h, &d);
    if (!pixs || d != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixCountPixels(pixs, &sum, tab8);
    *pfract = (l_float32)sum / (l_float32)(w * h);

    if (!tab)
        FREE(tab8);
    return 0;
}


/*!
 *  pixaFindWidthHeightRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of width/height ratios for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT NUMA *
pixaFindWidthHeightRatio(PIXA  *pixa)
{
l_int32  i, n, w, h;
NUMA    *na;
PIX     *pixt;

    PROCNAME("pixaFindWidthHeightRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        numaAddNumber(na, (l_float32)w / (l_float32)h);
        pixDestroy(&pixt);
    }
    return na;
}


/*!
 *  pixaFindWidthHeightProduct()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of width*height products for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
LEPTONICA_EXPORT NUMA *
pixaFindWidthHeightProduct(PIXA  *pixa)
{
l_int32  i, n, w, h;
NUMA    *na;
PIX     *pixt;

    PROCNAME("pixaFindWidthHeightProduct");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        numaAddNumber(na, w * h);
        pixDestroy(&pixt);
    }
    return na;
}


/*!
 *  pixFindOverlapFraction()
 *
 *      Input:  pixs1, pixs2 (1 bpp)
 *              x2, y2 (location in pixs1 of UL corner of pixs2)
 *              tab (<optional> pixel sum table, can be null)
 *              &ratio (<return> ratio fg intersection to fg union)
 *              &noverlap (<optional return> number of overlapping pixels)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The UL corner of pixs2 is placed at (x2, y2) in pixs1.
 *      (2) This measure is similar to the correlation.
 */
LEPTONICA_EXPORT l_int32
pixFindOverlapFraction(PIX        *pixs1,
                       PIX        *pixs2,
                       l_int32     x2,
                       l_int32     y2,
                       l_int32    *tab,
                       l_float32  *pratio,
                       l_int32    *pnoverlap)
{
l_int32  *tab8;
l_int32   w, h, nintersect, nunion;
PIX      *pixt;

    PROCNAME("pixFindOverlapFraction");

    if (!pratio)
        return ERROR_INT("&ratio not defined", procName, 1);
    *pratio = 0.0;
    if (!pixs1 || pixGetDepth(pixs1) != 1)
        return ERROR_INT("pixs1 not defined or not 1 bpp", procName, 1);
    if (!pixs2 || pixGetDepth(pixs2) != 1)
        return ERROR_INT("pixs2 not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixGetDimensions(pixs2, &w, &h, NULL);
    pixt = pixCopy(NULL, pixs1);
    pixRasterop(pixt, x2, y2, w, h, PIX_MASK, pixs2, 0, 0);  /* AND */
    pixCountPixels(pixt, &nintersect, tab8);
    if (pnoverlap)
        *pnoverlap = nintersect;
    pixCopy(pixt, pixs1);
    pixRasterop(pixt, x2, y2, w, h, PIX_PAINT, pixs2, 0, 0);  /* OR */
    pixCountPixels(pixt, &nunion, tab8);
    *pratio = (l_float32)nintersect / (l_float32)nunion;

    if (!tab)
        FREE(tab8);
    pixDestroy(&pixt);
    return 0;
}


/*-------------------------------------------------------------*
 *                Extract rectangular region                   *
 *-------------------------------------------------------------*/
/*!
 *  pixClipRectangle()
 *
 *      Input:  pixs
 *              box  (requested clipping region; const)
 *              &boxc (<optional return> actual box of clipped region)
 *      Return: clipped pix, or null on error or if rectangle
 *              doesn't intersect pixs
 *
 *  Notes:
 *
 *  This should be simple, but there are choices to be made.
 *  The box is defined relative to the pix coordinates.  However,
 *  if the box is not contained within the pix, we have two choices:
 *
 *      (1) clip the box to the pix
 *      (2) make a new pix equal to the full box dimensions,
 *          but let rasterop do the clipping and positioning
 *          of the src with respect to the dest
 *
 *  Choice (2) immediately brings up the problem of what pixel values
 *  to use that were not taken from the src.  For example, on a grayscale
 *  image, do you want the pixels not taken from the src to be black
 *  or white or something else?  To implement choice 2, one needs to
 *  specify the color of these extra pixels.
 *
 *  So we adopt (1), and clip the box first, if necessary,
 *  before making the dest pix and doing the rasterop.  But there
 *  is another issue to consider.  If you want to paste the
 *  clipped pix back into pixs, it must be properly aligned, and
 *  it is necessary to use the clipped box for alignment.
 *  Accordingly, this function has a third (optional) argument, which is
 *  the input box clipped to the src pix.
 */
LEPTONICA_EXPORT PIX *
pixClipRectangle(PIX   *pixs,
                 BOX   *box,
                 BOX  **pboxc)
{
l_int32  w, h, d, bx, by, bw, bh;
BOX     *boxc;
PIX     *pixd;

    PROCNAME("pixClipRectangle");

    if (pboxc)
        *pboxc = NULL;
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!box)
        return (PIX *)ERROR_PTR("box not defined", procName, NULL);

        /* Clip the input box to the pix */
    pixGetDimensions(pixs, &w, &h, &d);
    if ((boxc = boxClipToRectangle(box, w, h)) == NULL) {
        L_WARNING("box doesn't overlap pix", procName);
        return NULL;
    }
    boxGetGeometry(boxc, &bx, &by, &bw, &bh);

        /* Extract the block */
    if ((pixd = pixCreate(bw, bh, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixRasterop(pixd, 0, 0, bw, bh, PIX_SRC, pixs, bx, by);

    if (pboxc)
        *pboxc = boxc;
    else
        boxDestroy(&boxc);

    return pixd;
}


/*!
 *  pixResizeToMatch()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp; colormap ok)
 *              pixt  (can be null; we use only the size)
 *              w, h (ignored if pixt is defined)
 *      Return: pixd (resized to match) or null on error
 *
 *  Notes:
 *      (1) This resizes pixs to make pixd, without scaling, by either
 *          cropping or extending separately in both width and height.
 *          Extension is done by replicating the last row or column.
 *          This is useful in a situation where, due to scaling
 *          operations, two images that are expected to be the
 *          same size can differ slightly in each dimension.
 *      (2) You can use either an existing pixt or specify
 *          both @w and @h.  If pixt is defined, the values
 *          in @w and @h are ignored.
 *      (3) If pixt is larger than pixs (or if w and/or d is larger
 *          than the dimension of pixs, replicate the outer row and
 *          column of pixels in pixs into pixd.
 */
LEPTONICA_EXPORT PIX *
pixResizeToMatch(PIX     *pixs,
                 PIX     *pixt,
                 l_int32  w,
                 l_int32  h)
{
l_int32  i, j, ws, hs, d;
PIX     *pixd;

    PROCNAME("pixResizeToMatch");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixt && (w <= 0 || h <= 0))
        return (PIX *)ERROR_PTR("both w and h not > 0", procName, NULL);

    if (pixt)  /* redefine w, h */
        pixGetDimensions(pixt, &w, &h, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (ws == w && hs == h)
        return pixCopy(NULL, pixs);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    pixRasterop(pixd, 0, 0, ws, hs, PIX_SRC, pixs, 0, 0);
    if (ws >= w && hs >= h)
        return pixd;

        /* Replicate the last column and then the last row */
    if (ws < w) {
        for (j = ws; j < w; j++)
            pixRasterop(pixd, j, 0, 1, h, PIX_SRC, pixd, ws - 1, 0);
    }
    if (hs < h) {
        for (i = hs; i < h; i++)
            pixRasterop(pixd, 0, i, w, 1, PIX_SRC, pixd, 0, hs - 1);
    }

    return pixd;
}


/*---------------------------------------------------------------------*
 *                           Clip to Foreground                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixClipToForeground()
 *
 *      Input:  pixs (1 bpp)
 *              &pixd  (<optional return> clipped pix returned)
 *              &box   (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if there are no fg pixels
 *
 *  Notes:
 *      (1) At least one of {&pixd, &box} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 */
LEPTONICA_EXPORT l_int32
pixClipToForeground(PIX   *pixs,
                    PIX  **ppixd,
                    BOX  **pbox)
{
l_int32    w, h, wpl, nfullwords, extra, i, j;
l_int32    minx, miny, maxx, maxy;
l_uint32   result, mask;
l_uint32  *data, *line;
BOX       *box;

    PROCNAME("pixClipToForeground");

    if (!ppixd && !pbox)
        return ERROR_INT("neither &pixd nor &box defined", procName, 1);
    if (ppixd)
        *ppixd = NULL;
    if (pbox)
        *pbox = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    nfullwords = w / 32;
    extra = w & 31;
    mask = ~rmask32[32 - extra];
    wpl = pixGetWpl(pixs);
    data = pixGetData(pixs);

    result = 0;
    for (i = 0, miny = 0; i < h; i++, miny++) {
        line = data + i * wpl;
        for (j = 0; j < nfullwords; j++)
            result |= line[j];
        if (extra)
            result |= (line[j] & mask);
        if (result)
            break;
    }
    if (miny == h)  /* no ON pixels */
        return 1;

    result = 0;
    for (i = h - 1, maxy = h - 1; i >= 0; i--, maxy--) {
        line = data + i * wpl;
        for (j = 0; j < nfullwords; j++)
            result |= line[j];
        if (extra)
            result |= (line[j] & mask);
        if (result)
            break;
    }

    minx = 0;
    for (j = 0, minx = 0; j < w; j++, minx++) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            if (GET_DATA_BIT(line, j))
                goto minx_found;
        }
    }

minx_found:
    for (j = w - 1, maxx = w - 1; j >= 0; j--, maxx--) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            if (GET_DATA_BIT(line, j))
                goto maxx_found;
        }
    }

maxx_found:
    box = boxCreate(minx, miny, maxx - minx + 1, maxy - miny + 1);

    if (ppixd)
        *ppixd = pixClipRectangle(pixs, box, NULL);
    if (pbox)
        *pbox = box;
    else
        boxDestroy(&box);

    return 0;
}


/*!
 *  pixClipBoxToForeground()
 *
 *      Input:  pixs (1 bpp)
 *              boxs  (<optional> ; use full image if null)
 *              &pixd  (<optional return> clipped pix returned)
 *              &boxd  (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if there are no fg pixels
 *
 *  Notes:
 *      (1) At least one of {&pixd, &boxd} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 *      (3) Do not use &pixs for the 3rd arg or &boxs for the 4th arg;
 *          this will leak memory.
 */
LEPTONICA_EXPORT l_int32
pixClipBoxToForeground(PIX   *pixs,
                       BOX   *boxs,
                       PIX  **ppixd,
                       BOX  **pboxd)
{
l_int32  w, h, bx, by, bw, bh, cbw, cbh, left, right, top, bottom;
BOX     *boxt, *boxd;

    PROCNAME("pixClipBoxToForeground");

    if (!ppixd && !pboxd)
        return ERROR_INT("neither &pixd nor &boxd defined", procName, 1);
    if (ppixd) *ppixd = NULL;
    if (pboxd) *pboxd = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!boxs)
        return pixClipToForeground(pixs, ppixd, pboxd);

    pixGetDimensions(pixs, &w, &h, NULL);
    boxGetGeometry(boxs, &bx, &by, &bw, &bh);
    cbw = L_MIN(bw, w - bx);
    cbh = L_MIN(bh, h - by);
    if (cbw < 0 || cbh < 0)
        return ERROR_INT("box not within image", procName, 1);
    boxt = boxCreate(bx, by, cbw, cbh);

    if (pixScanForForeground(pixs, boxt, L_FROM_LEFT, &left)) {
        boxDestroy(&boxt);
        return 1;
    }
    pixScanForForeground(pixs, boxt, L_FROM_RIGHT, &right);
    pixScanForForeground(pixs, boxt, L_FROM_TOP, &top);
    pixScanForForeground(pixs, boxt, L_FROM_BOTTOM, &bottom);

    boxd = boxCreate(left, top, right - left + 1, bottom - top + 1);
    if (ppixd)
        *ppixd = pixClipRectangle(pixs, boxd, NULL);
    if (pboxd)
        *pboxd = boxd;
    else
        boxDestroy(&boxd);

    boxDestroy(&boxt);
    return 0;
}


/*!
 *  pixScanForForeground()
 *
 *      Input:  pixs (1 bpp)
 *              box  (<optional> within which the search is conducted)
 *              scanflag (direction of scan; e.g., L_FROM_LEFT)
 *              &loc (location in scan direction of first black pixel)
 *      Return: 0 if OK; 1 on error or if no fg pixels are found
 *
 *  Notes:
 *      (1) If there are no fg pixels, the position is set to 0.
 *          Caller must check the return value!
 *      (2) Use @box == NULL to scan from edge of pixs
 */
LEPTONICA_EXPORT l_int32
pixScanForForeground(PIX      *pixs,
                     BOX      *box,
                     l_int32   scanflag,
                     l_int32  *ploc)
{
l_int32    bx, by, bw, bh, x, xstart, xend, y, ystart, yend, wpl;
l_uint32  *data, *line;
BOX       *boxt;

    PROCNAME("pixScanForForeground");

    if (!ploc)
        return ERROR_INT("&ploc not defined", procName, 1);
    *ploc = 0;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

        /* Clip box to pixs if it exists */
    pixGetDimensions(pixs, &bw, &bh, NULL);
    if (box) {
        if ((boxt = boxClipToRectangle(box, bw, bh)) == NULL)
            return ERROR_INT("invalid box", procName, 1);
        boxGetGeometry(boxt, &bx, &by, &bw, &bh);
        boxDestroy(&boxt);
    }
    else
        bx = by = 0;
    xstart = bx;
    ystart = by;
    xend = bx + bw - 1;
    yend = by + bh - 1;

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    if (scanflag == L_FROM_LEFT) {
        for (x = xstart; x <= xend; x++) {
            for (y = ystart; y <= yend; y++) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x)) {
                    *ploc = x;
                    return 0;
                }
            }
        }
    }
    else if (scanflag == L_FROM_RIGHT) {
        for (x = xend; x >= xstart; x--) {
            for (y = ystart; y <= yend; y++) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x)) {
                    *ploc = x;
                    return 0;
                }
            }
        }
    }
    else if (scanflag == L_FROM_TOP) {
        for (y = ystart; y <= yend; y++) {
            line = data + y * wpl;
            for (x = xstart; x <= xend; x++) {
                if (GET_DATA_BIT(line, x)) {
                    *ploc = y;
                    return 0;
                }
            }
        }
    }
    else if (scanflag == L_FROM_BOTTOM) {
        for (y = yend; y >= ystart; y--) {
            line = data + y * wpl;
            for (x = xstart; x <= xend; x++) {
                if (GET_DATA_BIT(line, x)) {
                    *ploc = y;
                    return 0;
                }
            }
        }
    }
    else
        return ERROR_INT("invalid scanflag", procName, 1);

    return 1;  /* no fg found */
}


/*!
 *  pixClipBoxToEdges()
 *
 *      Input:  pixs (1 bpp)
 *              boxs  (<optional> ; use full image if null)
 *              lowthresh (threshold to choose clipping location)
 *              highthresh (threshold required to find an edge)
 *              maxwidth (max allowed width between low and high thresh locs)
 *              factor (sampling factor along pixel counting direction)
 *              &pixd  (<optional return> clipped pix returned)
 *              &boxd  (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if a fg edge is not found from
 *              all four sides.
 *
 *  Notes:
 *      (1) At least one of {&pixd, &boxd} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 *      (3) This function attempts to locate rectangular "image" regions
 *          of high-density fg pixels, that have well-defined edges
 *          on the four sides.
 *      (4) Edges are searched for on each side, iterating in order
 *          from left, right, top and bottom.  As each new edge is
 *          found, the search box is resized to use that location.
 *          Once an edge is found, it is held.  If no more edges
 *          are found in one iteration, the search fails.
 *      (5) See pixScanForEdge() for usage of the thresholds and @maxwidth.
 *      (6) The thresholds must be at least 1, and the low threshold
 *          cannot be larger than the high threshold.
 *      (7) If the low and high thresholds are both 1, this is equivalent
 *          to pixClipBoxToForeground().
 */
LEPTONICA_EXPORT l_int32
pixClipBoxToEdges(PIX     *pixs,
                  BOX     *boxs,
                  l_int32  lowthresh,
                  l_int32  highthresh,
                  l_int32  maxwidth,
                  l_int32  factor,
                  PIX    **ppixd,
                  BOX    **pboxd)
{
l_int32  w, h, bx, by, bw, bh, cbw, cbh, left, right, top, bottom;
l_int32  lfound, rfound, tfound, bfound, change;
BOX     *boxt, *boxd;

    PROCNAME("pixClipBoxToEdges");

    if (!ppixd && !pboxd)
        return ERROR_INT("neither &pixd nor &boxd defined", procName, 1);
    if (ppixd) *ppixd = NULL;
    if (pboxd) *pboxd = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (lowthresh < 1 || highthresh < 1 ||
        lowthresh > highthresh || maxwidth < 1)
        return ERROR_INT("invalid thresholds", procName, 1);
    factor = L_MIN(1, factor);

    if (lowthresh == 1 && highthresh == 1)
        return pixClipBoxToForeground(pixs, boxs, ppixd, pboxd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (boxs) {
        boxGetGeometry(boxs, &bx, &by, &bw, &bh);
        cbw = L_MIN(bw, w - bx);
        cbh = L_MIN(bh, h - by);
        if (cbw < 0 || cbh < 0)
            return ERROR_INT("box not within image", procName, 1);
        boxt = boxCreate(bx, by, cbw, cbh);
    }
    else
        boxt = boxCreate(0, 0, w, h);

    lfound = rfound = tfound = bfound = 0;
    while (!lfound || !rfound || !tfound || !bfound) {
        change = 0;
        if (!lfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_LEFT, &left)) {
                lfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, left, L_FROM_LEFT);
            }
        }
        if (!rfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_RIGHT, &right)) {
                rfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, right, L_FROM_RIGHT);
            }
        }
        if (!tfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_TOP, &top)) {
                tfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, top, L_FROM_TOP);
            }
        }
        if (!bfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_BOTTOM, &bottom)) {
                bfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, bottom, L_FROM_BOTTOM);
            }
        }

#if DEBUG_EDGES
        fprintf(stderr, "iter: %d %d %d %d\n", lfound, rfound, tfound, bfound);
#endif  /* DEBUG_EDGES */

        if (change == 0) break;
    }
    boxDestroy(&boxt);

    if (change == 0)
        return ERROR_INT("not all edges found", procName, 1);

    boxd = boxCreate(left, top, right - left + 1, bottom - top + 1);
    if (ppixd)
        *ppixd = pixClipRectangle(pixs, boxd, NULL);
    if (pboxd)
        *pboxd = boxd;
    else
        boxDestroy(&boxd);

    return 0;
}


/*!
 *  pixScanForEdge()
 *
 *      Input:  pixs (1 bpp)
 *              box  (<optional> within which the search is conducted)
 *              lowthresh (threshold to choose clipping location)
 *              highthresh (threshold required to find an edge)
 *              maxwidth (max allowed width between low and high thresh locs)
 *              factor (sampling factor along pixel counting direction)
 *              scanflag (direction of scan; e.g., L_FROM_LEFT)
 *              &loc (location in scan direction of first black pixel)
 *      Return: 0 if OK; 1 on error or if the edge is not found
 *
 *  Notes:
 *      (1) If there are no fg pixels, the position is set to 0.
 *          Caller must check the return value!
 *      (2) Use @box == NULL to scan from edge of pixs
 *      (3) As the scan progresses, the location where the sum of
 *          pixels equals or excees @lowthresh is noted (loc).  The
 *          scan is stopped when the sum of pixels equals or exceeds
 *          @highthresh.  If the scan distance between loc and that
 *          point does not exceed @maxwidth, an edge is found and
 *          its position is taken to be loc.  @maxwidth implicitly
 *          sets a minimum on the required gradient of the edge.
 *      (4) The thresholds must be at least 1, and the low threshold
 *          cannot be larger than the high threshold.
 */
LEPTONICA_EXPORT l_int32
pixScanForEdge(PIX      *pixs,
               BOX      *box,
               l_int32   lowthresh,
               l_int32   highthresh,
               l_int32   maxwidth,
               l_int32   factor,
               l_int32   scanflag,
               l_int32  *ploc)
{
l_int32    bx, by, bw, bh, foundmin, loc, sum, wpl;
l_int32    x, xstart, xend, y, ystart, yend;
l_uint32  *data, *line;
BOX       *boxt;

    PROCNAME("pixScanForEdge");

    if (!ploc)
        return ERROR_INT("&ploc not defined", procName, 1);
    *ploc = 0;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (lowthresh < 1 || highthresh < 1 ||
        lowthresh > highthresh || maxwidth < 1)
        return ERROR_INT("invalid thresholds", procName, 1);
    factor = L_MIN(1, factor);

        /* Clip box to pixs if it exists */
    pixGetDimensions(pixs, &bw, &bh, NULL);
    if (box) {
        if ((boxt = boxClipToRectangle(box, bw, bh)) == NULL)
            return ERROR_INT("invalid box", procName, 1);
        boxGetGeometry(boxt, &bx, &by, &bw, &bh);
        boxDestroy(&boxt);
    }
    else
        bx = by = 0;
    xstart = bx;
    ystart = by;
    xend = bx + bw - 1;
    yend = by + bh - 1;

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    foundmin = 0;
    if (scanflag == L_FROM_LEFT) {
        for (x = xstart; x <= xend; x++) {
            sum = 0;
            for (y = ystart; y <= yend; y += factor) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {  /* save the loc of the beginning of the edge */
                foundmin = 1;
                loc = x;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Left: x = %d, loc = %d\n", x, loc);
#endif  /* DEBUG_EDGES */
                if (x - loc < maxwidth) {
                    *ploc = loc;
                    return 0;
                }
                else return 1;
            }
        }
    }
    else if (scanflag == L_FROM_RIGHT) {
        for (x = xend; x >= xstart; x--) {
            sum = 0;
            for (y = ystart; y <= yend; y += factor) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = x;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Right: x = %d, loc = %d\n", x, loc);
#endif  /* DEBUG_EDGES */
                if (loc - x < maxwidth) {
                    *ploc = loc;
                    return 0;
                }
                else return 1;
            }
        }
    }
    else if (scanflag == L_FROM_TOP) {
        for (y = ystart; y <= yend; y++) {
            sum = 0;
            line = data + y * wpl;
            for (x = xstart; x <= xend; x += factor) {
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = y;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Top: y = %d, loc = %d\n", y, loc);
#endif  /* DEBUG_EDGES */
                if (y - loc < maxwidth) {
                    *ploc = loc;
                    return 0;
                }
                else return 1;
            }
        }
    }
    else if (scanflag == L_FROM_BOTTOM) {
        for (y = yend; y >= ystart; y--) {
            sum = 0;
            line = data + y * wpl;
            for (x = xstart; x <= xend; x += factor) {
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = y;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Bottom: y = %d, loc = %d\n", y, loc);
#endif  /* DEBUG_EDGES */
                if (loc - y < maxwidth) {
                    *ploc = loc;
                    return 0;
                }
                else return 1;
            }
        }
    }
    else
        return ERROR_INT("invalid scanflag", procName, 1);

    return 1;  /* edge not found */
}


/*!
 *  pixAverageOnLine()
 *
 *      Input:  pixs (1 bpp or 8 bpp; no colormap)
 *              x1, y1 (starting pt for line)
 *              x2, y2 (end pt for line)
 *              factor (sampling; >= 1)
 *      Return: average of pixel values along line, or null on error.
 *
 *  Notes:
 *      (1) The line must be either horizontal or vertical, so either
 *          y1 == y2 (horizontal) or x1 == x2 (vertical).
 *      (2) If horizontal, x1 must be <= x2.
 *          If vertical, y1 must be <= y2.
 *          characterize the intensity smoothness along a line.
 *      (3) Input end points are clipped to the pix.
 */
LEPTONICA_EXPORT l_float32
pixAverageOnLine(PIX     *pixs,
                 l_int32  x1,
                 l_int32  y1,
                 l_int32  x2,
                 l_int32  y2,
                 l_int32  factor)
{
l_int32    i, j, w, h, d, direction, count, wpl;
l_uint32  *data, *line;
l_float32  sum;

    PROCNAME("pixAverageOnLine");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8)
        return ERROR_INT("d not 1 or 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs has a colormap", procName, 1);
    if (x1 > x2 || y1 > y2)
        return ERROR_INT("x1 > x2 or y1 > y2", procName, 1);
    if (y1 == y2) {
        x1 = L_MAX(0, x1);
        x2 = L_MIN(w - 1, x2);
        y1 = L_MAX(0, L_MIN(y1, h - 1));
        direction = L_HORIZONTAL_LINE;
    }
    else if (x1 == x2) {
        y1 = L_MAX(0, y1);
        y2 = L_MIN(h - 1, y2);
        x1 = L_MAX(0, L_MIN(x1, w - 1));
        direction = L_VERTICAL_LINE;
    }
    else
        return ERROR_INT("line neither horiz nor vert", procName, 1);
    if (factor < 1) {
        L_WARNING("factor must be >= 1; setting to 1", procName);
        factor = 1;
    }

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    sum = 0;
    if (direction == L_HORIZONTAL_LINE) {
        line = data + y1 * wpl;
        for (j = x1, count = 0; j <= x2; count++, j += factor) {
            if (d == 1)
                sum += GET_DATA_BIT(line, j);
            else  /* d == 8 */
                sum += GET_DATA_BYTE(line, j);
        }
    }
    else if (direction == L_VERTICAL_LINE) {
        for (i = y1, count = 0; i <= y2; count++, i += factor) {
            line = data + i * wpl;
            if (d == 1)
                sum += GET_DATA_BIT(line, x1);
            else  /* d == 8 */
                sum += GET_DATA_BYTE(line, x1);
        }
    }

    return sum / (l_float32)count;
}


/*!
 *  pixAverageIntensityProfile()
 *
 *      Input:  pixs (any depth; colormap OK)
 *              fract (fraction of image width or height to be used)
 *              dir (averaging direction: L_HORIZONTAL_LINE or L_VERTICAL_LINE)
 *              first, last (span of rows or columns to measure)
 *              factor1 (sampling along fast scan direction; >= 1)
 *              factor2 (sampling along slow scan direction; >= 1)
 *      Return: na (of reversal profile), or null on error.
 *
 *  Notes:
 *      (1) If d != 1 bpp, colormaps are removed and the result
 *          is converted to 8 bpp.
 *      (2) If @dir == L_HORIZONTAL_LINE, the intensity is averaged
 *          along each horizontal raster line (sampled by @factor1),
 *          and the profile is the array of these averages in the
 *          vertical direction between @first and @last raster lines,
 *          and sampled by @factor2.
 *      (3) If @dir == L_VERTICAL_LINE, the intensity is averaged
 *          along each vertical line (sampled by @factor1),
 *          and the profile is the array of these averages in the
 *          horizontal direction between @first and @last columns,
 *          and sampled by @factor2.
 *      (4) The averages are measured over the central @fract of the image.
 *          Use @fract == 1.0 to average across the entire width or height.
 */
LEPTONICA_EXPORT NUMA *
pixAverageIntensityProfile(PIX       *pixs,
                           l_float32  fract,
                           l_int32    dir,
                           l_int32    first,
                           l_int32    last,
                           l_int32    factor1,
                           l_int32    factor2)
{
l_int32    i, j, w, h, d, start, end;
l_float32  ave;
NUMA      *nad;
PIX       *pixr, *pixg;

    PROCNAME("pixAverageIntensityProfile");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0)
        return (NUMA *)ERROR_PTR("fract < 0.0 or > 1.0", procName, NULL);
    if (dir != L_HORIZONTAL_LINE && dir != L_VERTICAL_LINE)
        return (NUMA *)ERROR_PTR("invalid direction", procName, NULL);
    if (first < 0) first = 0;
    if (last < first)
        return (NUMA *)ERROR_PTR("last must be >= first", procName, NULL);
    if (factor1 < 1) {
        L_WARNING("factor1 must be >= 1; setting to 1", procName);
        factor1 = 1;
    }
    if (factor2 < 1) {
        L_WARNING("factor2 must be >= 1; setting to 1", procName);
        factor2 = 1;
    }

        /* Use 1 or 8 bpp, without colormap */
    if (pixGetColormap(pixs))
        pixr = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixr = pixClone(pixs);
    pixGetDimensions(pixr, &w, &h, &d);
    if (d == 1)
        pixg = pixClone(pixr);
    else
        pixg = pixConvertTo8(pixr, 0);

    nad = numaCreate(0);  /* output: samples in slow scan direction */
    numaSetXParameters(nad, 0, factor2);
    if (dir == L_HORIZONTAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)w);
        end = w - start;
        if (last > h - 1) {
            L_WARNING("last > h - 1; clipping", procName);
            last = h - 1;
        }
        for (i = first; i <= last; i += factor2) {
            ave = pixAverageOnLine(pixg, start, i, end, i, factor1);
            numaAddNumber(nad, ave);
        }
    } else if (dir == L_VERTICAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)h);
        end = h - start;
        if (last > w - 1) {
            L_WARNING("last > w - 1; clipping", procName);
            last = w - 1;
        }
        for (j = first; j <= last; j += factor2) {
            ave = pixAverageOnLine(pixg, j, start, j, end, factor1);
            numaAddNumber(nad, ave);
        }
    }

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    return nad;
}


/*---------------------------------------------------------------------*
 *                     Rank row and column transforms                  *
 *---------------------------------------------------------------------*/
/*!
 *  pixRankRowTransform()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *      Return: pixd (with pixels sorted in each row, from
 *                    min to max value)
 *
 * Notes:
 *     (1) The time is O(n) in the number of pixels and runs about
 *         100 Mpixels/sec on a 3 GHz machine.
 */
LEPTONICA_EXPORT PIX *
pixRankRowTransform(PIX  *pixs)
{
l_int32    i, j, k, m, w, h, wpl, val;
l_int32    histo[256];
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixRankRowTransform");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has a colormap", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateTemplateNoInit(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (i = 0; i < h; i++) {
        memset(histo, 0, 1024);
        lines = datas + i * wpl;
        lined = datad + i * wpl;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            histo[val]++;
        }
        for (m = 0, j = 0; m < 256; m++) {
            for (k = 0; k < histo[m]; k++, j++)
                SET_DATA_BYTE(lined, j, m);
        }
    }

    return pixd;
}


/*!
 *  pixRankColumnTransform()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *      Return: pixd (with pixels sorted in each column, from
 *                    min to max value)
 *
 * Notes:
 *     (1) The time is O(n) in the number of pixels and runs about
 *         50 Mpixels/sec on a 3 GHz machine.
 */
LEPTONICA_EXPORT PIX *
pixRankColumnTransform(PIX  *pixs)
{
l_int32    i, j, k, m, w, h, wpl, val; 
l_int32    histo[256];
l_uint32  *datas, *datad;
void     **lines8, **lined8;
PIX       *pixd;

    PROCNAME("pixRankColumnTransform");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has a colormap", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateTemplateNoInit(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    lines8 = pixGetLinePtrs(pixs, NULL);
    lined8 = pixGetLinePtrs(pixd, NULL);
    for (j = 0; j < w; j++) {
        memset(histo, 0, 1024);
        for (i = 0; i < h; i++) {
            val = GET_DATA_BYTE(lines8[i], j);
            histo[val]++;
        }
        for (m = 0, i = 0; m < 256; m++) {
            for (k = 0; k < histo[m]; k++, i++)
                SET_DATA_BYTE(lined8[i], j, m);
        }
    }

    FREE(lines8);
    FREE(lined8);
    return pixd;
}

