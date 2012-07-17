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
 *                  Box creation, destruction and copy                 *
 *---------------------------------------------------------------------*/
/*!
 *  boxCreate()
 *
 *      Input:  x, y, w, h
 *      Return: box, or null on error
 *
 *  Notes:
 *      (1) This clips the box to the +quad.  If no part of the
 *          box is in the +quad, this returns NULL.
 *      (2) We allow you to make a box with w = 0 and/or h = 0.
 *          This does not represent a valid region, but it is useful
 *          as a placeholder in a boxa for which the index of the
 *          box in the boxa is important.  This is an atypical
 *          situation; usually you want to put only valid boxes with
 *          nonzero width and height in a boxa.  If you have a boxa
 *          with invalid boxes, the accessor boxaGetValidBox()
 *          will return NULL on each invalid box.
 *      (3) If you want to create only valid boxes, use boxCreateValid(),
 *          which returns NULL if either w or h is 0.
 */
LEPTONICA_EXPORT BOX *
boxCreate(l_int32  x,
          l_int32  y,
          l_int32  w,
          l_int32  h)
{
BOX  *box;

    PROCNAME("boxCreate");

    if (w < 0 || h < 0)
        return (BOX *)ERROR_PTR("w and h not both >= 0", procName, NULL);
    if (x < 0) {  /* take part in +quad */
        w = w + x;
        x = 0;
        if (w <= 0)
            return (BOX *)ERROR_PTR("x < 0 and box off +quad", procName, NULL);
    }
    if (y < 0) {  /* take part in +quad */
        h = h + y;
        y = 0;
        if (h <= 0)
            return (BOX *)ERROR_PTR("y < 0 and box off +quad", procName, NULL);
    }

    if ((box = (BOX *)CALLOC(1, sizeof(BOX))) == NULL)
        return (BOX *)ERROR_PTR("box not made", procName, NULL);
    boxSetGeometry(box, x, y, w, h);
    box->refcount = 1;

    return box;
}


/*!
 *  boxCopy()
 *
 *      Input:  box
 *      Return: copy of box, or null on error
 */
LEPTONICA_EXPORT BOX *
boxCopy(BOX  *box)
{
BOX  *boxc;

    PROCNAME("boxCopy");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);

    boxc = boxCreate(box->x, box->y, box->w, box->h);

    return boxc;
}


/*!
 *  boxClone()
 *
 *      Input:  box
 *      Return: ptr to same box, or null on error
 */
LEPTONICA_EXPORT BOX *
boxClone(BOX  *box)
{

    PROCNAME("boxClone");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);

    boxChangeRefcount(box, 1);
    return box;
}


/*!
 *  boxDestroy()
 *
 *      Input:  &box (<will be set to null before returning>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the box.
 *      (2) Always nulls the input ptr.
 */
