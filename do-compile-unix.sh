#!/bin/bash

# generic linux build. try this first on any machine.
g++ -Isrc -s src/sfk.cpp src/sfkext.cpp src/sfkpack.cpp -o sfk

# to compile SFK for old linux systems with lib5:
# g++-3.3 -s sfk.cpp sfkext.cpp sfkpack.cpp -DSFK_LIB5 -o sfk-linux-lib5.exe

# to compile for Mac, use
# g++ -DMAC_OS_X sfk.cpp sfkext.cpp sfkpack.cpp -o sfkmac

# generic ARM compile
# g++ -DLINUX_ARM -DSFK_STATIC -static -s sfk.cpp sfkext.cpp sfkpack.cpp -o sfkarm

# Note: option -s strips debug symbols
# Note: you may try to add -DSFK_STATIC -static on any compile
#       to make sfk independent from local shared libraries