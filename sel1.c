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
 *  sel1.c
 *
 *      Basic ops on Sels and Selas
 *
 *         Create/destroy/copy:
 *            SELA      *selaCreate()
 *            void       selaDestroy()
 *            SEL       *selCreate()
 *            void       selDestroy()
 *            SEL       *selCopy()
 *            SEL       *selCreateBrick()
 *            SEL       *selCreateComb()
 *
 *         Helper proc:
 *            l_int32  **create2dIntArray()
 *
 *         Extension of sela:
 *            SELA      *selaAddSel()
 *            l_int32    selaExtendArray()
 *
 *         Accessors:
 *            l_int32    selaGetCount()
 *            SEL       *selaGetSel()
 *            char      *selGetName()
 *            l_int32    selSetName()
 *            l_int32    selaFindSelByName()
 *            l_int32    selGetElement()
 *            l_int32    selSetElement()
 *            l_int32    selGetParameters()
 *            l_int32    selSetOrigin()
 *            l_int32    selGetTypeAtOrigin()
 *            char      *selaGetBrickName()
 *            char      *selaGetCombName()
 *     static char      *selaComputeCompositeParameters()
 *            l_int32    getCompositeParameters()
 *            SARRAY    *selaGetSelnames()
 *
 *         Max translations for erosion and hmt
 *            l_int32    selFindMaxTranslations()
 *
 *         Rotation by multiples of 90 degrees
 *            SEL       *selRotateOrth()
 *
 *         Sela and Sel serialized I/O
 *            SELA      *selaRead()
 *            SELA      *selaReadStream()
 *            SEL       *selRead()
 *            SEL       *selReadStream()
 *            l_int32    selaWrite()
 *            l_int32    selaWriteStream()
 *            l_int32    selWrite()
 *            l_int32    selWriteStream()
 *       
 *         Building custom hit-miss sels from compiled strings
 *            SEL       *selCreateFromString()
 *            char      *selPrintToString()     [for debugging]
 *
 *         Building custom hit-miss sels from a simple file format
 *            SELA      *selaCreateFromFile()
 *            static SEL *selCreateFromSArray()
 *
 *         Making hit-only sels from Pta and Pix
 *            SEL       *selCreateFromPta()
 *            SEL       *selCreateFromPix()
 *
 *         Making hit-miss sels from Pix and image files
 *            SEL       *selReadFromColorImage()
 *            SEL       *selCreateFromColorPix()
 *
 *         Printable display of sel
 *            PIX       *selDisplayInPix()
 *            PIX       *selaDisplayInPix()
 *
 *     Usage notes:
 *        In this file we have seven functions that make sels:
 *          (1)  selCreate(), with input (h, w, [name])
 *               The generic function.  Roll your own, using selSetElement().
 *          (2)  selCreateBrick(), with input (h, w, cy, cx, val)
 *               The most popular function.  Makes a rectangular sel of
 *               all hits, misses or don't-cares.  We have many morphology
 *               operations that create a sel of all hits, use it, and
 *               destroy it.
 *          (3)  selCreateFromString() with input (text, h, w, [name])
 *               Adam Langley's clever function, allows you to make a hit-miss
 *               sel from a string in code that is geometrically laid out
 *               just like the actual sel.
 *          (4)  selaCreateFromFile() with input (filename)
 *               This parses a simple file format to create an array of
 *               hit-miss sels.  The sel data uses the same encoding
 *               as in (3), with geometrical layout enforced.
 *          (5)  selCreateFromPta() with input (pta, cy, cx, [name])
 *               Another way to make a sel with only hits.
 *          (6)  selCreateFromPix() with input (pix, cy, cx, [name])
 *               Yet another way to make a sel from hits.
 *          (7)  selCreateFromColorPix() with input (pix, name).
 *               Another way to make a general hit-miss sel, starting with
 *               an image editor.
 *        In addition, there are three functions in selgen.c that
 *        automatically generate a hit-miss sel from a pix and
 *        a number of parameters.  This is useful for problems like
 *        "find all patterns that look like this one."
 *
 *        Consistency, being the hobgoblin of small minds,
 *        is adhered to here in the dimensioning and accessing of sels.
 *        Everything is done in standard matrix (row, column) order.
 *        When we set specific elements in a sel, we likewise use
 *        (row, col) ordering:
 *             selSetElement(), with input (row, col, type)
 */

