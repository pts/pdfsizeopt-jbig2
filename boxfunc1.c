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
 *   boxfunc1.c
 *
 *      Box geometry
 *           l_int32   boxContains()
 *           l_int32   boxIntersects()
 *           BOXA     *boxaContainedInBox()
 *           BOXA     *boxaIntersectsBox()
 *           BOXA     *boxaClipToBox()
 *           BOXA     *boxaCombineOverlaps()
 *           BOX      *boxOverlapRegion()
 *           BOX      *boxBoundingRegion()
 *           l_int32   boxOverlapFraction()
 *           l_int32   boxContainsPt()
 *           BOX      *boxaGetNearestToPt()
 *           l_int32   boxIntersectByLine()
 *           l_int32   boxGetCenter()
 *           BOX      *boxClipToRectangle()
 *           BOX      *boxRelocateOneSide()
 *           BOX      *boxAdjustSides()
 *           l_int32   boxEqual()
 *           l_int32   boxaEqual()
 *
 *      Boxa combination
 *           l_int32   boxaJoin()
 *
 *      Other boxa functions
 *           l_int32   boxaGetExtent()
 *           l_int32   boxaGetCoverage()
 *           l_int32   boxaSizeRange()
 *           l_int32   boxaLocationRange()
 *           BOXA     *boxaSelectBySize()
 *           NUMA     *boxaMakeSizeIndicator()
 *           BOXA     *boxaSelectWithIndicator()
 *           BOXA     *boxaPermutePseudorandom()
 *           BOXA     *boxaPermuteRandom()
 *           l_int32   boxaSwapBoxes()
 *           PTA      *boxaConvertToPta()
 *           BOXA     *ptaConvertToBoxa()
 *
 */

#include "allheaders.h"

/*---------------------------------------------------------------------*
 *                             Box geometry                            *
 *---------------------------------------------------------------------*/
/*!
 *  boxContains()
 *
 *      Input:  box1, box2
 *              &result (<return> 1 if box2 is entirely contained within
 *                       box1, and 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxContains(BOX     *box1,
            BOX     *box2,
            l_int32 *presult)
{
    PROCNAME("boxContains");

    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    if ((box1->x <= box2->x) &&
        (box1->y <= box2->y) &&
        (box1->x + box1->w >= box2->x + box2->w) &&
        (box1->y + box1->h >= box2->y + box2->h))
        *presult = 1;
    else
        *presult = 0;
    return 0;
}
                


/*!
 *  boxIntersects()
 *
 *      Input:  box1, box2
 *              &result (<return> 1 if any part of box2 is contained
 *                      in box1, and 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxIntersects(BOX      *box1,
              BOX      *box2,
              l_int32  *presult)
{
l_int32  left1, left2, top1, top2, right1, right2, bot1, bot2;

    PROCNAME("boxIntersects");

    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    left1 = box1->x;
    left2 = box2->x;
    top1 = box1->y;
    top2 = box2->y;
    right1 = box1->x + box1->w - 1;
    bot1 = box1->y + box1->h - 1;
    right2 = box2->x + box2->w - 1;
    bot2 = box2->y + box2->h - 1;
    if ((bot2 >= top1) && (bot1 >= top2) &&
         (right1 >= left2) && (right2 >= left1))
        *presult = 1;
    else
        *presult = 0;
    return 0;
}


/*!
 *  boxOverlapRegion()
 *
 *      Input:  box1, box2 (two boxes)
 *      Return: box (of overlap region between input boxes),
 *              or null if no overlap or on error
 */
LEPTONICA_EXPORT BOX *
boxOverlapRegion(BOX  *box1,
                 BOX  *box2)
{
l_int32  x, y, w, h, left1, left2, top1, top2, right1, right2, bot1, bot2;

    PROCNAME("boxOverlapRegion");

    if (!box1)
        return (BOX *)ERROR_PTR("box1 not defined", procName, NULL);
    if (!box2)
        return (BOX *)ERROR_PTR("box2 not defined", procName, NULL);

    left1 = box1->x;
    left2 = box2->x;
    top1 = box1->y;
    top2 = box2->y;
    right1 = box1->x + box1->w - 1;
    bot1 = box1->y + box1->h - 1;
    right2 = box2->x + box2->w - 1;
    bot2 = box2->y + box2->h - 1;
    if ((bot2 < top1) || (bot1 < top2) ||
         (right1 < left2) || (right2 < left1))
        return NULL;

    x = (left1 > left2) ? left1 : left2;
    y = (top1 > top2) ? top1 : top2;
    w = L_MIN(right1 - x + 1, right2 - x + 1);
    h = L_MIN(bot1 - y + 1, bot2 - y + 1);
    return boxCreate(x, y, w, h);
}


