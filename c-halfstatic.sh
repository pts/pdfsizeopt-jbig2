#! /bin/bash --
# by pts@fazekas.hu at Thu Jul 19 14:51:33 CEST 2012
set -ex
rm -f *.o

gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. \
    zall.c

gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-sign-compare \
    -I. \
    pngall.c

gcc -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall -Wno-uninitialized -Wno-sign-compare -Wno-unused-parameter \
    -I. \
    leptonica.c

g++ -fno-exceptions -fno-rtti -s -O2 -c \
    -ffunction-sections -fdata-sections \
    -W -Wall \
    -I. \
    jbig2arith.cc jbig2.cc jbig2enc.cc jbig2sym.cc

#g++ -Wl,--gc-sections,--print-gc-sections
g++ -Wl,--gc-sections \
    -fno-exceptions -fno-rtti -s -o jbig2.halfstatic *.o

echo OK.
: OK.