#include <string.h>
#include "allheaders.h"

    /* MS VC++ can't handle array initialization with static consts ! */
#define L_BUF_SIZE5      256

static const l_int32  INITIAL_PTR_ARRAYSIZE6 = 50;  /* n'import quoi */
static const l_int32  MANY_SELS = 1000;

static SEL *selCreateFromSArray(SARRAY *sa, l_int32 first, l_int32 last);

struct CompParameterMap
{
    l_int32  size;
    l_int32  size1;
    l_int32  size2;
    char     selnameh1[20];
    char     selnameh2[20];
    char     selnamev1[20];
    char     selnamev2[20];
};

static const struct CompParameterMap  comp_parameter_map[] =
    { { 2, 2, 1, "sel_2h", "", "sel_2v", "" },
      { 3, 3, 1, "sel_3h", "", "sel_3v", "" },
      { 4, 2, 2, "sel_2h", "sel_comb_4h", "sel_2v", "sel_comb_4v" },
      { 5, 5, 1, "sel_5h", "", "sel_5v", "" },
      { 6, 3, 2, "sel_3h", "sel_comb_6h", "sel_3v", "sel_comb_6v" },
      { 7, 7, 1, "sel_7h", "", "sel_7v", "" },
      { 8, 4, 2, "sel_4h", "sel_comb_8h", "sel_4v", "sel_comb_8v" },
      { 9, 3, 3, "sel_3h", "sel_comb_9h", "sel_3v", "sel_comb_9v" },
      { 10, 5, 2, "sel_5h", "sel_comb_10h", "sel_5v", "sel_comb_10v" },
      { 11, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 12, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 13, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 14, 7, 2, "sel_7h", "sel_comb_14h", "sel_7v", "sel_comb_14v" },
      { 15, 5, 3, "sel_5h", "sel_comb_15h", "sel_5v", "sel_comb_15v" },
      { 16, 4, 4, "sel_4h", "sel_comb_16h", "sel_4v", "sel_comb_16v" },
      { 17, 4, 4, "sel_4h", "sel_comb_16h", "sel_4v", "sel_comb_16v" },
      { 18, 6, 3, "sel_6h", "sel_comb_18h", "sel_6v", "sel_comb_18v" },
      { 19, 5, 4, "sel_5h", "sel_comb_20h", "sel_5v", "sel_comb_20v" },
      { 20, 5, 4, "sel_5h", "sel_comb_20h", "sel_5v", "sel_comb_20v" },
      { 21, 7, 3, "sel_7h", "sel_comb_21h", "sel_7v", "sel_comb_21v" },
      { 22, 11, 2, "sel_11h", "sel_comb_22h", "sel_11v", "sel_comb_22v" },
      { 23, 6, 4, "sel_6h", "sel_comb_24h", "sel_6v", "sel_comb_24v" },
      { 24, 6, 4, "sel_6h", "sel_comb_24h", "sel_6v", "sel_comb_24v" },
      { 25, 5, 5, "sel_5h", "sel_comb_25h", "sel_5v", "sel_comb_25v" },
      { 26, 5, 5, "sel_5h", "sel_comb_25h", "sel_5v", "sel_comb_25v" },
      { 27, 9, 3, "sel_9h", "sel_comb_27h", "sel_9v", "sel_comb_27v" },
      { 28, 7, 4, "sel_7h", "sel_comb_28h", "sel_7v", "sel_comb_28v" },
      { 29, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 30, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 31, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 32, 8, 4, "sel_8h", "sel_comb_32h", "sel_8v", "sel_comb_32v" },
      { 33, 11, 3, "sel_11h", "sel_comb_33h", "sel_11v", "sel_comb_33v" },
      { 34, 7, 5, "sel_7h", "sel_comb_35h", "sel_7v", "sel_comb_35v" },
      { 35, 7, 5, "sel_7h", "sel_comb_35h", "sel_7v", "sel_comb_35v" },
      { 36, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 37, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 38, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 39, 13, 3, "sel_13h", "sel_comb_39h", "sel_13v", "sel_comb_39v" },
      { 40, 8, 5, "sel_8h", "sel_comb_40h", "sel_8v", "sel_comb_40v" },
      { 41, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 42, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 43, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 44, 11, 4, "sel_11h", "sel_comb_44h", "sel_11v", "sel_comb_44v" },
      { 45, 9, 5, "sel_9h", "sel_comb_45h", "sel_9v", "sel_comb_45v" },
      { 46, 9, 5, "sel_9h", "sel_comb_45h", "sel_9v", "sel_comb_45v" },
      { 47, 8, 6, "sel_8h", "sel_comb_48h", "sel_8v", "sel_comb_48v" },
      { 48, 8, 6, "sel_8h", "sel_comb_48h", "sel_8v", "sel_comb_48v" },
      { 49, 7, 7, "sel_7h", "sel_comb_49h", "sel_7v", "sel_comb_49v" },
      { 50, 10, 5, "sel_10h", "sel_comb_50h", "sel_10v", "sel_comb_50v" },
      { 51, 10, 5, "sel_10h", "sel_comb_50h", "sel_10v", "sel_comb_50v" },
      { 52, 13, 4, "sel_13h", "sel_comb_52h", "sel_13v", "sel_comb_52v" },
      { 53, 9, 6, "sel_9h", "sel_comb_54h", "sel_9v", "sel_comb_54v" },
      { 54, 9, 6, "sel_9h", "sel_comb_54h", "sel_9v", "sel_comb_54v" },
      { 55, 11, 5, "sel_11h", "sel_comb_55h", "sel_11v", "sel_comb_55v" },
      { 56, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 57, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 58, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 59, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 60, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 61, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 62, 9, 7, "sel_9h", "sel_comb_63h", "sel_9v", "sel_comb_63v" },
      { 63, 9, 7, "sel_9h", "sel_comb_63h", "sel_9v", "sel_comb_63v" } };