/*!
 *  boxBoundingRegion()
 *
 *      Input:  box1, box2 (two boxes)
 *      Return: box (of bounding region containing the input boxes),
 *              or null on error
 */
LEPTONICA_EXPORT BOX *
boxBoundingRegion(BOX  *box1,
                  BOX  *box2)
{
l_int32  left, top, right1, right2, right, bot1, bot2, bot;

    PROCNAME("boxBoundingRegion");

    if (!box1)
        return (BOX *)ERROR_PTR("box1 not defined", procName, NULL);
    if (!box2)
        return (BOX *)ERROR_PTR("box2 not defined", procName, NULL);

    left = L_MIN(box1->x, box2->x);
    top = L_MIN(box1->y, box2->y);
    right1 = box1->x + box1->w - 1;
    right2 = box2->x + box2->w - 1;
    right = L_MAX(right1, right2);
    bot1 = box1->y + box1->h - 1;
    bot2 = box2->y + box2->h - 1;
    bot = L_MAX(bot1, bot2);
    return boxCreate(left, top, right - left + 1, bot - top + 1);
}


/*!
 *  boxContainsPt()
 *
 *      Input:  box
 *              x, y (a point)
 *              &contains (<return> 1 if box contains point; 0 otherwise)
 *      Return: 0 if OK, 1 on error.
 */
LEPTONICA_EXPORT l_int32
boxContainsPt(BOX       *box,
              l_float32  x,
              l_float32  y,
              l_int32   *pcontains)
{
l_int32  bx, by, bw, bh;

    PROCNAME("boxContainsPt");

    if (!pcontains)
        return ERROR_INT("&contains not defined", procName, 1);
    *pcontains = 0;
    if (!box)
        return ERROR_INT("&box not defined", procName, 1);
    boxGetGeometry(box, &bx, &by, &bw, &bh);
    if (x >= bx && x < bx + bw && y >= by && y < by + bh)
        *pcontains = 1;
    return 0;
}


/*!
 *  boxGetCenter()
 *
 *      Input:  box
 *              &cx, &cy (<return> location of center of box)
 *      Return  0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxGetCenter(BOX        *box,
             l_float32  *pcx,
             l_float32  *pcy)
{
l_int32  x, y, w, h;

    PROCNAME("boxGetCenter");

    if (!pcx || !pcy)
        return ERROR_INT("&cx, &cy not both defined", procName, 1);
    *pcx = *pcy = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    boxGetGeometry(box, &x, &y, &w, &h);
    *pcx = (l_float32)(x + 0.5 * w);
    *pcy = (l_float32)(y + 0.5 * h);

    return 0;
}


/*!
 *  boxClipToRectangle()
 *
 *      Input:  box
 *              wi, hi (rectangle representing image)
 *      Return: part of box within given rectangle, or NULL on error
 *              or if box is entirely outside the rectangle
 *
 *  Notes:
 *      (1) This can be used to clip a rectangle to an image.
 *          The clipping rectangle is assumed to have a UL corner at (0, 0),
 *          and a LR corner at (wi - 1, hi - 1).
 */
LEPTONICA_EXPORT BOX *
boxClipToRectangle(BOX     *box,
                   l_int32  wi,
                   l_int32  hi)
{
BOX  *boxd;

    PROCNAME("boxClipToRectangle");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);
    if (box->x >= wi || box->y >= hi ||
        box->x + box->w <= 0 || box->y + box->h <= 0)
        return (BOX *)ERROR_PTR("box outside rectangle", procName, NULL);

    boxd = boxCopy(box);
    if (boxd->x < 0) {
        boxd->w += boxd->x;
        boxd->x = 0;
    }
    if (boxd->y < 0) {
        boxd->h += boxd->y;
        boxd->y = 0;
    }
    if (boxd->x + boxd->w > wi)
        boxd->w = wi - boxd->x;
    if (boxd->y + boxd->h > hi)
        boxd->h = hi - boxd->y;
    return boxd;
}


