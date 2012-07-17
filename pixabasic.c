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
 *   pixabasic.c
 *
 *      Pixa creation, destruction, copying
 *           PIXA     *pixaCreate()
 *           PIXA     *pixaCreateFromPix()
 *           PIXA     *pixaCreateFromBoxa()
 *           PIXA     *pixaSplitPix()
 *           void      pixaDestroy()
 *           PIXA     *pixaCopy()
 *
 *      Pixa addition
 *           l_int32   pixaAddPix()
 *           l_int32   pixaExtendArray()
 *           l_int32   pixaExtendArrayToSize()
 *           l_int32   pixaAddBox()
 *
 *      Pixa accessors
 *           l_int32   pixaGetCount()
 *           l_int32   pixaChangeRefcount()
 *           PIX      *pixaGetPix()
 *           l_int32   pixaGetPixDimensions()
 *           PIX      *pixaGetBoxa()
 *           l_int32   pixaGetBoxaCount()
 *           BOX      *pixaGetBox()
 *           l_int32   pixaGetBoxGeometry()
 *           PIX     **pixaGetPixArray()
 *
 *      Pixa array modifiers
 *           l_int32   pixaReplacePix()
 *           l_int32   pixaInsertPix()
 *           l_int32   pixaRemovePix()
 *           l_int32   pixaInitFull()
 *           l_int32   pixaClear()
 *
 *      Pixa combination
 *           PIXA     *pixaJoin()
 *
 *      Pixaa creation, destruction
 *           PIXAA    *pixaaCreate()
 *           PIXAA    *pixaaCreateFromPixa()
 *           void      pixaaDestroy()
 *
 *      Pixaa addition
 *           l_int32   pixaaAddPixa()
 *           l_int32   pixaaExtendArray()
 *           l_int32   pixaaAddBox()
 *
 *      Pixaa accessors
 *           l_int32   pixaaGetCount()
 *           PIXA     *pixaaGetPixa()
 *           BOXA     *pixaaGetBoxa()
 *
 *      Pixa serialized I/O  (requires png support)
 *           PIXA     *pixaRead()
 *           PIXA     *pixaReadStream()
 *           l_int32   pixaWrite()
 *           l_int32   pixaWriteStream()
 *
 *      Pixaa serialized I/O  (requires png support)
 *           PIXAA    *pixaaRead()
 *           PIXAA    *pixaaReadStream()
 *           l_int32   pixaaWrite()
 *           l_int32   pixaaWriteStream()
 *
 *   
 *   Important note on reference counting:
 *     Reference counting for the Pixa is analogous to that for the Boxa.
 *     See pix.h for details.   pixaCopy() provides three possible modes
 *     of copy.  The basic rule is that however a Pixa is obtained
 *     (e.g., from pixaCreate*(), pixaCopy(), or a Pixaa accessor),
 *     it is necessary to call pixaDestroy() on it.
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE3 = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                    Pixa creation, destruction, copy                 *
 *---------------------------------------------------------------------*/
/*!
 *  pixaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: pixa, or null on error
 */
