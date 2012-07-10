#! /bin/bash --
# by pts@fazekas.hu at Tue Jul 10 21:28:12 CEST 2012
set -ex
cp config_auto_dynamiclinux.h config_auto.h
rm -f *.o
gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-unused -Wno-sign-compare \
    -DHAVE_CONFIG_H \
    affine.c arithlow.c bbuffer.c binexpand.c binexpandlow.c \
    binreduce.c binreducelow.c bmf.c bmpio.c boxbasic.c \
    boxfunc1.c boxfunc2.c boxfunc3.c bytearray.c colorcontent.c \
    colormap.c colormorph.c colorquant1.c colorspace.c \
    compare.c conncomp.c convolve.c convolvelow.c correlscore.c \
    dwacomb.2.c dwacomblow.2.c edge.c enhance.c fmorphgen.1.c \
    fmorphgenlow.1.c fpix1.c fpix2.c gifiostub.c gplot.c \
    graphics.c graymorph.c graymorphlow.c grayquant.c \
    grayquantlow.c heap.c jbclass.c jpegiostub.c kernel.c \
    morphapp.c morph.c morphdwa.c morphseq.c numabasic.c \
    numafunc1.c numafunc2.c paintcmap.c pdfio.c pix1.c pix2.c \
    pix3.c pix4.c pix5.c pixabasic.c pixacc.c pixafunc1.c \
    pixafunc2.c pixarith.c pixconv.c pixtiling.c pngio.c \
    pnmio.c psio2.c ptabasic.c ptafunc1.c ptra.c queue.c \
    readfile.c rop.c ropiplow.c roplow.c rotateorth.c \
    rotateorthlow.c sarray.c scale.c scalelow.c seedfill.c \
    seedfilllow.c sel1.c sel2.c shear.c spixio.c stack.c \
    textops.c tiffiostub.c utils.c webpiostub.c writefile.c \
    zlibmem.c \
;
g++ -fno-exceptions -fno-rtti -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. \
    jbig2arith.cc jbig2.cc jbig2enc.cc jbig2sym.cc

#g++ -Wl,--gc-sections,--print-gc-sections
g++ -Wl,--gc-sections -fno-exceptions -fno-rtti -s -o jbig2 *.o -lpng -lz

: OK.
