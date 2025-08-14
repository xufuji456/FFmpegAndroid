#!/bin/bash

archbit=$1

if [ $archbit -eq 32 ];then
  echo "build for 32bit"
  CPU='arm'
  API=21
else
  echo "build for 64bit"
  CPU='arm64'
  API=21
fi

export NDK_ROOT=/Users/frank/Downloads/android-ndk-r22b
export TOOLCHAIN=$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64
export PATH=$TOOLCHAIN/bin:$PATH

function build() {
 ./Configure android-$CPU \
 -D__ANDROID_API__=$API \
 no-shared \
 no-ssl2 \
 no-ssl3 \
 no-comp \
 no-hw \
 no-engine \
 --prefix=$(pwd)/android/$CPU \
 --openssldir=$(pwd)/android/$CPU \
 -DPAGE_SIZE=16384
 make
 make install
}

build
echo "building openssl done..."
