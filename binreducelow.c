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

/*-------------------------------------------------------------------*
 *                 Low-level rank filtered reduction                 *
 *-------------------------------------------------------------------*/
/*!
 *  reduceRankBinary2Low()
 *
 *  Rank filtering is done to the UL corner of each 2x2 pixel block,
 *  using only logical operations.
 *
 *  Then these pixels are chosen in the 2x subsampling process,
 *  subsampled, as described above in reduceBinary2Low().
 */
LEPTONICA_EXPORT void
reduceRankBinary2Low(l_uint32  *datad,
                     l_int32    wpld,
                     l_uint32  *datas,
                     l_int32    hs,
                     l_int32    wpls,
                     l_uint8   *tab,
                     l_int32    level)
{
l_int32    i, id, j, wplsi;
l_uint8    byte0, byte1;
l_uint16   shortd;
l_uint32   word1, word2, word3, word4;
l_uint32  *lines, *lined;

        /* e.g., if ws = 65: wd = 32, wpls = 3, wpld = 1 --> trouble */
    wplsi = L_MIN(wpls, 2 * wpld);  /* iterate over this number of words */

    switch (level)
    {

    case 1:
        for (i = 0, id = 0; i < hs - 1; i += 2, id++) {
            lines = datas + i * wpls;
            lined = datad + id * wpld;
            for (j = 0; j < wplsi; j++) {
                word1 = *(lines + j);
                word2 = *(lines + wpls + j);

                    /* OR/OR */
                word2 = word1 | word2;
                word2 = word2 | (word2 << 1);

                word2 = word2 & 0xaaaaaaaa;  /* mask */
                word1 = word2 | (word2 << 7);  /* fold; data in bytes 0 & 2 */
                byte0 = word1 >> 24;
                byte1 = (word1 >> 8) & 0xff;
                shortd = (tab[byte0] << 8) | tab[byte1];
                SET_DATA_TWO_BYTES(lined, j, shortd);
            }
        }
        break;

    case 2:
        for (i = 0, id = 0; i < hs - 1; i += 2, id++) {
            lines = datas + i * wpls;
            lined = datad + id * wpld;
            for (j = 0; j < wplsi; j++) {
                word1 = *(lines + j);
                word2 = *(lines + wpls + j);

                    /* (AND/OR) OR (OR/AND) */
                word3 = word1 & word2;
                word3 = word3 | (word3 << 1);
                word4 = word1 | word2;
                word4 = word4 & (word4 << 1);
                word2 = word3 | word4;

                word2 = word2 & 0xaaaaaaaa;  /* mask */
                word1 = word2 | (word2 << 7);  /* fold; data in bytes 0 & 2 */
                byte0 = word1 >> 24;
                byte1 = (word1 >> 8) & 0xff;
                shortd = (tab[byte0] << 8) | tab[byte1];
                SET_DATA_TWO_BYTES(lined, j, shortd);
            }
        }
        break;

    case 3:
        for (i = 0, id = 0; i < hs - 1; i += 2, id++) {
            lines = datas + i * wpls;
            lined = datad + id * wpld;
            for (j = 0; j < wplsi; j++) {
                word1 = *(lines + j);
                word2 = *(lines + wpls + j);

                    /* (AND/OR) AND (OR/AND) */
                word3 = word1 & word2;
                word3 = word3 | (word3 << 1);
                word4 = word1 | word2;
                word4 = word4 & (word4 << 1);
                word2 = word3 & word4;

                word2 = word2 & 0xaaaaaaaa;  /* mask */
                word1 = word2 | (word2 << 7);  /* fold; data in bytes 0 & 2 */
                byte0 = word1 >> 24;
                byte1 = (word1 >> 8) & 0xff;
                shortd = (tab[byte0] << 8) | tab[byte1];
                SET_DATA_TWO_BYTES(lined, j, shortd);
            }
        }
        break;

    case 4:
        for (i = 0, id = 0; i < hs - 1; i += 2, id++) {
            lines = datas + i * wpls;
            lined = datad + id * wpld;
            for (j = 0; j < wplsi; j++) {
                word1 = *(lines + j);
                word2 = *(lines + wpls + j);

                    /* AND/AND */
                word2 = word1 & word2;
                word2 = word2 & (word2 << 1);

                word2 = word2 & 0xaaaaaaaa;  /* mask */
                word1 = word2 | (word2 << 7);  /* fold; data in bytes 0 & 2 */
                byte0 = word1 >> 24;
                byte1 = (word1 >> 8) & 0xff;
                shortd = (tab[byte0] << 8) | tab[byte1];
                SET_DATA_TWO_BYTES(lined, j, shortd);
            }
        }
        break;
    }

    return;
}
