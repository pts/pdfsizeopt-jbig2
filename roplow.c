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
 *  roplow.c
 *
 *      Low level dest-only
 *           void            rasteropUniLow()
 *           static void     rasteropUniWordAlignedlLow()
 *           static void     rasteropUniGeneralLow()
 *
 *      Low level src and dest
 *           void            rasteropLow()
 *           static void     rasteropWordAlignedLow()
 *           static void     rasteropVAlignedLow()
 *           static void     rasteropGeneralLow()
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#define COMBINE_PARTIAL(d, s, m)     ( ((d) & ~(m)) | ((s) & (m)) )

static const l_int32  SHIFT_LEFT  = 0;
static const l_int32  SHIFT_RIGHT = 1;

static void rasteropUniWordAlignedLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                      l_int32 dy, l_int32  dw, l_int32 dh,
                                      l_int32 op);

static void rasteropUniGeneralLow(l_uint32 *datad, l_int32 dwpl, l_int32 dx,
                                  l_int32 dy, l_int32 dw, l_int32  dh,
                                  l_int32 op);

static const l_uint32 lmask32[] = {0x0,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff};

#if !RMASK32_DEFINED
#define RMASK32_DEFINED 1
static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
#endif


/*--------------------------------------------------------------------*
 *                     Low-level dest-only rasterops                  *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dpixw  (width of dest)
 *              dpixh  (height of dest)
 *              depth  (depth of src and dest)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 *
 *  Action: scales width, performs clipping, checks alignment, and
 *          dispatches for the rasterop.
 */
LEPTONICA_EXPORT void
rasteropUniLow(l_uint32  *datad,
               l_int32    dpixw,
               l_int32    dpixh,
               l_int32    depth,
               l_int32    dwpl,
               l_int32    dx,
               l_int32    dy,
               l_int32    dw,
               l_int32    dh,
               l_int32    op)
{
l_int32  dhangw, dhangh;

   /* -------------------------------------------------------*
    *            scale horizontal dimensions by depth
    * -------------------------------------------------------*/
    if (depth != 1) {
        dpixw *= depth;
        dx *= depth;
        dw *= depth;
    }

   /* -------------------------------------------------------*
    *            clip rectangle to dest image
    * -------------------------------------------------------*/
       /* first, clip horizontally (dx, dw) */
    if (dx < 0) {
        dw += dx;  /* reduce dw */
        dx = 0;
    }
    dhangw = dx + dw - dpixw;  /* rect ovhang dest to right */
    if (dhangw > 0)
        dw -= dhangw;  /* reduce dw */

       /* then, clip vertically (dy, dh) */
    if (dy < 0) {
        dh += dy;  /* reduce dh */
        dy = 0;
    }
    dhangh = dy + dh - dpixh;  /* rect ovhang dest below */
    if (dhangh > 0)
        dh -= dhangh;  /* reduce dh */

        /* if clipped entirely, quit */
    if ((dw <= 0) || (dh <= 0))
        return;

   /* -------------------------------------------------------*
    *       dispatch to aligned or non-aligned blitters
    * -------------------------------------------------------*/
    if ((dx & 31) == 0)
        rasteropUniWordAlignedLow(datad, dwpl, dx, dy, dw, dh, op);
    else
        rasteropUniGeneralLow(datad, dwpl, dx, dy, dw, dh, op);
    return;
}



/*--------------------------------------------------------------------*
 *           Static low-level uni rasterop with word alignment        *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniWordAlignedLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 *
 *  This is called when the dest rect is left aligned
 *  on (32-bit) word boundaries.   That is: dx & 31 == 0.
 *
 *  We make an optimized implementation of this because
 *  it is a common case: e.g., operating on a full dest image.
 */
