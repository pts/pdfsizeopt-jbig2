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
 *  pnmio.c
 *
 *      Stream interface
 *          PIX             *pixReadStreamPnm()
 *          l_int32          readHeaderPnm()
 *          l_int32          freadHeaderPnm()
 *          l_int32          pixWriteStreamPnm()
 *          l_int32          pixWriteStreamAsciiPnm()
 *
 *      Read/write to memory   [not on windows]
 *          PIX             *pixReadMemPnm()
 *          l_int32          sreadHeaderPnm()
 *          l_int32          pixWriteMemPnm()
 *
 *      Local helpers
 *          static l_int32   pnmReadNextAsciiValue();
 *          static l_int32   pnmSkipCommentLines();
 *       
 *      These are here by popular demand, with the help of Mattias
 *      Kregert (mattias@kregert.se), who provided the first implementation.
 *
 *      The pnm formats are exceedingly simple, because they have
 *      no compression and no colormaps.  They support images that
 *      are 1 bpp; 2, 4, 8 and 16 bpp grayscale; and rgb.
 *
 *      The original pnm formats ("ascii") are included for completeness,
 *      but their use is deprecated for all but tiny iconic images.
 *      They are extremely wasteful of memory; for example, the P1 binary
 *      ascii format is 16 times as big as the packed uncompressed
 *      format, because 2 characters are used to represent every bit
 *      (pixel) in the image.  Reading is slow because we check for extra
 *      white space and EOL at every sample value.
 * 
 *      The packed pnm formats ("raw") give file sizes similar to
 *      bmp files, which are uncompressed packed.  However, bmp
 *      are more flexible, because they can support colormaps.
 *
 *      We don't differentiate between the different types ("pbm",
 *      "pgm", "ppm") at the interface level, because this is really a
 *      "distinction without a difference."  You read a file, you get
 *      the appropriate Pix.  You write a file from a Pix, you get the
 *      appropriate type of file.  If there is a colormap on the Pix,
 *      and the Pix is more than 1 bpp, you get either an 8 bpp pgm
 *      or a 24 bpp RGB pnm, depending on whether the colormap colors
 *      are gray or rgb, respectively.
 *
 *      This follows the general policy that the I/O routines don't
 *      make decisions about the content of the image -- you do that
 *      with image processing before you write it out to file.
 *      The I/O routines just try to make the closest connection
 *      possible between the file and the Pix in memory.
 */

#include <string.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  USE_PNMIO   /* defined in environ.h */
/* --------------------------------------------*/


static l_int32 pnmReadNextAsciiValue(FILE  *fp, l_int32 *pval);
static l_int32 pnmSkipCommentLines(FILE  *fp);

    /* a sanity check on the size read from file */
static const l_int32  MAX_PNM_WIDTH = 100000;
static const l_int32  MAX_PNM_HEIGHT = 100000;


/*--------------------------------------------------------------------*
 *                          Stream interface                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixReadStreamPnm()
 *
 *      Input:  stream opened for read
 *      Return: pix, or null on error
 */
LEPTONICA_EXPORT PIX *
pixReadStreamPnm(FILE  *fp)
{
l_uint8    val8, rval8, gval8, bval8;
l_uint16   val16;
l_int32    w, h, d, bpl, wpl, i, j, type;
l_int32    val, rval, gval, bval;
l_uint32   rgbval;
l_uint32  *line, *data;
PIX       *pix;

    PROCNAME("pixReadStreamPnm");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);

    if (freadHeaderPnm(fp, &pix, &w, &h, &d, &type, NULL, NULL))
        return (PIX *)ERROR_PTR( "pix not made", procName, NULL);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);

        /* Old "ascii" format */
    if (type <= 3) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (type == 1 || type == 2) {
                    if (pnmReadNextAsciiValue(fp, &val))
                        return (PIX *)ERROR_PTR( "read abend", procName, pix);
                    pixSetPixel(pix, j, i, val);
                }
                else {  /* type == 3 */
                    if (pnmReadNextAsciiValue(fp, &rval))
                        return (PIX *)ERROR_PTR( "read abend", procName, pix);
                    if (pnmReadNextAsciiValue(fp, &gval))
                        return (PIX *)ERROR_PTR( "read abend", procName, pix);
                    if (pnmReadNextAsciiValue(fp, &bval))
                        return (PIX *)ERROR_PTR( "read abend", procName, pix);
                    composeRGBPixel(rval, gval, bval, &rgbval);
                    pixSetPixel(pix, j, i, rgbval);
                }
            }
        }
        return pix;
    }

        /* "raw" format for 1 bpp */
    if (type == 4) {
        bpl = (d * w + 7) / 8;
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < bpl; j++) {
                if (fread(&val8, 1, 1, fp) != 1)
                    return (PIX *)ERROR_PTR( "read error in 4", procName, pix);
                SET_DATA_BYTE(line, j, val8);
            }
        }
        return pix;
    }

        /* "raw" format for grayscale */
    if (type == 5) {
        bpl = (d * w + 7) / 8;
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            if (d != 16) {
                for (j = 0; j < w; j++) {
                    if (fread(&val8, 1, 1, fp) != 1)
                        return (PIX *)ERROR_PTR( "error in 5", procName, pix);
                    if (d == 2)
                        SET_DATA_DIBIT(line, j, val8);
                    else if (d == 4)
                        SET_DATA_QBIT(line, j, val8);
                    else  /* d == 8 */
                        SET_DATA_BYTE(line, j, val8);
                }
            }
            else {  /* d == 16 */
                for (j = 0; j < w; j++) {
                    if (fread(&val16, 2, 1, fp) != 1)
                        return (PIX *)ERROR_PTR( "16 bpp error", procName, pix);
                    SET_DATA_TWO_BYTES(line, j, val16);
                }
            }
        }
        return pix;
    }

        /* "raw" format, type == 6; rgb */
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < wpl; j++) {
            if (fread(&rval8, 1, 1, fp) != 1)
                return (PIX *)ERROR_PTR( "read error type 6", procName, pix);
            if (fread(&gval8, 1, 1, fp) != 1)
                return (PIX *)ERROR_PTR( "read error type 6", procName, pix);
            if (fread(&bval8, 1, 1, fp) != 1)
                return (PIX *)ERROR_PTR( "read error type 6", procName, pix);
            composeRGBPixel(rval8, gval8, bval8, &rgbval);
            line[j] = rgbval;
        }
    }
    return pix;
}


