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
 *  pix3.c
 *
 *    This file has these operations:
 *
 *      (1) Mask-directed operations
 *      (2) Full-image bit-logical operations
 *      (3) Foreground pixel counting operations on 1 bpp images
 *      (4) Sum of pixel values
 *      (5) Mirrored tiling of a smaller image
 *
 *
 *    Masked operations
 *           l_int32     pixSetMasked()
 *           l_int32     pixSetMaskedGeneral()
 *           l_int32     pixCombineMasked()
 *           l_int32     pixCombineMaskedGeneral()
 *           l_int32     pixPaintThroughMask()
 *           PIX        *pixPaintSelfThroughMask()
 *           PIX        *pixMakeMaskFromLUT()
 *           PIX        *pixSetUnderTransparency()
 *
 *    One and two-image boolean operations on arbitrary depth images
 *           PIX        *pixInvert()
 *           PIX        *pixOr()
 *           PIX        *pixAnd()
 *           PIX        *pixXor()
 *           PIX        *pixSubtract()
 *
 *    Foreground pixel counting in 1 bpp images
 *           l_int32     pixZero()
 *           l_int32     pixCountPixels()
 *           NUMA       *pixaCountPixels()
 *           l_int32     pixCountPixelsInRow()
 *           NUMA       *pixCountPixelsByRow()
 *           NUMA       *pixCountPixelsByColumn()
 *           NUMA       *pixSumPixelsByRow()
 *           NUMA       *pixSumPixelsByColumn()
 *           l_int32     pixThresholdPixelSum()
 *           l_int32    *makePixelSumTab8()
 *           l_int32    *makePixelCentroidTab8()
 *
 *    Sum of pixel values
 *           l_int32     pixSumPixelValues()
 *
 *    Mirrored tiling
 *           PIX        *pixMirroredTiling()
 *
 *    Static helper function
 *           static l_int32  findTilePatchCenter()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static l_int32 findTilePatchCenter(PIX *pixs, BOX *box, l_int32 dir,
                                   l_uint32 targdist, l_uint32 *pdist,
                                   l_int32 *pxc, l_int32 *pyc);

#ifndef  NO_CONSOLE_IO
#define   EQUAL_SIZE_WARNING      0
#endif  /* ~NO_CONSOLE_IO */


/*!
 *  pixSetMaskedGeneral()
 *
 *      Input:  pixd (8, 16 or 32 bpp)
 *              pixm (<optional> 1 bpp mask; no operation if null)
 *              val (value to set at each masked pixel)
 *              x, y (location of UL corner of pixm relative to pixd;
 *                    can be negative)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation.
 *      (2) Alignment is explicit.  If you want the UL corners of
 *          the two images to be aligned, use pixSetMasked().
 *      (3) A typical use would be painting through the foreground
 *          of a small binary mask pixm, located somewhere on a
 *          larger pixd.  Other pixels in pixd are not changed.
 *      (4) You can visualize this as painting the color through
 *          the mask, as a stencil.
 *      (5) This uses rasterop to handle clipping and different depths of pixd.
 *      (6) If pixd has a colormap, you should call pixPaintThroughMask().
 *      (7) Why is this function here, if pixPaintThroughMask() does the
 *          same thing, and does it more generally?  I've retained it here
 *          to show how one can paint through a mask using only full
 *          image rasterops, rather than pixel peeking in pixm and poking
 *          in pixd.  It's somewhat baroque, but I found it amusing.
 */
LEPTONICA_EXPORT l_int32
pixSetMaskedGeneral(PIX      *pixd,
                    PIX      *pixm,
                    l_uint32  val,
                    l_int32   x,
                    l_int32   y)
{
l_int32    wm, hm, d;
PIX       *pixmu, *pixc;

    PROCNAME("pixSetMaskedGeneral");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixm)  /* nothing to do */
        return 0;

    d = pixGetDepth(pixd);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixd not 8, 16 or 32 bpp", procName, 1);
    if (pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);

        /* Unpack binary to depth d, with inversion:  1 --> 0, 0 --> 0xff... */
    if ((pixmu = pixUnpackBinary(pixm, d, 1)) == NULL)
        return ERROR_INT("pixmu not made", procName, 1);

        /* Clear stenciled pixels in pixd */
    pixGetDimensions(pixm, &wm, &hm, NULL);
    pixRasterop(pixd, x, y, wm, hm, PIX_SRC & PIX_DST, pixmu, 0, 0);

        /* Generate image with requisite color */
    if ((pixc = pixCreateTemplate(pixmu)) == NULL)
        return ERROR_INT("pixc not made", procName, 1);
    pixSetAllArbitrary(pixc, val);

        /* Invert stencil mask, and paint color color into stencil */
    pixInvert(pixmu, pixmu);
    pixAnd(pixmu, pixmu, pixc);

        /* Finally, repaint stenciled pixels, with val, in pixd */
    pixRasterop(pixd, x, y, wm, hm, PIX_SRC | PIX_DST, pixmu, 0, 0);

    pixDestroy(&pixmu);
    pixDestroy(&pixc);
    return 0;
}


