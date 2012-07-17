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
 *  sarrayCreateInitialized()
 *
 *      Input:  n (size of string ptr array to be alloc'd)
 *              initstr (string to be initialized on the full array)
 *      Return: sarray, or null on error
 */
LEPTONICA_EXPORT SARRAY *
sarrayCreateInitialized(l_int32  n,
                        char    *initstr)
{
l_int32  i;
SARRAY  *sa;

    PROCNAME("sarrayCreateInitialized");

    if (n <= 0)
        return (SARRAY *)ERROR_PTR("n must be > 0", procName, NULL);
    if (!initstr)
        return (SARRAY *)ERROR_PTR("initstr not defined", procName, NULL);

    sa = sarrayCreate(n);
    for (i = 0; i < n; i++)
        sarrayAddString(sa, initstr, L_COPY);
    return sa;
}


/*!
 *  sarrayCreateLinesFromString()
 *
 *      Input:  string
 *              blankflag  (0 to exclude blank lines; 1 to include)
 *      Return: sarray, or null on error
 *
 *  Notes:
 *      (1) This finds the number of line substrings, each of which
 *          ends with a newline, and puts a copy of each substring
 *          in a new sarray.
 *      (2) The newline characters are removed from each substring.
 */
LEPTONICA_EXPORT SARRAY *
sarrayCreateLinesFromString(char    *string,
                            l_int32  blankflag)
{
l_int32  i, nsub, size, startptr;
char    *cstring, *substring;
SARRAY  *sa;

    PROCNAME("sarrayCreateLinesFromString");

    if (!string)
        return (SARRAY *)ERROR_PTR("textstr not defined", procName, NULL);

        /* find the number of lines */
    size = strlen(string);
    nsub = 0;
    for (i = 0; i < size; i++) {
        if (string[i] == '\n')
            nsub++;
    }

    if ((sa = sarrayCreate(nsub)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);

    if (blankflag) {  /* keep blank lines as null strings */
            /* Make a copy for munging */
        if ((cstring = stringNew(string)) == NULL)
            return (SARRAY *)ERROR_PTR("cstring not made", procName, NULL);
            /* We'll insert nulls like strtok */
        startptr = 0;
        for (i = 0; i < size; i++) {
            if (cstring[i] == '\n') {
                cstring[i] = '\0';
                if (i > 0 && cstring[i - 1] == '\r')
                    cstring[i - 1] = '\0';  /* also remove Windows CR */
                if ((substring = stringNew(cstring + startptr)) == NULL)
                    return (SARRAY *)ERROR_PTR("substring not made",
                                                procName, NULL);
                sarrayAddString(sa, substring, L_INSERT);
/*                fprintf(stderr, "substring = %s\n", substring); */
                startptr = i + 1;
            }
        }
        if (startptr < size) {  /* no newline at end of last line */
            if ((substring = stringNew(cstring + startptr)) == NULL)
                return (SARRAY *)ERROR_PTR("substring not made",
                                            procName, NULL);
            sarrayAddString(sa, substring, L_INSERT);
/*            fprintf(stderr, "substring = %s\n", substring); */
        }
        FREE(cstring);
    }
    else {  /* remove blank lines; use strtok */
        sarraySplitString(sa, string, "\r\n");
    }

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

    sarrayChangeRefcount(sa, -1);
    if (sarrayGetRefcount(sa) <= 0) {
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
 *  sarrayCopy()
 *
 *      Input:  sarray
 *      Return: copy of sarray, or null on error
 */
LEPTONICA_EXPORT SARRAY *
sarrayCopy(SARRAY  *sa)
{
l_int32  i;
SARRAY  *csa;

    PROCNAME("sarrayCopy");

    if (!sa)
        return (SARRAY *)ERROR_PTR("sa not defined", procName, NULL);

    if ((csa = sarrayCreate(sa->nalloc)) == NULL)
        return (SARRAY *)ERROR_PTR("csa not made", procName, NULL);

    for (i = 0; i < sa->n; i++)
        sarrayAddString(csa, sa->array[i], L_COPY);

    return csa;
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


/*!
 *  sarrayReplaceString()
 *
 *      Input:  sarray
 *              index (of string within sarray to be replaced)
 *              newstr (string to replace existing one)
 *              copyflag (L_INSERT, L_COPY)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys an existing string and replaces it with
 *          the new string or a copy of it.
 *      (2) By design, an sarray is always compacted, so there are
 *          never any holes (null ptrs) in the ptr array up to the
 *          current count.
 */
LEPTONICA_EXPORT l_int32
sarrayReplaceString(SARRAY  *sa,
                    l_int32  index,
                    char    *newstr,
                    l_int32  copyflag)
{
char    *str;
l_int32  n;

    PROCNAME("sarrayReplaceString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    n = sarrayGetCount(sa);
    if (index < 0 || index >= n)
        return ERROR_INT("array index out of bounds", procName, 1);
    if (!newstr)
        return ERROR_INT("newstr not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return ERROR_INT("invalid copyflag", procName, 1);

    FREE(sa->array[index]);
    if (copyflag == L_INSERT)
        str = newstr;
    else  /* L_COPY */
        str = stringNew(newstr);
    sa->array[index] = str;
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
 *  sarrayGetArray()
 *
 *      Input:  sarray
 *              &nalloc  (<optional return> number allocated string ptrs)
 *              &n  (<optional return> number allocated strings)
 *      Return: ptr to string array, or null on error
 *
 *  Notes:
 *      (1) Caution: the returned array is not a copy, so caller
 *          must not destroy it!
 */
LEPTONICA_EXPORT char **
sarrayGetArray(SARRAY   *sa,
               l_int32  *pnalloc,
               l_int32  *pn)
{
char  **array;

    PROCNAME("sarrayGetArray");

    if (!sa)
        return (char **)ERROR_PTR("sa not defined", procName, NULL);

    array = sa->array;
    if (pnalloc) *pnalloc = sa->nalloc;
    if (pn) *pn = sa->n;

    return array;
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


/*!
 *  sarrayGetRefCount()
 *
 *      Input:  sarray
 *      Return: refcount, or UNDEF on error
 */
LEPTONICA_EXPORT l_int32
sarrayGetRefcount(SARRAY  *sa)
{
    PROCNAME("sarrayGetRefcount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, UNDEF);
    return sa->refcount;
}


/*!
 *  sarrayChangeRefCount()
 *
 *      Input:  sarray
 *              delta (change to be applied)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
sarrayChangeRefcount(SARRAY  *sa,
		     l_int32  delta)
{
    PROCNAME("sarrayChangeRefcount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, UNDEF);
    sa->refcount += delta;
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Conversion to string                           *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayToString()
 *
 *      Input:  sarray
 *              addnlflag (flag: 0 adds nothing to each substring
 *                               1 adds '\n' to each substring
 *                               2 adds ' ' to each substring)
 *      Return: dest string, or null on error
 *
 *  Notes:
 *      (1) Concatenates all the strings in the sarray, preserving
 *          all white space.
 *      (2) If addnlflag != 0, adds either a '\n' or a ' ' after
 *          each substring.
 *      (3) This function was NOT implemented as:
 *            for (i = 0; i < n; i++)
 *                     strcat(dest, sarrayGetString(sa, i, L_NOCOPY));
 *          Do you see why?
 */
LEPTONICA_EXPORT char *
sarrayToString(SARRAY  *sa,
               l_int32  addnlflag)
{
    PROCNAME("sarrayToString");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);

    return sarrayToStringRange(sa, 0, 0, addnlflag);
}


/*!
 *  sarrayToStringRange()
 *
 *      Input: sarray
 *             first  (index of first string to use; starts with 0)
 *             nstrings (number of strings to append into the result; use
 *                       0 to append to the end of the sarray)
 *             addnlflag (flag: 0 adds nothing to each substring
 *                              1 adds '\n' to each substring
 *                              2 adds ' ' to each substring)
 *      Return: dest string, or null on error
 *
 *  Notes:
 *      (1) Concatenates the specified strings inthe sarray, preserving
 *          all white space.
 *      (2) If addnlflag != 0, adds either a '\n' or a ' ' after
 *          each substring.
 *      (3) If the sarray is empty, this returns a string with just
 *          the character corresponding to @addnlflag.
 */
LEPTONICA_EXPORT char *
sarrayToStringRange(SARRAY  *sa,
                    l_int32  first,
                    l_int32  nstrings,
                    l_int32  addnlflag)
{
char    *dest, *src, *str;
l_int32  n, i, last, size, index, len;

    PROCNAME("sarrayToStringRange");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);
    if (addnlflag != 0 && addnlflag != 1 && addnlflag != 2)
        return (char *)ERROR_PTR("invalid addnlflag", procName, NULL);

    n = sarrayGetCount(sa);

        /* Empty sa; return char corresponding to addnlflag only */
    if (n == 0) {
        if (first == 0) {
            if (addnlflag == 0)
                return stringNew("");
            if (addnlflag == 1)
                return stringNew("\n");
	    else  /* addnlflag == 2) */
                return stringNew(" ");
        }
        else
            return (char *)ERROR_PTR("first not valid", procName, NULL);
    }

    if (first < 0 || first >= n)
        return (char *)ERROR_PTR("first not valid", procName, NULL);
    if (nstrings == 0 || (nstrings > n - first))
        nstrings = n - first;  /* no overflow */
    last = first + nstrings - 1;

    size = 0;
    for (i = first; i <= last; i++) {
        if ((str = sarrayGetString(sa, i, L_NOCOPY)) == NULL)
            return (char *)ERROR_PTR("str not found", procName, NULL);
        size += strlen(str) + 2;
    }

    if ((dest = (char *)CALLOC(size + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);

    index = 0;
    for (i = first; i <= last; i++) {
        src = sarrayGetString(sa, i, L_NOCOPY);
        len = strlen(src);
        memcpy(dest + index, src, len);
        index += len;
        if (addnlflag == 1) {
            dest[index] = '\n';
            index++;
        }
        else if (addnlflag == 2) {
            dest[index] = ' ';
            index++;
        }
    }

    return dest;
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


/*----------------------------------------------------------------------*
 *                              Filter sarray                           *
 *----------------------------------------------------------------------*/
/*!
 *  sarraySelectBySubstring()
 *
 *      Input:  sain (input sarray)
 *              substr (<optional> substring for matching; can be NULL)
 *      Return: saout (output sarray, filtered with substring) or null on error
 *
 *  Notes:
 *      (1) This selects all strings in sain that have substr as a substring.
 *          Note that we can't use strncmp() because we're looking for
 *          a match to the substring anywhere within each filename.
 *      (2) If substr == NULL, returns a copy of the sarray.
 */
LEPTONICA_EXPORT SARRAY *
sarraySelectBySubstring(SARRAY      *sain,
                        const char  *substr)
{
char    *str;
l_int32  n, i, offset, found;
SARRAY  *saout;

    PROCNAME("sarraySelectBySubstring");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);

    n = sarrayGetCount(sain);
    if (!substr || n == 0)
        return sarrayCopy(sain);

    saout = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sain, i, L_NOCOPY);
        arrayFindSequence((l_uint8 *)str, strlen(str), (l_uint8 *)substr,
                          strlen(substr), &offset, &found);
        if (found)
            sarrayAddString(saout, str, L_COPY);
    }

    return saout;
}


/*----------------------------------------------------------------------*
 *                                   Sort                               *
 *----------------------------------------------------------------------*/
/*!
 *  sarraySort()
 *
 *      Input:  saout (output sarray; can be NULL or equal to sain)
 *              sain (input sarray)
 *              sortorder (L_SORT_INCREASING or L_SORT_DECREASING)
 *      Return: saout (output sarray, sorted by ascii value), or null on error
 *
 *  Notes:
 *      (1) Set saout = sain for in-place; otherwise, set naout = NULL.
 *      (2) Shell sort, modified from K&R, 2nd edition, p.62.
 *          Slow but simple O(n logn) sort.
 */
LEPTONICA_EXPORT SARRAY *
sarraySort(SARRAY  *saout,
           SARRAY  *sain,
           l_int32  sortorder)
{
char   **array;
char    *tmp;
l_int32  n, i, j, gap;

    PROCNAME("sarraySort");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);

        /* Make saout if necessary; otherwise do in-place */
    if (!saout)
        saout = sarrayCopy(sain);
    else if (sain != saout)
        return (SARRAY *)ERROR_PTR("invalid: not in-place", procName, NULL);
    array = saout->array;  /* operate directly on the array */
    n = sarrayGetCount(saout);

        /* Shell sort */
    for (gap = n/2; gap > 0; gap = gap / 2) {
        for (i = gap; i < n; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                if ((sortorder == L_SORT_INCREASING &&
                     stringCompareLexical(array[j], array[j + gap])) ||
                    (sortorder == L_SORT_DECREASING &&
                     stringCompareLexical(array[j + gap], array[j])))
                {
                    tmp = array[j];
                    array[j] = array[j + gap];
                    array[j + gap] = tmp;
                }
            }
        }
    }

    return saout;
}


/*!
 *  stringCompareLexical()
 *
 *      Input:  str1
 *              str2
 *      Return: 1 if str1 > str2 (lexically); 0 otherwise
 *
 *  Notes:
 *      (1) If the lexical values are identical, return a 0, to
 *          indicate that no swapping is required to sort the strings.
 */
LEPTONICA_EXPORT l_int32
stringCompareLexical(const char *str1,
                     const char *str2)
{
l_int32  i, len1, len2, len;

    PROCNAME("sarrayCompareLexical");

    if (!str1)
        return ERROR_INT("str1 not defined", procName, 1);
    if (!str2)
        return ERROR_INT("str2 not defined", procName, 1);

    len1 = strlen(str1);
    len2 = strlen(str2);
    len = L_MIN(len1, len2);

    for (i = 0; i < len; i++) {
        if (str1[i] == str2[i])
            continue;
        if (str1[i] > str2[i])
            return 1;
        else
            return 0;
    }

    if (len1 > len2)
        return 1;
    else
        return 0;
}


/*----------------------------------------------------------------------*
 *                           Serialize for I/O                          *
 *----------------------------------------------------------------------*/

/*!
 *  sarrayReadStream()
 *
 *      Input:  stream
 *      Return: sarray, or null on error
 *
 *  Notes:
 *      (1) We store the size of each string along with the string.
 *      (2) This allows a string to have embedded newlines.  By reading
 *          the entire string, as determined by its size, we are
 *          not affected by any number of embedded newlines.
 */
LEPTONICA_EXPORT SARRAY *
sarrayReadStream(FILE  *fp)
{
char    *stringbuf;
l_int32  i, n, size, index, bufsize, version, ignore;
SARRAY  *sa;

    PROCNAME("sarrayReadStream");

    if (!fp)
        return (SARRAY *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nSarray Version %d\n", &version) != 1)
        return (SARRAY *)ERROR_PTR("not an sarray file", procName, NULL);
    if (version != SARRAY_VERSION_NUMBER)
        return (SARRAY *)ERROR_PTR("invalid sarray version", procName, NULL);
    if (fscanf(fp, "Number of strings = %d\n", &n) != 1)
        return (SARRAY *)ERROR_PTR("error on # strings", procName, NULL);

    if ((sa = sarrayCreate(n)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    bufsize = L_BUF_SIZE4 + 1;
    if ((stringbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL)
        return (SARRAY *)ERROR_PTR("stringbuf not made", procName, NULL);

    for (i = 0; i < n; i++) {
	    /* Get the size of the stored string */
        if (fscanf(fp, "%d[%d]:", &index, &size) != 2)
            return (SARRAY *)ERROR_PTR("error on string size", procName, NULL);
	    /* Expand the string buffer if necessary */
	if (size > bufsize - 5) {
            FREE(stringbuf);
	    bufsize = (l_int32)(1.5 * size);
            stringbuf = (char *)CALLOC(bufsize, sizeof(char));
	}
	    /* Read the stored string, plus leading spaces and trailing \n */
	if (fread(stringbuf, 1, size + 3, fp) != size + 3)
            return (SARRAY *)ERROR_PTR("error reading string", procName, NULL);
	    /* Remove the \n that was added by sarrayWriteStream() */
	stringbuf[size + 2] = '\0';
	    /* Copy it in, skipping the 2 leading spaces */
        sarrayAddString(sa, stringbuf + 2, L_COPY);
    }
    ignore = fscanf(fp, "\n");

    FREE(stringbuf);
    return sa;
}


/*!
 *  sarrayWriteStream()
 *
 *      Input:  stream
 *              sarray
 *      Returns 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This appends a '\n' to each string, which is stripped
 *          off by sarrayReadStream().
 */
LEPTONICA_EXPORT l_int32
sarrayWriteStream(FILE    *fp,
                  SARRAY  *sa)
{
l_int32  i, n, len;

    PROCNAME("sarrayWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    n = sarrayGetCount(sa);
    fprintf(fp, "\nSarray Version %d\n", SARRAY_VERSION_NUMBER);
    fprintf(fp, "Number of strings = %d\n", n);
    for (i = 0; i < n; i++) {
        len = strlen(sa->array[i]);
        fprintf(fp, "  %d[%d]:  %s\n", i, len, sa->array[i]);
    }
    fprintf(fp, "\n");

    return 0;
}


/*---------------------------------------------------------------------*
 *                           Directory filenames                       *
 *---------------------------------------------------------------------*/

/*!
 *  getSortedPathnamesInDirectory()
 *
 *      Input:  directory name
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              firstpage (0-based)
 *              npages (use 0 for all to the end)
 *      Return: sarray of sorted pathnames, or NULL on error
 *
 *  Notes:
 *      (1) If @substr is not NULL, only filenames that contain
 *          the substring can be returned.  If @substr == NULL,
 *          none of the filenames are filtered out.
 *      (2) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order.
 *          The full pathnames are returned for the requested sequence.
 *          If no files are found after filtering, returns an empty sarray.
 */
LEPTONICA_EXPORT SARRAY *
getSortedPathnamesInDirectory(const char  *dirname,
                              const char  *substr,
                              l_int32      firstpage,
                              l_int32      npages)
{
char    *fname, *fullname;
l_int32  i, nfiles, lastpage;
SARRAY  *sa, *safiles, *saout;

    PROCNAME("getSortedPathnamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    if ((sa = getFilenamesInDirectory(dirname)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    safiles = sarraySelectBySubstring(sa, substr);
    sarrayDestroy(&sa);
    nfiles = sarrayGetCount(safiles);
    if (nfiles == 0) {
        L_WARNING("no files found", procName);
        return safiles;
    }

    sarraySort(safiles, safiles, L_SORT_INCREASING);

    firstpage = L_MIN(L_MAX(firstpage, 0), nfiles - 1);
    if (npages == 0)
        npages = nfiles - firstpage;
    lastpage = L_MIN(firstpage + npages - 1, nfiles - 1);

    saout = sarrayCreate(lastpage - firstpage + 1);
    for (i = firstpage; i <= lastpage; i++) {
        fname = sarrayGetString(safiles, i, L_NOCOPY);
        fullname = genPathname(dirname, fname);
        sarrayAddString(saout, fullname, L_INSERT);
    }

    sarrayDestroy(&safiles);
    return saout;
}


/*!
 *  getFilenamesInDirectory()
 *
 *      Input:  directory name
 *      Return: sarray of file names, or NULL on error
 *
 *  Notes:
 *      (1) The versions compiled under unix and cygwin use the POSIX C
 *          library commands for handling directories.  For windows,
 *          there is a separate implementation.
 *      (2) It returns an array of filename tails; i.e., only the part of
 *          the path after the last slash.
 *      (3) Use of the d_type field of dirent is not portable:
 *          "According to POSIX, the dirent structure contains a field
 *          char d_name[] of unspecified size, with at most NAME_MAX
 *          characters preceding the terminating null character.  Use
 *          of other fields will harm the portability of your programs."
 *      (4) As a consequence of (3), we note several things:
 *           - MINGW doesn't have a d_type member.
 *           - Older versions of gcc (e.g., 2.95.3) return DT_UNKNOWN
 *             for d_type from all files.
 *          On these systems, this function will return directories
 *          (except for '.' and '..', which are eliminated using
 *          the d_name field).
 */

#ifndef _WIN32

LEPTONICA_EXPORT SARRAY *
getFilenamesInDirectory(const char  *dirname)
{
char           *name;
l_int32         len;
SARRAY         *safiles;
DIR            *pdir;
struct dirent  *pdirentry;

    PROCNAME("getFilenamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    if ((pdir = opendir(dirname)) == NULL)
        return (SARRAY *)ERROR_PTR("pdir not opened", procName, NULL);
    if ((safiles = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("safiles not made", procName, NULL);
    while ((pdirentry = readdir(pdir)))  {

        /* It's nice to ignore directories.  For this it is necessary to
         * define _BSD_SOURCE in the CC command, because the DT_DIR
         * flag is non-standard.  */ 
#if !defined(__SOLARIS__)
        if (pdirentry->d_type == DT_DIR)
            continue;
#endif

            /* Filter out "." and ".." if they're passed through */
        name = pdirentry->d_name;
        len = strlen(name);
        if (len == 1 && name[len - 1] == '.') continue;
        if (len == 2 && name[len - 1] == '.' && name[len - 2] == '.') continue;
        sarrayAddString(safiles, name, L_COPY);
    }
    closedir(pdir);

    return safiles;
}

#else  /* _WIN32 */

    /* http://msdn2.microsoft.com/en-us/library/aa365200(VS.85).aspx */
#include <windows.h>

LEPTONICA_EXPORT SARRAY *
getFilenamesInDirectory(const char  *dirname)
{
char             *pszDir;
char             *tempname;
HANDLE            hFind = INVALID_HANDLE_VALUE;
SARRAY           *safiles;
WIN32_FIND_DATAA  ffd;

    PROCNAME("getFilenamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    tempname = genPathname(dirname, NULL);
    pszDir = stringJoin(tempname, "\\*");
    FREE(tempname);

    if (strlen(pszDir) + 1 > MAX_PATH) {
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("dirname is too long", procName, NULL);
    }

    if ((safiles = sarrayCreate(0)) == NULL) {
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("safiles not made", procName, NULL);
    }

    hFind = FindFirstFileA(pszDir, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        sarrayDestroy(&safiles);
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("hFind not opened", procName, NULL);
    }

    while (FindNextFileA(hFind, &ffd) != 0) {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  /* skip dirs */
            continue;
        sarrayAddString(safiles, ffd.cFileName, L_COPY);
    }

    FindClose(hFind);
    FREE(pszDir);
    return safiles;
}

#endif  /* _WIN32 */

