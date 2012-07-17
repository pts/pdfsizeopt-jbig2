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
 *  pngio.c
 *                     
 *    Read png from file
 *          PIX        *pixReadStreamPng()
 *          l_int32     readHeaderPng()
 *          l_int32     freadHeaderPng()
 *          l_int32     sreadHeaderPng()
 *          l_int32     fgetPngResolution()
 *
 *    Write png to file
 *          l_int32     pixWritePng()  [ special top level ]
 *          l_int32     pixWriteStreamPng()
 *          
 *    Read and write of png to/from RGBA pix
 *          PIX        *pixReadRGBAPng();
 *          l_int32     pixWriteRGBAPng();
 *
 *    Setting flags for special modes
 *          void        l_pngSetStrip16To8()
 *          void        l_pngSetStripAlpha()
 *          void        l_pngSetWriteAlpha()
 *          void        l_pngSetZlibCompression()
 *
 *    Read/write to memory   [not on windows]
 *          PIX        *pixReadMemPng()
 *          l_int32     pixWriteMemPng()
 *
 *    Documentation: libpng.txt and example.c
 *
 *    On input (decompression from file), palette color images
 *    are read into an 8 bpp Pix with a colormap, and 24 bpp
 *    3 component color images are read into a 32 bpp Pix with
 *    rgb samples.  On output (compression to file), palette color
 *    images are written as 8 bpp with the colormap, and 32 bpp
 *    full color images are written compressed as a 24 bpp,
 *    3 component color image.
 *
 *    In the following, we use these abbreviations:
 *       bps == bit/sample
 *       spp == samples/pixel
 *       bpp == bits/pixel of image in Pix (memory)
 *    where each component is referred to as a "sample".
 *
 *    There are three special flags for determining the number or
 *    size of samples retained or written:
 *    (1) var_PNG_STRIP_16_to_8: default is TRUE.  This strips each
 *        16 bit sample down to 8 bps:
 *         - For 16 bps rgb (16 bps, 3 spp) --> 32 bpp rgb Pix
 *         - For 16 bps gray (16 bps, 1 spp) --> 8 bpp grayscale Pix
 *    (2) var_PNG_STRIP_ALPHA: default is TRUE.  This does not copy
 *        the alpha channel to the pix:
 *         - For 8 bps rgba (8 bps, 4 spp) --> 32 bpp rgb Pix
 *    (3) var_PNG_WRITE_ALPHA: default is FALSE.  The default generates
 *        an RGB png file with 3 spp.  If set to TRUE, this generates
 *        an RGBA png file with 4 spp, and writes the alpha channel.
 *    These are set with accessors.
 *
 *    Two convenience functions are included for reading the alpha
 *    channel (if it exists) into the pix, and for writing out the
 *    alpha sample of a pix to a png file:
 *        pixReadRGBAPng()
 *        pixWriteRGBAPng()
 *    These use two of the special flags, setting to the non-default
 *    value before use and resetting to default afterwards.
 *    In leptonica, we make almost no explicit use of the alpha channel.
 *        
 *    Another special flag, var_ZLIB_COMPRESSION, is used to determine
 *    the compression level.  Default is for standard png compression.
 *    The zlib compression value can be set [0 ... 9], with
 *         0     no compression (huge files)
 *         1     fastest compression
 *         -1    default compression  (equivalent to 6 in latest version)
 *         9     best compression
 *    If not set, we use the default compression in zlib.
 *    Note that if you are using the defined constants in zlib instead
 *    of the compression integers given above, you must include zlib.h.
 *
 *    Note: All special flags use global constants, so if used with
 *          multi-threaded applications, results can be non-deterministic.
 */

#include <string.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

#include "png.h"

/* ----------------Set defaults for read/write options ----------------- */
    /* strip 16 bpp --> 8 bpp on reading png; default is for stripping */
static l_int32   var_PNG_STRIP_16_TO_8 = 1;
    /* strip alpha on reading png; default is for stripping */
static l_int32   var_PNG_STRIP_ALPHA = 1;

#ifndef  NO_CONSOLE_IO
#define  DEBUG     0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *                              Reading png                            *
 *---------------------------------------------------------------------*/
/*!
 *  pixReadStreamPng()
 *
 *      Input:  stream
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) If called from pixReadStream(), the stream is positioned
 *          at the beginning of the file.
 *      (2) To do sequential reads of png format images from a stream,
 *          use pixReadStreamPng()
 */