/*!
 *  pixCombineMasked()
 *
 *      Input:  pixd (1 bpp, 8 bpp gray or 32 bpp rgb; no cmap)
 *              pixs (1 bpp, 8 bpp gray or 32 bpp rgb; no cmap)
 *              pixm (<optional> 1 bpp mask; no operation if NULL)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation; pixd is changed.
 *      (2) This sets each pixel in pixd that co-locates with an ON
 *          pixel in pixm to the corresponding value of pixs.
 *      (3) pixs and pixd must be the same depth and not colormapped.
 *      (4) All three input pix are aligned at the UL corner, and the
 *          operation is clipped to the intersection of all three images.
 *      (5) If pixm == NULL, it's a no-op.
 *      (6) Implementation: see notes in pixCombineMaskedGeneral().
 *          For 8 bpp selective masking, you might guess that it
 *          would be faster to generate an 8 bpp version of pixm,
 *          using pixConvert1To8(pixm, 0, 255), and then use a
 *          general combine operation
 *               d = (d & ~m) | (s & m)
 *          on a word-by-word basis.  Not always.  The word-by-word
 *          combine takes a time that is independent of the mask data.
 *          If the mask is relatively sparse, the byte-check method
 *          is actually faster!
 */
LEPTONICA_EXPORT l_int32
pixCombineMasked(PIX  *pixd,
                 PIX  *pixs,
                 PIX  *pixm)
{
l_int32    w, h, d, ws, hs, ds, wm, hm, dm, wmin, hmin;
l_int32    wpl, wpls, wplm, i, j, val;
l_uint32  *data, *datas, *datam, *line, *lines, *linem;
PIX       *pixt;

    PROCNAME("pixCombineMasked");

    if (!pixm)  /* nothing to do */
        return 0;
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixd, &w, &h, &d);
    pixGetDimensions(pixs, &ws, &hs, &ds);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (d != ds)
        return ERROR_INT("pixs and pixd depths differ", procName, 1);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (d != 1 && d != 8 && d != 32)
        return ERROR_INT("pixd not 1, 8 or 32 bpp", procName, 1);
    if (pixGetColormap(pixd) || pixGetColormap(pixs))
        return ERROR_INT("pixs and/or pixd is cmapped", procName, 1);

        /* For d = 1, use rasterop.  pixt is the part from pixs, under
         * the fg of pixm, that is to be combined with pixd.  We also
         * use pixt to remove all fg of pixd that is under the fg of pixm.
         * Then pixt and pixd are combined by ORing. */
    wmin = L_MIN(w, L_MIN(ws, wm));
    hmin = L_MIN(h, L_MIN(hs, hm));
    if (d == 1) {
        pixt = pixAnd(NULL, pixs, pixm);
        pixRasterop(pixd, 0, 0, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
                    pixm, 0, 0);
        pixRasterop(pixd, 0, 0, wmin, hmin, PIX_SRC | PIX_DST, pixt, 0, 0);
        pixDestroy(&pixt);
        return 0;
    }

    data = pixGetData(pixd);
    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpl = pixGetWpl(pixd);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    if (d == 8) {
        for (i = 0; i < hmin; i++) {
            line = data + i * wpl;
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wmin; j++) {
                if (GET_DATA_BIT(linem, j)) {
                   val = GET_DATA_BYTE(lines, j);
                   SET_DATA_BYTE(line, j, val);
                }
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < hmin; i++) {
            line = data + i * wpl;
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wmin; j++) {
                if (GET_DATA_BIT(linem, j))
                   line[j] = lines[j];
            }
        }
    }

    return 0;
}


