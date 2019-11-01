# FFmpegAndroid
android端基于FFmpeg库的使用。<br>
添加编译ffmpeg、shine、mp3lame源码的参考脚本<br>
目前音视频相关处理：<br>

- #### 音频剪切、拼接
- #### 音频混音
- #### 音频转码
- #### 音视频合成
- #### 音频抽取
- #### 音频解码播放
- #### 音频编码
- #### 视频抽取
- #### 视频剪切
- #### 视频转码
- #### 视频截图
- #### 视频降噪
- #### 视频抽帧
- #### 视频转GIF动图
- #### 视频添加水印
- #### 视频画面拼接
- #### 视频反序倒播
- #### 视频画中画
- #### 图片合成视频
- #### 视频解码播放
- #### 本地直播推流
- #### 实时直播推流
- #### 音视频解码播放
- #### OpenGL+GPUImage滤镜
- #### FFmpeg的AVFilter滤镜
- #### 使用mp3lame库进行mp3转码

- #### IjkPlayer的RTSP超低延时直播
- #### IjkPlayer的RTSP多路投屏直播

左边是ffplay客户端拉流播放，中间是web网页播放：

![动态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/gif/live.gif)

视频添加文字水印（文字白色背景可以改为透明）：

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/water_mark.png)

视频转成GIF动图：

![动态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/gif/VideoToGif.gif)

滤镜效果：

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/filter_balance.png)

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/filter_sketch.png)

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/filter_edge.png)

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/filter_grid.png)

视频画中画：

![静态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/picture_in_picture.png)

视频画面拼接：

![动态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/gif/horizontal.gif)

视频倒播：

![动态图片](https://github.com/xufuji456/FFmpegAndroid/blob/master/gif/reverse.gif)


mp3lame编译脚本：<br>
WORKING_DIR := $(call my-dir)<br>
include $(CLEAR_VARS)<br>
LOCAL_PATH := $(WORKING_DIR)<br>
LOCAL_ARM_MODE  := arm<br>
LOCAL_MODULE    := mp3lame<br>
LOCAL_SRC_FILES := mp3lame/bitstream.c mp3lame/encoder.c \<br>
                   mp3lame/fft.c mp3lame/gain_analysis.c mp3lame/id3tag.c \<br>
                   mp3lame/lame.c mp3lame/newmdct.c mp3lame/quantize.c \<br>
                   mp3lame/presets.c mp3lame/psymodel.c mp3lame/quantize_pvt.c \<br>
                   mp3lame/reservoir.c mp3lame/set_get.c mp3lame/tables.c \<br>
                   mp3lame/takehiro.c mp3lame/util.c mp3lame/vbrquantize.c \<br>
                   mp3lame/VbrTag.c mp3lame/version.c<br>
LOCAL_C_INCLUDES := mp3lame<br>
LOCAL_LDLIBS    := -llog -lz -lm -landroid<br>
include $(BUILD_STATIC_LIBRARY)<br>

***
<br><br>