/*!
 *  boxRelocateOneSide()
 *
 *      Input:  boxd (<optional>; this can be null, equal to boxs,
 *                    or different from boxs);
 *              boxs (starting box; to have one side relocated)
 *              loc (new location of the side that is changing)
 *              sideflag (L_FROM_LEFT, etc., indicating the side that moves)
 *      Return: boxd, or null on error or if the computed boxd has
 *              width or height <= 0.
 *
 *  Notes:
 *      (1) Set boxd == NULL to get new box; boxd == boxs for in-place;
 *          or otherwise to resize existing boxd.
 *      (2) For usage, suggest one of these:
 *               boxd = boxRelocateOneSide(NULL, boxs, ...);   // new
 *               boxRelocateOneSide(boxs, boxs, ...);          // in-place
 *               boxRelocateOneSide(boxd, boxs, ...);          // other
 */
LEPTONICA_EXPORT BOX *
boxRelocateOneSide(BOX     *boxd,
                   BOX     *boxs,
                   l_int32  loc,
                   l_int32  sideflag)
{
l_int32  x, y, w, h;

    PROCNAME("boxRelocateOneSide");

    if (!boxs)
        return (BOX *)ERROR_PTR("boxs not defined", procName, NULL);
    if (!boxd)
        boxd = boxCopy(boxs);

    boxGetGeometry(boxs, &x, &y, &w, &h);
    if (sideflag == L_FROM_LEFT)
        boxSetGeometry(boxd, loc, -1, w + x - loc, -1);
    else if (sideflag == L_FROM_RIGHT)
        boxSetGeometry(boxd, -1, -1, loc - x + 1, -1);
    else if (sideflag == L_FROM_TOP)
        boxSetGeometry(boxd, -1, loc, -1, h + y - loc);
    else if (sideflag == L_FROM_BOTTOM)
        boxSetGeometry(boxd, -1, -1, -1, loc - y + 1);
    return boxd;
}


/*!
 *  boxAdjustSides()
 *
 *      Input:  boxd  (<optional>; this can be null, equal to boxs,
 *                     or different from boxs)
 *              boxs  (starting box; to have sides adjusted)
 *              delleft, delright, deltop, delbot (changes in location of
 *                                                 each side)
 *      Return: boxd, or null on error or if the computed boxd has
 *              width or height <= 0.
 *
 *  Notes:
 *      (1) Set boxd == NULL to get new box; boxd == boxs for in-place;
 *          or otherwise to resize existing boxd.
 *      (2) For usage, suggest one of these:
 *               boxd = boxAdjustSides(NULL, boxs, ...);   // new
 *               boxAdjustSides(boxs, boxs, ...);          // in-place
 *               boxAdjustSides(boxd, boxs, ...);          // other
 *      (1) New box dimensions are cropped at left and top to x >= 0 and y >= 0.
 *      (2) For example, to expand in-place by 20 pixels on each side, use
 *             boxAdjustSides(box, box, -20, 20, -20, 20);
 */
LEPTONICA_EXPORT BOX *
boxAdjustSides(BOX     *boxd,
               BOX     *boxs,
               l_int32  delleft,
               l_int32  delright,
               l_int32  deltop,
               l_int32  delbot)
{
l_int32  x, y, w, h, xl, xr, yt, yb, wnew, hnew;

    PROCNAME("boxAdjustSides");

    if (!boxs)
        return (BOX *)ERROR_PTR("boxs not defined", procName, NULL);

    boxGetGeometry(boxs, &x, &y, &w, &h);
    xl = L_MAX(0, x + delleft);
    yt = L_MAX(0, y + deltop);
    xr = x + w + delright;  /* one pixel beyond right edge */
    yb = y + h + delbot;    /* one pixel below bottom edge */
    wnew = xr - xl;
    hnew = yb - yt;
    
    if (wnew < 1 || hnew < 1)
        return (BOX *)ERROR_PTR("boxd has 0 area", procName, NULL);

    if (!boxd)
        return boxCreate(xl, yt, wnew, hnew);
    else {
        boxSetGeometry(boxd, xl, yt, wnew, hnew);
        return boxd;
    }
}


/*----------------------------------------------------------------------*
 *                          Boxa Combination                            *
 *----------------------------------------------------------------------*/
/*!
 *  boxaJoin()
 *
 *      Input:  boxad  (dest boxa; add to this one)
 *              boxas  (source boxa; add from this one)
 *              istart  (starting index in nas)
 *              iend  (ending index in nas; use 0 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated box in boxas to boxad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend <= 0 means 'read to the end'
 */
