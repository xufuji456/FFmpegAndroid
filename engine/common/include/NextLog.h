#ifndef NEXT_LOG_H
#define NEXT_LOG_H

#define AV_LEVEL_FATAL 128
#define AV_LEVEL_ERROR 64
#define AV_LEVEL_WARN  32
#define AV_LEVEL_INFO  16
#define AV_LEVEL_DEBUG 8


int LogPrint(int level, const char *tag, const char *fmt, ...);
void SetLogLevel(int level);
void SetLogCallback(void (*callback)(void *, int, const char *), void *userdata);

typedef struct LogContext {
  int level;
  void *userdata;
  void (*callback)(void *arg, int level, const char *buf);
} LogContext;

#define NEXT_LOGD(...) LogPrint(AV_LEVEL_DEBUG, __VA_ARGS__)
#define NEXT_LOGI(...) LogPrint(AV_LEVEL_INFO, __VA_ARGS__)
#define NEXT_LOGW(...) LogPrint(AV_LEVEL_WARN, __VA_ARGS__)
#define NEXT_LOGE(...) LogPrint(AV_LEVEL_ERROR, __VA_ARGS__)

#endif
