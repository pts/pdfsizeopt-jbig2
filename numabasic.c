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
 *   numabasic.c
 *
 *      Numa creation, destruction, copy, clone, etc.
 *          NUMA        *numaCreate()
 *          NUMA        *numaCreateFromIArray()
 *          NUMA        *numaCreateFromFArray()
 *          void        *numaDestroy()
 *          NUMA        *numaCopy()
 *          NUMA        *numaClone()
 *          l_int32      numaEmpty()
 *
 *      Add/remove number (float or integer)
 *          l_int32      numaAddNumber()
 *          l_int32      numaExtendArray()
 *          l_int32      numaInsertNumber()
 *          l_int32      numaRemoveNumber()
 *          l_int32      numaReplaceNumber()
 *
 *      Numa accessors
 *          l_int32      numaGetCount()
 *          l_int32      numaSetCount()
 *          l_int32      numaGetIValue()
 *          l_int32      numaGetFValue()
 *          l_int32      numaSetValue()
 *          l_int32      numaShiftValue()
 *          l_int32     *numaGetIArray()
 *          l_float32   *numaGetFArray()
 *          l_int32      numaGetRefcount()
 *          l_int32      numaChangeRefcount()
 *          l_int32      numaGetXParameters()
 *          l_int32      numaSetXParameters()
 *          l_int32      numaCopyXParameters()
 *
 *      Serialize numa for I/O
 *          l_int32      numaRead()
 *          l_int32      numaReadStream()
 *          l_int32      numaWrite()
 *          l_int32      numaWriteStream()
 *
 *      Numaa creation, destruction
 *          NUMAA       *numaaCreate()
 *          void        *numaaDestroy()
 *
 *      Add Numa to Numaa
 *          l_int32      numaaAddNuma()
 *          l_int32      numaaExtendArray()
 *
 *      Numaa accessors
 *          l_int32      numaaGetCount()
 *          l_int32      numaaGetNumaCount()
 *          l_int32      numaaGetNumberCount()
 *          NUMA       **numaaGetPtrArray()
 *          NUMA        *numaaGetNuma()
 *          NUMA        *numaaReplaceNuma()
 *          l_int32      numaaGetValue()
 *          l_int32      numaaAddNumber()
 *
 *      Serialize numaa for I/O
 *          l_int32      numaaRead()
 *          l_int32      numaaReadStream()
 *          l_int32      numaaWrite()
 *          l_int32      numaaWriteStream()
 *
 *      Numa2d creation, destruction
 *          NUMA2D      *numa2dCreate()
 *          void        *numa2dDestroy()
 *
 *      Numa2d Accessors
 *          l_int32      numa2dAddNumber()
 *          l_int32      numa2dGetCount()
 *          NUMA        *numa2dGetNuma()
 *          l_int32      numa2dGetFValue()
 *          l_int32      numa2dGetIValue()
 *
 *      NumaHash creation, destruction
 *          NUMAHASH    *numaHashCreate()
 *          void        *numaHashDestroy()
 *
 *      NumaHash Accessors
 *          NUMA        *numaHashGetNuma()
 *          void        *numaHashAdd()
 *
 *    (1) The numa is a struct, not an array.  Always use the accessors
 *        in this file, never the fields directly.
 *
 *    (2) The number array holds l_float32 values.  It can also
 *        be used to store l_int32 values.
 *
 *    (3) Storing and retrieving numbers:
 *
 *       * to append a new number to the array, use numaAddNumber().  If
 *         the number is an int, it will will automatically be converted
 *         to l_float32 and stored.
 *
 *       * to reset a value stored in the array, use numaSetValue().
 *
 *       * to increment or decrement a value stored in the array,
 *         use numaShiftValue().
 *
 *       * to obtain a value from the array, use either numaGetIValue()
 *         or numaGetFValue(), depending on whether you are retrieving
 *         an integer or a float.  This avoids doing an explicit cast,
 *         such as
 *           (a) return a l_float32 and cast it to an l_int32
 *           (b) cast the return directly to (l_float32 *) to
 *               satisfy the function prototype, as in
 *                 numaGetFValue(na, index, (l_float32 *)&ival);   [ugly!]
 *
 *    (4) int <--> float conversions:
 *
 *        Tradition dictates that type conversions go automatically from
 *        l_int32 --> l_float32, even though it is possible to lose
 *        precision for large integers, whereas you must cast (l_int32)
 *        to go from l_float32 --> l_int32 because you're truncating
 *        to the integer value.
 *
 *    (5) As with other arrays in leptonica, the numa has both an allocated
 *        size and a count of the stored numbers.  When you add a number, it
 *        goes on the end of the array, and causes a realloc if the array
 *        is already filled.  However, in situations where you want to
 *        add numbers randomly into an array, such as when you build a
 *        histogram, you must set the count of stored numbers in advance.
 *        This is done with numaSetCount().  If you set a count larger
 *        than the allocated array, it does a realloc to the size requested.
 *
 *    (6) In situations where the data in a numa correspond to a function
 *        y(x), the values can be either at equal spacings in x or at
 *        arbitrary spacings.  For the former, we can represent all x values
 *        by two parameters: startx (corresponding to y[0]) and delx
 *        for the change in x for adjacent values y[i] and y[i+1].
 *        startx and delx are initialized to 0.0 and 1.0, rsp.
 *        For arbitrary spacings, we use a second numa, and the two
 *        numas are typically denoted nay and nax.
 *
 *    (7) The numa is also the basic struct used for histograms.  Every numa
 *        has startx and delx fields, initialized to 0.0 and 1.0, that can
 *        be used to represent the "x" value for the location of the
 *        first bin and the bin width, respectively.  Accessors are the
 *        numa*XParameters() functions.  All functions that make numa
 *        histograms must set these fields properly, and many functions
 *        that use numa histograms rely on the correctness of these values.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

