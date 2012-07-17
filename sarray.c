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
 *   sarray.c
 *
 *      Create/Destroy/Copy
 *          SARRAY    *sarrayCreate()
 *          SARRAY    *sarrayCreateInitialized()
 *          SARRAY    *sarrayCreateWordsFromString()
 *          SARRAY    *sarrayCreateLinesFromString()
 *          void      *sarrayDestroy()
 *          SARRAY    *sarrayCopy()
 *          SARRAY    *sarrayClone()
 *
 *      Add/Remove string
 *          l_int32    sarrayAddString()
 *          l_int32    sarrayExtendArray()
 *          char      *sarrayRemoveString()
 *          l_int32    sarrayReplaceString()
 *          l_int32    sarrayClear()
 *
 *      Accessors
 *          l_int32    sarrayGetCount()
 *          char     **sarrayGetArray()
 *          char      *sarrayGetString()
 *          l_int32    sarrayGetRefcount()
 *          l_int32    sarrayChangeRefcount()
 *
 *      Conversion back to string
 *          char      *sarrayToString()
 *          char      *sarrayToStringRange()
 *
 *      Concatenate 2 sarrays
 *          l_int32    sarrayConcatenate()
 *          l_int32    sarrayAppendRange()
 *
 *      Pad an sarray to be the same size as another sarray
 *          l_int32    sarrayPadToSameSize()
 *
 *      Convert word sarray to (formatted) line sarray
 *          SARRAY    *sarrayConvertWordsToLines()
 *
 *      Split string on separator list
 *          SARRAY    *sarraySplitString()
 *
 *      Filter sarray
 *          SARRAY    *sarraySelectBySubstring()
 *          SARRAY    *sarraySelectByRange()
 *          l_int32    sarrayParseRange()
 *
 *      Sort
 *          SARRAY    *sarraySort()
 *          l_int32    stringCompareLexical()
 *
 *      Serialize for I/O
 *          SARRAY    *sarrayRead()
 *          SARRAY    *sarrayReadStream()
 *          l_int32    sarrayWrite()
 *          l_int32    sarrayWriteStream()
 *          l_int32    sarrayAppend()
 *
 *      Directory filenames
 *          SARRAY    *getNumberedPathnamesInDirectory()
 *          SARRAY    *getSortedPathnamesInDirectory()
 *          SARRAY    *getFilenamesInDirectory()
 *
 *      Comments on usage:
 *
 *          These functions are important for efficient manipulation
 *          of string data.  They have been used in leptonica for
 *          generating and parsing text files, and for generating
 *          code for compilation.  The user is responsible for
 *          correctly disposing of strings that have been extracted
 *          from sarrays.
 *
 *            - When you want a string from an Sarray to inspect it, or
 *              plan to make a copy of it later, use sarrayGetString()
 *              with copyflag = 0.  In this case, you must neither free
 *              the string nor put it directly in another array.
 *              We provide the copyflag constant L_NOCOPY, which is 0,
 *              for this purpose:
 *                 str-not-owned = sarrayGetString(sa, index, L_NOCOPY);
 *              To extract a copy of a string, use:
 *                 str-owned = sarrayGetString(sa, index, L_COPY);
 *
 *            - When you want to insert a string that is in one
 *              array into another array (always leaving the first
 *              array intact), you have two options:
 *                 (1) use copyflag = L_COPY to make an immediate copy,
 *                     which you must then add to the second array
 *                     by insertion; namely,
 *                       str-owned = sarrayGetString(sa, index, L_COPY);
 *                       sarrayAddString(sa, str-owned, L_INSERT);
 *                 (2) use copyflag = L_NOCOPY to get another handle to
 *                     the string, in which case you must add
 *                     a copy of it to the second string array:
 *                       str-not-owned = sarrayGetString(sa, index, L_NOCOPY);
 *                       sarrayAddString(sa, str-not-owned, L_COPY).
 *
 *              In all cases, when you use copyflag = L_COPY to extract
 *              a string from an array, you must either free it
 *              or insert it in an array that will be freed later.
 */

#include <string.h>
#ifndef _WIN32
#include <dirent.h>     /* unix only */
#endif  /* ! _WIN32 */
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE4 = 50;     /* n'importe quoi */
static const l_int32  L_BUF_SIZE4 = 512;


/*--------------------------------------------------------------------------*
 *                   String array create/destroy/copy/extend                *
 *--------------------------------------------------------------------------*/
/*!
 *  sarrayCreate()
 *
 *      Input:  size of string ptr array to be alloc'd
 *              (use 0 for default)
 *      Return: sarray, or null on error
 */