/*!
 *  pixCombineMaskedGeneral()
 *
 *      Input:  pixd (1 bpp, 8 bpp gray or 32 bpp rgb)
 *              pixs (1 bpp, 8 bpp gray or 32 bpp rgb)
 *              pixm (<optional> 1 bpp mask)
 *              x, y (origin of pixs and pixm relative to pixd; can be negative)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation; pixd is changed.
 *      (2) This is a generalized version of pixCombinedMasked(), where
 *          the source and mask can be placed at the same (arbitrary)
 *          location relative to pixd.
 *      (3) pixs and pixd must be the same depth and not colormapped.
 *      (4) The UL corners of both pixs and pixm are aligned with
 *          the point (x, y) of pixd, and the operation is clipped to
 *          the intersection of all three images.
 *      (5) If pixm == NULL, it's a no-op.
 *      (6) Implementation.  There are two ways to do these.  In the first,
 *          we use rasterop, ORing the part of pixs under the mask
 *          with pixd (which has been appropriately cleared there first).
 *          In the second, the mask is used one pixel at a time to
 *          selectively replace pixels of pixd with those of pixs.
 *          Here, we use rasterop for 1 bpp and pixel-wise replacement
 *          for 8 and 32 bpp.  To use rasterop for 8 bpp, for example,
 *          we must first generate an 8 bpp version of the mask.
 *          The code is simple:
 *
 *             Pix *pixm8 = pixConvert1To8(NULL, pixm, 0, 255);
 *             Pix *pixt = pixAnd(NULL, pixs, pixm8);
 *             pixRasterop(pixd, x, y, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
 *                         pixm8, 0, 0);
 *             pixRasterop(pixd, x, y, wmin, hmin, PIX_SRC | PIX_DST,
 *                         pixt, 0, 0);
 *             pixDestroy(&pixt);
 *             pixDestroy(&pixm8);
 */
LEPTONICA_EXPORT l_int32
pixCombineMaskedGeneral(PIX      *pixd,
                        PIX      *pixs,
                        PIX      *pixm,
                        l_int32   x,
                        l_int32   y)
{
l_int32    d, w, h, ws, hs, ds, wm, hm, dm, wmin, hmin;
l_int32    wpl, wpls, wplm, i, j, val;
l_uint32  *data, *datas, *datam, *line, *lines, *linem;
PIX       *pixt;

    PROCNAME("pixCombineMaskedGeneral");

    if (!pixm)  /* nothing to do */
        return 0;
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixd, &w, &h, &d);
    pixGetDimensions(pixs, &ws, &hs, &ds);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (d != ds)
        return ERROR_INT("pixs and pixd depths differ", procName, 1);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (d != 1 && d != 8 && d != 32)
        return ERROR_INT("pixd not 1, 8 or 32 bpp", procName, 1);
    if (pixGetColormap(pixd) || pixGetColormap(pixs))
        return ERROR_INT("pixs and/or pixd is cmapped", procName, 1);

        /* For d = 1, use rasterop.  pixt is the part from pixs, under
         * the fg of pixm, that is to be combined with pixd.  We also
         * use pixt to remove all fg of pixd that is under the fg of pixm.
         * Then pixt and pixd are combined by ORing. */
    wmin = L_MIN(ws, wm);
    hmin = L_MIN(hs, hm);
    if (d == 1) {
        pixt = pixAnd(NULL, pixs, pixm);
        pixRasterop(pixd, x, y, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
                    pixm, 0, 0);
        pixRasterop(pixd, x, y, wmin, hmin, PIX_SRC | PIX_DST, pixt, 0, 0);
        pixDestroy(&pixt);
        return 0;
    }

    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wplm = pixGetWpl(pixm);
    datam = pixGetData(pixm);

    for (i = 0; i < hmin; i++) {
        if (y + i < 0 || y + i >= h) continue;
        line = data + (y + i) * wpl;
        lines = datas + i * wpls;
        linem = datam + i * wplm;
        for (j = 0; j < wmin; j++) {
            if (x + j < 0 || x + j >= w) continue;
            if (GET_DATA_BIT(linem, j)) {
                switch (d)
                {
                case 8:
                    val = GET_DATA_BYTE(lines, j);
                    SET_DATA_BYTE(line, x + j, val);
                    break;
                case 32:
                    *(line + x + j) = *(lines + j);
                    break;
                default:
                    return ERROR_INT("shouldn't get here", procName, 1);
                }
            }
        }
    }

    return 0;
}


/*!
 *  pixMakeMaskFromLUT()
 *
 *      Input:  pixs (2, 4 or 8 bpp; can be colormapped)
 *              tab (256-entry LUT; 1 means to write to mask)
 *      Return: pixd (1 bpp mask), or null on error
 *
 *  Notes:
 *      (1) This generates a 1 bpp mask image, where a 1 is written in
 *          the mask for each pixel in pixs that has a value corresponding
 *          to a 1 in the LUT.
 *      (2) The LUT should be of size 256.
 */
