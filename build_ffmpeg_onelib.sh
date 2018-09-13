cd ffmpeg

make clean

archbit=32
 
#===========================
if [ $archbit -eq 32 ];then
echo "build for 32bit"
#32bit
abi='armeabi'
cpu='arm'
arch='arm'
android='androideabi'
else
#64bit
echo "build for 64bit"
abi='arm64-v8a'
cpu='aarch64'
arch='arm64'
android='android'
fi

export NDK=/home/frank/android/android-ndk-r10e
export PREBUILT=$NDK/toolchains/$cpu-linux-$android-4.9/prebuilt
export PLATFORM=$NDK/platforms/android-21/arch-$cpu
export PREFIX=../ff-onelib

build_one(){
  ./configure --target-os=linux --prefix=$PREFIX \
--enable-cross-compile \
--enable-runtime-cpudetect \
--arch=$cpu \
--cc=$PREBUILT/linux-x86_64/bin/$cpu-linux-$android-gcc \
--cross-prefix=$PREBUILT/linux-x86_64/bin/$cpu-linux-$android- \
--disable-stripping \
--nm=$PREBUILT/linux-x86_64/bin/$cpu-linux-$android-nm \
--sysroot=$PLATFORM \
--enable-gpl --enable-static --disable-shared --enable-nonfree --enable-version3 --enable-small \
--enable-neon --enable-mediacodec --enable-asm \
--enable-zlib --disable-ffprobe --disable-ffplay --enable-ffmpeg --disable-debug \
--enable-jni \
--extra-cflags="-fpic -mfpu=neon -mcpu=cortex-a8 -mfloat-abi=softfp -marm -march=armv7-a" 
}
build_one

make
make install

$PREBUILT/linux-x86_64/bin/$cpu-linux-$android-ld -rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -L$PREFIX/lib -soname libffmpeg.so -shared -nostdlib -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so libavcodec/libavcodec.a libavfilter/libavfilter.a libswresample/libswresample.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a libpostproc/libpostproc.a libavdevice/libavdevice.a -lc -lm -lz -ldl -llog --dynamic-linker=/system/bin/linker $PREBUILT/linux-x86_64/lib/gcc/$cpu-linux-$android/4.9/libgcc.a

cd ..
