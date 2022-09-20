
#ifndef PUSHINTERFACE_H
#define PUSHINTERFACE_H

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"FrankLive",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FrankLive",__VA_ARGS__)

/***************relative to Java**************/
//error code for opening video encoder
const int ERROR_VIDEO_ENCODER_OPEN = 0x01;
//error code for video encoding
const int ERROR_VIDEO_ENCODE = 0x02;
//error code for opening audio encoder
const int ERROR_AUDIO_ENCODER_OPEN = 0x03;
//error code for audio encoding
const int ERROR_AUDIO_ENCODE = 0x04;
//error code for RTMP connecting
const int ERROR_RTMP_CONNECT = 0x05;
//error code for connecting stream
const int ERROR_RTMP_CONNECT_STREAM = 0x06;
//error code for sending packet
const int ERROR_RTMP_SEND_PACKET = 0x07;

/***************relative to Java**************/

#endif
