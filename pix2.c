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

    /* This is a global that determines the default 8 bpp alpha mask values
     * for rings at distance 1 and 2 from the border.  Declare extern
     * to use.  To change the values, use l_setAlphaMaskBorder(). */
LEPT_DLL l_float32  AlphaMaskBorderVals[2] = {0.0, 0.5};


#ifndef  NO_CONSOLE_IO
#define  DEBUG_SERIALIZE        0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                         Pixel poking                        *
 *-------------------------------------------------------------*/
/*!
 *  pixGetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This returns the value in the data array.  If the pix is
 *          colormapped, it returns the colormap index, not the rgb value.
 *      (2) Because of the function overhead and the parameter checking,
 *          this is much slower than using the GET_DATA_*() macros directly.
 *          Speed on a 1 Mpixel RGB image, using a 3 GHz machine:
 *            * pixGet/pixSet: ~25 Mpix/sec
 *            * GET_DATA/SET_DATA: ~350 MPix/sec
 *          If speed is important and you're doing random access into
 *          the pix, use pixGetLinePtrs() and the array access macros.
 */
LEPTONICA_EXPORT l_int32
pixGetPixel(PIX       *pix,
            l_int32    x,
            l_int32    y,
            l_uint32  *pval)
{
l_int32    w, h, d, wpl, val;
l_uint32  *line, *data;

    PROCNAME("pixGetPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        val = GET_DATA_BIT(line, x);
        break;
    case 2:
        val = GET_DATA_DIBIT(line, x);
        break;
    case 4:
        val = GET_DATA_QBIT(line, x);
        break;
    case 8:
        val = GET_DATA_BYTE(line, x);
        break;
    case 16:
        val = GET_DATA_TWO_BYTES(line, x);
        break;
    case 32:
        val = line[x];
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    *pval = val;
    return 0;
}


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
 *     Full image clear/set/set-to-arbitrary-value/invert      *
 *-------------------------------------------------------------*/
/*!
 *  pixClearAll()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Clears all data to 0.  For 1 bpp, this is white; for grayscale
 *          or color, this is black.
 *      (2) Caution: for colormapped pix, this sets the color to the first
 *          one in the colormap.  Be sure that this is the intended color!
 */
LEPTONICA_EXPORT l_int32
pixClearAll(PIX  *pix)
{
    PROCNAME("pixClearAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
        
    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_CLR, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAll()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Sets all data to 1.  For 1 bpp, this is black; for grayscale
 *          or color, this is white.
 *      (2) Caution: for colormapped pix, this sets the pixel value to the
 *          maximum value supported by the colormap: 2^d - 1.  However, this
 *          color may not be defined, because the colormap may not be full.
 */
LEPTONICA_EXPORT l_int32
pixSetAll(PIX  *pix)
{
l_int32   n;
PIXCMAP  *cmap;

    PROCNAME("pixSetAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (n < cmap->nalloc)  /* cmap is not full */
            return ERROR_INT("cmap entry does not exist", procName, 1);
    }

    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_SET, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAllArbitrary()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *              val  (value to set all pixels)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) For colormapped pix, be sure the value is the intended
 *          one in the colormap.
 *      (2) Caution: for colormapped pix, this sets each pixel to the
 *          color at the index equal to val.  Be sure that this index
 *          exists in the colormap and that it is the intended one!
 */
LEPTONICA_EXPORT l_int32
pixSetAllArbitrary(PIX      *pix,
                   l_uint32  val)
{
l_int32    n, i, j, w, h, d, wpl, npix;
l_uint32   maxval, wordval;
l_uint32  *data, *line;
PIXCMAP   *cmap;

    PROCNAME("pixSetAllArbitrary");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (val >= n) {
            L_WARNING("index not in colormap; using last color", procName);
            val = n - 1;
        }
    }

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)
        maxval = 0xffffffff;
    else
        maxval = (1 << d) - 1;
    if (val > maxval) {
        L_WARNING_INT("invalid pixel val; set to maxval = %d",
                      procName, maxval);
        val = maxval;
    }

        /* Set up word to tile with */
    wordval = 0;
    npix = 32 / d;    /* number of pixels per 32 bit word */
    for (j = 0; j < npix; j++)
        wordval |= (val << (j * d));

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < wpl; j++) {
            *(line + j) = wordval;
        }
    }

    return 0;
}


/*-------------------------------------------------------------*
 *     Rectangular region clear/set/set-to-arbitrary-value     *
 *-------------------------------------------------------------*/
/*!
 *  pixClearInRect()
 *
 *      Input:  pix (all depths; can be cmapped)
 *              box (in which all pixels will be cleared)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Clears all data in rect to 0.  For 1 bpp, this is white;
 *          for grayscale or color, this is black.
 *      (2) Caution: for colormapped pix, this sets the color to the first
 *          one in the colormap.  Be sure that this is the intended color!
 */
