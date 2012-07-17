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
 *   ptabasic.c
 *
 *      Pta creation, destruction, copy, clone, empty
 *           PTA      *ptaCreate()
 *           PTA      *ptaCreateFromNuma()
 *           void      ptaDestroy()
 *           PTA      *ptaCopy()
 *           PTA      *ptaClone()
 *           l_int32   ptaEmpty()
 *
 *      Pta array extension
 *           l_int32   ptaAddPt()
 *           l_int32   ptaExtendArrays()
 *
 *      Pta Accessors
 *           l_int32   ptaGetRefcount()
 *           l_int32   ptaChangeRefcount()
 *           l_int32   ptaGetCount()
 *           l_int32   ptaGetPt()
 *           l_int32   ptaGetIPt()
 *           l_int32   ptaSetPt()
 *           l_int32   ptaGetArrays()
 *
 *      Pta serialized for I/O
 *           PTA      *ptaRead()
 *           PTA      *ptaReadStream()
 *           l_int32   ptaWrite()
 *           l_int32   ptaWriteStream()
 *
 *      Ptaa creation, destruction
 *           PTAA     *ptaaCreate()
 *           void      ptaaDestroy()
 *
 *      Ptaa array extension
 *           l_int32   ptaaAddPta()
 *           l_int32   ptaaExtendArray()
 *
 *      Ptaa Accessors
 *           l_int32   ptaaGetCount()
 *           l_int32   ptaaGetPta()
 *           l_int32   ptaaGetPt()
 *
 *      Ptaa serialized for I/O
 *           PTAA     *ptaaRead()
 *           PTAA     *ptaaReadStream()
 *           l_int32   ptaaWrite()
 *           l_int32   ptaaWriteStream()
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                Pta creation, destruction, copy, clone               *
 *---------------------------------------------------------------------*/
/*!
 *  ptaCreate()
 *
 *      Input:  n  (initial array sizes)
 *      Return: pta, or null on error.
 */
