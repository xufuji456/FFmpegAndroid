# 前言
FFmpeg是一个跨平台的多媒体库，也是目前音视频领域应用最广泛的库。包括libavcodec、libavformat、libavutil、</br>
libavdevice、libavfilter、libswscale、libswresample、libpostproc等模块。其中avcodec用于编解码，</br>
avformat用于解封装，avutil是提供工具类，avdevice用于各平台的设备接入，avfilter提供滤镜操作，</br>
swscale提供图像缩放与像素格式转换，swresample提供音频重采样，postproc提供高级处理。</br>
更详细编译脚本可查看博客：[FFmpeg编译脚本分析](https://blog.csdn.net/u011686167/article/details/120390739)<br>

# 一、准备工作
## 1、下载ffmpeg源码
使用git命令：git clone https://github.com/FFmpeg/FFmpeg.git。

## 2、下载ndk源码
在Android开发者文档可以下载到ndk源码，[ndk下载链接](https://developer.android.com/ndk/downloads)。
根据实际需求选择对应ndk版本，建议下载ndk-r20b稳定版。mac电脑配置ndk系统环境变量步骤如下：
(1)、在命令行输入vim ~/.bash_profile
(2)、输入i进行编辑模式
(3)、编写export NDK_PATH=xxx/xxx/android-ndk-r20b
(4)、按下esc退出编辑模式，输入:wq保存并退出

# 二、编译ffmpeg源码
## 1、分析configure结构
在FFmpeg源码目录有个configure配置脚本，使用./configure --help进行查看，可以看到如下结构的编译选项：</br>

 | Help options:           | Description                 |
 |:------------------------|:----------------------------|
 | --help                  | print this message          |
 | --list-decoders         | show all available decoders |
 | --list-encoders         | show all available encoders |
 | --list-hwaccels         | show all available hardware |
 | --list-demuxers         | show all available demuxers |
 | --list-muxers           | show all available muxers   |
 | --list-parsers          | show all available parsers  |
 | --list-protocols        | show all available protocols|
 | --list-filters          | show all available filters  |

 | Licensing options:      | Description                 |
 |:------------------------|:----------------------------|
 | --enable-gpl            | allow use of GPL code       |
 | --enable-version3       | upgrade (L)GPL to version 3 |
 | --enable-nonfree        | allow use of nonfree code   |

 | Configuration options:  | Description                 |
 |:------------------------|:----------------------------|
 | --prefix=PREFIX         | install in PREFIX           |
 | --disable-static        | do not build static libs    |
 | --enable-shared         | build shared libraries [no] |
 | --enable-small          | optimize for size           |
 | --disable-all           | disable components, libs    |
 | --disable-autodetect    | disable detect external libs|

 | Program options:        | Description                 |
 |:------------------------|:----------------------------|
 | --disable-programs      | don't build command line    |
 | --disable-ffmpeg        | disable ffmpeg build        |
 | --disable-ffplay        | disable ffplay build        |
 | --disable-ffprobe       | disable ffprobe build       |

 | Component options:      | Description                 |
 |:------------------------|:----------------------------|
 | --disable-avdevice      | disable libavdevice build   |
 | --disable-avcodec       | disable libavcodec build    |
 | --disable-avformat      | disable libavformat build   |
 | --disable-swresample    | disable libswresample build |
 | --disable-swscale       | disable libswscale build    |
 | --disable-postproc      | disable libpostproc build   |
 | --disable-avfilter      | disable libavfilter build   |
 | --disable-pthreads      | disable pthreads            |
 | --disable-network       | disable network support     |

 | Individual component:   | Description                 |
 |:------------------------|:----------------------------|
 | --disable-everything    | disable all components      |
 | --disable-encoder=NAME  | disable encoder NAME        |
 | --enable-encoder=NAME   | enable encoder NAME         |
 | --disable-encoders      | disable all encoders        |
 | --disable-decoder=NAME  | disable decoder NAME        |
 | --enable-decoder=NAME   | enable decoder NAME         |
 | --disable-decoders      | disable all decoders        |
 | --disable-muxer=NAME    | disable muxer NAME          |
 | --enable-muxer=NAME     | enable muxer NAME           |
 | --disable-muxers        | disable all muxers          |
 | --disable-demuxer=NAME  | disable demuxer NAME        |
 | --enable-demuxer=NAME   | enable demuxer NAME         |
 | --disable-demuxers      | disable all demuxers        |
 | --enable-parser=NAME    | enable parser NAME          |
 | --disable-parser=NAME   | disable parser NAME         |
 | --disable-parsers       | disable all parsers         |
 | --enable-protocol=NAME  | enable protocol NAME        |
 | --disable-protocol=NAME | disable protocol NAME       |
 | --disable-protocols     | disable all protocols       |
 | --enable-filter=NAME    | enable filter NAME          |
 | --disable-filter=NAME   | disable filter NAME         |
 | --disable-filters       | disable all filters         |

 | External library:       | Description                              |
 |:------------------------|:-----------------------------------------|
 | --disable-avfoundation  | disable Apple AVFoundation framework     |
 | --enable-jni            | enable JNI support [no]                  |
 | --enable-libaom         | enable AV1 video codec via libaom [no]   |
 | --enable-libass         | enable libass subtitles rendering [no]   |
 | --enable-libdav1d       | enable AV1 decoding via libdav1d [no]    |
 | --enable-libfdk-aac     | enable AAC de/encoding via libfdk-aac    |
 | --enable-libfontconfig  | enable libfontconfig [no]                |
 | --enable-libfreetype    | enable libfreetype, needed for drawtext  |
 | --enable-libfribidi     | enable libfribidi, improves drawtext     |
 | --enable-libmp3lame     | enable MP3 encoding via libmp3lame [no]  |
 | --enable-libopencv      | enable video filtering via libopencv [no]|
 | --enable-libopenh264    | enable H.264 encoding via OpenH264 [no]  |
 | --enable-libopus        | enable Opus de/encoding via libopus [no] |
 | --enable-librtmp        | enable RTMP[E] support via librtmp [no]  |
 | --enable-libshine       | enable fixed-point MP3 encoding          |
 | --enable-libsoxr        | enable Include libsoxr resampling [no]   |
 | --enable-libspeex       | enable Speex de/encoding via libspeex    |
 | --enable-libtensorflow  | enable TensorFlow as a DNN module [no]   |
 | --enable-libvorbis      | enable Vorbis en/decoding via libvorbis  |
 | --enable-libvpx         | enable VP8 and VP9 de/encoding via libvpx|
 | --enable-libx264        | enable H.264 encoding via x264 [no]      |
 | --enable-libx265        | enable HEVC encoding via x265 [no]       |
 | --enable-mediacodec     | enable Android MediaCodec support [no]   |
 | --enable-opengl         | enable OpenGL rendering [no]             |
 | --enable-openssl        | enable openssl, needed for https support |

 | Toolchain options:      | Description                 |
 |:------------------------|:----------------------------|
 | --arch=ARCH             | select architecture         |
 | --cpu=CPU               | select the minimum CPU      |
 | --cross-prefix=PREFIX   | use PREFIX for compile tool |
 | --enable-cross-compile  | assume a cross-compiler     |
 | --sysroot=PATH          | root of cross-build tree    |
 | --target-os=OS          | compiler targets OS         |
 | --toolchain=NAME        | set tool according to name  |
 | --nm=NM                 | use nm tool NM [nm -g]      |
 | --ar=AR                 | use archive tool AR [ar]    |
 | --as=AS                 | use assembler AS            |
 | --strip=STRIP           | use strip tool STRIP [strip]|
 | --cc=CC                 | use C compiler CC [gcc]     |
 | --cxx=CXX               | use C compiler CXX [g++]    |
 | --ld=LD                 | use linker LD               |
 | --host-cc=HOSTCC        | use host C compiler HOSTCC  |
 | --host-cflags=HCFLAGS   | use HCFLAGS when compiling  |
 | --host-ld=HOSTLD        | use host linker HOSTLD      |
 | --host-ldflags=HLDFLAGS | use HLDFLAGS when linking   |
 | --host-extralibs=HLIBS  | use libs HLIBS when linking |
 | --host-os=OS            | compiler host OS            |
 | --extra-cflags=ECFLAGS  | add ECFLAGS to CFLAGS       |
 | --extra-cxxflags=ECFLAGS| add ECFLAGS to CXXFLAGS     |
 | --extra-ldflags=ELDFLAGS| add ELDFLAGS to LDFLAGS     |

## 2、修改so后缀
默认编译出来的so库包括avcodec、avformat、avutil、avdevice、avfilter、swscale、</br>
avresample、swresample、postproc，编译出来so是个软链接，真正so名字后缀带有一长串主版本号与子版本号，</br>
这样的so名字在Adnroid平台无法识别。所以我们需要修改一下，打开该文件并搜索SLIBNAME，找到如下命令行：</br>
```
SLIBNAME_WITH_MAJOR='$(SLIBNAME).$(LIBMAJOR)'

LIB_INSTALL_EXTRA_CMD='$$(RANLIB)"$(LIBDIR)/$(LIBNAME)"'

SLIB_INSTALL_NAME='$(SLIBNAME_WITH_VERSION)'

SLIB_INSTALL_LINKS='$(SLIBNAME_WITH_MAJOR)$(SLIBNAME)'
```
替换为：
```
SLIBNAME_WITH_MAJOR='$(SLIBPREF)$(FULLNAME)-$(LIBMAJOR)$(SLIBSUF)'

LIB_INSTALL_EXTRA_CMD='$$(RANLIB)"$(LIBDIR)/$(LIBNAME)"'

SLIB_INSTALL_NAME='$(SLIBNAME_WITH_MAJOR)'

SLIB_INSTALL_LINKS='$(SLIBNAME)'
```
## 3、修改hevc_mvs.c
在linux或者mac平台交叉编译过程中，可能在libavcodec/hevc_mvs.c报错：
```
libavcodec/hevc_mvs.c: In function 'derive_spatial_merge_candidates':
libavcodec/hevc_mvs.c:208:15: error: 'y0000000' undeclared (first use in this function)
((y ## v) >> s->ps.sps->log2_min_pu_size))
^
libavcodec/hevc_mvs.c:204:14: note: in definition of macro 'TAB_MVF'
tab_mvf[(y) * min_pu_width + x]
^
libavcodec/hevc_mvs.c:274:16: note: in expansion of macro 'TAB_MVF_PU'
(cand && !(TAB_MVF_PU(v).pred_flag == PF_INTRA))
^
libavcodec/hevc_mvs.c:368:23: note: in expansion of macro 'AVAILABLE'
is_available_b0 = AVAILABLE(cand_up_right, B0) &&
^
libavcodec/hevc_mvs.c:208:15: note: each undeclared identifier is reported only once for each function it appears in
((y ## v) >> s->ps.sps->log2_min_pu_size))
^
libavcodec/hevc_mvs.c:204:14: note: in definition of macro 'TAB_MVF'
tab_mvf[(y) * min_pu_width + x]
^
libavcodec/hevc_mvs.c:274:16: note: in expansion of macro 'TAB_MVF_PU'
(cand && !(TAB_MVF_PU(v).pred_flag == PF_INTRA))
```
原因是hevc_mvs.c会引用libavutil/timer.h文件(参考：ijkplayer#issues#4093)
```
#if CONFIG_LINUX_PERF
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#endif
```
在sys/ioctl.h中最终会引用asm/termbits .h文件有#define B0 0000000，导致与hevc_mvs.c的B0命名冲突。</br>
解决方法有2种：把B0改为b0、xB0改为xb0、yB0改为yb0。</br>
或者在脚本添加：export COMMON_FF_CFG_FLAGS="$COMMON_FF_CFG_FLAGS --disable-linux-perf"</br>

## 4、编译脚本
在ffmpeg源码根目录创建一个shell脚本，比如命名为build_ffmpeg.sh，以mac编译环境为例，</br>
ndk旧版本已经过时，google官方放弃了gcc交叉编译工具链，转而使用clang编译工具链，因为clang编译速度快、效率高。</br>
下面例子是采用ndk新版的clang进行编译(如果是使用虚拟机在linux环境交叉编译FFmpeg，下面的darwin改为linux)。具体如下：</br>
```
#!/bin/bash
make clean
set -e
archbit=64

if [ $archbit -eq 64 ];then
echo "build for 64bit"
ARCH=aarch64
CPU=armv8-a
API=21
PLATFORM=aarch64
ANDROID=android
CFLAGS=""
LDFLAGS=""

else
echo "build for 32bit"
ARCH=arm
CPU=armv7-a
API=16
PLATFORM=armv7a
ANDROID=androideabi
CFLAGS="-mfloat-abi=softfp -march=$CPU"
LDFLAGS="-Wl,--fix-cortex-a8"
fi

export NDK=/Users/xufulong/Library/Android/android-ndk-r20b
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin
export SYSROOT=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/sysroot
export CROSS_PREFIX=$TOOLCHAIN/$ARCH-linux-$ANDROID-
export CC=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang
export CXX=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang++
export PREFIX=../ffmpeg-android/$CPU

function build_android {
    ./configure \
    --prefix=$PREFIX \
    --cross-prefix=$CROSS_PREFIX \
    --target-os=android \
    --arch=$ARCH \
    --cpu=$CPU \
    --cc=$CC \
    --cxx=$CXX \
    --nm=$TOOLCHAIN/$ARCH-linux-$ANDROID-nm \
    --strip=$TOOLCHAIN/$ARCH-linux-$ANDROID-strip \
    --enable-cross-compile \
    --sysroot=$SYSROOT \
    --extra-cflags="$CFLAGS" \
    --extra-ldflags="$LDFLAGS" \
    --extra-ldexeflags=-pie \
    --enable-runtime-cpudetect \
    --disable-static \
    --enable-shared \
    --disable-ffprobe \
    --disable-ffplay \
    --disable-ffmpeg \
    --disable-debug \
    --disable-doc \
    --enable-avfilter \
    --enable-avresample \
    --enable-decoders \
    $ADDITIONAL_CONFIGURE_FLAG

    make
    make install
}
build_android
```
按照上面shell脚本分为4段。第一段make clean清除缓存，set -e设置编译出错后马上退出，archbit=xx指定cpu架构是32位还是64位。</br>
第二段if...else...fi用来条件编译不同cpu架构对应字段的值。第三段用export关键字声明宏定义，其中PREFIX是指定输出文件路径。</br>
第四段是一个执行函数，按照ffmpeg的configure规范进行编写。函数里面的enable代表开启，disable代表关闭，也就是对ffmpeg进行剪裁，</br>
根据我们需要的功能进行enable。make命令是执行编译，make install命令是执行安装。最后的build_android是执行函数。</br>
初次执行shell脚本，需要修改脚本权限，使用linux命令：chmod 777 build_ffmpeg.sh。执行脚本只需要一行命令，</br>
即在命令行输入./build_ffmpeg.sh。编译过程中，命令行会不断打印编译日志，等待命令行输出INSTALL xxx关键字代表编译完成。</br>

## 5、编译问题分析
编译过程中，可能会出现这样那样的问题，比如ndk配置不对、脚本语法不对。但不用慌，编译输出窗口会描述出错原因，</br>
在ffbuild/config.log会告诉你问题的具体原因所在，顺着思路一般可以找到问题的答案。</br>

# 三、so库剪裁
ffmpeg强大之处在于支持按需编译，进行弹性剪裁。可以使用--disable-everything关闭所有模块，</br>
可以使用enable/disable来开启关闭某个模块，或者某个编解码器、某个封装器、某个协议。</br>

## 1、encoders与decoders
encoder和decoder在libavcodec模块，可以用./configure --list-encoders或者ffmpeg --encoders查看支持的编码器。</br>
同样地，可以用./configure --list-decoders或者ffmpeg --decoders查看支持的解码器。我们可以先--disable-encoders，</br>
--enable-encoder=aac,h264 其中lib开头的表示第三方库。</br>

## 2、muxers与demuxers
muxer和demuxer在libavformat模块，可用./configure --list-muxers或者ffmpeg --muxers查看支持的封装器。</br>
同样地，可用./configure --list-demuxers或者ffmpeg --demuxers查看支持的解封装器。先--disable-muxers，再--enable-muxer=mp3</br>

## 3、protocols
protocol也是在libavformat模块中，./configure --list-protocols或者ffmpeg --protocols查看支持的协议，</br>

## 4、parsers
parser在libavfilter模块中，提供各种filter，比如音频：amix、atempo，视频：rotate、hflip，</br>
我们可以根据需求--enable-parser来开启，可使用./configure --list-parsers查看具体列表。</br>

# 四、编译过程分析
编译过程包括：预编译、编译、汇编、链接。详细可以查阅书籍<程序员的自我修养>

## 1、预编译
预编译处理(.c/.cpp->.i)，对源文件的伪指令进行处理。
伪指令包括：宏定义(#define)、条件编译指令(#ifdef #elseif #endif)、头文件包含指令(#include)

## 2、编译
编译(.i->.s)，对经过预编译处理文件进行语法分析，编译为汇编文件。

## 3、汇编
汇编(.i->.o)，把汇编代码翻译为机器码，也就是二进制的目标文件。

## 4、链接
链接(.o->.so/.a/.lib)，把所有目标文件链接成动态库、静态库文件。

## 5、FFmpeg的makefile分析
makefile主要发生在汇编阶段，把所有依赖的目标文件添加进去。以FFmpeg的AVFilter模块makefile为例(有省略)：
```
NAME = avfilter
DESC = FFmpeg audio/video filtering library

HEADERS = avfilter.h                                                    \
          buffersink.h                                                  \
          buffersrc.h                                                   \
          version.h                                                     \

OBJS = allfilters.o                                                     \
       audio.o                                                          \
       avfilter.o                                                       \
       avfiltergraph.o                                                  \
       buffersink.o                                                     \
       buffersrc.o                                                      \
       drawutils.o                                                      \
       fifo.o                                                           \
       formats.o                                                        \
       framepool.o                                                      \
       framequeue.o                                                     \
       graphdump.o                                                      \
       graphparser.o                                                    \
       transform.o                                                      \
       video.o                                                          \

# audio filters
OBJS-$(CONFIG_ACOPY_FILTER)                  += af_acopy.o
OBJS-$(CONFIG_ACROSSFADE_FILTER)             += af_afade.o
OBJS-$(CONFIG_ACROSSOVER_FILTER)             += af_acrossover.o
OBJS-$(CONFIG_ADELAY_FILTER)                 += af_adelay.o
OBJS-$(CONFIG_AECHO_FILTER)                  += af_aecho.o

# video filters
OBJS-$(CONFIG_BLACKDETECT_FILTER)            += vf_blackdetect.o
OBJS-$(CONFIG_DEBLOCK_FILTER)                += vf_deblock.o
OBJS-$(CONFIG_DELOGO_FILTER)                 += vf_delogo.o
OBJS-$(CONFIG_DRAWTEXT_FILTER)               += vf_drawtext.o
OBJS-$(CONFIG_EDGEDETECT_FILTER)             += vf_edgedetect.o
OBJS-$(CONFIG_EQ_FILTER)                     += vf_eq.o
OBJS-$(CONFIG_GBLUR_FILTER)                  += vf_gblur.o
OBJS-$(CONFIG_OVERLAY_FILTER)                += vf_overlay.o framesync.o
```