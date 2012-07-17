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


