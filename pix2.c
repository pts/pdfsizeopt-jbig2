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
 *  pix2.c
 *
 *    This file has these basic operations:
 *
 *      (1) Get and set: individual pixels, full image, rectangular region,
 *          pad pixels, border pixels, and color components for RGB
 *      (2) Add and remove border pixels
 *      (3) Endian byte swaps
 *      (4) Simple method for byte-processing images (instead of words)
 *
 *      Pixel poking
 *           l_int32     pixGetPixel()
 *           l_int32     pixSetPixel()
 *           l_int32     pixGetRGBPixel()
 *           l_int32     pixSetRGBPixel()
 *           l_int32     pixGetRandomPixel()
 *           l_int32     pixClearPixel()
 *           l_int32     pixFlipPixel()
 *           void        setPixelLow()
 *
 *      Full image clear/set/set-to-arbitrary-value
 *           l_int32     pixClearAll()
 *           l_int32     pixSetAll()
 *           l_int32     pixSetAllArbitrary()
 *           l_int32     pixSetBlackOrWhite()
 *
 *      Rectangular region clear/set/set-to-arbitrary-value/blend
 *           l_int32     pixClearInRect()
 *           l_int32     pixSetInRect()
 *           l_int32     pixSetInRectArbitrary()
 *           l_int32     pixBlendInRect()
 *
 *      Set pad bits
 *           l_int32     pixSetPadBits()
 *           l_int32     pixSetPadBitsBand()
 *
 *      Assign border pixels
 *           l_int32     pixSetOrClearBorder()
 *           l_int32     pixSetBorderVal()
 *           l_int32     pixSetBorderRingVal()
 *           l_int32     pixSetMirroredBorder()
 *           PIX        *pixCopyBorder()
 *
 *      Add and remove border
 *           PIX        *pixAddBorder()
 *           PIX        *pixAddBlackBorder()
 *           PIX        *pixAddBorderGeneral()
 *           PIX        *pixRemoveBorder()
 *           PIX        *pixRemoveBorderGeneral()
 *           PIX        *pixAddMirroredBorder()
 *           PIX        *pixAddRepeatedBorder()
 *           PIX        *pixAddMixedBorder()
 *
 *      Color sample setting and extraction
 *           PIX        *pixCreateRGBImage()
 *           PIX        *pixGetRGBComponent()
 *           l_int32     pixSetRGBComponent()
 *           PIX        *pixGetRGBComponentCmap()
 *           l_int32     composeRGBPixel()
 *           void        extractRGBValues()
 *           l_int32     extractMinMaxComponent()
 *           l_int32     pixGetRGBLine()
 *
 *      Conversion between big and little endians
 *           PIX        *pixEndianByteSwapNew()
 *           l_int32     pixEndianByteSwap()
 *           l_int32     lineEndianByteSwap()
 *           PIX        *pixEndianTwoByteSwapNew()
 *           l_int32     pixEndianTwoByteSwap()
 *
 *      Extract raster data as binary string
 *           l_int32     pixGetRasterData()
 *
 *      Setup helpers for 8 bpp byte processing
 *           l_uint8   **pixSetupByteProcessing()
 *           l_int32     pixCleanupByteProcessing()
 *
 *      Setting parameters for antialias masking with alpha transforms
 *           void        l_setAlphaMaskBorder()
 *
 *      *** indicates implicit assumption about RGB component ordering
 */


#include <string.h>
#include "allheaders.h"

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

#ifndef  NO_CONSOLE_IO
#define  DEBUG_SERIALIZE        0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                         Pixel poking                        *
 *-------------------------------------------------------------*/

/*!
 *  pixSetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              val (value to be inserted)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Warning: the input value is not checked for overflow with respect
 *          the the depth of @pix, and the sign bit (if any) is ignored.
 *          * For d == 1, @val > 0 sets the bit on.
 *          * For d == 2, 4, 8 and 16, @val is masked to the maximum allowable
 *            pixel value, and any (invalid) higher order bits are discarded.
 *      (2) See pixGetPixel() for information on performance.
 */
LEPTONICA_EXPORT l_int32
pixSetPixel(PIX      *pix,
            l_int32   x,
            l_int32   y,
            l_uint32  val)
{
l_int32    w, h, d, wpl;
l_uint32  *line, *data;

    PROCNAME("pixSetPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        if (val)
            SET_DATA_BIT(line, x);
        else
            CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        line[x] = val;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                         Set pad bits                        *
 *-------------------------------------------------------------*/
/*!
 *  pixSetPadBits()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              val  (0 or 1)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The pad bits are the bits that expand each scanline to a
 *          multiple of 32 bits.  They are usually not used in
 *          image processing operations.  When boundary conditions
 *          are important, as in seedfill, they must be set properly.
 *      (2) This sets the value of the pad bits (if any) in the last
 *          32-bit word in each scanline.
 *      (3) For 32 bpp pix, there are no pad bits, so this is a no-op.
 */
LEPTONICA_REAL_EXPORT l_int32
pixSetPadBits(PIX     *pix,
              l_int32  val)
{
l_int32    i, w, h, d, wpl, endbits, fullwords;
l_uint32   mask;
l_uint32  *data, *pword;

    PROCNAME("pixSetPadBits");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)  /* no padding exists for 32 bpp */
        return 0;  

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    endbits = 32 - ((w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = w * d / 32;

    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = 0; i < h; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }

    return 0;
}

/*-------------------------------------------------------------*
 *                Color sample setting and extraction          *
 *-------------------------------------------------------------*/

/*!
 *  composeRGBPixel()
 *
 *      Input:  rval, gval, bval
 *              &rgbpixel  (<return> 32-bit pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) A slower implementation uses macros:
 *            SET_DATA_BYTE(ppixel, COLOR_RED, rval);
 *            SET_DATA_BYTE(ppixel, COLOR_GREEN, gval);
 *            SET_DATA_BYTE(ppixel, COLOR_BLUE, bval);
 */
LEPTONICA_EXPORT l_int32
composeRGBPixel(l_int32    rval,
                l_int32    gval,
                l_int32    bval,
                l_uint32  *ppixel)
{
    PROCNAME("composeRGBPixel");

    if (!ppixel)
        return ERROR_INT("&pixel not defined", procName, 1);

    *ppixel = (rval << L_RED_SHIFT) | (gval << L_GREEN_SHIFT) |
              (bval << L_BLUE_SHIFT);
    return 0;
}
