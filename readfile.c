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
 *  readfile.c:  reads image on file into memory
 *
 *      Top-level functions for reading images from file
 *           PIXA      *pixaReadFiles()
 *           PIXA      *pixaReadFilesSA()
 *           PIX       *pixRead()
 *           PIX       *pixReadWithHint()
 *           PIX       *pixReadIndexed()
 *           PIX       *pixReadStream()
 *
 *      Read header information from file
 *           l_int32    pixReadHeader()
 *
 *      Format finders
 *           l_int32    findFileFormat()
 *           l_int32    findFileFormatStream()
 *           l_int32    findFileFormatBuffer()
 *           l_int32    fileFormatIsTiff()
 *
 *      Read from memory
 *           PIX       *pixReadMem()
 *           l_int32    pixReadHeaderMem()
 *
 *      Test function for I/O with different formats 
 *           l_int32    ioFormatTest()
 */

#include <string.h>
#include "allheaders.h"

    /*  choose type of PIX to be generated  */
enum {
    READ_24_BIT_COLOR = 0,     /* read in as 24 (really 32) bit pix */
    CONVERT_TO_PALETTE = 1,    /* convert to 8 bit colormapped pix */
    READ_GRAY = 2              /* read gray only */
};

    /* I found these from the source code to the unix file */
    /* command. man 1 file */
static const char JP2K_CODESTREAM[4] = { 0xff, 0x4f, 0xff, 0x51 };
static const char JP2K_IMAGE_DATA[12] = { 0x00, 0x00, 0x00, 0x0C,
                                          0x6A, 0x50, 0x20, 0x20,
                                          0x0D, 0x0A, 0x87, 0x0A };

/*---------------------------------------------------------------------*
 *          Top-level functions for reading images from file           *
 *---------------------------------------------------------------------*/

/*!
 *  pixRead()
 *
 *      Input:  filename (with full pathname or in local directory)
 *      Return: pix if OK; null on error
 */
LEPTONICA_REAL_EXPORT PIX *
pixRead(const char  *filename)
{
FILE  *fp;
PIX   *pix;

    PROCNAME("pixRead");

    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    if ((pix = pixReadStream(fp, 0)) == NULL) {
        fclose(fp);
        return (PIX *)ERROR_PTR("pix not read", procName, NULL);
    }

        /* Close the stream except if GIF under windows, because
         * DGifCloseFile() closes the windows file stream! */
    if (pixGetInputFormat(pix) != IFF_GIF)
        fclose(fp);
#ifndef _WIN32
    else  /* gif file */
        fclose(fp);
#endif  /* ! _WIN32 */

    return pix;
}


/*!
 *  pixReadStream()
 *
 *      Input:  fp (file stream)
 *              hint (bitwise OR of L_HINT_* values for jpeg; use 0 for no hint)
 *      Return: pix if OK; null on error
 *
 *  Notes:
 *      (1) The hint only applies to jpeg.
 */
