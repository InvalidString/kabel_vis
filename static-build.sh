#!/bin/sh

set -xe

inc="-I$HOME/src/raylib/src"
lib="-L$HOME/src/raylib/src"


clang -O3 -o kabel_vis_static main_static.c live.c dyn_arr.c kabel.c -lraylib -ldl -lm
