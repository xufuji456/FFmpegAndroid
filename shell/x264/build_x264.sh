#!/bin/bash

archbit=64
if [ $archbit -eq 32 ];then
echo "build for 32bit"
#32bit
ABI='armeabi-v7a'
CPU='arm'
ARCH='arm'
ANDROID='androideabi'
else
#64bit
echo "build for 64bit"
ABI='arm64-v8a'
CPU='aarch64'
ARCH='arm64'
ANDROID='android'
fi

export NDK=/home/frank/android/android-ndk-r10e 
export PREBUILT=$NDK/toolchains/$CPU-linux-$ANDROID-4.9/prebuilt
export PLATFORM=$NDK/platforms/android-21/arch-$ARCH
export TOOLCHAIN=$PREBUILT/linux-x86_64
export PREFIX=$(pwd)/android/$ABI

build_x264(){
./configure \
--prefix=$PREFIX \
--enable-static \
--disable-asm \
--enable-pic \
--host=arm-linux \
--cross-prefix=$TOOLCHAIN/bin/$CPU-linux-$ANDROID- \
--sysroot=$PLATFORM \
make clean
make
make install
}
build_x264
