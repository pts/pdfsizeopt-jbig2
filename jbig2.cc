// Copyright 2006 Google Inc. All Rights Reserved.
// Author: agl@imperialviolet.org (Adam Langley)
//
// Copyright (C) 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <allheaders.h>
#include <pix.h>

#include "jbig2enc.h"

#if defined(WIN32)
#define WINBINARY O_BINARY
#else
#define WINBINARY 0
#endif

static void
usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <input filenames...>\n", argv0);
  fprintf(stderr, "Some functions removed for pdfsizeopt.\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -d --duplicate-line-removal: use TPGD in generic region coder\n");
  fprintf(stderr, "  -p --pdf: produce PDF ready data\n");
  fprintf(stderr, "  -t <threshold>: set classification threshold for symbol coder (def: 0.85)\n"); 
  fprintf(stderr, "  -T <bw threshold>: set 1 bpp threshold (def: 188)\n");
  fprintf(stderr, "  -2: upsample 2x before thresholding\n");
  fprintf(stderr, "  -4: upsample 4x before thresholding\n");
  fprintf(stderr, "  -j --jpeg-output: write images from mixed input as JPEG\n");
  fprintf(stderr, "  -v: be verbose\n");
}

static bool verbose = false;


static void
pixInfo(PIX *pix, const char *msg) {
  if (msg != NULL) fprintf(stderr, "%s ", msg);
  if (pix == NULL) {
    fprintf(stderr, "NULL pointer!\n");
    return;
  }
  fprintf(stderr, "%u x %u (%d bits) %udpi x %udpi, refcount = %u\n",
          pix->w, pix->h, pix->d, pix->xres, pix->yres, pix->refcount);
}

#ifdef WIN32
// -----------------------------------------------------------------------------
// Windows, sadly, lacks asprintf
// -----------------------------------------------------------------------------
#include <stdarg.h>
int
asprintf(char **strp, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    const int required = vsnprintf(NULL, 0, fmt, va);
    char *const buffer = (char *) malloc(required + 1);
    const int ret = vsnprintf(buffer, required + 1, fmt, va);
    *strp = buffer;

    va_end(va);

    return ret;
}
#endif

static int write_all(int fd, const void *buf, ssize_t count) {
  const char *p = (const char*)buf;
  while (count > 0) {
    ssize_t got = write(fd, p, count);
    if (got < 0) return -1;
    p += got;
    count -= got;
  }
  return 0;
}

int
main(int argc, char **argv) {
  bool duplicate_line_removal = false;
  bool pdfmode = false;
  float threshold = 0.85;
  int bw_threshold = 188;
  bool up2 = false, up4 = false;
  l_int32 img_fmt = IFF_PNG;
  const char *img_ext = "png";
  int i;

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 ||
        strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
      continue;
    }

    if (strcmp(argv[i], "-d") == 0 ||
        strcmp(argv[i], "--duplicate-line-removal") == 0) {
      duplicate_line_removal = true;
      continue;
    }

    if (strcmp(argv[i], "-p") == 0 ||
        strcmp(argv[i], "--pdf") == 0) {
      pdfmode = true;
      continue;
    }

    if (strcmp(argv[i], "-2") == 0) {
      up2 = true;
      continue;
    }
    if (strcmp(argv[i], "-4") == 0) {
      up4 = true;
      continue;
    }

    if (strcmp(argv[i], "-j") == 0 ||
        strcmp(argv[i], "--jpeg-output") == 0) {
      img_ext = "jpg";
      img_fmt = IFF_JFIF_JPEG;
      continue;
    }

    if (strcmp(argv[i], "-t") == 0) {
      char *endptr;
      threshold = strtod(argv[i+1], &endptr);
      if (*endptr) {
        fprintf(stderr, "Cannot parse float value: %s\n", argv[i+1]);
        usage(argv[0]);
        return 1;
      }

      if (threshold > 0.9 || threshold < 0.4) {
        fprintf(stderr, "Invalid value for threshold\n");
        fprintf(stderr, "(must be between 0.4 and 0.9)\n");
        return 10;
      }
      i++;
      continue;
    }

    if (strcmp(argv[i], "-T") == 0) {
      char *endptr;
      bw_threshold = strtol(argv[i+1], &endptr, 10);
      if (*endptr) {
        fprintf(stderr, "Cannot parse int value: %s\n", argv[i+1]);
        usage(argv[0]);
        return 1;
      }
      if (bw_threshold < 0 || bw_threshold > 255) {
        fprintf(stderr, "Invalid bw threshold: (0..255)\n");
        return 11;
      }
      i++;
      continue;
    }

    if (strcmp(argv[i], "-v") == 0) {
      verbose = true;
      continue;
    }

    break;
  }

  if (i == argc) {
    fprintf(stderr, "No filename given\n\n");
    usage(argv[0]);
    return 4;
  }

  if (up2 && up4) {
    fprintf(stderr, "Can't have both -2 and -4!\n");
    return 6;
  }

#ifdef WIN32
  int result = setmode(1, WINBINARY);  // stdout.
  if (result == -1) {
    perror("Cannot set mode to binary for stdout.");
    return 1;
  }
#endif

  int pageno = -1;

  int numsubimages=0, subimage=0;
  while (i < argc) {
    if (subimage==numsubimages) {
      subimage = numsubimages = 0;
      FILE *fp;
      if ((fp=fopen(argv[i], "rb"))==NULL) {
        fprintf(stderr, "Unable to open \"%s\"", argv[i]);
        return 1;
      }
      l_int32 filetype;
      if (findFileFormatStream(fp, &filetype)) {
        fprintf(stderr, "Unable to get file format of \"%s\"", argv[i]);
        return 1;
      }
#if HAVE_LIBTIFF
      if (filetype==IFF_TIFF && tiffGetCount(fp, &numsubimages)) {
        fprintf(stderr, "Cannot process TIFF with subimages: \"%s\"", argv[i]);
        return 1;
      }
#endif
      fclose(fp);
    }

    PIX *source;
    if (numsubimages<=1) {
      source = pixRead(argv[i]);
    }
#if HAVE_LIBTIFF
    else {
      source = pixReadTiff(argv[i], subimage++);
    }
#endif

    if (!source) return 3;
    if (verbose)
      pixInfo(source, "source image:");

    PIX *pixl, *gray, *pixt;
    if ((pixl = pixRemoveColormap(source, REMOVE_CMAP_BASED_ON_SRC)) == NULL) {
      fprintf(stderr, "Failed to remove colormap from %s\n", argv[i]);
      return 1;
    }
    pixDestroy(&source);
    pageno++;

    if (pixl->d > 1) {
      if (pixl->d > 8) {
        gray = pixConvertRGBToGrayFast(pixl);
        if (!gray) return 1;
      } else {
        gray = pixClone(pixl);
      }
      if (up2) {
        pixt = pixScaleGray2xLIThresh(gray, bw_threshold);
      } else if (up4) {
        pixt = pixScaleGray4xLIThresh(gray, bw_threshold);
      } else {
        pixt = pixThresholdToBinary(gray, bw_threshold);
      }
      pixDestroy(&gray);
    } else {
      pixt = pixClone(pixl);
    }
    if (verbose)
      pixInfo(pixt, "thresholded image:");

    pixDestroy(&pixl);

    int length;
    uint8_t *ret;
    ret = jbig2_encode_generic(pixt, !pdfmode, 0, 0, duplicate_line_removal,
                               &length);
    if (0 > write_all(1, ret, length))
      abort();
    return 0;
  }
}

