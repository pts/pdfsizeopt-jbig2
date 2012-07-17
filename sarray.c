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
