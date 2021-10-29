# FFmpegAndroid

### [FFmpeg官方文档](https://ffmpeg.org/)
### [FFmpeg编译流程](https://github.com/xufuji456/FFmpegAndroid/blob/master/doc/FFmpeg_compile_shell.md)
### [FFmpeg常用命令行](https://github.com/xufuji456/FFmpegAndroid/blob/master/doc/FFmpeg_command_line.md)
### [NDK编译脚本](https://github.com/xufuji456/FFmpegAndroid/blob/master/doc/NDK_compile_shell.md)
### [JNI开发与调试](https://github.com/xufuji456/FFmpegAndroid/blob/master/doc/JNI_develop_debug.md)
### [音视频知识汇总](https://github.com/xufuji456/FFmpegAndroid/blob/master/doc/JNI_develop_debug.md)

### Usage:
### (1) Transform video format:
Select video file which you want to transform, and setting the output path.<br>
The simple command like this:<br>
ffmpeg -i %s -vcodec copy -acodec copy %s<br>
You could appoint the encoder, like this:<br>
ffmpeg -i %s -vcodec libx264 -acodec libmp3lame %s<br>
You could transform the video resolution, like this:<br>
ffmpeg -i %s -s 1080x720 %s<br>

### (2) Probing media format:
Select video or audio file from your file explorer, and click the button.<br>
When it finishes probing, the result of metadata will display on screen.<br>

### Joining the group to learn FFmpeg:
![preview](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/ffmpeg_group.png)

### Joining QQ group to learn FFmpeg:
![preview](https://github.com/xufuji456/FFmpegAndroid/blob/master/picture/ffmpeg_qq.png)

### Preview thumbnail when seeking:
![preview](https://github.com/xufuji456/FFmpegAndroid/blob/master/gif/preview.gif)