/*!
 *  freadHeaderPnm()
 *
 *      Input:  stream opened for read
 *              &pix (<optional return> use null to return only header data)
 *              &width (<return>)
 *              &height (<return>)
 *              &depth (<return>)
 *              &type (<return> pnm type)
 *              &bps (<optional return>, bits/sample)
 *              &spp (<optional return>, samples/pixel)
 *      Return: 0 if OK, 1 on error
 */
LEPTONICA_EXPORT l_int32
freadHeaderPnm(FILE     *fp,
               PIX     **ppix,
               l_int32  *pwidth,
               l_int32  *pheight,
               l_int32  *pdepth,
               l_int32  *ptype,
               l_int32  *pbps,
               l_int32  *pspp)
{
l_int32  w, h, d, type;
l_int32  maxval;

    PROCNAME("freadHeaderPnm");

    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (!pwidth || !pheight || !pdepth || !ptype)
        return ERROR_INT("input ptr(s) not defined", procName, 1);

    if (fscanf(fp, "P%d\n", &type) != 1)
        return ERROR_INT("invalid read for type", procName, 1);
    if (type < 1 || type > 6)
        return ERROR_INT("invalid pnm file", procName, 1);

    if (pnmSkipCommentLines(fp))
        return ERROR_INT("no data in file", procName, 1);

    if (fscanf(fp, "%d %d\n", &w, &h) != 2)
        return ERROR_INT("invalid read for w,h", procName, 1);
    if (w <= 0 || h <= 0 || w > MAX_PNM_WIDTH || h > MAX_PNM_HEIGHT)
        return ERROR_INT("invalid sizes", procName, 1);

        /* Get depth of pix */
    if (type == 1 || type == 4)
        d = 1;
    else if (type == 2 || type == 5) {
        if (fscanf(fp, "%d\n", &maxval) != 1)
            return ERROR_INT("invalid read for maxval (2,5)", procName, 1);
        if (maxval == 3)
            d = 2;
        else if (maxval == 15)
            d = 4;
        else if (maxval == 255)
            d = 8;
        else if (maxval == 0xffff)
            d = 16;
        else {
            fprintf(stderr, "maxval = %d\n", maxval);
            return ERROR_INT("invalid maxval", procName, 1);
        }
    }
    else {  /* type == 3 || type == 6; this is rgb  */
        if (fscanf(fp, "%d\n", &maxval) != 1)
            return ERROR_INT("invalid read for maxval (3,6)", procName, 1);
        if (maxval != 255)
            L_WARNING_INT("unexpected maxval = %d", procName, maxval);
        d = 32;
    }
    *pwidth = w;
    *pheight = h;
    *pdepth = d;
    *ptype = type;
    if (pbps) *pbps = (d == 32) ? 8 : d;
    if (pspp) *pspp = (d == 32) ? 3 : 1;
    
    if (!ppix)
        return 0;

    if ((*ppix = pixCreate(w, h, d)) == NULL)  /* return pix initialized to 0 */
        return ERROR_INT( "pix not made", procName, 1);
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/*--------------------------------------------------------------------*
 *                          Static helpers                            *
 *--------------------------------------------------------------------*/
/*!
 *  pnmReadNextAsciiValue()
 *
 *      Return: 0 if OK, 1 on error or EOF.
 *
 *  Notes:
 *      (1) This reads the next sample value in ascii from the the file.
 */
static l_int32
pnmReadNextAsciiValue(FILE  *fp,
                      l_int32 *pval)
{
l_int32   c, ignore;

    PROCNAME("pnmReadNextAsciiValue");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    do {  /* skip whitespace */
        if ((c = fgetc(fp)) == EOF)
            return 1;
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    fseek(fp, -1L, SEEK_CUR);        /* back up one byte */
    ignore = fscanf(fp, "%d", pval);
    return 0;
}


/*!
 *  pnmSkipCommentLines()
 *
 *      Return: 0 if OK, 1 on error or EOF
 *
 *  Notes:
 *      (1) Comment lines begin with '#'
 *      (2) Usage: caller should check return value for EOF
 */
static l_int32 
pnmSkipCommentLines(FILE  *fp)
{
l_int32  c;

    PROCNAME("pnmSkipCommentLines");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if ((c = fgetc(fp)) == EOF)
        return 1;
    if (c == '#') {
        do {  /* each line starting with '#' */
            do {  /* this entire line */
                if ((c = fgetc(fp)) == EOF)
                    return 1;
            } while (c != '\n');
            if ((c = fgetc(fp)) == EOF)
                return 1;
        } while (c == '#');
    }

        /* Back up one byte */
    fseek(fp, -1L, SEEK_CUR);
    return 0;
}

/* --------------------------------------------*/
#endif  /* USE_PNMIO */
/* --------------------------------------------*/
