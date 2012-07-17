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
 *  colormap.c
 *
 *      Colormap creation, copy, destruction, addition
 *           PIXCMAP    *pixcmapCreate()
 *           PIXCMAP    *pixcmapCreateRandom()
 *           PIXCMAP    *pixcmapCreateLinear()
 *           PIXCMAP    *pixcmapCopy()
 *           void        pixcmapDestroy()
 *           l_int32     pixcmapAddColor()
 *           l_int32     pixcmapAddNewColor()
 *           l_int32     pixcmapAddNearestColor()
 *           l_int32     pixcmapUsableColor()
 *           l_int32     pixcmapAddBlackOrWhite()
 *           l_int32     pixcmapSetBlackAndWhite()
 *           l_int32     pixcmapGetCount()
 *           l_int32     pixcmapGetDepth()
 *           l_int32     pixcmapGetMinDepth()
 *           l_int32     pixcmapGetFreeCount()
 *           l_int32     pixcmapClear()
 *
 *      Colormap random access and test
 *           l_int32     pixcmapGetColor()
 *           l_int32     pixcmapGetColor32()
 *           l_int32     pixcmapResetColor()
 *           l_int32     pixcmapGetIndex()
 *           l_int32     pixcmapHasColor()
 *           l_int32     pixcmapCountGrayColors()
 *           l_int32     pixcmapGetRankIntensity()
 *           l_int32     pixcmapGetNearestIndex()
 *           l_int32     pixcmapGetNearestGrayIndex()
 *           l_int32     pixcmapGetComponentRange()
 *           l_int32     pixcmapGetExtremeValue()
 *
 *      Colormap conversion
 *           PIXCMAP    *pixcmapGrayToColor()
 *           PIXCMAP    *pixcmapColorToGray()
 *
 *      Colormap I/O
 *           l_int32     pixcmapReadStream()
 *           l_int32     pixcmapWriteStream()
 *
 *      Extract colormap arrays and serialization
 *           l_int32     pixcmapToArrays()
 *           l_int32     pixcmapToRGBTable()
 *           l_int32     pixcmapSerializeToMemory()
 *           PIXCMAP    *pixcmapDeserializeFromMemory()
 *           char       *pixcmapConvertToHex()
 *
 *      Colormap transforms
 *           l_int32     pixcmapGammaTRC()
 *           l_int32     pixcmapContrastTRC()
 *           l_int32     pixcmapShiftIntensity()
 */

#include <string.h>
#include "allheaders.h"


/*-------------------------------------------------------------*
 *                Colormap creation and addition               *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapCreate()
 *
 *      Input:  depth (bpp, of pix)
 *      Return: cmap, or null on error
 */
