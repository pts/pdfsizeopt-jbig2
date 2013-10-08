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

#ifndef  NO_CONSOLE_IO
#define   EQUAL_SIZE_WARNING      0
#endif  /* ~NO_CONSOLE_IO */


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


/*-------------------------------------------------------------*
 *                         Pixel counting                      *
 *-------------------------------------------------------------*/

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