static const l_int32 INITIAL_PTR_ARRAYSIZE1 = 50;      /* n'importe quoi */


/*--------------------------------------------------------------------------*
 *               Numa creation, destruction, copy, clone, etc.              *
 *--------------------------------------------------------------------------*/
/*!
 *  numaCreate()
 *
 *      Input:  size of number array to be alloc'd (0 for default)
 *      Return: na, or null on error
 */
LEPTONICA_EXPORT NUMA *
numaCreate(l_int32  n)
{
NUMA  *na;

    PROCNAME("numaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE1;

    if ((na = (NUMA *)CALLOC(1, sizeof(NUMA))) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    if ((na->array = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (NUMA *)ERROR_PTR("number array not made", procName, NULL);

    na->nalloc = n;
    na->n = 0;
    na->refcount = 1;
    na->startx = 0.0;
    na->delx = 1.0;

    return na;
}

/*!
 *  numaDestroy()
 *
 *      Input:  &na (<to be nulled if it exists>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the numa.
 *      (2) Always nulls the input ptr.
 */
LEPTONICA_EXPORT void
numaDestroy(NUMA  **pna)
{
NUMA  *na;

    PROCNAME("numaDestroy");

    if (pna == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }

    if ((na = *pna) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the numa. */
    numaChangeRefcount(na, -1);
    if (numaGetRefcount(na) <= 0) {
        if (na->array)
            FREE(na->array);
        FREE(na);
    }

    *pna = NULL;
    return;
}


/*!
 *  numaCopy()
 *
 *      Input:  na
 *      Return: copy of numa, or null on error
 */
LEPTONICA_EXPORT NUMA *
numaCopy(NUMA  *na)
{
l_int32  i;
NUMA    *cna;

    PROCNAME("numaCopy");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);

    if ((cna = numaCreate(na->nalloc)) == NULL)
        return (NUMA *)ERROR_PTR("cna not made", procName, NULL);
    cna->startx = na->startx;
    cna->delx = na->delx;

    for (i = 0; i < na->n; i++)
        numaAddNumber(cna, na->array[i]);

    return cna;
}


/*!
 *  numaClone()
 *
 *      Input:  na
 *      Return: ptr to same numa, or null on error
 */
LEPTONICA_EXPORT NUMA *
numaClone(NUMA  *na)
{
    PROCNAME("numaClone");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);

    numaChangeRefcount(na, 1);
    return na;
}

/*--------------------------------------------------------------------------*
 *                 Number array: add number and extend array                *
 *--------------------------------------------------------------------------*/
/*!
 *  numaAddNumber()
 *
 *      Input:  na
 *              val  (float or int to be added; stored as a float)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaAddNumber(NUMA      *na,
              l_float32  val)
{
l_int32  n;

    PROCNAME("numaAddNumber");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    n = numaGetCount(na);
    if (n >= na->nalloc)
        numaExtendArray(na);
    na->array[n] = val;
    na->n++;
    return 0;
}


/*!
 *  numaExtendArray()
 *
 *      Input:  na
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaExtendArray(NUMA  *na)
{
    PROCNAME("numaExtendArray");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if ((na->array = (l_float32 *)reallocNew((void **)&na->array,
                                sizeof(l_float32) * na->nalloc,
                                2 * sizeof(l_float32) * na->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    na->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                            Numa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  numaGetCount()
 *
 *      Input:  na
 *      Return: count, or 0 if no numbers or on error
 */
LEPTONICA_EXPORT l_int32
numaGetCount(NUMA  *na)
{
    PROCNAME("numaGetCount");

    if (!na)
        return ERROR_INT("na not defined", procName, 0);
    return na->n;
}


/*!
 *  numaSetCount()
 *
 *      Input:  na
 *              newcount
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If newcount <= na->nalloc, this resets na->n.
 *          Using newcount = 0 is equivalent to numaEmpty().
 *      (2) If newcount > na->nalloc, this causes a realloc
 *          to a size na->nalloc = newcount.
 *      (3) All the previously unused values in na are set to 0.0.
 */
LEPTONICA_EXPORT l_int32
numaSetCount(NUMA    *na,
             l_int32  newcount)
{
    PROCNAME("numaSetCount");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (newcount > na->nalloc) {
        if ((na->array = (l_float32 *)reallocNew((void **)&na->array,
                         sizeof(l_float32) * na->nalloc,
                         sizeof(l_float32) * newcount)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        na->nalloc = newcount;
    }
    na->n = newcount;
    return 0;
}


/*!
 *  numaGetFValue()
 *
 *      Input:  na
 *              index (into numa)
 *              &val  (<return> float value; 0.0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0.0 in the returned ival is valid.
 */
LEPTONICA_EXPORT l_int32
numaGetFValue(NUMA       *na,
              l_int32     index,
              l_float32  *pval)
{
    PROCNAME("numaGetFValue");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    *pval = na->array[index];
    return 0;
}


/*!
 *  numaGetIValue()
 *
 *      Input:  na
 *              index (into numa)
 *              &ival  (<return> integer value; 0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0 in the returned ival is valid.
 */
LEPTONICA_REAL_EXPORT l_int32
numaGetIValue(NUMA     *na,
              l_int32   index,
              l_int32  *pival)
{
l_float32  val;

    PROCNAME("numaGetIValue");

    if (!pival)
        return ERROR_INT("&ival not defined", procName, 1);
    *pival = 0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    val = na->array[index];
    *pival = (l_int32)(val + L_SIGN(val) * 0.5);
    return 0;
}

/*!
 *  numaGetFArray()
 *
 *      Input:  na
 *              copyflag (L_NOCOPY or L_COPY)
 *      Return: either the bare internal array or a copy of it,
 *              or null on error
 *
 *  Notes:
 *      (1) If copyflag == L_COPY, it makes a copy which the caller
 *          is responsible for freeing.  Otherwise, it operates
 *          directly on the bare array of the numa.
 *      (2) Very important: for L_NOCOPY, any writes to the array
 *          will be in the numa.  Do not write beyond the size of
 *          the count field, because it will not be accessable
 *          from the numa!  If necessary, be sure to set the count
 *          field to a larger number (such as the alloc size)
 *          BEFORE calling this function.  Creating with numaMakeConstant()
 *          is another way to insure full initialization.
 */
LEPTONICA_EXPORT l_float32 *
numaGetFArray(NUMA    *na,
              l_int32  copyflag)
{
l_int32     i, n;
l_float32  *array;

    PROCNAME("numaGetFArray");

    if (!na)
        return (l_float32 *)ERROR_PTR("na not defined", procName, NULL);

    if (copyflag == L_NOCOPY)
        array = na->array;
    else {  /* copyflag == L_COPY */
        n = numaGetCount(na);
        if ((array = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
            return (l_float32 *)ERROR_PTR("array not made", procName, NULL);
        for (i = 0; i < n; i++)
            array[i] = na->array[i];
    }

    return array;
}


/*!
 *  numaGetRefCount()
 *
 *      Input:  na
 *      Return: refcount, or UNDEF on error
 */
LEPTONICA_EXPORT l_int32
numaGetRefcount(NUMA  *na)
{
    PROCNAME("numaGetRefcount");

    if (!na)
        return ERROR_INT("na not defined", procName, UNDEF);
    return na->refcount;
}


/*!
 *  numaChangeRefCount()
 *
 *      Input:  na
 *              delta (change to be applied)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaChangeRefcount(NUMA    *na,
                   l_int32  delta)
{
    PROCNAME("numaChangeRefcount");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    na->refcount += delta;
    return 0;
}


/*!
 *  numaGetXParameters()
 *
 *      Input:  na
 *              &startx (<optional return> startx)
 *              &delx (<optional return> delx)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaGetXParameters(NUMA       *na,
                   l_float32  *pstartx,
                   l_float32  *pdelx)
{
    PROCNAME("numaGetXParameters");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (pstartx) *pstartx = na->startx;
    if (pdelx) *pdelx = na->delx;
    return 0;
}


/*!
 *  numaSetXParameters()
 *
 *      Input:  na
 *              startx (x value corresponding to na[0])
 *              delx (difference in x values for the situation where the
 *                    elements of na correspond to the evaulation of a
 *                    function at equal intervals of size @delx)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaSetXParameters(NUMA      *na,
                   l_float32  startx,
                   l_float32  delx)
{
    PROCNAME("numaSetXParameters");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    na->startx = startx;
    na->delx = delx;
    return 0;
}

/*!
 *  numaReadStream()
 *
 *      Input:  stream
 *      Return: numa, or null on error
 */
LEPTONICA_EXPORT NUMA *
numaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
l_float32  val, startx, delx;
NUMA      *na;

    PROCNAME("numaReadStream");

    if (!fp)
        return (NUMA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nNuma Version %d\n", &version);
    if (ret != 1)
        return (NUMA *)ERROR_PTR("not a numa file", procName, NULL);
    if (version != NUMA_VERSION_NUMBER)
        return (NUMA *)ERROR_PTR("invalid numa version", procName, NULL);
    if (fscanf(fp, "Number of numbers = %d\n", &n) != 1)
        return (NUMA *)ERROR_PTR("invalid number of numbers", procName, NULL);

    if ((na = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "  [%d] = %f\n", &index, &val) != 2)
            return (NUMA *)ERROR_PTR("bad input data", procName, NULL);
        numaAddNumber(na, val);
    }

        /* Optional data */
    if (fscanf(fp, "startx = %f, delx = %f\n", &startx, &delx) == 2)
        numaSetXParameters(na, startx, delx);

    return na;
}

/*!
 *  numaWriteStream()
 *
 *      Input:  stream, na
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaWriteStream(FILE  *fp,
                NUMA  *na)
{
l_int32    i, n;
l_float32  startx, delx;

    PROCNAME("numaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    n = numaGetCount(na);
    fprintf(fp, "\nNuma Version %d\n", NUMA_VERSION_NUMBER);
    fprintf(fp, "Number of numbers = %d\n", n);
    for (i = 0; i < n; i++)
        fprintf(fp, "  [%d] = %f\n", i, na->array[i]);
    fprintf(fp, "\n");

        /* Optional data */
    numaGetXParameters(na, &startx, &delx);
    if (startx != 0.0 || delx != 1.0)
        fprintf(fp, "startx = %f, delx = %f\n", startx, delx);

    return 0;
}



/*--------------------------------------------------------------------------*
 *                     Numaa creation, destruction                          *
 *--------------------------------------------------------------------------*/
/*!
 *  numaaCreate()
 *
 *      Input:  size of numa ptr array to be alloc'd (0 for default)
 *      Return: naa, or null on error
 *
 */
LEPTONICA_EXPORT NUMAA *
numaaCreate(l_int32  n)
{
NUMAA  *naa;

    PROCNAME("numaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE1;

    if ((naa = (NUMAA *)CALLOC(1, sizeof(NUMAA))) == NULL)
        return (NUMAA *)ERROR_PTR("naa not made", procName, NULL);
    if ((naa->numa = (NUMA **)CALLOC(n, sizeof(NUMA *))) == NULL)
        return (NUMAA *)ERROR_PTR("numa ptr array not made", procName, NULL);

    naa->nalloc = n;
    naa->n = 0;

    return naa;
}

/*--------------------------------------------------------------------------*
 *                              Add Numa to Numaa                           *
 *--------------------------------------------------------------------------*/
/*!
 *  numaaAddNuma()
 *
 *      Input:  naa
 *              na   (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaaAddNuma(NUMAA   *naa,
             NUMA    *na,
             l_int32  copyflag)
{
l_int32  n;
NUMA    *nac;

    PROCNAME("numaaAddNuma");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (copyflag == L_INSERT)
        nac = na;
    else if (copyflag == L_COPY) {
        if ((nac = numaCopy(na)) == NULL)
            return ERROR_INT("nac not made", procName, 1);
    }
    else if (copyflag == L_CLONE)
        nac = numaClone(na);
    else
        return ERROR_INT("invalid copyflag", procName, 1);

    n = numaaGetCount(naa);
    if (n >= naa->nalloc)
        numaaExtendArray(naa);
    naa->numa[n] = nac;
    naa->n++;
    return 0;
}


/*!
 *  numaaExtendArray()
 *
 *      Input:  naa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaaExtendArray(NUMAA  *naa)
{
    PROCNAME("numaaExtendArray");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);

    if ((naa->numa = (NUMA **)reallocNew((void **)&naa->numa,
                              sizeof(NUMA *) * naa->nalloc,
                              2 * sizeof(NUMA *) * naa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    naa->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                           Numaa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  numaaGetCount()
 *
 *      Input:  naa
 *      Return: count (number of numa), or 0 if no numa or on error
 */
LEPTONICA_EXPORT l_int32
numaaGetCount(NUMAA  *naa)
{
    PROCNAME("numaaGetCount");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 0);
    return naa->n;
}

/*!
 *  numaaGetNumberCount()
 *
 *      Input:  naa
 *      Return: count (total number of numbers in the numaa),
 *                     or 0 if no numbers or on error
 */
LEPTONICA_EXPORT l_int32
numaaGetNumberCount(NUMAA  *naa)
{
NUMA    *na;
l_int32  n, sum, i;

    PROCNAME("numaaGetNumberCount");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 0);

    n = numaaGetCount(naa);
    for (sum = 0, i = 0; i < n; i++) {
        na = numaaGetNuma(naa, i, L_CLONE);
        sum += numaGetCount(na);
        numaDestroy(&na);
    }

    return sum;
}

/*!
 *  numaaGetNuma()
 *
 *      Input:  naa
 *              index  (to the index-th numa)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: numa, or null on error
 */
LEPTONICA_EXPORT NUMA *
numaaGetNuma(NUMAA   *naa,
             l_int32  index,
             l_int32  accessflag)
{
    PROCNAME("numaaGetNuma");

    if (!naa)
        return (NUMA *)ERROR_PTR("naa not defined", procName, NULL);
    if (index < 0 || index >= naa->n)
        return (NUMA *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return numaCopy(naa->numa[index]);
    else if (accessflag == L_CLONE)
        return numaClone(naa->numa[index]);
    else
        return (NUMA *)ERROR_PTR("invalid accessflag", procName, NULL);
}

/*!
 *  numaaReadStream()
 *
 *      Input:  stream
 *      Return: naa, or null on error
 */
LEPTONICA_EXPORT NUMAA *
numaaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
NUMA      *na;
NUMAA     *naa;

    PROCNAME("numaaReadStream");

    if (!fp)
        return (NUMAA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nNumaa Version %d\n", &version);
    if (ret != 1)
        return (NUMAA *)ERROR_PTR("not a numa file", procName, NULL);
    if (version != NUMA_VERSION_NUMBER)
        return (NUMAA *)ERROR_PTR("invalid numaa version", procName, NULL);
    if (fscanf(fp, "Number of numa = %d\n\n", &n) != 1)
        return (NUMAA *)ERROR_PTR("invalid number of numa", procName, NULL);
    if ((naa = numaaCreate(n)) == NULL)
        return (NUMAA *)ERROR_PTR("naa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "Numa[%d]:", &index) != 1)
            return (NUMAA *)ERROR_PTR("invalid numa header", procName, NULL);
        if ((na = numaReadStream(fp)) == NULL)
            return (NUMAA *)ERROR_PTR("na not made", procName, NULL);
        numaaAddNuma(naa, na, L_INSERT);
    }

    return naa;
}


/*!
 *  numaaWriteStream()
 *
 *      Input:  stream, naa
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
numaaWriteStream(FILE   *fp,
                 NUMAA  *naa)
{
l_int32  i, n;
NUMA    *na;

    PROCNAME("numaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);

    n = numaaGetCount(naa);
    fprintf(fp, "\nNumaa Version %d\n", NUMA_VERSION_NUMBER);
    fprintf(fp, "Number of numa = %d\n\n", n);
    for (i = 0; i < n; i++) {
        if ((na = numaaGetNuma(naa, i, L_CLONE)) == NULL)
            return ERROR_INT("na not found", procName, 1);
        fprintf(fp, "Numa[%d]:", i);
        numaWriteStream(fp, na);
        numaDestroy(&na);
    }

    return 0;
}

/*--------------------------------------------------------------------------*
 *               Number array hash: Creation and destruction                *
 *--------------------------------------------------------------------------*/
/*!
 *  numaHashCreate()
 *
 *      Input: nbuckets (the number of buckets in the hash table,
 *                       which should be prime.)
 *             initsize (initial size of each allocated numa; 0 for default)
 *      Return: ptr to new nahash, or null on error
 *
 *  Note: actual numa are created only as required by numaHashAdd()
 */
LEPTONICA_EXPORT NUMAHASH *
numaHashCreate(l_int32  nbuckets,
               l_int32  initsize)
{
NUMAHASH  *nahash;

    PROCNAME("numaHashCreate");

    if (nbuckets <= 0)
        return (NUMAHASH *)ERROR_PTR("negative hash size", procName, NULL);
    if ((nahash = (NUMAHASH *)CALLOC(1, sizeof(NUMAHASH))) == NULL)
        return (NUMAHASH *)ERROR_PTR("nahash not made", procName, NULL);
    if ((nahash->numa = (NUMA **)CALLOC(nbuckets, sizeof(NUMA *))) == NULL) {
        FREE(nahash);
        return (NUMAHASH *)ERROR_PTR("numa ptr array not made", procName, NULL);
    }

    nahash->nbuckets = nbuckets;
    nahash->initsize = initsize;
    return nahash;
}


/*!
 *  numaHashDestroy()
 *
 *      Input:  &nahash (<to be nulled, if it exists>)
 *      Return: void
 */
LEPTONICA_EXPORT void
numaHashDestroy(NUMAHASH **pnahash)
{
NUMAHASH  *nahash;
l_int32    i;

    PROCNAME("numaHashDestroy");

    if (pnahash == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((nahash = *pnahash) == NULL)
        return;

    for (i = 0; i < nahash->nbuckets; i++)
        numaDestroy(&nahash->numa[i]);
    FREE(nahash->numa);
    FREE(nahash);
    *pnahash = NULL;
}


/*--------------------------------------------------------------------------*
 *               Number array hash: Add elements and return numas
 *--------------------------------------------------------------------------*/
/*!
 *  numaHashGetNuma()
 *
 *      Input:  nahash
 *              key  (key to be hashed into a bucket number)
 *      Return: ptr to numa
 */
LEPTONICA_EXPORT NUMA *
numaHashGetNuma(NUMAHASH  *nahash,
                l_uint32   key)
{
l_int32  bucket;
NUMA    *na;

    PROCNAME("numaHashGetNuma");

    if (!nahash)
        return (NUMA *)ERROR_PTR("nahash not defined", procName, NULL);
    bucket = key % nahash->nbuckets;
    na = nahash->numa[bucket];
    if (na)
        return numaClone(na);
    else
        return NULL;
}

/*!
 *  numaHashAdd()
 *
 *      Input:  nahash
 *              key  (key to be hashed into a bucket number)
 *              value  (float value to be appended to the specific numa)
 *      Return: 0 if OK; 1 on error
 */
LEPTONICA_EXPORT l_int32
numaHashAdd(NUMAHASH  *nahash,
            l_uint32   key,
            l_float32  value)
{
l_int32  bucket;
NUMA    *na;

    PROCNAME("numaHashAdd");

    if (!nahash)
        return ERROR_INT("nahash not defined", procName, 1);
    bucket = key % nahash->nbuckets;
    na = nahash->numa[bucket];
    if (!na) {
        if ((na = numaCreate(nahash->initsize)) == NULL)
            return ERROR_INT("na not made", procName, 1);
        nahash->numa[bucket] = na;
    }
    numaAddNumber(na, value);
    return 0;
}

