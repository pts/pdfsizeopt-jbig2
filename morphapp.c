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
 *  morphapp.c
 *
 *      These are some useful and/or interesting composite
 *      image processing operations, of the type that are often
 *      useful in applications.  Most are morphological in
 *      nature.
 *
 *      Extraction of boundary pixels
 *            PIX       *pixExtractBoundary()
 *
 *      Selective morph sequence operation under mask
 *            PIX       *pixMorphSequenceMasked()
 *
 *      Selective morph sequence operation on each component
 *            PIX       *pixMorphSequenceByComponent()
 *            PIXA      *pixaMorphSequenceByComponent()
 *
 *      Selective morph sequence operation on each region
 *            PIX       *pixMorphSequenceByRegion()
 *            PIXA      *pixaMorphSequenceByRegion()
 *
 *      Union and intersection of parallel composite operations
 *            PIX       *pixUnionOfMorphOps()
 *            PIX       *pixIntersectionOfMorphOps()
 *
 *      Selective connected component filling
 *            PIX       *pixSelectiveConnCompFill()
 *
 *      Removal of matched patterns
 *            PIX       *pixRemoveMatchedPattern()
 *
 *      Display of matched patterns
 *            PIX       *pixDisplayMatchedPattern()
 *
 *      Iterative morphological seed filling (don't use for real work)
 *            PIX       *pixSeedfillMorph()
 *      
 *      Granulometry on binary images
 *            NUMA      *pixRunHistogramMorph()
 *
 *      Composite operations on grayscale images
 *            PIX       *pixTophat()
 *            PIX       *pixHDome()
 *            PIX       *pixFastTophat()
 *            PIX       *pixMorphGradient()
 *
 *      Centroid of component
 *            PTA       *pixaCentroids()
 *            l_int32    pixCentroid()
 */

#include "allheaders.h"

#define   SWAP(x, y)   {temp = (x); (x) = (y); (y) = temp;}


/*-----------------------------------------------------------------*
 *                   Extraction of boundary pixels                 *
 *-----------------------------------------------------------------*/
/*!
 *  pixExtractBoundary()
 *
 *      Input:  pixs (1 bpp)
 *              type (0 for background pixels; 1 for foreground pixels)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Extracts the fg or bg boundary pixels for each component.
 *          Components are assumed to end at the boundary of pixs.
 */
LEPTONICA_EXPORT PIX *
pixExtractBoundary(PIX     *pixs,
                   l_int32  type)
{
PIX  *pixd;

    PROCNAME("pixExtractBoundary");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (type == 0)
        pixd = pixDilateBrick(NULL, pixs, 3, 3);
    else
        pixd = pixErodeBrick(NULL, pixs, 3, 3);
    pixXor(pixd, pixd, pixs);
    return pixd;
}


/*-----------------------------------------------------------------*
 *           Selective morph sequence operation under mask         *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceMasked()
 *
 *      Input:  pixs (1 bpp)
 *              pixm (<optional> 1 bpp mask)
 *              sequence (string specifying sequence of operations)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This applies the morph sequence to the image, but only allows
 *          changes in pixs for pixels under the background of pixm.
 *      (5) If pixm is NULL, this is just pixMorphSequence().
 */
LEPTONICA_EXPORT PIX *
pixMorphSequenceMasked(PIX         *pixs,
                       PIX         *pixm,
                       const char  *sequence,
                       l_int32      dispsep)
{
PIX  *pixd;

    PROCNAME("pixMorphSequenceMasked");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    pixd = pixMorphSequence(pixs, sequence, dispsep);
    pixCombineMasked(pixd, pixs, pixm);  /* restore src pixels under mask fg */
    return pixd;
}


/*-----------------------------------------------------------------*
 *             Morph sequence operation on each component          *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceByComponent()
 *
 *      Input:  pixs (1 bpp)
 *              sequence (string specifying sequence)
 *              connectivity (4 or 8)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *              &boxa (<optional> return boxa of c.c. in pixs)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each c.c. in the input pix.
 *      (3) The dilation does NOT increase the c.c. size; it is clipped
 *          to the size of the original c.c.   This is necessary to
 *          keep the c.c. independent after the operation.
 *      (4) You can specify that the width and/or height must equal
 *          or exceed a minimum size for the operation to take place.
 *      (5) Use NULL for boxa to avoid returning the boxa.
 */
LEPTONICA_EXPORT PIX *
pixMorphSequenceByComponent(PIX         *pixs,
                            const char  *sequence,
                            l_int32      connectivity,
                            l_int32      minw,
                            l_int32      minh,
                            BOXA       **pboxa)
{
l_int32  n, i, x, y, w, h;
BOXA    *boxa;
PIX     *pix, *pixd;
PIXA    *pixas, *pixad;

    PROCNAME("pixMorphSequenceByComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

        /* Get the c.c. */
    if ((boxa = pixConnComp(pixs, &pixas, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("boxa not made", procName, NULL);

        /* Operate on each c.c. independently */
    pixad = pixaMorphSequenceByComponent(pixas, sequence, minw, minh);
    pixaDestroy(&pixas);
    boxaDestroy(&boxa);
    if (!pixad)
        return (PIX *)ERROR_PTR("pixad not made", procName, NULL);

        /* Display the result out into pixd */
    pixd = pixCreateTemplate(pixs);
    n = pixaGetCount(pixad);
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixad, i, &x, &y, &w, &h);
        pix = pixaGetPix(pixad, i, L_CLONE);
        pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix, 0, 0);
        pixDestroy(&pix);
    }

    if (pboxa)
        *pboxa = pixaGetBoxa(pixad, L_CLONE);
    pixaDestroy(&pixad);
    return pixd;
}


/*!
 *  pixaMorphSequenceByComponent()
 *
 *      Input:  pixas (of 1 bpp pix)
 *              sequence (string specifying sequence)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each c.c. in the input pixa.
 *      (3) You can specify that the width and/or height must equal
 *          or exceed a minimum size for the operation to take place.
 *      (4) The input pixa should have a boxa giving the locations
 *          of the pix components.
 */
LEPTONICA_EXPORT PIXA *
pixaMorphSequenceByComponent(PIXA        *pixas,
                             const char  *sequence,
                             l_int32      minw,
                             l_int32      minh)
{
l_int32  n, i, w, h, d;
BOX     *box;
PIX     *pixt1, *pixt2;
PIXA    *pixad;

    PROCNAME("pixaMorphSequenceByComponent");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);
    if ((n = pixaGetCount(pixas)) == 0)
        return (PIXA *)ERROR_PTR("no pix in pixas", procName, NULL);
    if (n != pixaGetBoxaCount(pixas))
        L_WARNING("boxa size != n", procName);
    pixaGetPixDimensions(pixas, 0, NULL, NULL, &d);
    if (d != 1)
        return (PIXA *)ERROR_PTR("depth not 1 bpp", procName, NULL);

    if (!sequence)
        return (PIXA *)ERROR_PTR("sequence not defined", procName, NULL);
    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

    if ((pixad = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pixaGetPixDimensions(pixas, i, &w, &h, NULL);
        if (w >= minw && h >= minh) {
            if ((pixt1 = pixaGetPix(pixas, i, L_CLONE)) == NULL)
                return (PIXA *)ERROR_PTR("pixt1 not found", procName, NULL);
            if ((pixt2 = pixMorphCompSequence(pixt1, sequence, 0)) == NULL)
                return (PIXA *)ERROR_PTR("pixt2 not made", procName, NULL);
            pixaAddPix(pixad, pixt2, L_INSERT);
            box = pixaGetBox(pixas, i, L_COPY);
            pixaAddBox(pixad, box, L_INSERT);
            pixDestroy(&pixt1);
        }
    }

    return pixad;
}


/*-----------------------------------------------------------------*
 *              Morph sequence operation on each region            *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceByRegion()
 *
 *      Input:  pixs (1 bpp)
 *              pixm (mask specifying regions)
 *              sequence (string specifying sequence)
 *              connectivity (4 or 8, used on mask)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *              &boxa (<optional> return boxa of c.c. in pixm)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See pixMorphCompSequence() for composing operation sequences.
 *      (2) This operates separately on the region in pixs corresponding
 *          to each c.c. in the mask pixm.  It differs from
 *          pixMorphSequenceByComponent() in that the latter does not have
 *          a pixm (mask), but instead operates independently on each
 *          component in pixs.
 *      (3) Dilation will NOT increase the region size; the result
 *          is clipped to the size of the mask region.  This is necessary
 *          to make regions independent after the operation.
 *      (4) You can specify that the width and/or height of a region must
 *          equal or exceed a minimum size for the operation to take place.
 *      (5) Use NULL for @pboxa to avoid returning the boxa.
 */
LEPTONICA_EXPORT PIX *
pixMorphSequenceByRegion(PIX         *pixs,
                         PIX         *pixm,
                         const char  *sequence,
                         l_int32      connectivity,
                         l_int32      minw,
                         l_int32      minh,
                         BOXA       **pboxa)
{
l_int32  n, i, x, y, w, h;
BOXA    *boxa;
PIX     *pix, *pixd;
PIXA    *pixam, *pixad;

    PROCNAME("pixMorphSequenceByRegion");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixm)
        return (PIX *)ERROR_PTR("pixm not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1 || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixs and pixm not both 1 bpp", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

        /* Get the c.c. of the mask */
    if ((boxa = pixConnComp(pixm, &pixam, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("boxa not made", procName, NULL);

        /* Operate on each region in pixs independently */
    pixad = pixaMorphSequenceByRegion(pixs, pixam, sequence, minw, minh);
    pixaDestroy(&pixam);
    boxaDestroy(&boxa);
    if (!pixad)
        return (PIX *)ERROR_PTR("pixad not made", procName, NULL);

        /* Display the result out into pixd */
    pixd = pixCreateTemplate(pixs);
    n = pixaGetCount(pixad);
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixad, i, &x, &y, &w, &h);
        pix = pixaGetPix(pixad, i, L_CLONE);
        pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix, 0, 0);
        pixDestroy(&pix);
    }

    if (pboxa)
        *pboxa = pixaGetBoxa(pixad, L_CLONE);
    pixaDestroy(&pixad);
    return pixd;
}


/*!
 *  pixaMorphSequenceByRegion()
 *
 *      Input:  pixs (1 bpp)
 *              pixam (of 1 bpp mask elements)
 *              sequence (string specifying sequence)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each region in the input pixs
 *          defined by the components in pixam.
 *      (3) You can specify that the width and/or height of a mask
 *          component must equal or exceed a minimum size for the
 *          operation to take place.
 *      (4) The input pixam should have a boxa giving the locations
 *          of the regions in pixs.
 */
LEPTONICA_EXPORT PIXA *
pixaMorphSequenceByRegion(PIX         *pixs,
                          PIXA        *pixam,
                          const char  *sequence,
                          l_int32      minw,
                          l_int32      minh)
{
l_int32  n, i, w, h, d;
BOX     *box;
PIX     *pixt1, *pixt2, *pixt3;
PIXA    *pixad;

    PROCNAME("pixaMorphSequenceByRegion");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIXA *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (!pixam)
        return (PIXA *)ERROR_PTR("pixam not defined", procName, NULL);
    pixaGetPixDimensions(pixam, 0, NULL, NULL, &d);
    if (d != 1)
        return (PIXA *)ERROR_PTR("mask depth not 1 bpp", procName, NULL);
    if ((n = pixaGetCount(pixam)) == 0)
        return (PIXA *)ERROR_PTR("no regions specified", procName, NULL);
    if (n != pixaGetBoxaCount(pixam))
        L_WARNING("boxa size != n", procName);
    if (!sequence)
        return (PIXA *)ERROR_PTR("sequence not defined", procName, NULL);

    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

    if ((pixad = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);

        /* Use the rectangle to remove the appropriate part of pixs;
         * then AND with the mask component to get the actual fg
         * of pixs that is under the mask component. */
    for (i = 0; i < n; i++) {
        pixaGetPixDimensions(pixam, i, &w, &h, NULL);
        if (w >= minw && h >= minh) {
            if ((pixt1 = pixaGetPix(pixam, i, L_CLONE)) == NULL)
                return (PIXA *)ERROR_PTR("pixt1 not found", procName, NULL);
            box = pixaGetBox(pixam, i, L_COPY);
            pixt2 = pixClipRectangle(pixs, box, NULL);
            pixAnd(pixt2, pixt2, pixt1);
            if ((pixt3 = pixMorphCompSequence(pixt2, sequence, 0)) == NULL)
                return (PIXA *)ERROR_PTR("pixt3 not made", procName, NULL);
            pixaAddPix(pixad, pixt3, L_INSERT);
            pixaAddBox(pixad, box, L_INSERT);
            pixDestroy(&pixt1);
            pixDestroy(&pixt2);
        }
    }

    return pixad;
}


/*-----------------------------------------------------------------*
 *      Union and intersection of parallel composite operations    *
 *-----------------------------------------------------------------*/
/*!
 *  pixUnionOfMorphOps()
 *
 *      Input:  pixs (binary)
 *              sela
 *              type (L_MORPH_DILATE, etc.)
 *      Return: pixd (union of the specified morphological operation
 *                    on pixs for each Sel in the Sela), or null on error
 */
LEPTONICA_EXPORT PIX *
pixUnionOfMorphOps(PIX     *pixs,
                   SELA    *sela,
                   l_int32  type)
{
l_int32  n, i;
PIX     *pixt, *pixd;
SEL     *sel;

    PROCNAME("pixUnionOfMorphOps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    n = selaGetCount(sela);
    if (n == 0)
        return (PIX *)ERROR_PTR("no sels in sela", procName, NULL);
    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE &&
        type != L_MORPH_OPEN && type != L_MORPH_CLOSE &&
        type != L_MORPH_HMT)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixd = pixCreateTemplate(pixs);
    for (i = 0; i < n; i++) {
        sel = selaGetSel(sela, i);
        if (type == L_MORPH_DILATE)
            pixt = pixDilate(NULL, pixs, sel);
        else if (type == L_MORPH_ERODE)
            pixt = pixErode(NULL, pixs, sel);
        else if (type == L_MORPH_OPEN)
            pixt = pixOpen(NULL, pixs, sel);
        else if (type == L_MORPH_CLOSE)
            pixt = pixClose(NULL, pixs, sel);
        else  /* type == L_MORPH_HMT */
            pixt = pixHMT(NULL, pixs, sel);
        pixOr(pixd, pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*!
 *  pixIntersectionOfMorphOps()
 *
 *      Input:  pixs (binary)
 *              sela 
 *              type (L_MORPH_DILATE, etc.)
 *      Return: pixd (intersection of the specified morphological operation
 *                    on pixs for each Sel in the Sela), or null on error
 */
LEPTONICA_EXPORT PIX *
pixIntersectionOfMorphOps(PIX     *pixs,
                          SELA    *sela,
                          l_int32  type)
{
l_int32  n, i;
PIX     *pixt, *pixd;
SEL     *sel;

    PROCNAME("pixIntersectionOfMorphOps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    n = selaGetCount(sela);
    if (n == 0)
        return (PIX *)ERROR_PTR("no sels in sela", procName, NULL);
    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE &&
        type != L_MORPH_OPEN && type != L_MORPH_CLOSE &&
        type != L_MORPH_HMT)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixd = pixCreateTemplate(pixs);
    pixSetAll(pixd);
    for (i = 0; i < n; i++) {
        sel = selaGetSel(sela, i);
        if (type == L_MORPH_DILATE)
            pixt = pixDilate(NULL, pixs, sel);
        else if (type == L_MORPH_ERODE)
            pixt = pixErode(NULL, pixs, sel);
        else if (type == L_MORPH_OPEN)
            pixt = pixOpen(NULL, pixs, sel);
        else if (type == L_MORPH_CLOSE)
            pixt = pixClose(NULL, pixs, sel);
        else  /* type == L_MORPH_HMT */
            pixt = pixHMT(NULL, pixs, sel);
        pixAnd(pixd, pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}



/*-----------------------------------------------------------------*
 *                    Removal of matched patterns                  *
 *-----------------------------------------------------------------*/
/*!
 *  pixRemoveMatchedPattern()
 *
 *      Input:  pixs (input image, 1 bpp)
 *              pixp (pattern to be removed from image, 1 bpp)
 *              pixe (image after erosion by Sel that approximates pixp, 1 bpp)
 *              x0, y0 (center of Sel)
 *              dsize (number of pixels on each side by which pixp is
 *                     dilated before being subtracted from pixs;
 *                     valid values are {0, 1, 2, 3, 4})
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) This is in-place.
 *    (2) You can use various functions in selgen to create a Sel
 *        that is used to generate pixe from pixs.
 *    (3) This function is applied after pixe has been computed.
 *        It finds the centroid of each c.c., and subtracts
 *        (the appropriately dilated version of) pixp, with the center
 *        of the Sel used to align pixp with pixs.
 */
LEPTONICA_EXPORT l_int32
pixRemoveMatchedPattern(PIX     *pixs,
                        PIX     *pixp,
                        PIX     *pixe,
                        l_int32  x0,
                        l_int32  y0,
                        l_int32  dsize)
{
l_int32  i, nc, x, y, w, h, xb, yb;
BOXA    *boxa;
PIX     *pixt1, *pixt2;
PIXA    *pixa;
PTA     *pta;
SEL     *sel;

    PROCNAME("pixRemoveMatchedPattern");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixp)
        return ERROR_INT("pixp not defined", procName, 1);
    if (!pixe)
        return ERROR_INT("pixe not defined", procName, 1);
    if (pixGetDepth(pixs) != 1 || pixGetDepth(pixp) != 1 ||
        pixGetDepth(pixe) != 1)
        return ERROR_INT("all input pix not 1 bpp", procName, 1);
    if (dsize < 0 || dsize > 4)
        return ERROR_INT("dsize not in {0,1,2,3,4}", procName, 1);

        /* Find the connected components and their centroids */
    boxa = pixConnComp(pixe, &pixa, 8);
    if ((nc = boxaGetCount(boxa)) == 0) {
        L_WARNING("no matched patterns", procName);
        boxaDestroy(&boxa);
        pixaDestroy(&pixa);
        return 0;
    }
    pta = pixaCentroids(pixa);

        /* Optionally dilate the pattern, first adding a border that
         * is large enough to accommodate the dilated pixels */
    sel = NULL;
    if (dsize > 0) {
        sel = selCreateBrick(2 * dsize + 1, 2 * dsize + 1, dsize, dsize,
                             SEL_HIT);
        pixt1 = pixAddBorder(pixp, dsize, 0);
        pixt2 = pixDilate(NULL, pixt1, sel);
        selDestroy(&sel);
        pixDestroy(&pixt1);
    }
    else
        pixt2 = pixClone(pixp);

        /* Subtract out each dilated pattern.  The centroid of each
         * component is located at:
         *       (box->x + x, box->y + y)
         * and the 'center' of the pattern used in making pixe is located at
         *       (x0 + dsize, (y0 + dsize)
         * relative to the UL corner of the pattern.  The center of the
         * pattern is placed at the center of the component. */
    w = pixGetWidth(pixt2);
    h = pixGetHeight(pixt2);
    for (i = 0; i < nc; i++) {
        ptaGetIPt(pta, i, &x, &y);
        boxaGetBoxGeometry(boxa, i, &xb, &yb, NULL, NULL);
        pixRasterop(pixs, xb + x - x0 - dsize, yb + y - y0 - dsize,
                    w, h, PIX_DST & PIX_NOT(PIX_SRC), pixt2, 0, 0);
    }

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    ptaDestroy(&pta);
    pixDestroy(&pixt2);
    return 0;
}


/*-----------------------------------------------------------------*
 *                   Granulometry on binary images                 *
 *-----------------------------------------------------------------*/
/*!
 *  pixRunHistogramMorph()
 *
 *      Input:  pixs
 *              runtype (L_RUN_OFF, L_RUN_ON)
 *              direction (L_HORIZ, L_VERT)
 *              maxsize  (size of largest runlength counted)
 *      Return: numa of run-lengths
 */
LEPTONICA_EXPORT NUMA *
pixRunHistogramMorph(PIX     *pixs,
                     l_int32  runtype,
                     l_int32  direction,
                     l_int32  maxsize)
{
l_int32    count, i;
l_float32  val;
NUMA      *na, *nah;
PIX       *pixt1, *pixt2, *pixt3;
SEL       *sel_2a;

    PROCNAME("pixRunHistogramMorph");

    if (!pixs)
        return (NUMA *)ERROR_PTR("seed pix not defined", procName, NULL);
    if (runtype != L_RUN_OFF && runtype != L_RUN_ON)
        return (NUMA *)ERROR_PTR("invalid run type", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (NUMA *)ERROR_PTR("direction not in {L_HORIZ, L_VERT}",
                                 procName, NULL);

    if (pixGetDepth(pixs) != 1)
        return (NUMA *)ERROR_PTR("pixs must be binary", procName, NULL);

    if ((na = numaCreate(0)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);

    if (direction == L_HORIZ)
        sel_2a = selCreateBrick(1, 2, 0, 0, 1);
    else   /* direction == L_VERT */
        sel_2a = selCreateBrick(2, 1, 0, 0, 1);
    if (!sel_2a)
        return (NUMA *)ERROR_PTR("sel_2a not made", procName, NULL);

    if (runtype == L_RUN_OFF) {
        if ((pixt1 = pixCopy(NULL, pixs)) == NULL)
            return (NUMA *)ERROR_PTR("pix1 not made", procName, NULL);
        pixInvert(pixt1, pixt1);
    }
    else  /* runtype == L_RUN_ON */
        pixt1 = pixClone(pixs);

    if ((pixt2 = pixCreateTemplate(pixs)) == NULL)
        return (NUMA *)ERROR_PTR("pix2 not made", procName, NULL);
    if ((pixt3 = pixCreateTemplate(pixs)) == NULL)
        return (NUMA *)ERROR_PTR("pix3 not made", procName, NULL);

        /* Get pixel counts at different stages of erosion */
    pixCountPixels(pixt1, &count, NULL);
    numaAddNumber(na, count);
    pixErode(pixt2, pixt1, sel_2a);
    pixCountPixels(pixt2, &count, NULL);
    numaAddNumber(na, count);
    for (i = 0; i < maxsize / 2; i++) {
        pixErode(pixt3, pixt2, sel_2a);
        pixCountPixels(pixt3, &count, NULL);
        numaAddNumber(na, count);
        pixErode(pixt2, pixt3, sel_2a);
        pixCountPixels(pixt2, &count, NULL);
        numaAddNumber(na, count);
    }

        /* Compute length histogram */
    if ((nah = numaCreate(na->n)) == NULL)
        return (NUMA *)ERROR_PTR("nah not made", procName, NULL);
    numaAddNumber(nah, 0); /* number at length 0 */
    for (i = 1; i < na->n - 1; i++) {
        val = na->array[i+1] - 2 * na->array[i] + na->array[i-1];
        numaAddNumber(nah, val);
    }

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    selDestroy(&sel_2a);
    numaDestroy(&na);

    return nah;
}


/*-----------------------------------------------------------------*
 *                       Centroid of component                     *
 *-----------------------------------------------------------------*/
/*!
 *  pixaCentroids()
 *
 *      Input:  pixa of components (1 or 8 bpp)
 *      Return: pta of centroids relative to the UL corner of
 *              each pix, or null on error
 *
 *  Notes:
 *      (1) An error message is returned if any pix has something other
 *          than 1 bpp or 8 bpp depth, and the centroid from that pix
 *          is saved as (0, 0).
 */
LEPTONICA_EXPORT PTA *
pixaCentroids(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *centtab = NULL;
l_int32   *sumtab = NULL;
l_float32  x, y;
PIX       *pix;
PTA       *pta;

    PROCNAME("pixaCentroids");

    if (!pixa)
        return (PTA *)ERROR_PTR("pixa not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PTA *)ERROR_PTR("no pix in pixa", procName, NULL);

    if ((pta = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);
    centtab = makePixelCentroidTab8();
    sumtab = makePixelSumTab8();

    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (pixCentroid(pix, centtab, sumtab, &x, &y) == 1)
            L_ERROR_INT("centroid failure for pix %d", procName, i);
        pixDestroy(&pix);
        ptaAddPt(pta, x, y);
    }

    FREE(centtab);
    FREE(sumtab);
    return pta;
}


/*!
 *  pixCentroid()
 *
 *      Input:  pix (1 or 8 bpp)
 *              centtab (<optional> table for finding centroids; can be null)
 *              sumtab (<optional> table for finding pixel sums; can be null)
 *              &xave, &yave (<return> coordinates of centroid, relative to
 *                            the UL corner of the pix)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Any table not passed in will be made internally and destroyed
 *          after use.
 */
LEPTONICA_EXPORT l_int32
pixCentroid(PIX        *pix,
            l_int32    *centtab,
            l_int32    *sumtab,
            l_float32  *pxave,
            l_float32  *pyave)
{
l_int32    w, h, d, i, j, wpl, pixsum, rowsum, val;
l_float32  xsum, ysum;
l_uint32  *data, *line;
l_uint32   word;
l_uint8    byte;
l_int32   *ctab, *stab;

    PROCNAME("pixCentroid");

    if (!pxave || !pyave)
        return ERROR_INT("&pxave and &pyave not defined", procName, 1);
    *pxave = *pyave = 0.0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8)
        return ERROR_INT("pix not 1 or 8 bpp", procName, 1);

    if (!centtab)
        ctab = makePixelCentroidTab8();
    else
        ctab = centtab;
    if (!sumtab)
        stab = makePixelSumTab8();
    else
        stab = sumtab;

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    xsum = ysum = 0.0;
    pixsum = 0;
    if (d == 1) {
        for (i = 0; i < h; i++) {
                /* The body of this loop computes the sum of the set
                 * (1) bits on this row, weighted by their distance
                 * from the left edge of pix, and accumulates that into
                 * xsum; it accumulates their distance from the top
                 * edge of pix into ysum, and their total count into
                 * pixsum.  It's equivalent to
                 * for (j = 0; j < w; j++) {
                 *     if (GET_DATA_BIT(line, j)) {
                 *         xsum += j;
                 *         ysum += i;
                 *         pixsum++;
                 *     }
                 * }
                 */
            line = data + wpl * i;
            rowsum = 0;
            for (j = 0; j < wpl; j++) {
                word = line[j];
                if (word) {
                    byte = word & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 24) * stab[byte];
                    byte = (word >> 8) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 16) * stab[byte];
                    byte = (word >> 16) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 8) * stab[byte];
                    byte = (word >> 24) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + j * 32 * stab[byte];
                }
            }
            pixsum += rowsum;
            ysum += rowsum * i;
        }
        if (pixsum == 0)
            L_WARNING("no ON pixels in pix", procName);
        else {
            *pxave = xsum / (l_float32)pixsum;
            *pyave = ysum / (l_float32)pixsum;
        }
    }
    else {  /* d == 8 */
        for (i = 0; i < h; i++) {
            line = data + wpl * i;
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(line, j);
                xsum += val * j;
                ysum += val * i;
                pixsum += val;
            }
        }
        if (pixsum == 0)
            L_WARNING("all pixels are 0", procName);
        else {
            *pxave = xsum / (l_float32)pixsum;
            *pyave = ysum / (l_float32)pixsum;
        }
    }

    if (!centtab) FREE(ctab);
    if (!sumtab) FREE(stab);
    return 0;
}