LEPTONICA_EXPORT void
boxDestroy(BOX  **pbox)
{
BOX  *box;

    PROCNAME("boxDestroy");

    if (pbox == NULL) {
        L_WARNING("ptr address is null!", procName);
        return;
    }
    if ((box = *pbox) == NULL)
        return;

    boxChangeRefcount(box, -1);
    if (boxGetRefcount(box) <= 0)
        FREE(box);
    *pbox = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                              Box accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxGetGeometry()
 *
 *      Input:  box
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxGetGeometry(BOX      *box,
               l_int32  *px,
               l_int32  *py,
               l_int32  *pw,
               l_int32  *ph)
{
    PROCNAME("boxGetGeometry");

    if (px) *px = 0;
    if (py) *py = 0;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (px) *px = box->x;
    if (py) *py = box->y;
    if (pw) *pw = box->w;
    if (ph) *ph = box->h;
    return 0;
}


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
 *  boxaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: boxa, or null on error
 */
LEPTONICA_EXPORT BOXA *
boxaCreate(l_int32  n)
{
BOXA  *boxa;

    PROCNAME("boxaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE2;

    if ((boxa = (BOXA *)CALLOC(1, sizeof(BOXA))) == NULL)
        return (BOXA *)ERROR_PTR("boxa not made", procName, NULL);
    boxa->n = 0;
    boxa->nalloc = n;
    boxa->refcount = 1;

    if ((boxa->box = (BOX **)CALLOC(n, sizeof(BOX *))) == NULL)
        return (BOXA *)ERROR_PTR("boxa ptrs not made", procName, NULL);

    return boxa;
}


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
 *  boxaDestroy()
 *
 *      Input:  &boxa (<will be set to null before returning>)
 *      Return: void
 *
 *  Note:
 *      - Decrements the ref count and, if 0, destroys the boxa.
 *      - Always nulls the input ptr.
 */
LEPTONICA_EXPORT void
boxaDestroy(BOXA  **pboxa)
{
l_int32  i;
BOXA    *boxa;

    PROCNAME("boxaDestroy");

    if (pboxa == NULL) {
        L_WARNING("ptr address is null!", procName);
        return;
    }

    if ((boxa = *pboxa) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the boxa. */
    boxa->refcount--;
    if (boxa->refcount <= 0) {
        for (i = 0; i < boxa->n; i++)
            boxDestroy(&boxa->box[i]);
        FREE(boxa->box);
        FREE(boxa);
    }

    *pboxa = NULL;
    return;
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


/*!
 *  boxaExtendArrayToSize()
 *
 *      Input:  boxa
 *              size (new size of boxa array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new boxa ptr array to @size.
 */
LEPTONICA_EXPORT l_int32
boxaExtendArrayToSize(BOXA    *boxa,
                      l_int32  size)
{
    PROCNAME("boxaExtendArrayToSize");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    if (size > boxa->nalloc) {
        if ((boxa->box = (BOX **)reallocNew((void **)&boxa->box,
                                            sizeof(BOX *) * boxa->nalloc,
                                            size * sizeof(BOX *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        boxa->nalloc = size;
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                             Boxa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxaGetCount()
 *
 *      Input:  boxa
 *      Return: count (of all boxes); 0 if no boxes or on error
 */
LEPTONICA_EXPORT l_int32
boxaGetCount(BOXA  *boxa)
{
    PROCNAME("boxaGetCount");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 0);
    return boxa->n;
}


/*!
 *  boxaGetBox()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              accessflag  (L_COPY or L_CLONE)
 *      Return: box, or null on error
 */
LEPTONICA_EXPORT BOX *
boxaGetBox(BOXA    *boxa,
           l_int32  index,
           l_int32  accessflag)
{
    PROCNAME("boxaGetBox");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (index < 0 || index >= boxa->n)
        return (BOX *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return boxCopy(boxa->box[index]);
    else if (accessflag == L_CLONE)
        return boxClone(boxa->box[index]);
    else
        return (BOX *)ERROR_PTR("invalid accessflag", procName, NULL);
}


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


/*---------------------------------------------------------------------*
 *                        Boxa array modifiers                         *
 *---------------------------------------------------------------------*/
/*!
 *  boxaReplaceBox()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              box (insert to replace existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place replacement of one box.
 *      (2) The previous box at that location is destroyed.
 */
LEPTONICA_EXPORT l_int32
boxaReplaceBox(BOXA    *boxa,
               l_int32  index,
               BOX     *box)
{
    PROCNAME("boxaReplaceBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (index < 0 || index >= boxa->n)
        return ERROR_INT("index not valid", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    boxDestroy(&(boxa->box[index]));
    boxa->box[index] = box;
    return 0;
}


/*!
 *  boxaInsertBox()
 *
 *      Input:  boxa
 *              index (location in boxa to insert new value)
 *              box (new box to be inserted)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts box[i] --> box[i + 1] for all i >= index,
 *          and then inserts box as box[index].
 *      (2) To insert at the beginning of the array, set index = 0.
 *      (3) To append to the array, it's easier to use boxaAddBox().
 *      (4) This should not be used repeatedly to insert into large arrays,
 *          because the function is O(n).
 */
LEPTONICA_EXPORT l_int32
boxaInsertBox(BOXA    *boxa,
              l_int32  index,
              BOX     *box)
{
l_int32  i, n;
BOX    **array;

    PROCNAME("boxaInsertBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    if (n >= boxa->nalloc)
        boxaExtendArray(boxa);
    array = boxa->box;
    boxa->n++;
    for (i = n; i > index; i--)
        array[i] = array[i - 1];
    array[index] = box;

    return 0;
}


/*!
 *  boxaRemoveBox()
 *
 *      Input:  boxa
 *              index (of box to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes box[index] and then shifts
 *          box[i] --> box[i - 1] for all i > index.
 *      (2) It should not be used repeatedly to remove boxes from
 *          large arrays, because the function is O(n).
 */
LEPTONICA_EXPORT l_int32
boxaRemoveBox(BOXA    *boxa,
              l_int32  index)
{
l_int32  i, n;
BOX    **array;

    PROCNAME("boxaRemoveBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    array = boxa->box;
    boxDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    boxa->n--;

    return 0;
}


/*!
 *  boxaInitFull()
 *
 *      Input:  boxa (typically empty)
 *              box (to be replicated into the entire ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a boxa by filling up the entire box ptr array
 *          with copies of @box.  Any existing boxes are destroyed.
 *          After this oepration, the number of boxes is equal to
 *          the number of allocated ptrs.
 *      (2) Note that we use boxaReplaceBox() instead of boxaInsertBox().
 *          They both have the same effect when inserting into a NULL ptr
 *          in the boxa ptr array:
 *      (3) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of boxes into a boxa.
 *          To randomly insert boxes into a boxa, up to some index "max":
 *             Boxa *boxa = boxaCreate(max);
 *             Box *box = boxCreate(...);
 *             boxaInitFull(boxa, box);
 *          If we have an existing boxa with a smaller ptr array, it can
 *          be reused:
 *             boxaExtendArrayToSize(boxa, max);
 *             Box *box = boxCreate(...);
 *             boxaInitFull(boxa, box);
 *          The initialization allows the boxa to always be properly
 *          filled, even if all the boxes are not later replaced.
 *          If you want to know which boxes have been replaced, you can
 *          initialize the array with invalid boxes that have 
 *          w = 0 and/or h = 0.  Then boxaGetValidBox() will return
 *          NULL for the invalid boxes.
 */
LEPTONICA_EXPORT l_int32
boxaInitFull(BOXA  *boxa,
             BOX   *box)
{
l_int32  i, n;
BOX     *boxt;

    PROCNAME("boxaInitFull");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    n = boxa->nalloc;
    boxa->n = n;
    for (i = 0; i < n; i++) {
        boxt = boxCopy(box);
        boxaReplaceBox(boxa, i, boxt);
    }
    return 0;
}


/*!
 *  boxaClear()
 *
 *      Input:  boxa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all boxes in the boxa, setting the ptrs
 *          to null.  The number of allocated boxes, n, is set to 0.
 */
LEPTONICA_EXPORT l_int32
boxaClear(BOXA  *boxa)
{
l_int32  i, n;

    PROCNAME("boxaClear");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++)
        boxDestroy(&boxa->box[i]);
    boxa->n = 0;
    return 0;
}


/*--------------------------------------------------------------------------*
 *                     Boxaa creation, destruction                          *
 *--------------------------------------------------------------------------*/
/*!
 *  boxaaCreate()
 *
 *      Input:  size of boxa ptr array to be alloc'd (0 for default)
 *      Return: baa, or null on error
 */
LEPTONICA_EXPORT BOXAA *
boxaaCreate(l_int32  n)
{
BOXAA  *baa;

    PROCNAME("boxaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE2;

    if ((baa = (BOXAA *)CALLOC(1, sizeof(BOXAA))) == NULL)
        return (BOXAA *)ERROR_PTR("baa not made", procName, NULL);
    if ((baa->boxa = (BOXA **)CALLOC(n, sizeof(BOXA *))) == NULL)
        return (BOXAA *)ERROR_PTR("boxa ptr array not made", procName, NULL);

    baa->nalloc = n;
    baa->n = 0;

    return baa;
}


/*--------------------------------------------------------------------------*
 *                              Add Boxa to Boxaa                           *
 *--------------------------------------------------------------------------*/
/*!
 *  boxaaAddBoxa()
 *
 *      Input:  boxaa
 *              boxa     (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaaAddBoxa(BOXAA   *baa,
             BOXA    *ba,
             l_int32  copyflag)
{
l_int32  n;
BOXA    *bac;

    PROCNAME("boxaaAddBoxa");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!ba)
        return ERROR_INT("ba not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    if (copyflag == L_INSERT)
        bac = ba;
    else
        bac = boxaCopy(ba, copyflag);

    n = boxaaGetCount(baa);
    if (n >= baa->nalloc)
        boxaaExtendArray(baa);
    baa->boxa[n] = bac;
    baa->n++;
    return 0;
}


/*!
 *  boxaaExtendArray()
 *
 *      Input:  boxaa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaaExtendArray(BOXAA  *baa)
{

    PROCNAME("boxaaExtendArray");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    if ((baa->boxa = (BOXA **)reallocNew((void **)&baa->boxa,
                              sizeof(BOXA *) * baa->nalloc,
                              2 * sizeof(BOXA *) * baa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    baa->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                           Boxaa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  boxaaGetCount()
 *
 *      Input:  boxaa
 *      Return: count (number of boxa), or 0 if no boxa or on error
 */
LEPTONICA_EXPORT l_int32
boxaaGetCount(BOXAA  *baa)
{
    PROCNAME("boxaaGetCount");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 0);
    return baa->n;
}


/*!
 *  boxaaGetBoxa()
 *
 *      Input:  boxaa
 *              index  (to the index-th boxa)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: boxa, or null on error
 */
LEPTONICA_EXPORT BOXA *
boxaaGetBoxa(BOXAA   *baa,
             l_int32  index,
             l_int32  accessflag)
{
l_int32  n;

    PROCNAME("boxaaGetBoxa");

    if (!baa)
        return (BOXA *)ERROR_PTR("baa not defined", procName, NULL);
    n = boxaaGetCount(baa);
    if (index < 0 || index >= n)
        return (BOXA *)ERROR_PTR("index not valid", procName, NULL);
    if (accessflag != L_COPY && accessflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid accessflag", procName, NULL);

    return boxaCopy(baa->boxa[index], accessflag);
}


/*!
 *  boxaaReadStream()
 *
 *      Input:  stream
 *      Return: boxaa, or null on error
 */
LEPTONICA_EXPORT BOXAA *
boxaaReadStream(FILE  *fp)
{
l_int32  n, i, x, y, w, h, version;
l_int32  ignore;
BOXA    *boxa;
BOXAA   *baa;

    PROCNAME("boxaaReadStream");

    if (!fp)
        return (BOXAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nBoxaa Version %d\n", &version) != 1)
        return (BOXAA *)ERROR_PTR("not a boxaa file", procName, NULL);
    if (version != BOXAA_VERSION_NUMBER)
        return (BOXAA *)ERROR_PTR("invalid boxa version", procName, NULL);
    if (fscanf(fp, "Number of boxa = %d\n", &n) != 1)
        return (BOXAA *)ERROR_PTR("not a boxaa file", procName, NULL);

    if ((baa = boxaaCreate(n)) == NULL)
        return (BOXAA *)ERROR_PTR("boxaa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "\nBoxa[%d] extent: x = %d, y = %d, w = %d, h = %d",
                   &ignore, &x, &y, &w, &h) != 5)
            return (BOXAA *)ERROR_PTR("boxa descr not valid", procName, NULL);
        if ((boxa = boxaReadStream(fp)) == NULL)
            return (BOXAA *)ERROR_PTR("boxa not made", procName, NULL);
        boxaaAddBoxa(baa, boxa, L_INSERT);
    }

    return baa;
}

/*!
 *  boxaaWriteStream()
 *
 *      Input: stream
 *             boxaa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaaWriteStream(FILE   *fp,
                 BOXAA  *baa)
{
l_int32  n, i, x, y, w, h;
BOX     *box;
BOXA    *boxa;

    PROCNAME("boxaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    n = boxaaGetCount(baa);
    fprintf(fp, "\nBoxaa Version %d\n", BOXAA_VERSION_NUMBER);
    fprintf(fp, "Number of boxa = %d\n", n);

    for (i = 0; i < n; i++) {
        if ((boxa = boxaaGetBoxa(baa, i, L_CLONE)) == NULL)
            return ERROR_INT("boxa not found", procName, 1);
        boxaGetExtent(boxa, NULL, NULL, &box);
        boxGetGeometry(box, &x, &y, &w, &h);
        fprintf(fp, "\nBoxa[%d] extent: x = %d, y = %d, w = %d, h = %d",
                i, x, y, w, h);
        boxaWriteStream(fp, boxa);
        boxDestroy(&box);
        boxaDestroy(&boxa);
    }
    return 0;
}


/*!
 *  boxaReadStream()
 *
 *      Input:  stream
 *      Return: boxa, or null on error
 */
LEPTONICA_EXPORT BOXA *
boxaReadStream(FILE  *fp)
{
l_int32  n, i, x, y, w, h, version;
l_int32  ignore;
BOX     *box;
BOXA    *boxa;

    PROCNAME("boxaReadStream");

    if (!fp)
        return (BOXA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nBoxa Version %d\n", &version) != 1)
        return (BOXA *)ERROR_PTR("not a boxa file", procName, NULL);
    if (version != BOXA_VERSION_NUMBER)
        return (BOXA *)ERROR_PTR("invalid boxa version", procName, NULL);
    if (fscanf(fp, "Number of boxes = %d\n", &n) != 1)
        return (BOXA *)ERROR_PTR("not a boxa file", procName, NULL);

    if ((boxa = boxaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("boxa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "  Box[%d]: x = %d, y = %d, w = %d, h = %d\n",
                &ignore, &x, &y, &w, &h) != 5)
            return (BOXA *)ERROR_PTR("box descr not valid", procName, NULL);
        if ((box = boxCreate(x, y, w, h)) == NULL)
            return (BOXA *)ERROR_PTR("box not made", procName, NULL);
        boxaAddBox(boxa, box, L_INSERT);
    }

    return boxa;
}


/*!
 *  boxaWriteStream()
 *
 *      Input: stream
 *             boxa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
boxaWriteStream(FILE  *fp,
                BOXA  *boxa)
{
l_int32  n, i;
BOX     *box;

    PROCNAME("boxaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    fprintf(fp, "\nBoxa Version %d\n", BOXA_VERSION_NUMBER);
    fprintf(fp, "Number of boxes = %d\n", n);
    for (i = 0; i < n; i++) {
        if ((box = boxaGetBox(boxa, i, L_CLONE)) == NULL)
            return ERROR_INT("box not found", procName, 1);
        fprintf(fp, "  Box[%d]: x = %d, y = %d, w = %d, h = %d\n",
                i, box->x, box->y, box->w, box->h);
        boxDestroy(&box);
    }
    return 0;
}
