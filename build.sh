#!/bin/sh

set -xe

inc="-I$HOME/src/raylib/src"
lib="-L$HOME/src/raylib/src"

./live-build.sh

gcc -o kabel_vis main.c "-Wl,-rpath=." -lraylib -ldl -lm