static void
rasteropUniWordAlignedLow(l_uint32  *datad,
                          l_int32    dwpl,
                          l_int32    dx,
                          l_int32    dy,
                          l_int32    dw,
                          l_int32    dh,
                          l_int32    op)
{
l_int32    nfullw;     /* number of full words */
l_uint32  *pfword;     /* ptr to first word */
l_int32    lwbits;     /* number of ovrhang bits in last partial word */
l_uint32   lwmask;     /* mask for last partial word */
l_uint32  *lined;
l_int32    i, j;

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
    nfullw = dw >> 5;
    lwbits = dw & 31;
    if (lwbits)
        lwmask = lmask32[lwbits];
    pfword = datad + dwpl * dy + (dx >> 5);
    

    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_NOT(PIX_DST):
        for (i = 0; i < dh; i++) {
            lined = pfword + i * dwpl;
            for (j = 0; j < nfullw; j++) {
                *lined = ~(*lined);
                lined++;
            }
            if (lwbits)
                *lined = COMBINE_PARTIAL(*lined, ~(*lined), lwmask);
        }
        break;
    default:
        fprintf(stderr, "Operation %d not permitted here!\n", op);
    }

    return;
}


/*--------------------------------------------------------------------*
 *        Static low-level uni rasterop without word alignment        *
 *--------------------------------------------------------------------*/
/*!
 *  rasteropUniGeneralLow()
 *
 *      Input:  datad  (ptr to dest image data)
 *              dwpl   (wpl of dest)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *      Return: void
 */
static void
rasteropUniGeneralLow(l_uint32  *datad,
                      l_int32    dwpl,
                      l_int32    dx,
                      l_int32    dy,
                      l_int32    dw,
                      l_int32    dh,
                      l_int32    op)
{
l_int32    dfwpartb;   /* boolean (1, 0) if first dest word is partial */
l_int32    dfwpart2b;  /* boolean (1, 0) if first dest word is doubly partial */
l_uint32   dfwmask;    /* mask for first partial dest word */
l_int32    dfwbits;    /* first word dest bits in ovrhang */
l_uint32  *pdfwpart;   /* ptr to first partial dest word */
l_int32    dfwfullb;   /* boolean (1, 0) if there exists a full dest word */
l_int32    dnfullw;    /* number of full words in dest */
l_uint32  *pdfwfull;   /* ptr to first full dest word */
l_int32    dlwpartb;   /* boolean (1, 0) if last dest word is partial */
l_uint32   dlwmask;    /* mask for last partial dest word */
l_int32    dlwbits;    /* last word dest bits in ovrhang */
l_uint32  *pdlwpart;   /* ptr to last partial dest word */
l_int32    i, j;


    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* is the first word partial? */
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    }
    else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
    }

        /* is the first word doubly partial? */
    if (dw >= dfwbits)  /* if not */
        dfwpart2b = 0;
    else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    }
    else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0)  /* if not */
            dfwfullb = 0;
        else {  /* if so */
            dfwfullb = 1;
            if (dfwpartb)
                pdfwfull = pdfwpart + 1;
            else
                pdfwfull = datad + dwpl * dy + (dx >> 5);
        }
    }

        /* is the last word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0)  /* if not */
        dlwpartb = 0;
    else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        if (dfwpartb)
            pdlwpart = pdfwpart + 1 + dnfullw;
        else
            pdlwpart = datad + dwpl * dy + (dx >> 5) + dnfullw;
    }


    /*--------------------------------------------------------*
     *            Now we're ready to do the ops               *
     *--------------------------------------------------------*/
    switch (op)
    {
    case PIX_NOT(PIX_DST):
            /* do the first partial word */
        if (dfwpartb) {
            for (i = 0; i < dh; i++) {
                *pdfwpart = COMBINE_PARTIAL(*pdfwpart, ~(*pdfwpart), dfwmask);
                pdfwpart += dwpl;
            }
        }

            /* do the full words */
        if (dfwfullb) {
            for (i = 0; i < dh; i++) {
                for (j = 0; j < dnfullw; j++)
                    *(pdfwfull + j) = ~(*(pdfwfull + j));
                pdfwfull += dwpl;
            }
        }

            /* do the last partial word */
        if (dlwpartb) {
            for (i = 0; i < dh; i++) {
                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, ~(*pdlwpart), dlwmask);
                pdlwpart += dwpl;
            }
        }
        break;
    default:
        fprintf(stderr, "Operation %d not permitted here!\n", op);
    }

    return;
}