/*------------------------------------------------------------------------*
 *                      Create / Destroy / Copy                           *
 *------------------------------------------------------------------------*/

/*!
 *  selCreate()
 *
 *      Input:  height, width
 *              name (<optional> sel name; can be null)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) selCreate() initializes all values to 0.
 *      (2) After this call, (cy,cx) and nonzero data values must be
 *          assigned.  If a text name is not assigned here, it will
 *          be needed later when the sel is put into a sela.
 */
LEPTONICA_EXPORT SEL *
selCreate(l_int32      height,
          l_int32      width,
          const char  *name)
{
SEL  *sel;

    PROCNAME("selCreate");

    if ((sel = (SEL *)CALLOC(1, sizeof(SEL))) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    if (name)
        sel->name = stringNew(name);
    sel->sy = height;
    sel->sx = width;
    if ((sel->data = create2dIntArray(height, width)) == NULL)
        return (SEL *)ERROR_PTR("data not allocated", procName, NULL);

    return sel;
}


/*!
 *  selDestroy()
 *
 *      Input:  &sel (<to be nulled>)
 *      Return: void
 */
LEPTONICA_EXPORT void
selDestroy(SEL  **psel)
{
l_int32  i;
SEL     *sel;

    PROCNAME("selDestroy");

    if (psel == NULL)  {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }
    if ((sel = *psel) == NULL)
        return;

    for (i = 0; i < sel->sy; i++)
        FREE(sel->data[i]);
    FREE(sel->data);
    if (sel->name)
        FREE(sel->name);
    FREE(sel);

    *psel = NULL;
    return;
}


/*!
 *  selCopy() 
 *
 *      Input:  sel
 *      Return: a copy of the sel, or null on error
 */
LEPTONICA_EXPORT SEL *
selCopy(SEL  *sel)
{
l_int32  sx, sy, cx, cy, i, j;
SEL     *csel;

    PROCNAME("selCopy");

    if (!sel)
        return (SEL *)ERROR_PTR("sel not defined", procName, NULL);

    if ((csel = (SEL *)CALLOC(1, sizeof(SEL))) == NULL)
        return (SEL *)ERROR_PTR("csel not made", procName, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    csel->sy = sy;
    csel->sx = sx;
    csel->cy = cy;
    csel->cx = cx;

    if ((csel->data = create2dIntArray(sy, sx)) == NULL)
        return (SEL *)ERROR_PTR("sel data not made", procName, NULL);

    for (i = 0; i < sy; i++)
        for (j = 0; j < sx; j++)
            csel->data[i][j] = sel->data[i][j];

    if (sel->name)
        csel->name = stringNew(sel->name);

    return csel;
}


/*!
 *  selCreateBrick()
 *
 *      Input:  height, width
 *              cy, cx  (origin, relative to UL corner at 0,0)
 *              type  (SEL_HIT, SEL_MISS, or SEL_DONT_CARE)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) This is a rectangular sel of all hits, misses or don't cares.
 */
LEPTONICA_EXPORT SEL *
selCreateBrick(l_int32  h,
               l_int32  w,
               l_int32  cy,
               l_int32  cx,
               l_int32  type)
{
l_int32  i, j;
SEL     *sel;

    PROCNAME("selCreateBrick");

    if (h <= 0 || w <= 0)
        return (SEL *)ERROR_PTR("h and w must both be > 0", procName, NULL);
    if (type != SEL_HIT && type != SEL_MISS && type != SEL_DONT_CARE)
        return (SEL *)ERROR_PTR("invalid sel element type", procName, NULL);

    if ((sel = selCreate(h, w, NULL)) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    selSetOrigin(sel, cy, cx);
    for (i = 0; i < h; i++)
        for (j = 0; j < w; j++)
            sel->data[i][j] = type;

    return sel;
}


/*!
 *  selCreateComb()
 *
 *      Input:  factor1 (contiguous space between comb tines)
 *              factor2 (number of comb tines)
 *              direction (L_HORIZ, L_VERT)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) This generates a comb Sel of hits with the origin as
 *          near the center as possible.
 */
LEPTONICA_EXPORT SEL *
selCreateComb(l_int32  factor1,
              l_int32  factor2,
              l_int32  direction)
{
l_int32  i, size, z;
SEL     *sel;

    PROCNAME("selCreateComb");

    if (factor1 < 1 || factor2 < 1)
        return (SEL *)ERROR_PTR("factors must be >= 1", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (SEL *)ERROR_PTR("invalid direction", procName, NULL);

    size = factor1 * factor2;
    if (direction == L_HORIZ) {
        sel = selCreate(1, size, NULL);
        selSetOrigin(sel, 0, size / 2);
    }
    else {
        sel = selCreate(size, 1, NULL);
        selSetOrigin(sel, size / 2, 0);
    }

    for (i = 0; i < factor2; i++) {
        if (factor2 & 1)  /* odd */
            z = factor1 / 2 + i * factor1;
        else
            z = factor1 / 2 + i * factor1;
/*        fprintf(stderr, "i = %d, factor1 = %d, factor2 = %d, z = %d\n",
                        i, factor1, factor2, z); */
        if (direction == L_HORIZ)
            selSetElement(sel, 0, z, SEL_HIT);
        else
            selSetElement(sel, z, 0, SEL_HIT);
    }

    return sel;
}


/*!
 *  create2dIntArray()
 *
 *      Input:  sy (rows == height)
 *              sx (columns == width)
 *      Return: doubly indexed array (i.e., an array of sy row pointers,
 *              each of which points to an array of sx ints)
 *
 *  Notes:
 *      (1) The array[sy][sx] is indexed in standard "matrix notation",
 *          with the row index first.
 */
LEPTONICA_EXPORT l_int32 **
create2dIntArray(l_int32  sy,
                 l_int32  sx)
{
l_int32    i;
l_int32  **array;

    PROCNAME("create2dIntArray");

    if ((array = (l_int32 **)CALLOC(sy, sizeof(l_int32 *))) == NULL)
        return (l_int32 **)ERROR_PTR("ptr array not made", procName, NULL);

    for (i = 0; i < sy; i++) {
        if ((array[i] = (l_int32 *)CALLOC(sx, sizeof(l_int32))) == NULL)
            return (l_int32 **)ERROR_PTR("array not made", procName, NULL);
    }

    return array;
}



/*------------------------------------------------------------------------*
 *                           Extension of sela                            *
 *------------------------------------------------------------------------*/

/*!
 *  selaExtendArray()
 *
 *      Input:  sela
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
selaExtendArray(SELA  *sela)
{
    PROCNAME("selaExtendArray");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);
    
    if ((sela->sel = (SEL **)reallocNew((void **)&sela->sel,
                              sizeof(SEL *) * sela->nalloc,
                              2 * sizeof(SEL *) * sela->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    sela->nalloc = 2 * sela->nalloc;
    return 0;
}



/*----------------------------------------------------------------------*
 *                               Accessors                              *
 *----------------------------------------------------------------------*/
/*!
 *  selaGetCount()
 *
 *      Input:  sela
 *      Return: count, or 0 on error
 */
LEPTONICA_EXPORT l_int32
selaGetCount(SELA  *sela)
{
    PROCNAME("selaGetCount");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 0);

    return sela->n;
}


/*!
 *  selGetName()
 *
 *      Input:  sel
 *      Return: sel name (not copied), or null if no name or on error
 */
LEPTONICA_EXPORT char *
selGetName(SEL  *sel)
{
    PROCNAME("selGetName");

    if (!sel)
        return (char *)ERROR_PTR("sel not defined", procName, NULL);

    return sel->name;
}


/*!
 *  selSetElement()
 *
 *      Input:  sel
 *              row
 *              col
 *              type  (SEL_HIT, SEL_MISS, SEL_DONT_CARE)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Because we use row and column to index into an array,
 *          they are always non-negative.  The location of the origin
 *          (and the type of operation) determine the actual
 *          direction of the rasterop.
 */
LEPTONICA_EXPORT l_int32
selSetElement(SEL     *sel,
              l_int32  row,
              l_int32  col,
              l_int32  type)
{
    PROCNAME("selSetElement");

    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (type != SEL_HIT && type != SEL_MISS && type != SEL_DONT_CARE)
        return ERROR_INT("invalid sel element type", procName, 1);
    if (row < 0 || row >= sel->sy)
        return ERROR_INT("sel row out of bounds", procName, 1);
    if (col < 0 || col >= sel->sx)
        return ERROR_INT("sel col out of bounds", procName, 1);

    sel->data[row][col] = type;
    return 0;
}


/*!
 *  selGetParameters()
 *
 *      Input:  sel
 *              &sy, &sx, &cy, &cx (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
selGetParameters(SEL      *sel,
                 l_int32  *psy,
                 l_int32  *psx,
                 l_int32  *pcy,
                 l_int32  *pcx)
{
    PROCNAME("selGetParameters");

    if (psy) *psy = 0;
    if (psx) *psx = 0;
    if (pcy) *pcy = 0;
    if (pcx) *pcx = 0;
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (psy) *psy = sel->sy; 
    if (psx) *psx = sel->sx; 
    if (pcy) *pcy = sel->cy; 
    if (pcx) *pcx = sel->cx; 
    return 0;
}


/*!
 *  selSetOrigin()
 *
 *      Input:  sel
 *              cy, cx
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
selSetOrigin(SEL     *sel,
             l_int32  cy,
             l_int32  cx)
{
    PROCNAME("selSetOrigin");

    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    sel->cy = cy;
    sel->cx = cx;
    return 0;
}

/*----------------------------------------------------------------------*
 *                Max translations for erosion and hmt                  *
 *----------------------------------------------------------------------*/
/*!
 *  selFindMaxTranslations()
 *
 *      Input:  sel
 *              &xp, &yp, &xn, &yn  (<return> max shifts)
 *      Return: 0 if OK; 1 on error
 *
 *  Note: these are the maximum shifts for the erosion operation.
 *        For example, when j < cx, the shift of the image
 *        is +x to the cx.  This is a positive xp shift.
 */
LEPTONICA_EXPORT l_int32
selFindMaxTranslations(SEL      *sel,
                       l_int32  *pxp,
                       l_int32  *pyp,
                       l_int32  *pxn,
                       l_int32  *pyn)
{
l_int32  sx, sy, cx, cy, i, j;
l_int32  maxxp, maxyp, maxxn, maxyn;

    PROCNAME("selaFindMaxTranslations");

    if (!pxp || !pyp || !pxn || !pyn)
        return ERROR_INT("&xp (etc) defined", procName, 1);
    *pxp = *pyp = *pxn = *pyn = 0;
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    selGetParameters(sel, &sy, &sx, &cy, &cx);

    maxxp = maxyp = maxxn = maxyn = 0;
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            if (sel->data[i][j] == 1) {
                maxxp = L_MAX(maxxp, cx - j);
                maxyp = L_MAX(maxyp, cy - i);
                maxxn = L_MAX(maxxn, j - cx);
                maxyn = L_MAX(maxyn, i - cy);
            }
        }
    }

    *pxp = maxxp;
    *pyp = maxyp;
    *pxn = maxxn;
    *pyn = maxyn;

    return 0;
}
