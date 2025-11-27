#include "NextLog.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define TIME_BUF_SIZE 32
#define PRINT_BUF_SIZE 1024
#define OUTPUT_BUF_SIZE 1256

static LogContext kLogContext = {0};

#if defined(__APPLE__)
static void GetLocalTimeString(char *buf, int buf_size) {
  struct timeval t;
  gettimeofday(&t, NULL);
  struct tm *ptm = localtime(&t.tv_sec);
  snprintf(buf, buf_size, "%02d-%02d %02d:%02d:%02d.%03d", ptm->tm_mon + 1,
           ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
           static_cast<int>(t.tv_usec / 1000));
  return;
}

static char GetLogLevelChar(int lev) {
  switch (lev) {
  case AV_LEVEL_INFO:
    return 'I';
  case AV_LEVEL_DEBUG:
    return 'D';
  case AV_LEVEL_WARN:
    return 'W';
  case AV_LEVEL_ERROR:
    return 'E';
  case AV_LEVEL_FATAL:
    return 'F';
  default:
    return ' ';
  }
  return ' ';
}
#endif

static void FormatLog(int level, const char *tag, int id, char *input_buf,
                      char *output_buf, int out_buf_size) {
#if defined(__APPLE__) || defined(__HARMONY__)
    int pid = getpid();
#endif
#if defined(__APPLE__)
    uint64_t atid;
    pthread_threadid_np(NULL, &atid);
    char level = GetLogLevelChar(level);
    char time_buf[TIME_BUF_SIZE] = {0};
    GetLocalTimeString(time_buf, TIME_BUF_SIZE);
#elif defined(__HARMONY__)
    pid_t tid = syscall(SYS_gettid);
#endif
    if (id > 0) {
#if defined(__APPLE__)
        snprintf(output_buf, out_buf_size,
                 "%s %d 0x%" PRIu64 " %c [%s]: [id @ %04d] %s", time_buf, pid, atid,
                 level, tag, id, input_buf);
#elif defined(__HARMONY__)
        snprintf(output_buf, out_buf_size, "%d %d [%s]: [id @ %04d] %s", pid, tid, tag,
                 id, input_buf);
#else
        snprintf(output_buf, out_buf_size, "[%s]: [id @ %04d] %s", tag, id, input_buf);
#endif
    } else {
#if defined(__APPLE__)
        snprintf(output_buf, out_buf_size, "%s %d 0x%" PRIu64 " %c [%s]: %s", time_buf,
                 pid, atid, level, tag, input_buf);
#elif defined(__HARMONY__)
        snprintf(output_buf, out_buf_size, "%d %d [%s]: %s", pid, tid, tag, input_buf);
#else
        snprintf(output_buf, out_buf_size, "[%s]: %s", tag, input_buf);
#endif
    }

}

int LogPrint(int level, const char *tag, const char *fmt, ...) {
    if (kLogContext.level < level) {
        return 0;
    }

    char print_buf[PRINT_BUF_SIZE] = {0};
    char output_buf[OUTPUT_BUF_SIZE] = {0};
    va_list args;
    int printed;
    va_start(args, fmt);
    printed = vsnprintf(print_buf, PRINT_BUF_SIZE - 1, fmt, args);
    va_end(args);

    if (printed <= 0) {
        return printed;
    }

    FormatLog(level, tag, 0, print_buf, output_buf, OUTPUT_BUF_SIZE);
    if (kLogContext.callback) {
        kLogContext.callback(kLogContext.userdata, level, output_buf);
        return 0;
    }

    return 0;
}

void SetLogLevel(int level) {
    kLogContext.level = level;
}

void SetLogCallback(void (*callback)(void *, int, const char *), void *userdata) {
    kLogContext.callback = callback;
    kLogContext.userdata = userdata;
}
