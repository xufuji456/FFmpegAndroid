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
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64
export CC=$TOOLCHAIN/bin/$ARCH-linux-$ANDROID$API-clang
export CXX=$TOOLCHAIN/bin/$ARCH-linux-$ANDROID$API-clang++
export AR=$TOOLCHAIN/bin/$ARCH-linux-$ANDROID-AR
export RANLIB=$TOOLCHAIN/bin/$ARCH-linux-$ANDROID-ranlib
export STRIP=$TOOLCHAIN/bin/$ARCH-linux-$ANDROID-strip
export LD=$TOOLCHAIN/bin/ld
export PREFIX=$(pwd)/android/$ARCH

function build_lame() {
./configure \
--host=$ARCH-linux-$ANDROID \
--prefix=$PREFIX \
--enable-static \
--disable-shared \
--disable-frontend \
--with-sysroot=$TOOLCHAIN/sysroot \
LDFLAGS="-Wl,-z,max-page-size=16384"
make
make install
}

build_lame
echo "building mp3lame done..."