LEPTONICA_EXPORT PIXCMAP *
pixcmapCreate(l_int32  depth)
{
RGBA_QUAD  *cta;
PIXCMAP    *cmap;

    PROCNAME("pixcmapCreate");

    if (depth != 1 && depth != 2 && depth !=4 && depth != 8)
        return (PIXCMAP *)ERROR_PTR("depth not in {1,2,4,8}", procName, NULL);

    if ((cmap = (PIXCMAP *)CALLOC(1, sizeof(PIXCMAP))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmap not made", procName, NULL);
    cmap->depth = depth;
    cmap->nalloc = 1 << depth;
    if ((cta = (RGBA_QUAD *)CALLOC(cmap->nalloc, sizeof(RGBA_QUAD))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cta not made", procName, NULL);
    cmap->array = cta;
    cmap->n = 0;

    return cmap;
}


/*!
 *  pixcmapCreateLinear()
 *
 *      Input:  d (depth of pix for this colormap; 1, 2, 4 or 8)
 *              nlevels (valid in range [2, 2^d])
 *      Return: cmap, or null on error
 *
 *  Notes:
 *      (1) Colormap has equally spaced gray color values
 *          from black (0, 0, 0) to white (255, 255, 255).
 */
LEPTONICA_EXPORT PIXCMAP *
pixcmapCreateLinear(l_int32  d,
                    l_int32  nlevels)
{
l_int32   maxlevels, i, val;
PIXCMAP  *cmap;

    PROCNAME("pixcmapCreateLinear");

    if (d != 1 && d != 2 && d !=4 && d != 8)
        return (PIXCMAP *)ERROR_PTR("d not in {1, 2, 4, 8}", procName, NULL);
    maxlevels = 1 << d;
    if (nlevels < 2 || nlevels > maxlevels)
        return (PIXCMAP *)ERROR_PTR("invalid nlevels", procName, NULL);

    cmap = pixcmapCreate(d);
    for (i = 0; i < nlevels; i++) {
        val = (255 * i) / (nlevels - 1);
        pixcmapAddColor(cmap, val, val, val);
    }
    return cmap;
}


/*!
 *  pixcmapCopy()
 *
 *      Input:  cmaps 
 *      Return: cmapd, or null on error
 */
LEPTONICA_EXPORT PIXCMAP *
pixcmapCopy(PIXCMAP  *cmaps)
{
l_int32   nbytes;
PIXCMAP  *cmapd;

    PROCNAME("pixcmapCopy");

    if (!cmaps)
        return (PIXCMAP *)ERROR_PTR("cmaps not defined", procName, NULL);

    if ((cmapd = (PIXCMAP *)CALLOC(1, sizeof(PIXCMAP))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmapd not made", procName, NULL);
    nbytes = cmaps->nalloc * sizeof(RGBA_QUAD);
    if ((cmapd->array = (void *)CALLOC(1, nbytes)) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmap array not made", procName, NULL);
    memcpy(cmapd->array, cmaps->array, nbytes);
    cmapd->n = cmaps->n;
    cmapd->nalloc = cmaps->nalloc;
    cmapd->depth = cmaps->depth;

    return cmapd;
}


/*!
 *  pixcmapDestroy()
 *
 *      Input:  &cmap (<set to null>)
 *      Return: void
 */
LEPTONICA_EXPORT void
pixcmapDestroy(PIXCMAP  **pcmap)
{
PIXCMAP  *cmap;

    PROCNAME("pixcmapDestroy");

    if (pcmap == NULL) {
        L_WARNING("ptr address is null!", procName);
        return;
    }

    if ((cmap = *pcmap) == NULL)
        return;

    FREE(cmap->array);
    FREE(cmap);
    *pcmap = NULL;

    return;
}


/*!
 *  pixcmapAddColor()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap entry to be added; each number
 *                                is in range [0, ... 255])
 *      Return: 0 if OK, 1 on error
 *
 *  Note: always adds the color if there is room.
 */
LEPTONICA_EXPORT l_int32
pixcmapAddColor(PIXCMAP  *cmap,
                l_int32   rval,
                l_int32   gval,
                l_int32   bval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapAddColor");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (cmap->n >= cmap->nalloc)
        return ERROR_INT("no free color entries", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    cta[cmap->n].red = rval;
    cta[cmap->n].green = gval;
    cta[cmap->n].blue = bval;
    cmap->n++;

    return 0;
}


/*!
 *  pixcmapGetCount()
 *
 *      Input:  cmap
 *      Return: count, or 0 on error
 */
LEPTONICA_EXPORT l_int32
pixcmapGetCount(PIXCMAP  *cmap)
{
    PROCNAME("pixcmapGetCount");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 0);

    return cmap->n;
}


/*!
 *  pixcmapGetMinDepth()
 *
 *      Input:  cmap
 *              &mindepth (<return> minimum depth to support the colormap)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) On error, &mindepth is returned as 0.
 */
LEPTONICA_EXPORT l_int32
pixcmapGetMinDepth(PIXCMAP  *cmap,
                   l_int32  *pmindepth)
{
l_int32  ncolors;

    PROCNAME("pixcmapGetMinDepth");

    if (!pmindepth)
        return ERROR_INT("&mindepth not defined", procName, 1);
    *pmindepth = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (ncolors <= 4)
        *pmindepth = 2;
    else if (ncolors <= 16)
        *pmindepth = 4;
    else  /* ncolors > 16 */
        *pmindepth = 8;

    return 0;
}


/*-------------------------------------------------------------*
 *                      Colormap random access                 *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapGetColor()
 *
 *      Input:  cmap
 *              index
 *              &rval, &gval, &bval (<return> each color value in l_int32)
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 */
LEPTONICA_EXPORT l_int32
pixcmapGetColor(PIXCMAP  *cmap,
                l_int32   index,
                l_int32  *prval,
                l_int32  *pgval,
                l_int32  *pbval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetColor");

    if (!prval || !pgval || !pbval)
        return ERROR_INT("&rval, &gval, &bval not all defined", procName, 1);
    *prval = *pgval = *pbval = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (index < 0 || index >= cmap->n)
        return ERROR_INT("index out of bounds", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    *prval = cta[index].red;
    *pgval = cta[index].green;
    *pbval = cta[index].blue;

    return 0;
}


/*!
 *  pixcmapResetColor()
 *
 *      Input:  cmap
 *              index
 *              rval, gval, bval (colormap entry to be reset; each number
 *                                is in range [0, ... 255])
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 *
 *  Notes:
 *      (1) This resets sets the color of an entry that has already
 *          been set and included in the count of colors.
 */
LEPTONICA_EXPORT l_int32
pixcmapResetColor(PIXCMAP  *cmap,
                  l_int32   index,
                  l_int32   rval,
                  l_int32   gval,
                  l_int32   bval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapResetColor");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (index < 0 || index >= cmap->n)
        return ERROR_INT("index out of bounds", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    cta[index].red = rval;
    cta[index].green = gval;
    cta[index].blue = bval;

    return 0;
}


/*!
 *  pixcmapGetIndex()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap colors to search for; each number
 *                                is in range [0, ... 255])
 *              &index (<return>)
 *      Return: 0 if found, 1 if not found (caller must check)
 */
LEPTONICA_EXPORT l_int32
pixcmapGetIndex(PIXCMAP  *cmap,
                l_int32   rval,
                l_int32   gval,
                l_int32   bval,
                l_int32  *pindex)
{
l_int32     n, i;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    n = pixcmapGetCount(cmap);

    cta = (RGBA_QUAD *)cmap->array;
    for (i = 0; i < n; i++) {
        if (rval == cta[i].red && gval == cta[i].green && bval == cta[i].blue) {
            *pindex = i;
            return 0;
        }
    }

    return 1;
}


/*!
 *  pixcmapHasColor()
 *
 *      Input:  cmap
 *              &color (<return> TRUE if cmap has color; FALSE otherwise)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
pixcmapHasColor(PIXCMAP  *cmap,
                l_int32  *pcolor)
{
l_int32   n, i;
l_int32  *rmap, *gmap, *bmap;

    PROCNAME("pixcmapHasColor");

    if (!pcolor)
        return ERROR_INT("&color not defined", procName, 1);
    *pcolor = FALSE;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if (pixcmapToArrays(cmap, &rmap, &gmap, &bmap))
        return ERROR_INT("colormap arrays not made", procName, 1);
    n = pixcmapGetCount(cmap);
    for (i = 0; i < n; i++) {
        if ((rmap[i] != gmap[i]) || (rmap[i] != bmap[i])) {
            *pcolor = TRUE;
            break;
        }
    }

    FREE(rmap);
    FREE(gmap);
    FREE(bmap);
    return 0;
}


/*!
 *  pixcmapGetNearestIndex()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap colors to search for; each number
 *                                is in range [0, ... 255])
 *              &index (<return> the index of the nearest color)
 *      Return: 0 if OK, 1 on error (caller must check)
 *
 *  Notes:
 *      (1) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 *      (2) Nearest color is that which is the least sum-of-squares distance
 *          from the target color.
 */
LEPTONICA_EXPORT l_int32
pixcmapGetNearestIndex(PIXCMAP  *cmap,
                       l_int32   rval,
                       l_int32   gval,
                       l_int32   bval,
                       l_int32  *pindex)
{
l_int32     i, n, delta, dist, mindist;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetNearestIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = UNDEF;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if ((cta = (RGBA_QUAD *)cmap->array) == NULL)
        return ERROR_INT("cta not defined(!)", procName, 1);
    n = pixcmapGetCount(cmap);

    mindist = 3 * 255 * 255 + 1;
    for (i = 0; i < n; i++) {
        delta = cta[i].red - rval;
        dist = delta * delta;
        delta = cta[i].green - gval;
        dist += delta * delta;
        delta = cta[i].blue - bval;
        dist += delta * delta;
        if (dist < mindist) {
            *pindex = i; 
            if (dist == 0)
                break;
            mindist = dist;
        }
    }

    return 0;
}


/*!
 *  pixcmapGetNearestGrayIndex()
 *
 *      Input:  cmap
 *              val (gray value to search for; in range [0, ... 255])
 *              &index (<return> the index of the nearest color)
 *      Return: 0 if OK, 1 on error (caller must check)
 *
 *  Notes:
 *      (1) This should be used on gray colormaps.  It uses only the
 *          green value of the colormap.
 *      (2) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 */
LEPTONICA_EXPORT l_int32
pixcmapGetNearestGrayIndex(PIXCMAP  *cmap,
                           l_int32   val,
                           l_int32  *pindex)
{
l_int32     i, n, dist, mindist;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetNearestGrayIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (val < 0 || val > 255)
        return ERROR_INT("val not in [0 ... 255]", procName, 1);

    if ((cta = (RGBA_QUAD *)cmap->array) == NULL)
        return ERROR_INT("cta not defined(!)", procName, 1);
    n = pixcmapGetCount(cmap);

    mindist = 256;
    for (i = 0; i < n; i++) {
        dist = cta[i].green - val;
        dist = L_ABS(dist);
        if (dist < mindist) {
            *pindex = i;
            if (dist == 0)
                break;
            mindist = dist;
        }
    }

    return 0;
}


/*!
 *  pixcmapGetExtremeValue()
 *
 *      Input:  cmap
 *              type (L_SELECT_MIN or L_SELECT_MAX)
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Returns for selected components the extreme value
 *          (either min or max) of the color component that is
 *          found in the colormap.
 */
LEPTONICA_EXPORT l_int32
pixcmapGetExtremeValue(PIXCMAP  *cmap,
                       l_int32   type,
                       l_int32  *prval,
                       l_int32  *pgval,
                       l_int32  *pbval)
{
l_int32  i, n, rval, gval, bval, extrval, extgval, extbval;

    PROCNAME("pixcmapGetExtremeValue");

    if (!prval && !pgval && !pbval)
        return ERROR_INT("no result requested for return", procName, 1);
    if (prval) *prval = 0;
    if (pgval) *pgval = 0;
    if (pbval) *pbval = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (type != L_SELECT_MIN && type != L_SELECT_MAX)
        return ERROR_INT("invalid type", procName, 1);

    if (type == L_SELECT_MIN) {
        extrval = 100000;
        extgval = 100000;
        extbval = 100000;
    }
    else {
        extrval = 0;
        extgval = 0;
        extbval = 0;
    }

    n = pixcmapGetCount(cmap);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if ((type == L_SELECT_MIN && rval < extrval) ||
            (type == L_SELECT_MAX && rval > extrval))
            extrval = rval;
        if ((type == L_SELECT_MIN && gval < extgval) ||
            (type == L_SELECT_MAX && gval > extgval))
            extgval = gval;
        if ((type == L_SELECT_MIN && bval < extbval) ||
            (type == L_SELECT_MAX && bval > extbval))
            extbval = bval;
    }
    if (prval) *prval = extrval;
    if (pgval) *pgval = extgval;
    if (pbval) *pbval = extbval;
    return 0;
}


/*-------------------------------------------------------------*
 *                       Colormap conversion                   *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapGrayToColor()
 *
 *      Input:  color
 *      Return: cmap, or null on error
 *
 *  Notes:
 *      (1) This creates a colormap that maps from gray to
 *          a specific color.  In the mapping, each component
 *          is faded to white, depending on the gray value.
 *      (2) In use, this is simply attached to a grayscale pix
 *          to give it the input color.
 */
LEPTONICA_EXPORT PIXCMAP *
pixcmapGrayToColor(l_uint32  color)
{
l_int32   i, rval, gval, bval;
PIXCMAP  *cmap;

    extractRGBValues(color, &rval, &gval, &bval);
    cmap = pixcmapCreate(8);
    for (i = 0; i < 256; i++) {
        pixcmapAddColor(cmap, rval + (i * (255 - rval)) / 255,
                        gval + (i * (255 - gval)) / 255,
                        bval + (i * (255 - bval)) / 255);
    }

    return cmap;
}


/*!
 *  pixcmapColorToGray()
 *
 *      Input:  cmap
 *              rwt, gwt, bwt  (non-negative; these should add to 1.0)
 *      Return: cmap (gray), or null on error
 *
 *  Notes:
 *      (1) This creates a gray colormap from an arbitrary colormap.
 *      (2) In use, attach the output gray colormap to the pix
 *          (or a copy of it) that provided the input colormap.
 */
LEPTONICA_EXPORT PIXCMAP *
pixcmapColorToGray(PIXCMAP   *cmaps,
                   l_float32  rwt,
                   l_float32  gwt,
                   l_float32  bwt)
{
l_int32    i, n, rval, gval, bval, val;
l_float32  sum;
PIXCMAP   *cmapd;

    PROCNAME("pixcmapColorToGray");

    if (!cmaps)
        return (PIXCMAP *)ERROR_PTR("cmaps not defined", procName, NULL);
    if (rwt < 0.0 || gwt < 0.0 || bwt < 0.0)
        return (PIXCMAP *)ERROR_PTR("weights not all >= 0.0", procName, NULL);

        /* Make sure the sum of weights is 1.0; otherwise, you can get
         * overflow in the gray value. */
    sum = rwt + gwt + bwt;
    if (sum == 0.0) {
        L_WARNING("all weights zero; setting equal to 1/3", procName);
        rwt = gwt = bwt = 0.33333;
        sum = 1.0;
    }
    if (L_ABS(sum - 1.0) > 0.0001) {  /* maintain ratios with sum == 1.0 */
        L_WARNING("weights don't sum to 1; maintaining ratios", procName);
        rwt = rwt / sum;
        gwt = gwt / sum;
        bwt = bwt / sum;
    }

    cmapd = pixcmapCopy(cmaps);
    n = pixcmapGetCount(cmapd);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmapd, i, &rval, &gval, &bval);
        val = (l_int32)(rwt * rval + gwt * gval + bwt * bval + 0.5);
        pixcmapResetColor(cmapd, i, val, val, val);
    }
        
    return cmapd;
}


/*----------------------------------------------------------------------*
 *               Extract colormap arrays and serialization              *
 *----------------------------------------------------------------------*/
/*!
 *  pixcmapToArrays()
 *
 *      Input:  colormap
 *              &rmap, &gmap, &bmap  (<return> colormap arrays)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixcmapToArrays(PIXCMAP   *cmap,
                l_int32  **prmap,
                l_int32  **pgmap,
                l_int32  **pbmap)
{
l_int32    *rmap, *gmap, *bmap;
l_int32     i, ncolors;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapToArrays");

    if (!prmap || !pgmap || !pbmap)
        return ERROR_INT("&rmap, &gmap, &bmap not all defined", procName, 1);
    *prmap = *pgmap = *pbmap = NULL;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (((rmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL) ||
        ((gmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL) ||
        ((bmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL))
            return ERROR_INT("calloc fail for *map", procName, 1);
    *prmap = rmap;
    *pgmap = gmap;
    *pbmap = bmap;

    cta = (RGBA_QUAD *)cmap->array;
    for (i = 0; i < ncolors; i++) {
        rmap[i] = cta[i].red;
        gmap[i] = cta[i].green;
        bmap[i] = cta[i].blue;
    }

    return 0;
}


/*!
 *  pixcmapToRGBTable()
 *
 *      Input:  colormap
 *              &tab (<return> table of rgba values for the colormap)
 *              &ncolors (<optional return> size of table)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixcmapToRGBTable(PIXCMAP    *cmap,
                  l_uint32  **ptab,
                  l_int32    *pncolors)
{
l_int32    i, ncolors, rval, gval, bval;
l_uint32  *tab;

    PROCNAME("pixcmapToRGBTable");

    if (!ptab)
        return ERROR_INT("&tab not defined", procName, 1);
    *ptab = NULL;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (pncolors)
        *pncolors = ncolors;
    if ((tab = (l_uint32 *)CALLOC(ncolors, sizeof(l_uint32))) == NULL)
        return ERROR_INT("tab not made", procName, 1);
    *ptab = tab;
   
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, &tab[i]);
    }

/*    for (i = 0; i < ncolors; i++)
        fprintf(stderr, "Color[%d] = %x\n", i, tab[i]); */

    return 0;
}
