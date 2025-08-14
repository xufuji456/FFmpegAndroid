#!/bin/bash

archbit=$1

if [ $archbit -eq 32 ];then
  echo "build for 32bit"
  ARCH='arm'
  API=21
  ANDROID='androideabi'
else
  echo "build for 64bit"
  ARCH='aarch64'
  API=21
  ANDROID='android'
fi

export NDK=/Users/frank/Downloads/android-ndk-r22b
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$COMPILE_OS-x86_64
export PREFIX=$(pwd)/android/$ARCH-16k

function build_lame() {
./configure \
--host=$ARCH-linux-$ANDROID \
--prefix=$PREFIX \
--enable-static \
--disable-shared \
--disable-frontend \
--with-sysroot=$TOOLCHAIN/sysroot \
CFLAGS="-DPAGE_SIZE=16384" \
LDFLAGS="-DPAGE_SIZE=16384"
make
make install
}

build_lame
echo "building mp3lame done..."

