#! /bin/bash --
# by pts@fazekas.hu at Tue Jul 10 23:07:40 CEST 2012
#
set -ex
rm -f *.o
#i586-mingw32msvc-gcc -fdump-tree-fixupcfg-lineno -g -c \

# Please note that -ffunction-sections, -fdata-sections and
# -Wl,--gc-sections doesn't seem to have any effect, even unused code gets
# compiled and linked.
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
    jbig2arith.cc jbig2.cc jbig2enc.cc

#g++ -Wl,--gc-sections,--print-gc-sections
i586-mingw32msvc-g++ -Wl,--gc-sections \
    -fno-exceptions -fno-rtti -s -o jbig2.uncompressed.exe *.o
cp -a jbig2.uncompressed.exe jbig2.exe
upx.pts --brute jbig2.exe

: OK.