LEPTONICA_EXPORT PIXA *
pixaCreate(l_int32  n)
{
PIXA  *pixa;

    PROCNAME("pixaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE3;

    if ((pixa = (PIXA *)CALLOC(1, sizeof(PIXA))) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    pixa->n = 0;
    pixa->nalloc = n;
    pixa->refcount = 1;
    
    if ((pixa->pix = (PIX **)CALLOC(n, sizeof(PIX *))) == NULL)
        return (PIXA *)ERROR_PTR("pix ptrs not made", procName, NULL);
    if ((pixa->boxa = boxaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("boxa not made", procName, NULL);

    return pixa;
}


/*!
 *  pixaDestroy()
 *
 *      Input:  &pixa (<can be nulled>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pixa.
 *      (2) Always nulls the input ptr.
 */
LEPTONICA_REAL_EXPORT void
pixaDestroy(PIXA  **ppixa)
{
l_int32  i;
PIXA    *pixa;

    PROCNAME("pixaDestroy");

    if (ppixa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixa = *ppixa) == NULL)
        return;

        /* Decrement the refcount.  If it is 0, destroy the pixa. */
    pixaChangeRefcount(pixa, -1);
    if (pixa->refcount <= 0) {
        for (i = 0; i < pixa->n; i++)
            pixDestroy(&pixa->pix[i]);
        FREE(pixa->pix);
        boxaDestroy(&pixa->boxa);
        FREE(pixa);
    }

    *ppixa = NULL;
    return;
}


/*!
 *  pixaCopy()
 *
 *      Input:  pixas
 *              copyflag:
 *                L_COPY makes a new pixa and copies each pix and each box
 *                L_CLONE gives a new ref-counted handle to the input pixa
 *                L_COPY_CLONE makes a new pixa and inserts clones of
 *                    all pix and boxes
 *      Return: new pixa, or null on error
 *
 *  Note: see pix.h for description of the copy types.
 */
LEPTONICA_EXPORT PIXA *
pixaCopy(PIXA    *pixa,
         l_int32  copyflag)
{
l_int32  i;
BOX     *boxc;
PIX     *pixc;
PIXA    *pixac;

    PROCNAME("pixaCopy");

    if (!pixa)
        return (PIXA *)ERROR_PTR("pixa not defined", procName, NULL);

    if (copyflag == L_CLONE) {
        pixaChangeRefcount(pixa, 1);
        return pixa;
    }

    if (copyflag != L_COPY && copyflag != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if ((pixac = pixaCreate(pixa->n)) == NULL)
        return (PIXA *)ERROR_PTR("pixac not made", procName, NULL);
    for (i = 0; i < pixa->n; i++) {
        if (copyflag == L_COPY) {
            pixc = pixaGetPix(pixa, i, L_COPY);
            boxc = pixaGetBox(pixa, i, L_COPY);
        }
        else {  /* copy-clone */
            pixc = pixaGetPix(pixa, i, L_CLONE);
            boxc = pixaGetBox(pixa, i, L_CLONE);
        }
        pixaAddPix(pixac, pixc, L_INSERT);
        pixaAddBox(pixac, boxc, L_INSERT);
    }

    return pixac;
}



/*---------------------------------------------------------------------*
 *                              Pixa addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaAddPix()
 *
 *      Input:  pixa
 *              pix  (to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaAddPix(PIXA    *pixa,
           PIX     *pix,
           l_int32  copyflag)
{
l_int32  n;
PIX     *pixc;

    PROCNAME("pixaAddPix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (copyflag == L_INSERT)
        pixc = pix;
    else if (copyflag == L_COPY)
        pixc = pixCopy(NULL, pix);
    else if (copyflag == L_CLONE)
        pixc = pixClone(pix);
    else
        return ERROR_INT("invalid copyflag", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not made", procName, 1);

    n = pixaGetCount(pixa);
    if (n >= pixa->nalloc)
        pixaExtendArray(pixa);
    pixa->pix[n] = pixc;
    pixa->n++;

    return 0;
}


/*!
 *  pixaExtendArray()
 *
 *      Input:  pixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Doubles the size of the pixa and boxa ptr arrays.
 */
LEPTONICA_EXPORT l_int32
pixaExtendArray(PIXA  *pixa)
{
    PROCNAME("pixaExtendArray");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    return pixaExtendArrayToSize(pixa, 2 * pixa->nalloc);
}


/*!
 *  pixaExtendArrayToSize()
 *
 *      Input:  pixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new pixa and boxa ptrs arrays to @size.
 *          The pixa and boxa ptr arrays must always be equal in size.
 */
LEPTONICA_EXPORT l_int32
pixaExtendArrayToSize(PIXA    *pixa,
                      l_int32  size)
{
    PROCNAME("pixaExtendArrayToSize");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    if (size > pixa->nalloc) {
        if ((pixa->pix = (PIX **)reallocNew((void **)&pixa->pix,
                                 sizeof(PIX *) * pixa->nalloc,
                                 size * sizeof(PIX *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        pixa->nalloc = size;
    }
    return boxaExtendArrayToSize(pixa->boxa, size);
}


/*!
 *  pixaAddBox()
 *
 *      Input:  pixa
 *              box
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaAddBox(PIXA    *pixa,
           BOX     *box,
           l_int32  copyflag)
{
    PROCNAME("pixaAddBox");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    boxaAddBox(pixa->boxa, box, copyflag);
    return 0;
}



/*---------------------------------------------------------------------*
 *                             Pixa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaGetCount()
 *
 *      Input:  pixa
 *      Return: count, or 0 if no pixa
 */
LEPTONICA_EXPORT l_int32
pixaGetCount(PIXA  *pixa)
{
    PROCNAME("pixaGetCount");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 0);

    return pixa->n;
}


/*!
 *  pixaChangeRefcount()
 *
 *      Input:  pixa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaChangeRefcount(PIXA    *pixa,
                   l_int32  delta)
{
    PROCNAME("pixaChangeRefcount");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    pixa->refcount += delta;
    return 0;
}


/*!
 *  pixaGetPix()
 *
 *      Input:  pixa
 *              index  (to the index-th pix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: pix, or null on error
 */
LEPTONICA_EXPORT PIX *
pixaGetPix(PIXA    *pixa,
           l_int32  index,
           l_int32  accesstype)
{
    PROCNAME("pixaGetPix");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (index < 0 || index >= pixa->n)
        return (PIX *)ERROR_PTR("index not valid", procName, NULL);

    if (accesstype == L_COPY)
        return pixCopy(NULL, pixa->pix[index]);
    else if (accesstype == L_CLONE)
        return pixClone(pixa->pix[index]);
    else
        return (PIX *)ERROR_PTR("invalid accesstype", procName, NULL);
}


/*!
 *  pixaGetBoxa()
 *
 *      Input:  pixa
 *              accesstype  (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: boxa, or null on error
 */
LEPTONICA_EXPORT BOXA *
pixaGetBoxa(PIXA    *pixa,
            l_int32  accesstype)
{
    PROCNAME("pixaGetBoxa");

    if (!pixa)
        return (BOXA *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!pixa->boxa)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (BOXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    return boxaCopy(pixa->boxa, accesstype);
}


/*!
 *  pixaGetBox()
 *
 *      Input:  pixa
 *              index  (to the index-th pix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: box (if null, not automatically an error), or null on error
 *
 *  Notes:
 *      (1) There is always a boxa with a pixa, and it is initialized so
 *          that each box ptr is NULL.
 *      (2) In general, we expect that there is either a box associated
 *          with each pix, or no boxes at all in the boxa.
 *      (3) Having no boxes is thus not an automatic error.  Whether it
 *          is an actual error is determined by the calling program.
 *          If the caller expects to get a box, it is an error; see, e.g.,
 *          pixaGetBoxGeometry().
 */
LEPTONICA_EXPORT BOX *
pixaGetBox(PIXA    *pixa,
           l_int32  index,
           l_int32  accesstype)
{
BOX  *box;

    PROCNAME("pixaGetBox");

    if (!pixa)
        return (BOX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!pixa->boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (index < 0 || index >= pixa->boxa->n)
        return (BOX *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOX *)ERROR_PTR("invalid accesstype", procName, NULL);

    box = pixa->boxa->box[index];
    if (box) {
        if (accesstype == L_COPY)
            return boxCopy(box);
        else  /* accesstype == L_CLONE */
            return boxClone(box);
    }
    else
        return NULL;
}


/*---------------------------------------------------------------------*
 *                       Pixa array modifiers                          *
 *---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*
 *                    Pixaa creation and destruction                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaCreate()
 *
 *      Input:  n  (initial number of pixa ptrs)
 *      Return: pixaa, or null on error
 *
 *  Notes:
 *      (1) A pixaa provides a 2-level hierarchy of images.
 *          A common use is for segmentation masks, which are
 *          inexpensive to store in png format.
 *      (2) For example, suppose you want a mask for each textline
 *          in a two-column page.  The textline masks for each column
 *          can be represented by a pixa, of which there are 2 in the pixaa.
 *          The boxes for the textline mask components within a column
 *          can have their origin referred to the column rather than the page.
 *          Then the boxa field can be used to represent the two box (regions)
 *          for the columns, and the (x,y) components of each box can
 *          be used to get the absolute position of the textlines on
 *          the page.
 */
LEPTONICA_EXPORT PIXAA *
pixaaCreate(l_int32  n)
{
PIXAA  *pixaa;

    PROCNAME("pixaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE3;

    if ((pixaa = (PIXAA *)CALLOC(1, sizeof(PIXAA))) == NULL)
        return (PIXAA *)ERROR_PTR("pixaa not made", procName, NULL);
    pixaa->n = 0;
    pixaa->nalloc = n;
    
    if ((pixaa->pixa = (PIXA **)CALLOC(n, sizeof(PIXA *))) == NULL)
        return (PIXAA *)ERROR_PTR("pixa ptrs not made", procName, NULL);
    pixaa->boxa = boxaCreate(n);

    return pixaa;
}


/*!
 *  pixaaDestroy()
 *
 *      Input:  &pixaa <to be nulled>
 *      Return: void
 */
LEPTONICA_EXPORT void
pixaaDestroy(PIXAA  **ppixaa)
{
l_int32  i;
PIXAA   *pixaa;

    PROCNAME("pixaaDestroy");

    if (ppixaa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixaa = *ppixaa) == NULL)
        return;

    for (i = 0; i < pixaa->n; i++)
        pixaDestroy(&pixaa->pixa[i]);
    FREE(pixaa->pixa);
    boxaDestroy(&pixaa->boxa);

    FREE(pixaa);
    *ppixaa = NULL;

    return;
}


/*---------------------------------------------------------------------*
 *                             Pixaa addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaAddPixa()
 *
 *      Input:  pixaa
 *              pixa  (to be added)
 *              copyflag:
 *                L_INSERT inserts the pixa directly
 *                L_COPY makes a new pixa and copies each pix and each box
 *                L_CLONE gives a new handle to the input pixa
 *                L_COPY_CLONE makes a new pixa and inserts clones of
 *                    all pix and boxes
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaaAddPixa(PIXAA   *pixaa,
             PIXA    *pixa,
             l_int32  copyflag)
{
l_int32  n;
PIXA    *pixac;

    PROCNAME("pixaaAddPixa");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY &&
        copyflag != L_CLONE && copyflag != L_COPY_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    if (copyflag == L_INSERT)
        pixac = pixa;
    else {
        if ((pixac = pixaCopy(pixa, copyflag)) == NULL)
            return ERROR_INT("pixac not made", procName, 1);
    }

    n = pixaaGetCount(pixaa);
    if (n >= pixaa->nalloc)
        pixaaExtendArray(pixaa);
    pixaa->pixa[n] = pixac;
    pixaa->n++;

    return 0;
}


/*!
 *  pixaaExtendArray()
 *
 *      Input:  pixaa
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
pixaaExtendArray(PIXAA  *pixaa)
{
    PROCNAME("pixaaExtendArray");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);

    if ((pixaa->pixa = (PIXA **)reallocNew((void **)&pixaa->pixa,
                             sizeof(PIXA *) * pixaa->nalloc,
                             2 * sizeof(PIXA *) * pixaa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    pixaa->nalloc = 2 * pixaa->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                            Pixaa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaGetCount()
 *
 *      Input:  pixaa
 *      Return: count, or 0 if no pixaa
 */
LEPTONICA_EXPORT l_int32
pixaaGetCount(PIXAA  *pixaa)
{
    PROCNAME("pixaaGetCount");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 0);

    return pixaa->n;
}


/*!
 *  pixaaGetPixa()
 *
 *      Input:  pixaa
 *              index  (to the index-th pixa)
 *              accesstype  (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: pixa, or null on error
 *
 *  Notes:
 *      (1) L_COPY makes a new pixa with a copy of every pix
 *      (2) L_CLONE just makes a new reference to the pixa,
 *          and bumps the counter.  You would use this, for example,
 *          when you need to extract some data from a pix within a
 *          pixa within a pixaa.
 *      (3) L_COPY_CLONE makes a new pixa with a clone of every pix
 *          and box
 *      (4) In all cases, you must invoke pixaDestroy() on the returned pixa
 */
LEPTONICA_EXPORT PIXA *
pixaaGetPixa(PIXAA   *pixaa,
             l_int32  index,
             l_int32  accesstype)
{
PIXA  *pixa;

    PROCNAME("pixaaGetPixa");

    if (!pixaa)
        return (PIXA *)ERROR_PTR("pixaa not defined", procName, NULL);
    if (index < 0 || index >= pixaa->n)
        return (PIXA *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    if ((pixa = pixaa->pixa[index]) == NULL)  /* shouldn't happen! */
        return (PIXA *)ERROR_PTR("no pixa[index]", procName, NULL);
    return pixaCopy(pixa, accesstype);
}