LEPTONICA_REAL_EXPORT PTA *
ptaCreate(l_int32  n)
{
PTA  *pta;

    PROCNAME("ptaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pta = (PTA *)CALLOC(1, sizeof(PTA))) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    pta->n = 0;
    pta->nalloc = n;
    ptaChangeRefcount(pta, 1);  /* sets to 1 */

    if ((pta->x = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (PTA *)ERROR_PTR("x array not made", procName, NULL);
    if ((pta->y = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (PTA *)ERROR_PTR("y array not made", procName, NULL);

    return pta;
}

/*!
 *  ptaDestroy()
 *
 *      Input:  &pta (<to be nulled>)
 *      Return: void
 *
 *  Note:
 *      - Decrements the ref count and, if 0, destroys the pta.
 *      - Always nulls the input ptr.
 */
LEPTONICA_REAL_EXPORT void
ptaDestroy(PTA  **ppta)
{
PTA  *pta;

    PROCNAME("ptaDestroy");

    if (ppta == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pta = *ppta) == NULL)
        return;

    ptaChangeRefcount(pta, -1);
    if (ptaGetRefcount(pta) <= 0) {
        FREE(pta->x);
        FREE(pta->y);
        FREE(pta);
    }

    *ppta = NULL;
    return;
}


/*!
 *  ptaCopy()
 *
 *      Input:  pta
 *      Return: copy of pta, or null on error
 */
LEPTONICA_EXPORT PTA *
ptaCopy(PTA  *pta)
{
l_int32    i;
l_float32  x, y;
PTA       *npta;

    PROCNAME("ptaCopy");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    if ((npta = ptaCreate(pta->nalloc)) == NULL)
        return (PTA *)ERROR_PTR("npta not made", procName, NULL);

    for (i = 0; i < pta->n; i++) {
        ptaGetPt(pta, i, &x, &y);
        ptaAddPt(npta, x, y);
    }

    return npta;
}


/*!
 *  ptaClone()
 *
 *      Input:  pta
 *      Return: ptr to same pta, or null on error
 */
LEPTONICA_EXPORT PTA *
ptaClone(PTA  *pta)
{
    PROCNAME("ptaClone");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    ptaChangeRefcount(pta, 1);
    return pta;
}


/*---------------------------------------------------------------------*
 *                         Pta array extension                         *
 *---------------------------------------------------------------------*/
/*!
 *  ptaAddPt()
 *
 *      Input:  pta
 *              x, y
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_REAL_EXPORT l_int32
ptaAddPt(PTA       *pta,
         l_float32  x,
         l_float32  y)
{
l_int32  n;

    PROCNAME("ptaAddPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = pta->n;
    if (n >= pta->nalloc)
        ptaExtendArrays(pta);
    pta->x[n] = x;
    pta->y[n] = y;
    pta->n++;

    return 0;
}


/*!
 *  ptaExtendArrays()
 *
 *      Input:  pta
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
ptaExtendArrays(PTA  *pta)
{
    PROCNAME("ptaExtendArrays");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((pta->x = (l_float32 *)reallocNew((void **)&pta->x,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new x array not returned", procName, 1);
    if ((pta->y = (l_float32 *)reallocNew((void **)&pta->y,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new y array not returned", procName, 1);

    pta->nalloc = 2 * pta->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Pta accessors                             *
 *---------------------------------------------------------------------*/
LEPTONICA_EXPORT l_int32
ptaGetRefcount(PTA  *pta)
{
    PROCNAME("ptaGetRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    return pta->refcount;
}


LEPTONICA_EXPORT l_int32
ptaChangeRefcount(PTA     *pta,
                  l_int32  delta)
{
    PROCNAME("ptaChangeRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    pta->refcount += delta;
    return 0;
}


/*!
 *  ptaGetCount()
 *
 *      Input:  pta
 *      Return: count, or 0 if no pta
 */
LEPTONICA_EXPORT l_int32
ptaGetCount(PTA  *pta)
{
    PROCNAME("ptaGetCount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 0);

    return pta->n;
}


/*!
 *  ptaGetPt()
 *
 *      Input:  pta
 *              index  (into arrays)
 *              &x (<optional return> float x value)
 *              &y (<optional return> float y value)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
ptaGetPt(PTA        *pta,
         l_int32     index,
         l_float32  *px,
         l_float32  *py)
{
    PROCNAME("ptaGetPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = pta->x[index];
    if (py) *py = pta->y[index];
    return 0;
}


/*!
 *  ptaGetIPt()
 *
 *      Input:  pta
 *              index  (into arrays)
 *              &x (<optional return> integer x value)
 *              &y (<optional return> integer y value)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
ptaGetIPt(PTA      *pta,
          l_int32   index,
          l_int32  *px,
          l_int32  *py)
{
    PROCNAME("ptaGetIPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = (l_int32)(pta->x[index] + 0.5);
    if (py) *py = (l_int32)(pta->y[index] + 0.5);
    return 0;
}

/*---------------------------------------------------------------------*
 *                          PTAA array extension                       *
 *---------------------------------------------------------------------*/

/*!
 *  ptaaExtendArray()
 *
 *      Input:  ptaa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
ptaaExtendArray(PTAA  *ptaa)
{
    PROCNAME("ptaaExtendArray");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    if ((ptaa->pta = (PTA **)reallocNew((void **)&ptaa->pta,
                             sizeof(PTA *) * ptaa->nalloc,
                             2 * sizeof(PTA *) * ptaa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    ptaa->nalloc = 2 * ptaa->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                          Ptaa accessors                             *
 *---------------------------------------------------------------------*/
/*!
 *  ptaaGetCount()
 *
 *      Input:  ptaa
 *      Return: count, or 0 if no ptaa
 */
LEPTONICA_EXPORT l_int32
ptaaGetCount(PTAA  *ptaa)
{
    PROCNAME("ptaaGetCount");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 0);

    return ptaa->n;
}
