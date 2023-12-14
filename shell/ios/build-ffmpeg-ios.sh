#!/bin/sh

#reference: https://github.com/kewlbear/FFmpeg-iOS-build-script.git

#require
#https://github.com/libav/gas-preprocessor
#yasm homebrew

FF_VERSION="6.0"
if [[ $FFMPEG_VERSION != "" ]]; then
  FF_VERSION=$FFMPEG_VERSION
fi
SOURCE="ffmpeg-$FF_VERSION"
FAT="FFmpeg-iOS"

SCRATCH="scratch"
THIN=`pwd`/"thin"

LIPO="y"
COMPILE="y"
ARCHS="arm64 armv7 x86_64"
DEPLOYMENT_TARGET="13.0"

#export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:libx264.pc
#X264=`pwd`/x264-ios
#FDK_AAC=`pwd`/fdk-aac-ios

CONFIGURE_FLAGS="--enable-cross-compile
--disable-debug \
--disable-programs \
--disable-doc \
--enable-pic \
--disable-audiotoolbox \
--enable-static \
--disable-shared \
--enable-hwaccels \
--enable-neon \
--enable-avfilter \
--disable-indevs \
--disable-outdevs \
--disable-decoders \
--enable-decoder=av1,dvvideo,flv,gif,h263,h264,hevc,mjpeg,mpeg1video,mpeg2video,mpegvideo,mpeg4,\
msmpeg4v2,msmpeg4v3,msvideo1,png,tscc,tscc2,vc1,vp8,vp9,webp,wmv1,wmv2,wmv3,zlib,\
aac,aac_at,aac_latm,ac3,ac3_at,adpcm_ima_qt,adpcm_ima_wav,adpcm_ms,alac,amrnb,amrwb,ape,dolby_e,\
eac3,eac3_at,flac,g722,g726,g729,m4a,mp3float,mp3,mp3adufloat,mp3_at,mp3adu,mp3on4,opus,pcm_alaw,\
pcm_mulaw,pcm_dvd,pcm_f16le,pcm_f24le,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_s16be,pcm_s16le,\
pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_s64be,pcm_s64le,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,\
pcm_u32be,pcm_u32le,pcm_vidc,pcm_zork,truehd,truespeech,vorbis,wmav1,wmav2,\
ssa,ass,dvbsub,dvdsub,pgssub,mov_text,sami,srt,subrip,text,webvtt \
"

if [ "$X264" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
fi

if [ "$FDK_AAC" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree"
fi

if [ "$*" ]
then
	if [ "$*" = "lipo" ]
	then
		# skip compile
		COMPILE=
	else
		ARCHS="$*"
		if [ $# -eq 1 ]
		then
			# skip lipo
			LIPO=
		fi
	fi
fi

if [ "$COMPILE" ]
then
	if [ ! `which yasm` ]
	then
		echo 'Yasm not found'
		if [ ! `which brew` ]
		then
			echo 'Homebrew not found. Trying to install...'
      ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
			|| exit 1
		fi
		echo 'Trying to install Yasm...'
		brew install yasm || exit 1
	fi
	if [ ! `which gas-preprocessor.pl` ]
	then
		echo 'gas-preprocessor.pl not found. Trying to install...'
		(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
			-o /usr/local/bin/gas-preprocessor.pl \
			&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
			|| exit 1
	fi

	if [ ! -r $SOURCE ]
	then
		echo 'FFmpeg source not found. Trying to download...'
		curl http://www.ffmpeg.org/releases/$SOURCE.tar.bz2 | tar xj \
			|| exit 1
	fi

	CWD=`pwd`
	for ARCH in $ARCHS
	do
		echo "building $ARCH..."
		mkdir -p "$SCRATCH/$ARCH"
		cd "$SCRATCH/$ARCH"

		CFLAGS="-arch $ARCH"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		    PLATFORM="iPhoneSimulator"
		    CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		CC="xcrun -sdk $XCRUN_SDK clang"

		if [ "$ARCH" = "arm64" ]
		then
		    AS="gas-preprocessor.pl -arch aarch64 -- $CC"
		else
		    AS="gas-preprocessor.pl -- $CC"
		fi

		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"
		if [ "$X264" ]
		then
			CFLAGS="$CFLAGS -I$X264/include"
			LDFLAGS="$LDFLAGS -L$X264/lib"
		fi
		if [ "$FDK_AAC" ]
		then
			CFLAGS="$CFLAGS -I$FDK_AAC/include"
			LDFLAGS="$LDFLAGS -L$FDK_AAC/lib"
		fi

		TMPDIR=${TMPDIR/%\/} $CWD/$SOURCE/configure \
		    --target-os=darwin \
		    --arch=$ARCH \
		    --cc="$CC" \
		    --as="$AS" \
		    $CONFIGURE_FLAGS \
		    --extra-cflags="$CFLAGS" \
		    --extra-ldflags="$LDFLAGS" \
		    --prefix="$THIN/$ARCH" \
		|| exit 1

		make -j8 install $EXPORT || exit 1
		cd $CWD
	done
fi

if [ "$LIPO" ]
then
	echo "building fat binaries..."
	mkdir -p $FAT/lib
	set - $ARCHS
	CWD=`pwd`
	cd $THIN/$1/lib
	for LIB in *.a
	do
		cd $CWD
		echo lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB 1>&2
		lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB || exit 1
	done

	cd $CWD
	cp -rf $THIN/$1/include $FAT
fi

echo Done
