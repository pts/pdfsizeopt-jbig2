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
