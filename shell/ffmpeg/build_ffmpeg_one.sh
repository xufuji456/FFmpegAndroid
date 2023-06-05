#!/bin/bash
make clean
set -e
archbit=64

if [ $archbit -eq 32 ];then
  echo "build for 32bit"
  ARCH='arm'
  CPU='armv7-a'
  ABI='armeabi-v7a'
  API=21
  PLATFORM='armv7a'
  PLATFORM_ARCH='arm'
  ANDROID='androideabi'
  OPTIMIZE_CFLAGS="-march=$CPU -mfpu=neon -mfloat-abi=softfp -marm"
else
  echo "build for 64bit"
  ARCH='aarch64'
  CPU='armv8-a'
  ABI='arm64-v8a'
  API=21
  PLATFORM='aarch64'
  PLATFORM_ARCH='arm64'
  ANDROID='android'
  OPTIMIZE_CFLAGS="-march=$CPU -mfpu=neon -mfloat-abi=softfp"
fi

uname=`uname`
if [ $uname = "Darwin" ];then
  echo "compile on mac"
  COMPILE_PLAT="darwin"
elif [ $uname = "Linux" ]; then
  echo "compile on linux"
  COMPILE_PLAT="linux"
else
  echo "don't support $uname"
fi

export NDK=/Users/xufulong/Library/Android/sdk/ndk-bundle
export TOOL=$NDK/toolchains/llvm/prebuilt/$COMPILE_PLAT-x86_64
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$COMPILE_PLAT-x86_64/bin
export SYSROOT=$NDK/toolchains/llvm/prebuilt/$COMPILE_PLAT-x86_64/sysroot
export CROSS_PREFIX=$TOOLCHAIN/$ARCH-linux-$ANDROID-
export CC=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang
export CXX=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang++
export PLATFORM_API=$NDK/platforms/android-$API/arch-$PLATFORM_ARCH
export PREFIX=../ffmpeg-android/$ABI
export ADDITIONAL_CONFIGURE_FLAG="--cpu=$CPU"

THIRD_LIB=$PREFIX
export EXTRA_CFLAGS="-Os -fPIC $OPTIMIZE_CFLAGS -I$THIRD_LIB/include"
export EXTRA_LDFLAGS="-lc -lm -ldl -llog -lgcc -lz -L$THIRD_LIB/lib"

function build_one() {
  ./configure \
  --target-os=android \
  --prefix=$PREFIX \
  --cross-prefix=$CROSS_PREFIX \
  --enable-cross-compile \
  --arch=$ARCH \
  --cpu=$CPU \
  --cc=$CC \
  --cxx=$CXX \
  --nm=$TOOLCHAIN/$ARCH-linux-$ANDROID-nm \
  --strip=$TOOLCHAIN/$ARCH-linux-$ANDROID-strip \
  --enable-cross-compile \
  --sysroot=$SYSROOT \
  --enable-hwaccels \
  --enable-static \
  --disable-shared \
  --disable-doc \
  --enable-neon \
  --enable-asm \
  --disable-small \
  --enable-jni \
  --enable-mediacodec \
  --enable-decoder=h264_mediacodec \
  --enable-decoder=hevc_mediacodec \
  --enable-decoder=mpeg4_mediacodec \
  --enable-decoder=vp9_mediacodec \
  --disable-ffmpeg \
  --disable-ffplay \
  --disable-ffprobe \
  --disable-debug \
  --enable-gpl \
  --disable-avdevice \
  --disable-indevs \
  --disable-outdevs \
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
  --disable-postproc \
  --enable-filters \
  --enable-encoders \
  --enable-libmp3lame \
  --enable-libx264 \
  --enable-encoder=libmp3lame,libx264 \
  --disable-encoder=a64multi,a64multi5,alias_pix,amv,apng,aptx,aptx_hd,asv1,asv2,avrp,avui,cinepak,cljr,\
comfortnoise,dpx,ffvhuff,fits,hap,ilbc_at,mlp,nellymoser,pam,pbm,pcx,pgm,pgmyuv,ppm,prores,prores_aw,\
prores_ks,qtrle,r10k,r210,ra_144,roq,roq_dpcm,rv10,rv20,s302m,sbc,sgi,snow,sunrast,svq1,targa,tta,utvideo,\
v210,v308,v408,v410,vc2,wrapped_avframe,xbm,xface,xsub,xwd,y41p,zmbv \
  --disable-decoders \
  --enable-decoder=mjpeg,mpeg4,h263,h264,flv,hevc,wmv3,msmpeg4v3,msmpeg4v2,msvideo1,vc1,mpeg1video,mpeg2video,\
aac,ac3,ac3_fixed,m4a,amrnb,amrwb,vorbis,wmav2,truehd,tscc,tscc2,dvvideo,msrle,cinepak,indeo5,vp8,vp9,av1,\
mp3float,mp3,mp3adufloat,mp3adu,mp3on4float,mp3on4,aac_fixed,aac_latm,eac3,png,wmav1,wmv1,wmv2,\
pcm_alaw,pcm_dvd,pcm_f16le,pcm_f24le,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,zlib,flac,opus,mlp,\
pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_s64be,pcm_s64le,pcm_mulaw,alac,adpcm_ms,\
pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_vidc,pcm_zork,adpcm_ima_qt,adpcm_ima_wav,gif \
  --enable-muxers \
  --enable-parsers \
  --enable-nonfree \
  --enable-protocols \
  --enable-openssl \
  --enable-protocol=https \
  --disable-demuxers \
  --enable-demuxer=aac,ac3,alaw,amr,amrnb,amrwb,ape,asf,asf_o,avi,cavsvideo,codec2,concat,dnxhd,eac3,flac,flv,\
gif,gif_pipe,h263,h264,hevc,hls,image2,image2pipe,jpeg_pipe,lrc,m4v,matroska,webm,mjpeg,mov,mp4,m4a,3gp,mp3,mpeg,\
mpegts,mv,ogg,png_pipe,realtext,rm,rtp,rtsp,pcm_s16be,pcm_s16le,pcm_s32be,pcm_s32le,sdp,srt,swf,\
vc1,wav,webm_dash,manifest,xmv,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,mpegvideo,mulaw,sami,srt \
$ADDITIONAL_CONFIGURE_FLAG
make
make install
}

build_one

function link_one_ffmpeg() {
  $TOOLCHAIN/$ARCH-linux-$ANDROID-ld -rpath-link=$PLATFORM_API/usr/lib -L$PLATFORM_API/usr/lib \
  -L$PREFIX/lib -soname libffmpeg.so \
  -shared -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so \
  $PREFIX/lib/libavcodec.a \
  $PREFIX/lib/libavfilter.a \
  $PREFIX/lib/libswresample.a \
  $PREFIX/lib/libavformat.a \
  $PREFIX/lib/libavutil.a \
  $PREFIX/lib/libswscale.a \
  $PREFIX/lib/libmp3lame.a \
  $PREFIX/lib/libx264.a \
  $PREFIX/lib/libssl.a \
  $PREFIX/lib/libcrypto.a \
  -lc -lm -lz -ldl -llog --dynamic-linker=/system/bin/linker $TOOL/lib/gcc/$ARCH-linux-$ANDROID/4.9.x/libgcc_real.a
}

link_one_ffmpeg

#mp3lame
#--enable-libmp3lame \
#--enable-encoder=libmp3lame \

#x264
#--enable-libx264 \
#--enable-encoder=libx264 \

#https
#--enable-openssl \
#--enable-protocol=https \