LEPTONICA_EXPORT SARRAY *
sarrayCreate(l_int32  n)
{
SARRAY  *sa;

    PROCNAME("sarrayCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE4;

    if ((sa = (SARRAY *)CALLOC(1, sizeof(SARRAY))) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    if ((sa->array = (char **)CALLOC(n, sizeof(char *))) == NULL)
        return (SARRAY *)ERROR_PTR("ptr array not made", procName, NULL);

    sa->nalloc = n;
    sa->n = 0;
    sa->refcount = 1;
    return sa;
}


/*!
 *  sarrayDestroy()
 *
 *      Input:  &sarray <to be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the sarray.
 *      (2) Always nulls the input ptr.
 */
LEPTONICA_EXPORT void
sarrayDestroy(SARRAY  **psa)
{
l_int32  i;
SARRAY  *sa;

    PROCNAME("sarrayDestroy");

    if (psa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }
    if ((sa = *psa) == NULL)
        return;

    sa->refcount -= 1;
    if (sa->refcount <= 0) {
        if (sa->array) {
            for (i = 0; i < sa->n; i++) {
                if (sa->array[i])
                    FREE(sa->array[i]);
            }
            FREE(sa->array);
        }
        FREE(sa);
    }

    *psa = NULL;
    return;
}

        /*!
 *  sarrayAddString()
 *
 *      Input:  sarray
 *              string  (string to be added)
 *              copyflag (L_INSERT, L_COPY)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Legacy usage decrees that we always use 0 to insert a string
 *          directly and 1 to insert a copy of the string.  The
 *          enums for L_INSERT and L_COPY agree with this convention,
 *          and will not change in the future.
 *      (2) See usage comments at the top of this file.
 */
LEPTONICA_EXPORT l_int32
sarrayAddString(SARRAY  *sa,
                char    *string,
                l_int32  copyflag)
{
l_int32  n;

    PROCNAME("sarrayAddString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!string)
        return ERROR_INT("string not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return ERROR_INT("invalid copyflag", procName, 1);
    
    n = sarrayGetCount(sa);
    if (n >= sa->nalloc)
        sarrayExtendArray(sa);

    if (copyflag == L_INSERT)
        sa->array[n] = string;
    else  /* L_COPY */
        sa->array[n] = stringNew(string);
    sa->n++;

    return 0;
}


/*!
 *  sarrayExtendArray()
 *
 *      Input:  sarray
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
sarrayExtendArray(SARRAY  *sa)
{
    PROCNAME("sarrayExtendArray");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    if ((sa->array = (char **)reallocNew((void **)&sa->array,
                              sizeof(char *) * sa->nalloc,
                              2 * sizeof(char *) * sa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    sa->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                               Accessors                              *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayGetCount()
 *
 *      Input:  sarray
 *      Return: count, or 0 if no strings or on error
 */
LEPTONICA_EXPORT l_int32
sarrayGetCount(SARRAY  *sa)
{
    PROCNAME("sarrayGetCount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 0);
    return sa->n;
}
        

/*!
 *  sarrayGetString()
 *
 *      Input:  sarray
 *              index   (to the index-th string)
 *              copyflag  (L_NOCOPY or L_COPY)
 *      Return: string, or null on error
 *
 *  Notes:
 *      (1) Legacy usage decrees that we always use 0 to get the
 *          pointer to the string itself, and 1 to get a copy of
 *          the string.
 *      (2) See usage comments at the top of this file.
 *      (3) To get a pointer to the string itself, use for copyflag:
 *             L_NOCOPY or 0 or FALSE
 *          To get a copy of the string, use for copyflag:
 *             L_COPY or 1 or TRUE
 *          The const values of L_NOCOPY and L_COPY are guaranteed not
 *          to change.
 */
LEPTONICA_EXPORT char *
sarrayGetString(SARRAY  *sa,
                l_int32  index,
                l_int32  copyflag)
{
    PROCNAME("sarrayGetString");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);
    if (index < 0 || index >= sa->n)
        return (char *)ERROR_PTR("index not valid", procName, NULL);
    if (copyflag != L_NOCOPY && copyflag != L_COPY)
        return (char *)ERROR_PTR("invalid copyflag", procName, NULL);

    if (copyflag == L_NOCOPY)
        return sa->array[index];
    else  /* L_COPY */
        return stringNew(sa->array[index]);
}


/*----------------------------------------------------------------------*
 *                    Split string on separator list                    *
 *----------------------------------------------------------------------*/
/*
 *  sarraySplitString()
 *
 *      Input:  sa (to append to; typically empty initially)
 *              str (string to split; not changed)
 *              separators (characters that split input string)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) This uses strtokSafe().  See the notes there in utils.c.
 */
LEPTONICA_EXPORT l_int32
sarraySplitString(SARRAY      *sa,
                  const char  *str,
                  const char  *separators)
{
char  *cstr, *substr, *saveptr;

    PROCNAME("sarraySplitString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!str)
        return ERROR_INT("str not defined", procName, 1);
    if (!separators)
        return ERROR_INT("separators not defined", procName, 1);

    cstr = stringNew(str);  /* preserves const-ness of input str */
    substr = strtokSafe(cstr, separators, &saveptr);
    if (substr)
        sarrayAddString(sa, substr, L_INSERT);
    while ((substr = strtokSafe(NULL, separators, &saveptr)))
        sarrayAddString(sa, substr, L_INSERT);
    FREE(cstr);

    return 0;
}
