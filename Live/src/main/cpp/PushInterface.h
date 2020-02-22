
#ifndef PUSHINTERFACE_H
#define PUSHINTERFACE_H

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"FrankLive",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FrankLive",__VA_ARGS__)

#define DELETE(obj) if(obj){ delete obj; obj = 0; }

#define DEBUG 0

#endif
