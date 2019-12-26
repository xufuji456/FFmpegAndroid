make clean
cd compat
rm -rf strtod.d
rm -rf strtod.o
cd ../

set -e

archbit=32

if [ $archbit -eq 32 ];then
echo "build for 32bit"
#32bit
ABI='armeabi-v7a'
CPU='arm'
ARCH='arm'
ANDROID='androideabi'
NATIVE_CPU='armv7-a'
OPTIMIZE_CFLAGS="-march=$NATIVE_CPU -mcpu=cortex-a8 -mfpu=vfpv3-d16 -mfloat-abi=softfp -mthumb"
else
#64bit
echo "build for 64bit"
ABI='arm64-v8a'
CPU='aarch64'
ARCH='arm64'
ANDROID='android'
NATIVE_CPU='armv8-a'
OPTIMIZE_CFLAGS=""
fi

export NDK=/Users/frank/Library/Android/android-ndk-r10e
export PREBUILT=$NDK/toolchains/$CPU-linux-$ANDROID-4.9/prebuilt
export PLATFORM=$NDK/platforms/android-21/arch-$ARCH
export TOOLCHAIN=$PREBUILT/darwin-x86_64
export PREFIX=../ffmpeg-android/$ABI
export ADDITIONAL_CONFIGURE_FLAG="--cpu=$NATIVE_CPU"

LAMEDIR=$PREFIX
export EXTRA_CFLAGS="-Os -fpic $OPTIMIZE_CFLAGS -I$LAMEDIR/include"
export EXTRA_LDFLAGS="-lc -lm -ldl -llog -lgcc -lz -L$LAMEDIR/lib"

build_one(){
  ./configure --target-os=linux --prefix=$PREFIX \
--enable-cross-compile \
--arch=$CPU \
--cc=$TOOLCHAIN/bin/$CPU-linux-$ANDROID-gcc \
--cross-prefix=$TOOLCHAIN/bin/$CPU-linux-$ANDROID- \
--sysroot=$PLATFORM \
--enable-neon \
--enable-hwaccels \
--enable-static \
--disable-shared \
--disable-doc \
--enable-asm \
--enable-small \
--disable-ffmpeg \
--disable-ffplay \
--disable-ffprobe \
--disable-ffserver \
--disable-debug \
--disable-gpl \
--disable-avdevice \
--disable-indevs \
--disable-outdevs \
--disable-avresample \
--extra-cflags="$EXTRA_CFLAGS" \
--extra-ldflags="$EXTRA_LDFLAGS" \
--enable-avcodec \
--enable-avformat \
--enable-avutil \
--enable-swresample \
--enable-swscale \
--enable-avfilter \
--enable-network \
--enable-bsfs \
--enable-postproc \
--enable-filters \
--enable-encoders \
--enable-libmp3lame \
--enable-encoder=libmp3lame \
--disable-decoders \
--enable-decoder=mpeg4,h264,flv,gif,hevc,vp9,wmv3,png,ljpeg,jpeg2000,mjpeg,\
aac,m4a,amrnb,amrwb,ape,dolby_e,dst,flac,opus,vorbis,wavesynth,wavpack,wmav2,\
mp3float,mp3,mp3_at,mp3adufloat,mp3adu,mp3on4float,mp3on4,aac_fixed,aac_at,aac_latm,pcm_s16be,pcm_s16le \
--enable-muxers \
--enable-parsers \
--enable-protocols \
--disable-demuxers \
--enable-demuxer=aac,ac3,amr,amrnb,amrwb,ape,asf,asf_o,ast,avi,caf,cavsvideo,codec2,concat,data,dnxhd,flac,flv,g722,g729,\
gif,gif_pipe,h264,hevc,hls,image2,image2pipe,ingenient,jpeg_pipe,lavfi,lrc,m4v,matroska,webm,mjpeg,mov,mp4,m4a,3gp,mp3,mpeg,\
mpegts,mv,ogg,png_pipe,realtext,rm,rtp,rtsp,s16be,s16le,s24be,s24le,s32be,s32le,sdp,srt,swf,u16be,u16le,u24be,u24le,u32be,u32le,\
vc1,wav,webm_dash,manifest,xmv,f32be,f32le,f64be,f64le \
$ADDITIONAL_CONFIGURE_FLAG
make
make install

$TOOLCHAIN/bin/$CPU-linux-$ANDROID-ld -rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib \
-L$PREFIX/lib -soname libffmpeg.so \
-shared -nostdlib -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so \
$PREFIX/lib/libavcodec.a \
$PREFIX/lib/libavfilter.a \
$PREFIX/lib/libswresample.a \
$PREFIX/lib/libavformat.a \
$PREFIX/lib/libavutil.a \
$PREFIX/lib/libswscale.a \
$PREFIX/lib/libmp3lame.a \
-lc -lm -lz -ldl -llog --dynamic-linker=/system/bin/linker $TOOLCHAIN/lib/gcc/$CPU-linux-$ANDROID/4.9/libgcc.a
}

build_one