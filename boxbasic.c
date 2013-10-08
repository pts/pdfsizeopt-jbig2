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
 *   boxbasic.c
 *
 *   Basic 'class' functions for box, boxa and boxaa,
 *   including accessors and serialization.
 *
 *      Box creation, copy, clone, destruction
 *           BOX      *boxCreate()
 *           BOX      *boxCreateValid()
 *           BOX      *boxCopy()
 *           BOX      *boxClone()
 *           void      boxDestroy()
 *
 *      Box accessors
 *           l_int32   boxGetGeometry()
 *           l_int32   boxSetGeometry()
 *           l_int32   boxGetRefcount()
 *           l_int32   boxChangeRefcount()
 *
 *      Boxa creation, copy, destruction
 *           BOXA     *boxaCreate()
 *           BOXA     *boxaCopy()
 *           void      boxaDestroy()
 *
 *      Boxa array extension
 *           l_int32   boxaAddBox()
 *           l_int32   boxaExtendArray()
 *           l_int32   boxaExtendArrayToSize()
 *
 *      Boxa accessors
 *           l_int32   boxaGetCount()
 *           l_int32   boxaGetValidCount()
 *           BOX      *boxaGetBox()
 *           BOX      *boxaGetValidBox()
 *           l_int32   boxaGetBoxGeometry()
 *
 *      Boxa array modifiers
 *           l_int32   boxaReplaceBox()
 *           l_int32   boxaInsertBox()
 *           l_int32   boxaRemoveBox()
 *           l_int32   boxaInitFull()
 *           l_int32   boxaClear()
 *
 *      Boxaa creation, copy, destruction
 *           BOXAA    *boxaaCreate()
 *           BOXAA    *boxaaCopy()
 *           void      boxaaDestroy()
 *
 *      Boxaa array extension
 *           l_int32   boxaaAddBoxa()
 *           l_int32   boxaaExtendArray()
 *
 *      Boxaa accessors
 *           l_int32   boxaaGetCount()
 *           l_int32   boxaaGetBoxCount()
 *           BOXA     *boxaaGetBoxa()
 *
 *      Boxa array modifiers
 *           l_int32   boxaaReplaceBoxa()
 *           l_int32   boxaaInsertBoxa()
 *           l_int32   boxaaRemoveBoxa()
 *           l_int32   boxaaAddBox()
 *
 *      Boxaa serialized I/O
 *           BOXAA    *boxaaRead()
 *           BOXAA    *boxaaReadStream()
 *           l_int32   boxaaWrite()
 *           l_int32   boxaaWriteStream()
 *
 *      Boxa serialized I/O
 *           BOXA     *boxaRead()
 *           BOXA     *boxaReadStream()
 *           l_int32   boxaWrite()
 *           l_int32   boxaWriteStream()
 *
 *      Box print (for debug)
 *           l_int32   boxPrintStreamInfo()
 *
 *   Most functions use only valid boxes, which are boxes that have both
 *   width and height > 0.  However, a few functions, such as
 *   boxaGetMedian() do not assume that all boxes are valid.  For any
 *   function that can use a boxa with invalid boxes, it is convenient 
 *   to use these accessors:
 *       boxaGetValidCount()   :  count of valid boxes
 *       boxaGetValidBox()     :  returns NULL for invalid boxes
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE2 = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                              Box accessors                          *
 *---------------------------------------------------------------------*/

/*!
 *  boxSetGeometry()
 *
 *      Input:  box
 *              x, y, w, h (use -1 to leave unchanged)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxSetGeometry(BOX     *box,
               l_int32  x,
               l_int32  y,
               l_int32  w,
               l_int32  h)
{
    PROCNAME("boxSetGeometry");

    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (x != -1) box->x = x;
    if (y != -1) box->y = y;
    if (w != -1) box->w = w;
    if (h != -1) box->h = h;
    return 0;
}


LEPTONICA_EXPORT l_int32
boxGetRefcount(BOX  *box)
{
    PROCNAME("boxGetRefcount");

    if (!box)
        return ERROR_INT("box not defined", procName, UNDEF);

    return box->refcount;
}


LEPTONICA_EXPORT l_int32
boxChangeRefcount(BOX     *box,
                  l_int32  delta)
{
    PROCNAME("boxChangeRefcount");

    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    box->refcount += delta;
    return 0;
}


/*---------------------------------------------------------------------*
 *             Boxa creation, destruction, copy, extension             *
 *---------------------------------------------------------------------*/

/*!
 *  boxaCopy()
 *
 *      Input:  boxa
 *              copyflag (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: new boxa, or null on error
 *
 *  Notes:
 *      (1) See pix.h for description of the copyflag.
 *      (2) The copy-clone makes a new boxa that holds clones of each box.
 */
LEPTONICA_EXPORT BOXA *
boxaCopy(BOXA    *boxa,
         l_int32  copyflag)
{
l_int32  i;
BOX     *boxc;
BOXA    *boxac;

    PROCNAME("boxaCopy");

    if (!boxa)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);

    if (copyflag == L_CLONE) {
        boxa->refcount++;
        return boxa;
    }

    if (copyflag != L_COPY && copyflag != L_COPY_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if ((boxac = boxaCreate(boxa->nalloc)) == NULL)
        return (BOXA *)ERROR_PTR("boxac not made", procName, NULL);
    for (i = 0; i < boxa->n; i++) {
        if (copyflag == L_COPY)
            boxc = boxaGetBox(boxa, i, L_COPY);
        else   /* copy-clone */
            boxc = boxaGetBox(boxa, i, L_CLONE);
        boxaAddBox(boxac, boxc, L_INSERT);
    }
    return boxac;
}

/*!
 *  boxaAddBox()
 *
 *      Input:  boxa
 *              box  (to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaAddBox(BOXA    *boxa,
           BOX     *box,
           l_int32  copyflag)
{
l_int32  n;
BOX     *boxc;

    PROCNAME("boxaAddBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    if (copyflag == L_INSERT)
        boxc = box;
    else if (copyflag == L_COPY)
        boxc = boxCopy(box);
    else if (copyflag == L_CLONE)
        boxc = boxClone(box);
    else
        return ERROR_INT("invalid copyflag", procName, 1);
    if (!boxc)
        return ERROR_INT("boxc not made", procName, 1);

    n = boxaGetCount(boxa);
    if (n >= boxa->nalloc)
        boxaExtendArray(boxa);
    boxa->box[n] = boxc;
    boxa->n++;

    return 0;
}


/*!
 *  boxaExtendArray()
 *
 *      Input:  boxa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Reallocs with doubled size of ptr array.
 */
LEPTONICA_EXPORT l_int32
boxaExtendArray(BOXA  *boxa)
{
    PROCNAME("boxaExtendArray");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    return boxaExtendArrayToSize(boxa, 2 * boxa->nalloc);
}

/*---------------------------------------------------------------------*
 *                             Boxa accessors                          *
 *---------------------------------------------------------------------*/

/*!
 *  boxaGetBoxGeometry()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaGetBoxGeometry(BOXA     *boxa,
                   l_int32   index,
                   l_int32  *px,
                   l_int32  *py,
                   l_int32  *pw,
                   l_int32  *ph)
{
BOX  *box;

    PROCNAME("boxaGetBoxGeometry");

    if (px) *px = 0;
    if (py) *py = 0;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (index < 0 || index >= boxa->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((box = boxaGetBox(boxa, index, L_CLONE)) == NULL)
        return ERROR_INT("box not found!", procName, 1);
    boxGetGeometry(box, px, py, pw, ph);
    boxDestroy(&box);
    return 0;
}