LEPTONICA_EXPORT PIX *
pixReadStream(FILE    *fp,
              l_int32  hint)
{
l_int32  format;
PIX     *pix;

    PROCNAME("pixReadStream");

    if (!fp)
        return (PIX *)ERROR_PTR("stream not defined", procName, NULL);
    pix = NULL;

    findFileFormatStream(fp, &format);
    switch (format)
    {
#if USE_BMPIO
    case IFF_BMP:
        if ((pix = pixReadStreamBmp(fp)) == NULL )
            return (PIX *)ERROR_PTR( "bmp: no pix returned", procName, NULL);
        break;
#endif

#if HAVE_LIBJPEG
    case IFF_JFIF_JPEG:
        if ((pix = pixReadStreamJpeg(fp, READ_24_BIT_COLOR, 1, NULL, hint))
                == NULL)
            return (PIX *)ERROR_PTR( "jpeg: no pix returned", procName, NULL);
        break;
#endif

    case IFF_PNG:
        if ((pix = pixReadStreamPng(fp)) == NULL)
            return (PIX *)ERROR_PTR("png: no pix returned", procName, NULL);
        break;

#if HAVE_LIBTIFF
    case IFF_TIFF:
    case IFF_TIFF_PACKBITS:
    case IFF_TIFF_RLE:
    case IFF_TIFF_G3:
    case IFF_TIFF_G4:
    case IFF_TIFF_LZW:
    case IFF_TIFF_ZIP:
        if ((pix = pixReadStreamTiff(fp, 0)) == NULL)  /* page 0 by default */
            return (PIX *)ERROR_PTR("tiff: no pix returned", procName, NULL);
        break;
#endif

    case IFF_PNM:
        if ((pix = pixReadStreamPnm(fp)) == NULL)
            return (PIX *)ERROR_PTR("pnm: no pix returned", procName, NULL);
        break;

#if HAVE_LIBGIF
    case IFF_GIF:
        if ((pix = pixReadStreamGif(fp)) == NULL)
            return (PIX *)ERROR_PTR("gif: no pix returned", procName, NULL);
        break;
#endif

#if USE_JP2IO
    case IFF_JP2:
        return (PIX *)ERROR_PTR("jp2: format not supported", procName, NULL);
        break;
#endif

#if HAVE_LIBWEBP
    case IFF_WEBP:
        if ((pix = pixReadStreamWebP(fp)) == NULL)
            return (PIX *)ERROR_PTR("webp: no pix returned", procName, NULL);
        break;
#endif

#if USE_SPIXIO
    case IFF_SPIX:
        if ((pix = pixReadStreamSpix(fp)) == NULL)
            return (PIX *)ERROR_PTR("spix: no pix returned", procName, NULL);
        break;
#endif

    case IFF_UNKNOWN:
    default:
        return (PIX *)ERROR_PTR( "Unknown format: no pix returned",
                procName, NULL);
        break;
    }

    if (pix)
        pixSetInputFormat(pix, format);
    return pix;
}



/*---------------------------------------------------------------------*
 *                            Format finders                           *
 *---------------------------------------------------------------------*/


/*!
 *  findFileFormatStream()
 *
 *      Input:  fp (file stream)
 *              &format (<return>)
 *      Return: 0 if OK, 1 on error or if format is not recognized
 *
 *  Notes:
 *      (1) Important: Side effect -- this resets fp to BOF.
 */
LEPTONICA_REAL_EXPORT l_int32
findFileFormatStream(FILE     *fp,
                     l_int32  *pformat)
{
l_uint8  firstbytes[12];
l_int32  format;

    PROCNAME("findFileFormatStream");

    if (!pformat)
        return ERROR_INT("&format not defined", procName, 1);
    *pformat = IFF_UNKNOWN;
    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);

    rewind(fp);
    if (fnbytesInFile(fp) < 12)
        return ERROR_INT("truncated file", procName, 1);

    if (fread((char *)&firstbytes, 1, 12, fp) != 12)
        return ERROR_INT("failed to read first 12 bytes of file", procName, 1);
    rewind(fp);

    findFileFormatBuffer(firstbytes, &format);
#if HAVE_LIBTIFF
    if (format == IFF_TIFF) {
        findTiffCompression(fp, &format);
        rewind(fp);
    }
#endif
    *pformat = format;
    if (format == IFF_UNKNOWN)
        return 1;
    else
        return 0;
}


/*!
 *  findFileFormatBuffer()
 *
 *      Input:  byte buffer (at least 12 bytes in size; we can't check)
 *              &format (<return>)
 *      Return: 0 if OK, 1 on error or if format is not recognized
 *
 *  Notes:
 *      (1) This determines the file format from the first 12 bytes in
 *          the compressed data stream, which are stored in memory.
 *      (2) For tiff files, this returns IFF_TIFF.  The specific tiff
 *          compression is then determined using findTiffCompression().
 */