LEPTONICA_EXPORT PIX *
pixMakeMaskFromLUT(PIX      *pixs,
                   l_int32  *tab)
{
l_int32    w, h, d, i, j, val, wpls, wpld;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixMakeMaskFromLUT");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!tab)
        return (PIX *)ERROR_PTR("tab not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("pix not 2, 4 or 8 bpp", procName, NULL);

    pixd = pixCreate(w, h, 1);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            if (d == 2)
                val = GET_DATA_DIBIT(lines, j);
            else if (d == 4)
                val = GET_DATA_QBIT(lines, j);
            else  /* d == 8 */
                val = GET_DATA_BYTE(lines, j);
            if (tab[val] == 1)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*-------------------------------------------------------------*
 *    One and two-image boolean ops on arbitrary depth images  *
 *-------------------------------------------------------------*/
/*!
 *  pixInvert()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This inverts pixs, for all pixel depths.
 *      (2) There are 3 cases:
 *           (a) pixd == null,   ~src --> new pixd
 *           (b) pixd == pixs,   ~src --> src  (in-place)
 *           (c) pixd != pixs,   ~src --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *           (a) pixd = pixInvert(NULL, pixs);
 *           (b) pixInvert(pixs, pixs);
 *           (c) pixInvert(pixd, pixs);
 */
LEPTONICA_EXPORT PIX *
pixInvert(PIX  *pixd,
          PIX  *pixs)
{
    PROCNAME("pixInvert");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_NOT(PIX_DST), NULL, 0, 0);   /* invert pixd */

    return pixd;
}


/*!
 *  pixOr()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the union of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 | src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 | src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 | src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixOr(NULL, pixs1, pixs2);
 *            (b) pixOr(pixs1, pixs1, pixs2);
 *            (c) pixOr(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
LEPTONICA_EXPORT PIX *
pixOr(PIX  *pixd,
      PIX  *pixs1,
      PIX  *pixs2)
{
    PROCNAME("pixOr");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 | src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC | PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixAnd()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the intersection of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2 
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 & src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 & src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 & src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixAnd(NULL, pixs1, pixs2);
 *            (b) pixAnd(pixs1, pixs1, pixs2);
 *            (c) pixAnd(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
LEPTONICA_EXPORT PIX *
pixAnd(PIX  *pixd,
       PIX  *pixs1,
       PIX  *pixs2)
{
    PROCNAME("pixAnd");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 & src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC & PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixXor()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the XOR of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 ^ src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 ^ src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 ^ src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixXor(NULL, pixs1, pixs2);
 *            (b) pixXor(pixs1, pixs1, pixs2);
 *            (c) pixXor(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
LEPTONICA_EXPORT PIX *
pixXor(PIX  *pixd,
       PIX  *pixs1,
       PIX  *pixs2)
{
    PROCNAME("pixXor");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 ^ src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC ^ PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixSubtract()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     equal to pixs2, or different from both pixs1 and pixs2)
 *              pixs1 (can be == pixd)
 *              pixs2 (can be == pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the set subtraction of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) Source pixs2 is always subtracted from source pixs1.
 *          The result is
 *                  pixs1 \ pixs2 = pixs1 & (~pixs2)
 *      (3) There are 4 cases:
 *            (a) pixd == null,   (src1 - src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 - src2) --> src1  (in-place)
 *            (c) pixd == pixs2,  (src1 - src2) --> src2  (in-place)
 *            (d) pixd != pixs1 && pixd != pixs2),
 *                                 (src1 - src2) --> input pixd
 *      (4) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixSubtract(NULL, pixs1, pixs2);
 *            (b) pixSubtract(pixs1, pixs1, pixs2);
 *            (c) pixSubtract(pixs2, pixs1, pixs2);
 *            (d) pixSubtract(pixd, pixs1, pixs2);
 *      (5) The size of the result is determined by pixs1.
 *      (6) The depths of pixs1 and pixs2 must be equal.
 */
LEPTONICA_EXPORT PIX *
pixSubtract(PIX  *pixd,
            PIX  *pixs1,
            PIX  *pixs2)
{
l_int32  w, h;

    PROCNAME("pixSubtract");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

    pixGetDimensions(pixs1, &w, &h, NULL);
    if (!pixd) {
        pixd = pixCopy(NULL, pixs1);
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }
    else if (pixd == pixs1) {
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }
    else if (pixd == pixs2) {
        pixRasterop(pixd, 0, 0, w, h, PIX_NOT(PIX_DST) & PIX_SRC,
            pixs1, 0, 0);   /* src1 & (~src2)  */
    }
    else  { /* pixd != pixs1 && pixd != pixs2 */
        pixCopy(pixd, pixs1);  /* sizes pixd to pixs1 if unequal */
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }

    return pixd;
}


/*-------------------------------------------------------------*
 *                         Pixel counting                      *
 *-------------------------------------------------------------*/
/*!
 *  pixZero()
 *
 *      Input:  pix (all depths; not colormapped)
 *              &empty  (<return> 1 if all bits in image are 0; 0 otherwise)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) For a binary image, if there are no fg (black) pixels, empty = 1.
 *      (2) For a grayscale image, if all pixels are black (0), empty = 1.
 *      (3) For an RGB image, if all 4 components in every pixel is 0,
 *          empty = 1. 
 */
LEPTONICA_EXPORT l_int32
pixZero(PIX      *pix,
        l_int32  *pempty)
{
l_int32    w, h, wpl, i, j, fullwords, endbits;
l_uint32   endmask;
l_uint32  *data, *line;

    PROCNAME("pixZero");

    if (!pempty)
        return ERROR_INT("pempty not defined", procName, 1);
    *pempty = 1;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pixGetColormap(pix) != NULL)
        return ERROR_INT("pix is colormapped", procName, 1);

    w = pixGetWidth(pix) * pixGetDepth(pix);
    h = pixGetHeight(pix);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w / 32;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < fullwords; j++)
            if (*line++) {
                *pempty = 0;
                return 0;
            }
        if (endbits) {
            if (*line & endmask) {
                *pempty = 0;
                return 0;
            }
        }
    }

    return 0;
}


