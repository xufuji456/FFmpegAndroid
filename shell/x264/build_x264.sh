#!/bin/bash

make clean

archbit=$1
if [ $archbit -eq 32 ];then
echo "build for 32bit"
API=21
ABI='armeabi-v7a'
CPU='arm'
ANDROID='androideabi'
else
echo "build for 64bit"
API=21
ABI='arm64-v8a'
CPU='aarch64'
ANDROID='android'
fi

export NDK=/Users/frank/Downloads/android-ndk-r21e
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64
export TARGET=$CPU-linux-$ANDROID
export CC=$TOOLCHAIN/bin/$TARGET$API-clang
export AR=$TOOLCHAIN/bin/llvm-ar
export LD=$TOOLCHAIN/bin/ld
export PREFIX=$(pwd)/android/$ABI

function build_x264() {
./configure \
--prefix=$PREFIX \
--enable-static \
--disable-asm \
--enable-pic \
--host=$CPU-linux-$ANDROID \
--cross-prefix=$TOOLCHAIN/bin/$CPU-linux-$ANDROID- \
--sysroot=$TOOLCHAIN/sysroot \
--extra-ldflags="-Wl,-z,max-page-size=16384"

make
make install
}
build_x264

echo "build x264 done..."