LEPTONICA_EXPORT l_int32
findFileFormatBuffer(const l_uint8  *buf,
                     l_int32        *pformat)
{
l_uint16  twobytepw;

    PROCNAME("findFileFormatBuffer");

    if (!pformat)
        return ERROR_INT("&format not defined", procName, 1);
    *pformat = IFF_UNKNOWN;
    if (!buf)
        return ERROR_INT("byte buffer not defined", procName, 0);

        /* Check the bmp and tiff 2-byte header ids */
    ((char *)(&twobytepw))[0] = buf[0];
    ((char *)(&twobytepw))[1] = buf[1];

    if (convertOnBigEnd16(twobytepw) == BMP_ID) {
        *pformat = IFF_BMP;
        return 0;
    }

    if (twobytepw == TIFF_BIGEND_ID || twobytepw == TIFF_LITTLEEND_ID) {
        *pformat = IFF_TIFF;
        return 0;
    }

        /* Check for the p*m 2-byte header ids */
    if ((buf[0] == 'P' && buf[1] == '4') || /* newer packed */
        (buf[0] == 'P' && buf[1] == '1')) {  /* old format */
        *pformat = IFF_PNM;
        return 0;
    }

    if ((buf[0] == 'P' && buf[1] == '5') || /* newer */
        (buf[0] == 'P' && buf[1] == '2')) {  /* old */
        *pformat = IFF_PNM;
        return 0;
    }

    if ((buf[0] == 'P' && buf[1] == '6') || /* newer */
        (buf[0] == 'P' && buf[1] == '3')) {  /* old */
        *pformat = IFF_PNM;
        return 0;
    }

        /*  Consider the first 11 bytes of the standard JFIF JPEG header:
         *    - The first two bytes are the most important:  0xffd8.
         *    - The next two bytes are the jfif marker: 0xffe0.
         *      Not all jpeg files have this marker.
         *    - The next two bytes are the header length.
         *    - The next 5 bytes are a null-terminated string.
         *      For JFIF, the string is "JFIF", naturally.  For others it
         *      can be "Exif" or just about anything else.
         *    - Because of all this variability, we only check the first
         *      two byte marker.  All jpeg files are identified as
         *      IFF_JFIF_JPEG.  */
    if (buf[0] == 0xff && buf[1] == 0xd8) {
        *pformat = IFF_JFIF_JPEG;
        return 0;
    }

        /* Check for the 8 byte PNG signature (png_signature in png.c):
         *       {137, 80, 78, 71, 13, 10, 26, 10}      */
    if (buf[0] == 137 && buf[1] == 80  && buf[2] == 78  && buf[3] == 71  &&
        buf[4] == 13  && buf[5] == 10  && buf[6] == 26  && buf[7] == 10) {
        *pformat = IFF_PNG;
        return 0;
    }

        /* Look for "GIF87a" or "GIF89a" */
    if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == '8' &&
        (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a') {
        *pformat = IFF_GIF;
        return 0;
    }

        /* Check for both types of jp2k file */
    if (strncmp((const char *)buf, JP2K_CODESTREAM, 4) == 0 ||
        strncmp((const char *)buf, JP2K_IMAGE_DATA, 12) == 0) {
        *pformat = IFF_JP2;
        return 0;
    }

        /* Check for webp */
    if (buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F' &&
        buf[8] == 'W' && buf[9] == 'E' && buf[10] == 'B' && buf[11] == 'P') {
        *pformat = IFF_WEBP;
        return 0;
    }

        /* Check for "spix" serialized pix */
    if (buf[0] == 's' && buf[1] == 'p' && buf[2] == 'i' && buf[3] == 'x') {
        *pformat = IFF_SPIX;
        return 0;
    }

        /* File format identifier not found; unknown */
    return 1;
}


/*---------------------------------------------------------------------*
 *             Test function for I/O with different formats            *
 *---------------------------------------------------------------------*/
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */
