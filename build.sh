#!/bin/sh
rm -rf bin
mkdir bin

cd src

gcc main.c \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o ../bin/cgui

    # build release
    #-lglfw -lGLU -lGLEW -lGL -lm -O2 \
