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
 *   ptafunc1.c
 *
 *      Pta and Ptaa rearrangements
 *           PTA      *ptaSubsample()
 *           l_int32   ptaJoin()
 *           PTA      *ptaReverse()
 *           PTA      *ptaCyclicPerm()
 *           PTA      *ptaSort()
 *           PTA      *ptaRemoveDuplicates()
 *           PTAA     *ptaaSortByIndex()
 *
 *      Geometric
 *           BOX      *ptaGetBoundingRegion()
 *           l_int32  *ptaGetRange()
 *           PTA      *ptaGetInsideBox()
 *           PTA      *pixFindCornerPixels()
 *           l_int32   ptaContainsPt()
 *           l_int32   ptaTestIntersection()
 *           PTA      *ptaTransform()
 *
 *      Least Squares Fit
 *           l_int32   ptaGetLinearLSF()
 *           l_int32   ptaGetQuadraticLSF()
 *           l_int32   ptaGetCubicLSF()
 *           l_int32   ptaGetQuarticLSF()
 *           l_int32   applyLinearFit()
 *           l_int32   applyQuadraticFit()
 *           l_int32   applyCubicFit()
 *           l_int32   applyQuarticFit()
 *
 *      Interconversions with Pix
 *           l_int32   pixPlotAlongPta()
 *           PTA      *ptaGetPixelsFromPix()
 *           PIX      *pixGenerateFromPta()
 *           PTA      *ptaGetBoundaryPixels()
 *           PTAA     *ptaaGetBoundaryPixels()
 *
 *      Display Pta and Ptaa
 *           PIX      *pixDisplayPta()
 *           PIX      *pixDisplayPtaa()
 */

#include <string.h>
#include "allheaders.h"

    /* Default spreading factor for hashing pts in a plane */
static const l_int32  DEFAULT_SPREADING_FACTOR = 7500;


/*---------------------------------------------------------------------*
 *                           Pta rearrangements                        *
 *---------------------------------------------------------------------*/


/*!
 *  ptaJoin()
 *
 *      Input:  ptad  (dest pta; add to this one)
 *              ptas  (source pta; add from this one)
 *              istart  (starting index in ptas)
 *              iend  (ending index in ptas; use 0 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (2) iend <= 0 means 'read to the end'
 */
LEPTONICA_EXPORT l_int32
ptaJoin(PTA     *ptad,
        PTA     *ptas,
        l_int32  istart,
        l_int32  iend)
{
l_int32  ns, i, x, y;

    PROCNAME("ptaJoin");

    if (!ptad)
        return ERROR_INT("ptad not defined", procName, 1);
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    ns = ptaGetCount(ptas);
    if (istart < 0)
        istart = 0;
    if (istart >= ns)
        return ERROR_INT("istart out of bounds", procName, 1);
    if (iend <= 0)
        iend = ns - 1;
    if (iend >= ns)
        return ERROR_INT("iend out of bounds", procName, 1);
    if (istart > iend)
        return ERROR_INT("istart > iend; no pts", procName, 1);

    for (i = istart; i <= iend; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                            Least Squares Fit                        *
 *---------------------------------------------------------------------*/

/*!
 *  ptaGetPixelsFromPix()
 *
 *      Input:  pixs (1 bpp)
 *              box (<optional> can be null)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) Generates a pta of fg pixels in the pix, within the box.
 *          If box == NULL, it uses the entire pix.
 */
LEPTONICA_EXPORT PTA *
ptaGetPixelsFromPix(PIX  *pixs,
                    BOX  *box)
{
l_int32    i, j, w, h, wpl, xstart, xend, ystart, yend, bw, bh;
l_uint32  *data, *line;
PTA       *pta;

    PROCNAME("ptaGetPixelsFromPix");

    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PTA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    xstart = ystart = 0;
    xend = w - 1;
    yend = h - 1;
    if (box) {
        boxGetGeometry(box, &xstart, &ystart, &bw, &bh);
        xend = xstart + bw - 1;
        yend = ystart + bh - 1;
    }

    if ((pta = ptaCreate(0)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = ystart; i <= yend; i++) {
        line = data + i * wpl;
        for (j = xstart; j <= xend; j++) {
            if (GET_DATA_BIT(line, j))
                ptaAddPt(pta, j, i);
        }
    }

    return pta;
}