/*!
 *  pixCountPixels()
 *
 *      Input:  pix (1 bpp)
 *              &count (<return> count of ON pixels)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_REAL_EXPORT l_int32
pixCountPixels(PIX      *pix,
               l_int32  *pcount,
               l_int32  *tab8)
{
l_uint32   endmask;
l_int32    w, h, wpl, i, j;
l_int32    fullwords, endbits, sum;
l_int32   *tab;
l_uint32  *data;

    PROCNAME("pixCountPixels");

    if (!pcount)
        return ERROR_INT("pcount not defined", procName, 1);
    *pcount = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    pixGetDimensions(pix, &w, &h, NULL);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    sum = 0;
    for (i = 0; i < h; i++, data += wpl) {
        for (j = 0; j < fullwords; j++) {
            l_uint32 word = data[j];
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (endbits) {
            l_uint32 word = data[j] & endmask;
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
    }
    *pcount = sum;

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  pixaCountPixels()
 *
 *      Input:  pixa (array of 1 bpp pix)
 *      Return: na of ON pixels in each pix, or null on error
 */
LEPTONICA_EXPORT NUMA *
pixaCountPixels(PIXA  *pixa)
{
l_int32   d, i, n, count;
l_int32  *tab;
NUMA     *na;
PIX      *pix;

    PROCNAME("pixaCountPixels");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);

    if ((n = pixaGetCount(pixa)) == 0)
        return numaCreate(1);

    pix = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pix);
    pixDestroy(&pix);
    if (d != 1)
        return (NUMA *)ERROR_PTR("pixa not 1 bpp", procName, NULL);

    tab = makePixelSumTab8();
    if ((na = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        pixCountPixels(pix, &count, tab);
        numaAddNumber(na, count);
        pixDestroy(&pix);
    }
        
    FREE(tab);
    return na;
}


/*!
 *  pixCountPixelsInRow()
 *
 *      Input:  pix (1 bpp)
 *              row number
 *              &count (<return> sum of ON pixels in raster line)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixCountPixelsInRow(PIX      *pix,
                    l_int32   row,
                    l_int32  *pcount,
                    l_int32  *tab8)
{
l_uint32   word, endmask;
l_int32    j, w, h, wpl;
l_int32    fullwords, endbits, sum;
l_int32   *tab;
l_uint32  *line;

    PROCNAME("pixCountPixelsInRow");

    if (!pcount)
        return ERROR_INT("pcount not defined", procName, 1);
    *pcount = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    if (row < 0 || row >= h)
        return ERROR_INT("row out of bounds", procName, 1);
    wpl = pixGetWpl(pix);
    line = pixGetData(pix) + row * wpl;
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    sum = 0;
    for (j = 0; j < fullwords; j++) {
        word = line[j];
        if (word) {
            sum += tab[word & 0xff] +
                   tab[(word >> 8) & 0xff] +
                   tab[(word >> 16) & 0xff] +
                   tab[(word >> 24) & 0xff];
        }
    }
    if (endbits) {
        word = line[j] & endmask;
        if (word) {
            sum += tab[word & 0xff] +
                   tab[(word >> 8) & 0xff] +
                   tab[(word >> 16) & 0xff] +
                   tab[(word >> 24) & 0xff];
        }
    }
    *pcount = sum;

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  pixCountPixelsByRow()
 *
 *      Input:  pix (1 bpp)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: na of counts, or null on error
 */