LEPTONICA_EXPORT PIX *
pixReadStreamPng(FILE  *fp)
{
l_uint8      rval, gval, bval;
l_int32      i, j, k;
l_int32      wpl, d, spp, cindex;
l_uint32     png_transforms;
l_uint32    *data, *line, *ppixel;
int          num_palette, num_text;
png_byte     bit_depth, color_type, channels;
png_uint_32  w, h, rowbytes;
png_uint_32  xres, yres;
png_bytep    rowptr;
png_bytep   *row_pointers;
png_structp  png_ptr;
png_infop    info_ptr, end_info;
png_colorp   palette;
png_textp    text_ptr;  /* ptr to text_chunk */
PIX         *pix;
PIXCMAP     *cmap;

    PROCNAME("pixReadStreamPng");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);
    pix = NULL;

        /* Allocate the 3 data structures */
    if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                   (png_voidp)NULL, NULL, NULL)) == NULL)
        return (PIX *)ERROR_PTR("png_ptr not made", procName, NULL);

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return (PIX *)ERROR_PTR("info_ptr not made", procName, NULL);
    }

    if ((end_info = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return (PIX *)ERROR_PTR("end_info not made", procName, NULL);
    }

        /* Set up png setjmp error handling */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("internal png error", procName, NULL);
    }

    png_init_io(png_ptr, fp);

        /* ---------------------------------------------------------- *
         *  Set the transforms flags.  Whatever happens here,
         *  NEVER invert 1 bpp using PNG_TRANSFORM_INVERT_MONO.
         * ---------------------------------------------------------- */
        /* To strip 16 --> 8 bit depth, use PNG_TRANSFORM_STRIP_16 */
    if (var_PNG_STRIP_16_TO_8 == 1)   /* our default */
        png_transforms = PNG_TRANSFORM_STRIP_16;
    else
        png_transforms = PNG_TRANSFORM_IDENTITY;
        /* To remove alpha channel, use PNG_TRANSFORM_STRIP_ALPHA */
    if (var_PNG_STRIP_ALPHA == 1)   /* our default */
        png_transforms |= PNG_TRANSFORM_STRIP_ALPHA;

        /* Read it */
    png_read_png(png_ptr, info_ptr, png_transforms, NULL);

    row_pointers = png_get_rows(png_ptr, info_ptr);
    w = png_get_image_width(png_ptr, info_ptr);
    h = png_get_image_height(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);
    spp = channels;

    if (spp == 1)
        d = bit_depth;
    else if (spp == 2) {
        d = 2 * bit_depth;
        L_WARNING("there shouldn't be 2 spp!", procName);
    }
    else  /* spp == 3 (rgb), spp == 4 (rgba) */
        d = 4 * bit_depth;

        /* Remove if/when this is implemented for all bit_depths */
    if (spp == 3 && bit_depth != 8) {
        fprintf(stderr, "Help: spp = 3 and depth = %d != 8\n!!", bit_depth);
        return (PIX *)ERROR_PTR("not implemented for this depth",
            procName, NULL);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE ||
        color_type == PNG_COLOR_MASK_PALETTE) {   /* generate a colormap */
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
        cmap = pixcmapCreate(d);  /* spp == 1 */
        for (cindex = 0; cindex < num_palette; cindex++) {
            rval = palette[cindex].red;
            gval = palette[cindex].green;
            bval = palette[cindex].blue;
            pixcmapAddColor(cmap, rval, gval, bval);
        }
    }
    else
        cmap = NULL;

    if ((pix = pixCreate(w, h, d)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    }
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    pixSetColormap(pix, cmap);

    if (spp == 1) {   /* copy straight from buffer to pix */
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = 0; j < rowbytes; j++) {
                SET_DATA_BYTE(line, j, rowptr[j]);
            }
        }
    }
    else  {   /* spp == 3 or spp == 4 */
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = k = 0; j < w; j++) {
                SET_DATA_BYTE(ppixel, COLOR_RED, rowptr[k++]);
                SET_DATA_BYTE(ppixel, COLOR_GREEN, rowptr[k++]);
                SET_DATA_BYTE(ppixel, COLOR_BLUE, rowptr[k++]);
                if (spp == 4)
                    SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, rowptr[k++]);
                ppixel++;
            }
        }
    }

#if  DEBUG
    if (cmap) {
        for (i = 0; i < 16; i++) {
            fprintf(stderr, "[%d] = %d\n", i,
                   ((l_uint8 *)(cmap->array))[i]);
        }
    }
#endif  /* DEBUG */

        /* If there is no colormap, PNG defines black = 0 and
         * white = 1 by default for binary monochrome.  Therefore,
         * since we use the opposite definition, we must invert
         * the image in either of these cases:
         *    (i) there is no colormap (default)
         *    (ii) there is a colormap which defines black to
         *         be 0 and white to be 1.
         * We cannot use the PNG_TRANSFORM_INVERT_MONO flag
         * because that flag (since version 1.0.9) inverts 8 bpp
         * grayscale as well, which we don't want to do.
         * (It also doesn't work if there is a colormap.)
         * If there is a colormap that defines black = 1 and
         * white = 0, we don't need to do anything.
         * 
         * How do we check the polarity of the colormap?
         * The colormap determines the values of black and
         * white pixels in the following way:
         *     if black = 1 (255), white = 0
         *          255, 255, 255, 0, 0, 0, 0, 0, 0
         *     if black = 0, white = 1 (255)
         *          0, 0, 0, 0, 255, 255, 255, 0
         * So we test the first byte to see if it is 0;
         * if so, invert the data.  */
    if (d == 1 && (!cmap || (cmap && ((l_uint8 *)(cmap->array))[0] == 0x0))) {
/*        fprintf(stderr, "Inverting binary data on png read\n"); */
        pixInvert(pix, pix);
    }

    xres = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    yres = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    pixSetXRes(pix, (l_int32)((l_float32)xres / 39.37 + 0.5));  /* to ppi */
    pixSetYRes(pix, (l_int32)((l_float32)yres / 39.37 + 0.5));  /* to ppi */

        /* Get the text if there is any */
    png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
    if (num_text && text_ptr)
        pixSetText(pix, text_ptr->text);

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return pix;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBPNG */
/* --------------------------------------------*/