LEPTONICA_EXPORT l_int32
pixClearInRect(PIX  *pix,
               BOX  *box)
{
l_int32  x, y, w, h;

    PROCNAME("pixClearInRect");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    boxGetGeometry(box, &x, &y, &w, &h);
    pixRasterop(pix, x, y, w, h, PIX_CLR, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetInRect()
 *
 *      Input:  pix (all depths, can be cmapped)
 *              box (in which all pixels will be set)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Sets all data in rect to 1.  For 1 bpp, this is black;
 *          for grayscale or color, this is white.
 *      (2) Caution: for colormapped pix, this sets the pixel value to the
 *          maximum value supported by the colormap: 2^d - 1.  However, this
 *          color may not be defined, because the colormap may not be full.
 */
LEPTONICA_EXPORT l_int32
pixSetInRect(PIX  *pix,
             BOX  *box)
{
l_int32   n, x, y, w, h;
PIXCMAP  *cmap;

    PROCNAME("pixSetInRect");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (n < cmap->nalloc)  /* cmap is not full */
            return ERROR_INT("cmap entry does not exist", procName, 1);
    }

    boxGetGeometry(box, &x, &y, &w, &h);
    pixRasterop(pix, x, y, w, h, PIX_SET, NULL, 0, 0);
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
 *                     Add and remove border                   *
 *-------------------------------------------------------------*/
/*!
 *  pixAddBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              npix (number of pixels to be added to each side)
 *              val  (value of added border pixels)
 *      Return: pixd (with the added exterior pixels), or null on error
 *
 *  Notes:
 *      (1) See pixAddBorderGeneral() for values of white & black pixels.
 */
LEPTONICA_EXPORT PIX *
pixAddBorder(PIX      *pixs,
             l_int32   npix,
             l_uint32  val)
{
    PROCNAME("pixAddBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (npix == 0)
        return pixClone(pixs);
    return pixAddBorderGeneral(pixs, npix, npix, npix, npix, val);
}


/*!
 *  pixAddBorderGeneral()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot  (number of pixels added)
 *              val   (value of added border pixels)
 *      Return: pixd (with the added exterior pixels), or null on error
 *
 *  Notes:
 *      (1) For binary images:
 *             white:  val = 0
 *             black:  val = 1
 *          For grayscale images:
 *             white:  val = 2 ** d - 1
 *             black:  val = 0
 *          For rgb color images:
 *             white:  val = 0xffffff00
 *             black:  val = 0
 *          For colormapped images, use 'index' found this way:
 *             white: pixcmapGetRankIntensity(cmap, 1.0, &index);
 *             black: pixcmapGetRankIntensity(cmap, 0.0, &index);
 */
LEPTONICA_EXPORT PIX *
pixAddBorderGeneral(PIX      *pixs,
                    l_int32   left,
                    l_int32   right,
                    l_int32   top,
                    l_int32   bot,
                    l_uint32  val)
{
l_int32  ws, hs, wd, hd, d, op;
PIX     *pixd;

    PROCNAME("pixAddBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (left < 0 || right < 0 || top < 0 || bot < 0)
        return (PIX *)ERROR_PTR("negative border added!", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws + left + right;
    hd = hs + top + bot;
    if ((pixd = pixCreateNoInit(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

        /* Set the new border pixels */
    op = UNDEF;
    if (val == 0)
        op = PIX_CLR;
    else if ((d == 1 && val == 1) || (d == 2 && val == 3) ||
             (d == 4 && val == 0xf) || (d == 8 && val == 0xff) || 
             (d == 16 && val == 0xffff) || (d == 32 && (val >> 8) == 0xffffff))
        op = PIX_SET;
    if (op == UNDEF)
        pixSetAllArbitrary(pixd, val);   /* a little extra writing ! */
    else {
        pixRasterop(pixd, 0, 0, left, hd, op, NULL, 0, 0); 
        pixRasterop(pixd, wd - right, 0, right, hd, op, NULL, 0, 0); 
        pixRasterop(pixd, 0, 0, wd, top, op, NULL, 0, 0); 
        pixRasterop(pixd, 0, hd - bot, wd, bot, op, NULL, 0, 0); 
    }

        /* Copy pixs into the interior */
    pixRasterop(pixd, left, top, ws, hs, PIX_SRC, pixs, 0, 0);
    return pixd;
}


/*!
 *  pixRemoveBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              npix (number to be removed from each of the 4 sides)
 *      Return: pixd (with pixels removed around border), or null on error
 */
LEPTONICA_REAL_EXPORT PIX *
pixRemoveBorder(PIX     *pixs,
                l_int32  npix)
{
    PROCNAME("pixRemoveBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (npix == 0)
        return pixClone(pixs);
    return pixRemoveBorderGeneral(pixs, npix, npix, npix, npix);
}


/*!
 *  pixRemoveBorderGeneral()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot  (number of pixels added)
 *      Return: pixd (with pixels removed around border), or null on error
 */
LEPTONICA_EXPORT PIX *
pixRemoveBorderGeneral(PIX     *pixs,
                       l_int32  left,
                       l_int32  right,
                       l_int32  top,
                       l_int32  bot)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixRemoveBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (left < 0 || right < 0 || top < 0 || bot < 0)
        return (PIX *)ERROR_PTR("negative border removed!", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws - left - right;
    hd = hs - top - bot;
    if (wd <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (hd <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);
    if ((pixd = pixCreateNoInit(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixs, left, top);
    return pixd;
}


/*-------------------------------------------------------------*
 *                Color sample setting and extraction          *
 *-------------------------------------------------------------*/
/*!
 *  pixCreateRGBImage()
 *
 *      Input:  8 bpp red pix
 *              8 bpp green pix
 *              8 bpp blue pix
 *      Return: 32 bpp pix, interleaved with 4 samples/pixel,
 *              or null on error
 *
 *  Notes:
 *      (1) the 4th byte, sometimes called the "alpha channel",
 *          and which is often used for blending between different
 *          images, is left with 0 value.
 *      (2) see Note (4) in pix.h for details on storage of
 *          8-bit samples within each 32-bit word.
 *      (3) This implementation, setting the r, g and b components
 *          sequentially, is much faster than setting them in parallel
 *          by constructing an RGB dest pixel and writing it to dest.
 *          The reason is there are many more cache misses when reading
 *          from 3 input images simultaneously.
 */
LEPTONICA_EXPORT PIX *
pixCreateRGBImage(PIX  *pixr,
                  PIX  *pixg,
                  PIX  *pixb)
{
l_int32  wr, wg, wb, hr, hg, hb, dr, dg, db;
PIX     *pixd;

    PROCNAME("pixCreateRGBImage");

    if (!pixr)
        return (PIX *)ERROR_PTR("pixr not defined", procName, NULL);
    if (!pixg)
        return (PIX *)ERROR_PTR("pixg not defined", procName, NULL);
    if (!pixb)
        return (PIX *)ERROR_PTR("pixb not defined", procName, NULL);
    pixGetDimensions(pixr, &wr, &hr, &dr);
    pixGetDimensions(pixg, &wg, &hg, &dg);
    pixGetDimensions(pixb, &wb, &hb, &db);
    if (dr != 8 || dg != 8 || db != 8)
        return (PIX *)ERROR_PTR("input pix not all 8 bpp", procName, NULL);
    if (wr != wg || wr != wb)
        return (PIX *)ERROR_PTR("widths not the same", procName, NULL);
    if (hr != hg || hr != hb)
        return (PIX *)ERROR_PTR("heights not the same", procName, NULL);

    if ((pixd = pixCreate(wr, hr, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixr);
    pixSetRGBComponent(pixd, pixr, COLOR_RED);
    pixSetRGBComponent(pixd, pixg, COLOR_GREEN);
    pixSetRGBComponent(pixd, pixb, COLOR_BLUE);

    return pixd;
}


/*!
 *  pixGetRGBComponent()
 *
 *      Input:  pixs  (32 bpp)
 *              color  (one of {COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 *                      L_ALPHA_CHANNEL})
 *      Return: pixd, the selected 8 bpp component image of the
 *              input 32 bpp image, or null on error
 *
 *  Notes:
 *      (1) The alpha channel (in the 4th byte of each RGB pixel)
 *          is mostly ignored in leptonica.
 *      (2) Three calls to this function generate the three 8 bpp component
 *          images.  This is much faster than generating the three
 *          images in parallel, by extracting a src pixel and setting
 *          the pixels of each component image from it.  The reason is
 *          there are many more cache misses when writing to three
 *          output images simultaneously.
 */
LEPTONICA_EXPORT PIX *
pixGetRGBComponent(PIX     *pixs,
                   l_int32  color)
{
l_uint8    srcbyte;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;
l_int32    i, j, w, h;
l_int32    wpls, wpld;
PIX           *pixd;

    PROCNAME("pixGetRGBComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (color != COLOR_RED && color != COLOR_GREEN &&
        color != COLOR_BLUE && color != L_ALPHA_CHANNEL)
        return (PIX *)ERROR_PTR("invalid color", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            srcbyte = GET_DATA_BYTE(lines + j, color);
            SET_DATA_BYTE(lined, j, srcbyte);
        }
    }

    return pixd;
}


/*!
 *  pixSetRGBComponent()
 *
 *      Input:  pixd  (32 bpp)
 *              pixs  (8 bpp)
 *              color  (one of {COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 *                      L_ALPHA_CHANNEL})
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This places the 8 bpp pixel in pixs into the
 *          specified color component (properly interleaved) in pixd.
 *      (2) The alpha channel component mostly ignored in leptonica.
 */
LEPTONICA_EXPORT l_int32
pixSetRGBComponent(PIX     *pixd,
                   PIX     *pixs,
                   l_int32  color)
{
l_uint8    srcbyte;
l_int32    i, j, w, h;
l_int32    wpls, wpld;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;

    PROCNAME("pixSetRGBComponent");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not 32 bpp", procName, 1);
    if (pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if (color != COLOR_RED && color != COLOR_GREEN &&
        color != COLOR_BLUE && color != L_ALPHA_CHANNEL)
        return ERROR_INT("invalid color", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
        return ERROR_INT("sizes not commensurate", procName, 1);

    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            srcbyte = GET_DATA_BYTE(lines, j);
            SET_DATA_BYTE(lined + j, color, srcbyte);
        }
    }

    return 0;
}

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


/*!
 *  extractRGBValues()
 *
 *      Input:  pixel (32 bit)
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *      Return: void
 *
 *  Notes:
 *      (1) A slower implementation uses macros:
 *             *prval = GET_DATA_BYTE(&pixel, COLOR_RED);
 *             *pgval = GET_DATA_BYTE(&pixel, COLOR_GREEN);
 *             *pbval = GET_DATA_BYTE(&pixel, COLOR_BLUE);
 */
LEPTONICA_EXPORT void
extractRGBValues(l_uint32  pixel,
                 l_int32  *prval,
                 l_int32  *pgval,
                 l_int32  *pbval)
{
    if (prval) *prval = (pixel >> L_RED_SHIFT) & 0xff;
    if (pgval) *pgval = (pixel >> L_GREEN_SHIFT) & 0xff;
    if (pbval) *pbval = (pixel >> L_BLUE_SHIFT) & 0xff;
    return;
}


/*-------------------------------------------------------------*
 *                    Pixel endian conversion                  *
 *-------------------------------------------------------------*/
/*!
 *  pixEndianByteSwapNew()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is used to convert the data in a pix to a
 *          serialized byte buffer in raster order, and, for RGB,
 *          in order RGBA.  This requires flipping bytes within
 *          each 32-bit word for little-endian platforms, because the
 *          words have a MSB-to-the-left rule, whereas byte raster-order
 *          requires the left-most byte in each word to be byte 0.
 *          For big-endians, no swap is necessary, so this returns a clone.
 *      (2) Unlike pixEndianByteSwap(), which swaps the bytes in-place,
 *          this returns a new pix (or a clone).  We provide this
 *          because often when serialization is done, the source
 *          pix needs to be restored to canonical little-endian order,
 *          and this requires a second byte swap.  In such a situation,
 *          it is twice as fast to make a new pix in big-endian order,
 *          use it, and destroy it.
 */
LEPTONICA_EXPORT PIX *
pixEndianByteSwapNew(PIX  *pixs)
{
l_uint32  *datas, *datad;
l_int32    i, j, h, wpl;
l_uint32   word;
PIX       *pixd;

    PROCNAME("pixEndianByteSwapNew");
        
#ifdef L_BIG_ENDIAN

    return pixClone(pixs);

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, datas++, datad++) {
            word = *datas;
            *datad = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return pixd;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianByteSwap()
 *
 *      Input:  pixs
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap
 *          the bytes within a word; bytes 0 and 3 are swapped,
 *          and bytes 1 and 2 are swapped.
 *      (2) This is required for little-endians in situations
 *          where we convert from a serialized byte order that is
 *          in raster order, as one typically has in file formats,
 *          to one with MSB-to-the-left in each 32-bit word, or v.v.
 *          See pix.h for a description of the canonical format
 *          (MSB-to-the left) that is used for both little-endian
 *          and big-endian platforms.   For big-endians, the
 *          MSB-to-the-left word order has the bytes in raster
 *          order when serialized, so no byte flipping is required.
 */
LEPTONICA_EXPORT l_int32
pixEndianByteSwap(PIX  *pixs)
{
l_uint32  *data;
l_int32    i, j, h, wpl;
l_uint32   word;

    PROCNAME("pixEndianByteSwap");
        
#ifdef L_BIG_ENDIAN

    return 0;

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, data++) {
            word = *data;
            *data = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return 0;

#endif   /* L_BIG_ENDIAN */

}
