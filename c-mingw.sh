#! /bin/bash --
# by pts@fazekas.hu at Tue Jul 10 23:07:40 CEST 2012
#
# works with 
set -ex
rm -f *.o
#i586-mingw32msvc-gcc -fdump-tree-fixupcfg-lineno -g -c \

i586-mingw32msvc-gcc -s -O2 -c  \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. -DNO_VIZ \
    zall.c

i586-mingw32msvc-gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-sign-compare \
    -I. \
    pngall.c

i586-mingw32msvc-gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-unused-parameter -Wno-sign-compare \
    -Wno-strict-aliasing -fno-strict-aliasing \
    -I. \
    leptonica.c

i586-mingw32msvc-g++ -fno-exceptions -fno-rtti -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. \
    jbig2arith.cc jbig2.cc jbig2enc.cc jbig2sym.cc

#g++ -Wl,--gc-sections,--print-gc-sections
i586-mingw32msvc-g++ -Wl,--gc-sections \
    -fno-exceptions -fno-rtti -s -o jbig2.exe *.o

: OK.
