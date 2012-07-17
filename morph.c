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
 *  morph.c
 *
 *     Generic binary morphological ops implemented with rasterop
 *         PIX     *pixDilate()
 *         PIX     *pixErode()
 *         PIX     *pixHMT()
 *         PIX     *pixOpen()
 *         PIX     *pixClose()
 *         PIX     *pixCloseSafe()
 *         PIX     *pixOpenGeneralized()
 *         PIX     *pixCloseGeneralized()
 *
 *     Binary morphological (raster) ops with brick Sels
 *         PIX     *pixDilateBrick()
 *         PIX     *pixErodeBrick()
 *         PIX     *pixOpenBrick()
 *         PIX     *pixCloseBrick()
 *         PIX     *pixCloseSafeBrick()
 *
 *     Binary composed morphological (raster) ops with brick Sels
 *         l_int32  selectComposableSels()
 *         l_int32  selectComposableSizes()
 *         PIX     *pixDilateCompBrick()
 *         PIX     *pixErodeCompBrick()
 *         PIX     *pixOpenCompBrick()
 *         PIX     *pixCloseCompBrick()
 *         PIX     *pixCloseSafeCompBrick()
 *
 *     Functions associated with boundary conditions
 *         void     resetMorphBoundaryCondition()
 *         l_int32  getMorphBorderPixelColor()
 *      
 *     Static helpers for arg processing
 *         static PIX     *processMorphArgs1()
 *         static PIX     *processMorphArgs2()
 *
 *  You are provided with many simple ways to do binary morphology.
 *  In particular, if you are using brick Sels, there are six
 *  convenient methods, all specially tailored for separable operations
 *  on brick Sels.  A "brick" Sel is a Sel that is a rectangle
 *  of solid SEL_HITs with the origin at or near the center.
 *  Note that a brick Sel can have one dimension of size 1.
 *  This is very common.  All the brick Sel operations are
 *  separable, meaning the operation is done first in the horizontal
 *  direction and then in the vertical direction.  If one of the
 *  dimensions is 1, this is a special case where the operation is
 *  only performed in the other direction.
 *
 *  These six brick Sel methods are enumerated as follows:
 *
 *  (1) Brick Sels: pix*Brick(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable rasterop implementations.  The Sels are
 *      automatically generated, used, and destroyed at the end.
 *      You can get the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.
 *
 *  (2) Brick Sels: pix*CompBrick(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable, 2-way composite, rasterop implementations.
 *      The Sels are automatically generated, used, and destroyed at the end.
 *      You can get the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.  For large Sels, these are
 *      considerably faster than the corresponding pix*Brick() functions.
 *      N.B.:  The size of the Sels that are actually used are typically
 *      close to, but not exactly equal to, the size input to the function.
 *
 *  (3) Brick Sels: pix*BrickDwa(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable dwa (destination word accumulation)
 *      implementations.  They use auto-gen'd dwa code.  You can get
 *      the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.  This is typically
 *      about 3x faster than the analogous rasterop pix*Brick()
 *      function, but it has the limitation that the Sel size must
 *      be less than 63.  This is pre-set to work on a number
 *      of pre-generated Sels.  If you want to use other Sels, the
 *      code can be auto-gen'd for them; see the instructions in morphdwa.c.
 *
 *  (4) Same as (1), but you run it through pixMorphSequence(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  For example, you can
 *      specify a separable 11 x 17 brick opening as "o11.17",
 *      or you can specify the horizontal and vertical operations
 *      explicitly as "o11.1 + o1.11".  See morphseq.c for details.
 *
 *  (5) Same as (2), but you run it through pixMorphCompSequence(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  See morphseq.c for details.
 *
 *  (6) Same as (3), but you run it through pixMorphSequenceDwa(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  See morphseq.c for details.
 *
 *  If you are using Sels that are not bricks, you have two choices:
 *      (a) simplest: use the basic rasterop implementations (pixDilate(), ...)
 *      (b) fastest: generate the destination word accumumlation (dwa)
 *          code for your Sels and compile it with the library.
 *
 *      For an example, see flipdetect.c, which gives implementations
 *      using hit-miss Sels with both the rasterop and dwa versions.
 *      For the latter, the dwa code resides in fliphmtgen.c, and it
 *      was generated by prog/flipselgen.c.  Both the rasterop and dwa
 *      implementations are tested by prog/fliptest.c.
 *
 *  A global constant MORPH_BC is used to set the boundary conditions
 *  for rasterop-based binary morphology.  MORPH_BC, in morph.c,
 *  is set by default to ASYMMETRIC_MORPH_BC for a non-symmetric
 *  convention for boundary pixels in dilation and erosion:
 *      All pixels outside the image are assumed to be OFF
 *      for both dilation and erosion.
 *  To use a symmetric definition, see comments in pixErode()
 *  and reset MORPH_BC to SYMMETRIC_MORPH_BC, using 
 *  resetMorphBoundaryCondition().
 *
 *  Boundary artifacts are possible in closing when the non-symmetric
 *  boundary conditions are used, because foreground pixels very close
 *  to the edge can be removed.  This can be avoided by using either
 *  the symmetric boundary conditions or the function pixCloseSafe(),
 *  which adds a border before the operation and removes it afterwards.
 *
 *  The hit-miss transform (HMT) is the bit-and of 2 erosions:
 *     (erosion of the src by the hits)  &  (erosion of the bit-inverted
 *                                           src by the misses)
 *
 *  The 'generalized opening' is an HMT followed by a dilation that uses
 *  only the hits of the hit-miss Sel.
 *  The 'generalized closing' is a dilation (again, with the hits
 *  of a hit-miss Sel), followed by the HMT.
 *  Both of these 'generalized' functions are idempotent.
 *
 *  These functions are extensively tested in prog/binmorph1_reg.c,
 *  prog/binmorph2_reg.c, and prog/binmorph3_reg.c.
 */

#include <stdio.h>
#include <math.h>
#include "allheaders.h"

    /* Global constant; initialized here; must be declared extern
     * in other files to access it directly.  However, in most
     * cases that is not necessary, because it can be reset
     * using resetMorphBoundaryCondition().  */
LEPT_DLL l_int32  MORPH_BC = ASYMMETRIC_MORPH_BC;

    /* We accept this cost in extra rasterops for decomposing exactly. */
static const l_int32  ACCEPTABLE_COST = 5;

    /* Static helpers for arg processing */
static PIX * processMorphArgs1(PIX *pixd, PIX *pixs, SEL *sel, PIX **ppixt);
static PIX * processMorphArgs2(PIX *pixd, PIX *pixs, SEL *sel);


/*-----------------------------------------------------------------*
 *    Generic binary morphological ops implemented with rasterop   *
 *-----------------------------------------------------------------*/
/*!
 *  pixDilate()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) This dilates src using hits in Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixDilate(NULL, pixs, ...);
 *          (b) pixDilate(pixs, pixs, ...);
 *          (c) pixDilate(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixDilate(PIX  *pixd,
          PIX  *pixs,
          SEL  *sel)
{
l_int32  i, j, w, h, sx, sy, cx, cy, seldata;
PIX     *pixt;

    PROCNAME("pixDilate");

    if ((pixd = processMorphArgs1(pixd, pixs, sel, &pixt)) == NULL)
        return (PIX *)ERROR_PTR("processMorphArgs1 failed", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    pixClearAll(pixd);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            seldata = sel->data[i][j];
            if (seldata == 1) {   /* src | dst */
                pixRasterop(pixd, j - cx, i - cy, w, h, PIX_SRC | PIX_DST,
                            pixt, 0, 0);
            }
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixErode()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) This erodes src using hits in Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixErode(NULL, pixs, ...);
 *          (b) pixErode(pixs, pixs, ...);
 *          (c) pixErode(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixErode(PIX  *pixd,
         PIX  *pixs,
         SEL  *sel)
{
l_int32  i, j, w, h, sx, sy, cx, cy, seldata;
l_int32  xp, yp, xn, yn;
PIX     *pixt;

    PROCNAME("pixErode");

    if ((pixd = processMorphArgs1(pixd, pixs, sel, &pixt)) == NULL)
        return (PIX *)ERROR_PTR("processMorphArgs1 failed", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    pixSetAll(pixd);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            seldata = sel->data[i][j];
            if (seldata == 1) {   /* src & dst */
                    pixRasterop(pixd, cx - j, cy - i, w, h, PIX_SRC & PIX_DST,
                                pixt, 0, 0);
            }
        }
    }

        /* Clear near edges.  We do this for the asymmetric boundary
         * condition convention that implements erosion assuming all
         * pixels surrounding the image are OFF.  If you use a
         * use a symmetric b.c. convention, where the erosion is
         * implemented assuming pixels surrounding the image
         * are ON, these operations are omitted.  */
    if (MORPH_BC == ASYMMETRIC_MORPH_BC) {
        selFindMaxTranslations(sel, &xp, &yp, &xn, &yn);
        if (xp > 0)
            pixRasterop(pixd, 0, 0, xp, h, PIX_CLR, NULL, 0, 0);
        if (xn > 0)
            pixRasterop(pixd, w - xn, 0, xn, h, PIX_CLR, NULL, 0, 0);
        if (yp > 0)
            pixRasterop(pixd, 0, 0, w, yp, PIX_CLR, NULL, 0, 0);
        if (yn > 0)
            pixRasterop(pixd, 0, h - yn, w, yn, PIX_CLR, NULL, 0, 0);
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixOpen()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generic morphological opening, using hits in the Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpen(NULL, pixs, ...);
 *          (b) pixOpen(pixs, pixs, ...);
 *          (c) pixOpen(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixOpen(PIX  *pixd,
        PIX  *pixs,
        SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixOpen");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixErode(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixDilate(pixd, pixt, sel);
    pixDestroy(&pixt);

    return pixd;
}
    
    
/*!
 *  pixClose()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generic morphological closing, using hits in the Sel.
 *      (2) This implementation is a strict dual of the opening if
 *          symmetric boundary conditions are used (see notes at top
 *          of this file).
 *      (3) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (4) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixClose(NULL, pixs, ...);
 *          (b) pixClose(pixs, pixs, ...);
 *          (c) pixClose(pixd, pixs, ...);
 *      (5) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixClose(PIX  *pixd,
         PIX  *pixs,
         SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixClose");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixDilate(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixErode(pixd, pixt, sel);
    pixDestroy(&pixt);

    return pixd;
}

/*-----------------------------------------------------------------*
 *          Binary morphological (raster) ops with brick Sels      *
 *-----------------------------------------------------------------*/
/*!
 *  pixDilateBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixDilateBrick(NULL, pixs, ...);
 *          (b) pixDilateBrick(pixs, pixs, ...);
 *          (c) pixDilateBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixDilateBrick(PIX     *pixd,
               PIX     *pixs,
               l_int32  hsize,
               l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixDilateBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixDilate(pixd, pixs, sel);
        selDestroy(&sel);
    }
    else {
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixs, selh);
        pixd = pixDilate(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixErodeBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixErodeBrick(NULL, pixs, ...);
 *          (b) pixErodeBrick(pixs, pixs, ...);
 *          (c) pixErodeBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixErodeBrick(PIX     *pixd,
              PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixErodeBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixErode(pixd, pixs, sel);
        selDestroy(&sel);
    }
    else {
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixErode(NULL, pixs, selh);
        pixd = pixErode(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixOpenBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpenBrick(NULL, pixs, ...);
 *          (b) pixOpenBrick(pixs, pixs, ...);
 *          (c) pixOpenBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixOpenBrick(PIX     *pixd,
             PIX     *pixs,
             l_int32  hsize,
             l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixOpenBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixOpen(pixd, pixs, sel);
        selDestroy(&sel);
    }
    else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixErode(NULL, pixs, selh);
        pixd = pixErode(pixd, pixt, selv);
        pixDilate(pixt, pixd, selh);
        pixDilate(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixCloseBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseBrick(NULL, pixs, ...);
 *          (b) pixCloseBrick(pixs, pixs, ...);
 *          (c) pixCloseBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixCloseBrick(PIX     *pixd,
              PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixCloseBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixClose(pixd, pixs, sel);
        selDestroy(&sel);
    }
    else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixs, selh);
        pixd = pixDilate(pixd, pixt, selv);
        pixErode(pixt, pixd, selh);
        pixErode(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixCloseSafeBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) Safe closing adds a border of 0 pixels, of sufficient size so
 *          that all pixels in input image are processed within
 *          32-bit words in the expanded image.  As a result, there is
 *          no special processing for pixels near the boundary, and there
 *          are no boundary effects.  The border is removed at the end.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseBrick(NULL, pixs, ...);
 *          (b) pixCloseBrick(pixs, pixs, ...);
 *          (c) pixCloseBrick(pixd, pixs, ...);
 *      (7) The size of the result is determined by pixs.
 */
LEPTONICA_EXPORT PIX *
pixCloseSafeBrick(PIX     *pixd,
                  PIX     *pixs,
                  l_int32  hsize,
                  l_int32  vsize)
{
l_int32  maxtrans, bordsize;
PIX     *pixsb, *pixt, *pixdb;
SEL     *sel, *selh, *selv;

    PROCNAME("pixCloseSafeBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);

        /* Symmetric b.c. handles correctly without added pixels */
    if (MORPH_BC == SYMMETRIC_MORPH_BC)
        return pixCloseBrick(pixd, pixs, hsize, vsize);

    maxtrans = L_MAX(hsize / 2, vsize / 2);
    bordsize = 32 * ((maxtrans + 31) / 32);  /* full 32 bit words */
    pixsb = pixAddBorder(pixs, bordsize, 0);

    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixdb = pixClose(NULL, pixsb, sel);
        selDestroy(&sel);
    }
    else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixsb, selh);
        pixdb = pixDilate(NULL, pixt, selv);
        pixErode(pixt, pixdb, selh);
        pixErode(pixdb, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    pixt = pixRemoveBorder(pixdb, bordsize);
    pixDestroy(&pixsb);
    pixDestroy(&pixdb);

    if (!pixd)
        pixd = pixt;
    else {
        pixCopy(pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*-----------------------------------------------------------------*
 *               Static helpers for arg processing                 *
 *-----------------------------------------------------------------*/
/*!
 *  processMorphArgs1()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *              &pixt (<returned>)
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) This is used for generic erosion, dilation and HMT.
 */
static PIX *
processMorphArgs1(PIX   *pixd,
                  PIX   *pixs,
                  SEL   *sel,
                  PIX  **ppixt)
{
l_int32  sx, sy;

    PROCNAME("processMorphArgs1");

    if (!ppixt)
        return (PIX *)ERROR_PTR("&pixt not defined", procName, pixd);
    *ppixt = NULL;
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    selGetParameters(sel, &sx, &sy, NULL, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("sel of size 0", procName, pixd);

        /* We require pixd to exist and to be the same size as pixs.
         * Further, pixt must be a copy (or clone) of pixs.  */
    if (!pixd) {
        if ((pixd = pixCreateTemplate(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        *ppixt = pixClone(pixs);
    }
    else {
        pixResizeImageData(pixd, pixs);
        if (pixd == pixs) {  /* in-place; must make a copy of pixs */
            if ((*ppixt = pixCopy(NULL, pixs)) == NULL)
                return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
        }
        else
            *ppixt = pixClone(pixs);
    }
    return pixd;
}


/*!
 *  processMorphArgs2()
 *
 *  This is used for generic openings and closings.
 */
static PIX *
processMorphArgs2(PIX   *pixd,
                  PIX   *pixs,
                  SEL   *sel)
{
l_int32  sx, sy;

    PROCNAME("processMorphArgs2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    selGetParameters(sel, &sx, &sy, NULL, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("sel of size 0", procName, pixd);

    if (!pixd)
        return pixCreateTemplate(pixs);
    pixResizeImageData(pixd, pixs);
    return pixd;
}
