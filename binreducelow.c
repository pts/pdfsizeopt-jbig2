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
 *  binreducelow.c
 *
 *          Low-level subsampled reduction
 *                  void       reduceBinary2Low()
 *
 *          Low-level threshold reduction
 *                  void       reduceRankBinary2Low()
 *                  l_uint8   *makeSubsampleTab2x()
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "allheaders.h"



/*!
 *  makeSubsampleTab2x()
 *
 *  This table permutes the bits in a byte, from
 *      0 4 1 5 2 6 3 7
 *  to
 *      0 1 2 3 4 5 6 7
 */  
LEPTONICA_EXPORT l_uint8 *
makeSubsampleTab2x(void)
{
l_uint8  *tab;
l_int32   i;

    PROCNAME("makeSubsampleTab2x");

    if ((tab = (l_uint8 *) CALLOC(256, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 256; i++)
        tab[i] = ((i & 0x01)     ) |    /* 7 */
                 ((i & 0x04) >> 1) |    /* 6 */
                 ((i & 0x10) >> 2) |    /* 5 */
                 ((i & 0x40) >> 3) |    /* 4 */
                 ((i & 0x02) << 3) |    /* 3 */
                 ((i & 0x08) << 2) |    /* 2 */
                 ((i & 0x20) << 1) |    /* 1 */
                 ((i & 0x80)     );     /* 0 */

    return tab;
}

