#! /bin/bash --
# by pts@fazekas.hu at Tue Jul 17 13:45:54 CEST 2012
set -ex
rm -f *.o
gcc -g -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-sign-compare -Wno-unused-parameter \
    leptonica.c

g++ -fno-exceptions -fno-rtti -g -c \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. \
    jbig2arith.cc jbig2.cc jbig2enc.cc

#g++ -Wl,--gc-sections,--print-gc-sections
g++ -Wl,--gc-sections \
    -fno-exceptions -fno-rtti -o jbig2.debug *.o \
    -lpng -lz

: OK.
