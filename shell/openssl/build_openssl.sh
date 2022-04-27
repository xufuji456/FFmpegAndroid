#!/bin/bash
export NDK_ROOT=/Users/xufulong/Library/Android/android-ndk-r15c

build() {
API=21
CPU=$1
PLATFORM=$2
make clean
rm -rf $(pwd)/android/$CPU
export ANDROID_NDK_HOME=$NDK_ROOT
PATH=$ANDROID_NDK_HOME/toolchains/$PLATFORM-4.9/prebuilt/darwin-x86_64/bin:$PATH
./Configure android-$CPU -D__ANDROID_API__=$API no-shared no-ssl2 no-ssl3 no-comp no-hw no-engine --prefix=$(pwd)/android/$CPU --openssldir=$(pwd)/android/$CPU

make
make install
}

# build armv7
build arm arm-linux-androideabi

# build armv8
build arm64 aarch64-linux-android