LEPTONICA_EXPORT l_int32
boxaJoin(BOXA    *boxad,
         BOXA    *boxas,
         l_int32  istart,
         l_int32  iend)
{
l_int32  ns, i;
BOX     *box;

    PROCNAME("boxaJoin");

    if (!boxad)
        return ERROR_INT("boxad not defined", procName, 1);
    if (!boxas)
        return ERROR_INT("boxas not defined", procName, 1);
    if ((ns = boxaGetCount(boxas)) == 0) {
        L_INFO("empty boxas", procName);
        return 0;
    }
    if (istart < 0)
        istart = 0;
    if (istart >= ns)
        return ERROR_INT("istart out of bounds", procName, 1);
    if (iend <= 0)
        iend = ns - 1;
    if (iend >= ns)
        return ERROR_INT("iend out of bounds", procName, 1);
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        box = boxaGetBox(boxas, i, L_CLONE);
        boxaAddBox(boxad, box, L_INSERT);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                        Other Boxa functions                         *
 *---------------------------------------------------------------------*/
/*!
 *  boxaGetExtent()
 *
 *      Input:  boxa
 *              &w  (<optional return> width)
 *              &h  (<optional return> height)
 *              &box (<optional return>, minimum box containing all boxes
 *                    in boxa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The returned w and h are the minimum size image
 *          that would contain all boxes untranslated.
 *      (2) If there are no boxes, returned w and h are 0 and
 *          all parameters in the returned box are 0.
 */
LEPTONICA_EXPORT l_int32
boxaGetExtent(BOXA     *boxa,
              l_int32  *pw,
              l_int32  *ph,
              BOX     **pbox)
{
l_int32  i, n, x, y, w, h, xmax, ymax, xmin, ymin;

    PROCNAME("boxaGetExtent");

    if (!pw && !ph && !pbox)
        return ERROR_INT("no ptrs defined", procName, 1);
    if (pbox) *pbox = NULL;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    xmax = ymax = 0;
    xmin = ymin = 100000000;
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        xmin = L_MIN(xmin, x);
        ymin = L_MIN(ymin, y);
        xmax = L_MAX(xmax, x + w);
        ymax = L_MAX(ymax, y + h);
    }
    if (n == 0)
        xmin = ymin = 0;
    if (pw) *pw = xmax;
    if (ph) *ph = ymax;
    if (pbox)
      *pbox = boxCreate(xmin, ymin, xmax - xmin, ymax - ymin);

    return 0;
}


/*!
 *  boxaSelectBySize()
 *
 *      Input:  boxas
 *              width, height (threshold dimensions)
 *              type (L_SELECT_WIDTH, L_SELECT_HEIGHT,
 *                    L_SELECT_IF_EITHER, L_SELECT_IF_BOTH)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *              &changed (<optional return> 1 if changed; 0 if clone returned)
 *      Return: boxad (filtered set), or null on error
 *
 *  Notes:
 *      (1) The args specify constraints on the size of the
 *          components that are kept.
 *      (2) Uses box clones in the new boxa.
 *      (3) If the selection type is L_SELECT_WIDTH, the input
 *          height is ignored, and v.v.
 *      (4) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
LEPTONICA_EXPORT BOXA *
boxaSelectBySize(BOXA     *boxas,
                 l_int32   width,
                 l_int32   height,
                 l_int32   type,
                 l_int32   relation,
                 l_int32  *pchanged)
{
BOXA  *boxad;
NUMA  *na;

    PROCNAME("boxaSelectBySize");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (type != L_SELECT_WIDTH && type != L_SELECT_HEIGHT &&
        type != L_SELECT_IF_EITHER && type != L_SELECT_IF_BOTH)
        return (BOXA *)ERROR_PTR("invalid type", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT && 
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (BOXA *)ERROR_PTR("invalid relation", procName, NULL);
    if (pchanged) *pchanged = FALSE;
    
        /* Compute the indicator array for saving components */
    na = boxaMakeSizeIndicator(boxas, width, height, type, relation);

        /* Filter to get output */
    boxad = boxaSelectWithIndicator(boxas, na, pchanged);

    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaMakeSizeIndicator()
 *
 *      Input:  boxa
 *              width, height (threshold dimensions)
 *              type (L_SELECT_WIDTH, L_SELECT_HEIGHT,
 *                    L_SELECT_IF_EITHER, L_SELECT_IF_BOTH)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *      Return: na (indicator array), or null on error
 *
 *  Notes:
 *      (1) The args specify constraints on the size of the
 *          components that are kept.
 *      (2) If the selection type is L_SELECT_WIDTH, the input
 *          height is ignored, and v.v.
 *      (3) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