LEPTONICA_EXPORT NUMA *
pixCountPixelsByRow(PIX      *pix,
                    l_int32  *tab8)
{
l_int32   h, i, count;
l_int32  *tab;
NUMA     *na;

    PROCNAME("pixCountPixelsByRow");

    if (!pix || pixGetDepth(pix) != 1)
        return (NUMA *)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    h = pixGetHeight(pix);
    if ((na = numaCreate(h)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < h; i++) {
        pixCountPixelsInRow(pix, i, &count, tab);
        numaAddNumber(na, count);
    }

    if (!tab8)
        FREE(tab);

    return na;
}


/*!
 *  pixCountPixelsByColumn()
 *
 *      Input:  pix (1 bpp)
 *      Return: na of counts in each column, or null on error
 */
LEPTONICA_EXPORT NUMA *
pixCountPixelsByColumn(PIX  *pix)
{
l_int32     i, j, w, h, wpl;
l_uint32   *line, *data;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixCountPixelsByColumn");

    if (!pix || pixGetDepth(pix) != 1)
        return (NUMA *)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);

    pixGetDimensions(pix, &w, &h, NULL);
    if ((na = numaCreate(w)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, w);
    array = numaGetFArray(na, L_NOCOPY);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < w; j++) {
            if (GET_DATA_BIT(line, j))
                array[j] += 1.0;
        }
    }

    return na;
}


/*!
 *  pixSumPixelsByRow()
 *
 *      Input:  pix (1, 8 or 16 bpp; no colormap)
 *              tab8  (<optional> lookup table for 1 bpp; use null for 8 bpp)
 *      Return: na of pixel sums by row, or null on error
 *
 *  Notes:
 *      (1) To resample for a bin size different from 1, use
 *          numaUniformSampling() on the result of this function.
 */
LEPTONICA_EXPORT NUMA *
pixSumPixelsByRow(PIX      *pix,
                  l_int32  *tab8)
{
l_int32    i, j, w, h, d, wpl;
l_uint32  *line, *data;
l_float32  sum;
NUMA      *na;

    PROCNAME("pixSumPixelsByRow");

    if (!pix)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8 && d != 16)
        return (NUMA *)ERROR_PTR("pix not 1, 8 or 16 bpp", procName, NULL);
    if (pixGetColormap(pix) != NULL)
        return (NUMA *)ERROR_PTR("pix colormapped", procName, NULL);

    if (d == 1)
        return pixCountPixelsByRow(pix, tab8);

    if ((na = numaCreate(h)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        sum = 0.0;
        line = data + i * wpl;
        if (d == 8) {
            sum += w * 255;
            for (j = 0; j < w; j++)
                sum -= GET_DATA_BYTE(line, j);
        }
        else {  /* d == 16 */
            sum += w * 0xffff;
            for (j = 0; j < w; j++)
                sum -= GET_DATA_TWO_BYTES(line, j);
        }
        numaAddNumber(na, sum);
    }

    return na;
}


/*!
 *  pixSumPixelsByColumn()
 *
 *      Input:  pix (1, 8 or 16 bpp; no colormap)
 *      Return: na of pixel sums by column, or null on error
 *
 *  Notes:
 *      (1) To resample for a bin size different from 1, use
 *          numaUniformSampling() on the result of this function.
 */
LEPTONICA_EXPORT NUMA *
pixSumPixelsByColumn(PIX  *pix)
{
l_int32     i, j, w, h, d, wpl;
l_uint32   *line, *data;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixSumPixelsByColumn");

    if (!pix)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8 && d != 16)
        return (NUMA *)ERROR_PTR("pix not 1, 8 or 16 bpp", procName, NULL);
    if (pixGetColormap(pix) != NULL)
        return (NUMA *)ERROR_PTR("pix colormapped", procName, NULL);

    if (d == 1)
        return pixCountPixelsByColumn(pix);

    if ((na = numaCreate(w)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, w);
    array = numaGetFArray(na, L_NOCOPY);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        if (d == 8) {
            for (j = 0; j < w; j++)
                array[j] += 255 - GET_DATA_BYTE(line, j);
        }
        else {  /* d == 16 */
            for (j = 0; j < w; j++)
                array[j] += 0xffff - GET_DATA_TWO_BYTES(line, j);
        }
    }

    return na;
}


