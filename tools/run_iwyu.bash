#!/bin/bash

find . \
    -name '*.cc' \
    -and -not -name 'mkfont.cc' \
    -print0 |
parallel -0 \
    include-what-you-use \
        -Xiwyu --no_default_mappings \
        -std=c++20 \
        -ffreestanding \
        -nostdinc \
        -nostdinc++ \
        -I. \
        -Ibuild \
        -Ilibs \
        -Ilibs/posix \
        -w \
        {} 2>&1 |
    grep -v correct | sed -e :a -e '/^\n*$/{$d;N;};/\n$/ba' | cat -s