LEPTONICA_EXPORT NUMA *
boxaMakeSizeIndicator(BOXA     *boxa,
                      l_int32   width,
                      l_int32   height,
                      l_int32   type,
                      l_int32   relation)
{
l_int32  i, n, w, h, ival;
NUMA    *na;

    PROCNAME("boxaMakeSizeIndicator");

    if (!boxa)
        return (NUMA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (type != L_SELECT_WIDTH && type != L_SELECT_HEIGHT &&
        type != L_SELECT_IF_EITHER && type != L_SELECT_IF_BOTH)
        return (NUMA *)ERROR_PTR("invalid type", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT && 
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (NUMA *)ERROR_PTR("invalid relation", procName, NULL);

    n = boxaGetCount(boxa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        ival = 0;
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        switch (type)
        {
        case L_SELECT_WIDTH:
            if ((relation == L_SELECT_IF_LT && w < width) ||
                (relation == L_SELECT_IF_GT && w > width) ||
                (relation == L_SELECT_IF_LTE && w <= width) ||
                (relation == L_SELECT_IF_GTE && w >= width))
                ival = 1;
            break;
        case L_SELECT_HEIGHT:
            if ((relation == L_SELECT_IF_LT && h < height) ||
                (relation == L_SELECT_IF_GT && h > height) ||
                (relation == L_SELECT_IF_LTE && h <= height) ||
                (relation == L_SELECT_IF_GTE && h >= height))
                ival = 1;
            break;
        case L_SELECT_IF_EITHER:
            if (((relation == L_SELECT_IF_LT) && (w < width || h < height)) ||
                ((relation == L_SELECT_IF_GT) && (w > width || h > height)) ||
               ((relation == L_SELECT_IF_LTE) && (w <= width || h <= height)) ||
                ((relation == L_SELECT_IF_GTE) && (w >= width || h >= height)))
                    ival = 1;
            break;
        case L_SELECT_IF_BOTH:
            if (((relation == L_SELECT_IF_LT) && (w < width && h < height)) ||
                ((relation == L_SELECT_IF_GT) && (w > width && h > height)) ||
               ((relation == L_SELECT_IF_LTE) && (w <= width && h <= height)) ||
                ((relation == L_SELECT_IF_GTE) && (w >= width && h >= height)))
                    ival = 1;
            break;
        default:
            L_WARNING("can't get here!", procName);
        }
        numaAddNumber(na, ival);
    }

    return na;
}


/*!
 *  boxaSelectWithIndicator()
 *
 *      Input:  boxas
 *              na (indicator numa)
 *              &changed (<optional return> 1 if changed; 0 if clone returned)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) Returns a boxa clone if no components are removed.
 *      (2) Uses box clones in the new boxa.
 *      (3) The indicator numa has values 0 (ignore) and 1 (accept).
 */
LEPTONICA_EXPORT BOXA *
boxaSelectWithIndicator(BOXA     *boxas,
                        NUMA     *na,
                        l_int32  *pchanged)
{
l_int32  i, n, ival, nsave;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaSelectWithIndicator");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!na)
        return (BOXA *)ERROR_PTR("na not defined", procName, NULL);

    nsave = 0;
    n = numaGetCount(na);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &ival);
        if (ival == 1) nsave++;
    }

    if (nsave == n) {
        if (pchanged) *pchanged = FALSE;
        return boxaCopy(boxas, L_CLONE);
    }
    if (pchanged) *pchanged = TRUE;
    boxad = boxaCreate(nsave);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &ival);
        if (ival == 0) continue;
        box = boxaGetBox(boxas, i, L_CLONE);
        boxaAddBox(boxad, box, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxaSwapBoxes()
 *
 *      Input:  boxa
 *              i, j (two indices of boxes, that are to be swapped)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaSwapBoxes(BOXA    *boxa,
              l_int32  i,
              l_int32  j)
{
l_int32  n;
BOX     *box;

    PROCNAME("boxaSwapBoxes");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (i < 0 || i >= n)
        return ERROR_INT("i invalid", procName, 1);
    if (j < 0 || j >= n)
        return ERROR_INT("j invalid", procName, 1);
    if (i == j)
        return ERROR_INT("i == j", procName, 1);

    box = boxa->box[i];
    boxa->box[i] = boxa->box[j];
    boxa->box[j] = box;
    return 0;
}