/*!
 *  pixThresholdPixelSum()
 *
 *      Input:  pix (1 bpp)
 *              threshold
 *              &above (<return> 1 if above threshold;
 *                               0 if equal to or less than threshold)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This sums the ON pixels and returns immediately if the count
 *          goes above threshold.  It is therefore more efficient
 *          for matching images (by running this function on the xor of
 *          the 2 images) than using pixCountPixels(), which counts all
 *          pixels before returning.
 */
LEPTONICA_EXPORT l_int32
pixThresholdPixelSum(PIX      *pix,
                     l_int32   thresh,
                     l_int32  *pabove,
                     l_int32  *tab8)
{
l_uint32   word, endmask;
l_int32   *tab;
l_int32    w, h, wpl, i, j;
l_int32    fullwords, endbits, sum;
l_uint32  *line, *data;

    PROCNAME("pixThresholdPixelSum");

    if (!pabove)
        return ERROR_INT("pabove not defined", procName, 1);
    *pabove = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    pixGetDimensions(pix, &w, &h, NULL);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    sum = 0;
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < fullwords; j++) {
            word = line[j];
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (endbits) {
            word = line[j] & endmask;
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (sum > thresh) {
            *pabove = 1;
            if (!tab8)
                FREE(tab);
            return 0;
        }
    }

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  makePixelSumTab8()
 *
 *      Input:  void
 *      Return: table of 256 l_int32, or null on error
 *
 *  Notes:
 *      (1) This table of integers gives the number of 1 bits
 *          in the 8 bit index.
 */
LEPTONICA_EXPORT l_int32 *
makePixelSumTab8(void)
{
l_uint8   byte;
l_int32   i;
l_int32  *tab;

    PROCNAME("makePixelSumTab8");

    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 256; i++) {
        byte = (l_uint8)i;
        tab[i] = (byte & 0x1) +
                 ((byte >> 1) & 0x1) +
                 ((byte >> 2) & 0x1) +
                 ((byte >> 3) & 0x1) +
                 ((byte >> 4) & 0x1) +
                 ((byte >> 5) & 0x1) +
                 ((byte >> 6) & 0x1) +
                 ((byte >> 7) & 0x1);
    }

    return tab;
}


/*!
 *  makePixelCentroidTab8()
 *
 *      Input:  void
 *      Return: table of 256 l_int32, or null on error
 *
 *  Notes:
 *      (1) This table of integers gives the centroid weight of the 1 bits
 *          in the 8 bit index.  In other words, if sumtab is obtained by
 *          makePixelSumTab8, and centroidtab is obtained by
 *          makePixelCentroidTab8, then, for 1 <= i <= 255,
 *          centroidtab[i] / (float)sumtab[i]
 *          is the centroid of the 1 bits in the 8-bit index i, where the
 *          MSB is considered to have position 0 and the LSB is considered
 *          to have position 7.
 */
LEPTONICA_EXPORT l_int32 *
makePixelCentroidTab8(void)
{
l_int32   i;
l_int32  *tab;

    PROCNAME("makePixelCentroidTab8");

    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("tab not made", procName, NULL);

    tab[0] = 0;
    tab[1] = 7;
    for (i = 2; i < 4; i++) {
        tab[i] = tab[i - 2] + 6;
    }
    for (i = 4; i < 8; i++) {
        tab[i] = tab[i - 4] + 5;
    }
    for (i = 8; i < 16; i++) {
        tab[i] = tab[i - 8] + 4;
    }
    for (i = 16; i < 32; i++) {
        tab[i] = tab[i - 16] + 3;
    }
    for (i = 32; i < 64; i++) {
        tab[i] = tab[i - 32] + 2;
    }
    for (i = 64; i < 128; i++) {
        tab[i] = tab[i - 64] + 1;
    }
    for (i = 128; i < 256; i++) {
        tab[i] = tab[i - 128];
    }

    return tab;
}


/*-------------------------------------------------------------*
 *                       Sum of pixel values                   *
 *-------------------------------------------------------------*/
