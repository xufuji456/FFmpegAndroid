#!/bin/bash

NDK=/home/mn/bin/android-ndk-r15b
SYSROOT=$NDK/platforms/android-16/arch-arm/
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
PREFIX=../output
ADDI_CFLAGS="-marm"


function build_ffmpeg
{
    ./configure \
        --prefix=$PREFIX \
        --enable-shared \
        --enable-small \
        --disable-static \
        --disable-doc \
        --disable-ffmpeg \
        --disable-ffplay \
        --disable-ffprobe \
        --disable-ffserver \
        --disable-symver \
        --target-os=linux \
        --arch=arm \
        --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
        --enable-cross-compile \
        --sysroot=$SYSROOT \
        --extra-cflags="-Os -fpic $ADDI_CFLAGS" \
        --extra-ldflags="$ADDI_LDFLAGS" \
        $ADDITIONAL_CONFIGURE_FLAG
    make clean
    make
    make install
}

cd ffmpeg
build_ffmpeg