/*!
 *  pixSumPixelValues()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp; not cmapped)
 *              box (<optional> if null, use entire image)
 *              &sum (<return> sum of pixel values in region)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixSumPixelValues(PIX        *pix,
                  BOX        *box,
                  l_float64  *psum)
{
l_int32    w, h, d, wpl, i, j, xstart, xend, ystart, yend, bw, bh;
l_uint32  *data, *line;
l_float64  sum;
BOX       *boxc;

    PROCNAME("pixSumPixelValues");

    if (!psum)
        return ERROR_INT("psum not defined", procName, 1);
    *psum = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pixGetColormap(pix) != NULL)
        return ERROR_INT("pix is colormapped", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return ERROR_INT("pix not 1, 2, 4, 8, 16 or 32 bpp", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    boxc = NULL;
    if (box) {
        boxc = boxClipToRectangle(box, w, h);
        if (!boxc)
            return ERROR_INT("box outside image", procName, 1);
    }
    xstart = ystart = 0;
    xend = w;
    yend = h;
    if (boxc) {
        boxGetGeometry(boxc, &xstart, &ystart, &bw, &bh);
        xend = xstart + bw;  /* 1 past the end */
        yend = ystart + bh;  /* 1 past the end */
        boxDestroy(&boxc);
    }

    sum = 0;
    for (i = ystart; i < yend; i++) {
        line = data + i * wpl;
        for (j = xstart; j < xend; j++) {
            if (d == 1)
                sum += GET_DATA_BIT(line, j);
            else if (d == 2)
                sum += GET_DATA_DIBIT(line, j);
            else if (d == 4)
                sum += GET_DATA_QBIT(line, j);
            else if (d == 8)
                sum += GET_DATA_BYTE(line, j);
            else if (d == 16)
                sum += GET_DATA_TWO_BYTES(line, j);
            else if (d == 32)
                sum += line[j];
        }
    }
    *psum = sum;

    return 0;
}



/*!
 *  findTilePatchCenter()
 *
 *      Input:  pixs (8 or 16 bpp; distance function of a binary mask)
 *              box (region of pixs to search around)
 *              searchdir (L_HORIZ or L_VERT; direction to search)
 *              targdist (desired distance of selected patch center from fg)
 *              &dist (<return> actual distance of selected location)
 *              &xc, &yc (<return> location of selected patch center)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This looks for a patch of non-masked image, that is outside
 *          but near the input box.  The input pixs is a distance
 *          function giving the distance from the fg in a binary mask.
 *      (2) The target distance implicitly specifies a desired size
 *          for the patch.  The location of the center of the patch,
 *          and the actual distance from fg are returned.
 *      (3) If the target distance is larger than 255, a 16-bit distance
 *          transform is input.
 *      (4) It is assured that a square centered at (xc, yc) and of
 *          size 'dist' will not intersect with the fg of the binary
 *          mask that was used to generate pixs.
 *      (5) We search away from the component, in approximately
 *          the center 1/3 of its dimension.  This gives a better chance
 *          of finding patches that are close to the component.
 */
static l_int32
findTilePatchCenter(PIX       *pixs,
                    BOX       *box,
                    l_int32    searchdir,
                    l_uint32   targdist,
                    l_uint32  *pdist,
                    l_int32   *pxc,
                    l_int32   *pyc)
{
l_int32   w, h, bx, by, bw, bh, left, right, top, bot, i, j;
l_int32   xstart, xend, ystart, yend;
l_uint32  val, maxval;

    PROCNAME("findTilePatchCenter");

    if (!pdist || !pxc || !pyc)
        return ERROR_INT("&pdist, &pxc, &pyc not all defined", procName, 1);
    *pdist = *pxc = *pyc = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    boxGetGeometry(box, &bx, &by, &bw, &bh);

    if (searchdir == L_HORIZ) {
        left = bx;   /* distance to left of box */
        right = w - bx - bw + 1;   /* distance to right of box */
        ystart = by + bh / 3;
        yend = by + 2 * bh / 3;
        maxval = 0;
        if (left > right) {  /* search to left */
            for (j = bx - 1; j >= 0; j--) {
                for (i = ystart; i <= yend; i++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
        else {  /* search to right */
            for (j = bx + bw; j < w; j++) {
                for (i = ystart; i <= yend; i++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
    }
    else {  /* searchdir == L_VERT */
        top = by;    /* distance above box */
        bot = h - by - bh + 1;   /* distance below box */
        xstart = bx + bw / 3;
        xend = bx + 2 * bw / 3;
        maxval = 0;
        if (top > bot) {  /* search above */
            for (i = by - 1; i >= 0; i--) {
                for (j = xstart; j <=xend; j++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
        else {  /* search below */
            for (i = by + bh; i < h; i++) {
                for (j = xstart; j <=xend; j++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
    }


    pixGetPixel(pixs, *pxc, *pyc, pdist);
    return 0;
}

