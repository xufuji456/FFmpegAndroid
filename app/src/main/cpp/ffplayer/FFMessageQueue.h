
#ifndef FFMPEGANDROID_FFMESSAGEQUEUE_H
#define FFMPEGANDROID_FFMESSAGEQUEUE_H

#include <mutex>
#include <libavutil/mem.h>

/**************** msg  define begin ***************/

#define FFP_MSG_FLUSH                       0
#define FFP_MSG_ERROR                       100
#define FFP_MSG_PREPARED                    200
#define FFP_MSG_COMPLETED                   300
#define FFP_MSG_VIDEO_SIZE_CHANGED          400
#define FFP_MSG_SAR_CHANGED                 401
#define FFP_MSG_VIDEO_RENDERING_START       402
#define FFP_MSG_AUDIO_RENDERING_START       403
#define FFP_MSG_VIDEO_ROTATION_CHANGED      404
#define FFP_MSG_AUDIO_DECODED_START         405
#define FFP_MSG_VIDEO_DECODED_START         406
#define FFP_MSG_OPEN_INPUT                  407
#define FFP_MSG_FIND_STREAM_INFO            408
#define FFP_MSG_COMPONENT_OPEN              409
#define FFP_MSG_VIDEO_SEEK_RENDERING_START  410
#define FFP_MSG_AUDIO_SEEK_RENDERING_START  411
#define FFP_MSG_DECODER_OPEN_ERROR          412
#define FFP_MSG_DEMUX_ERROR                 413

#define FFP_MSG_BUFFERING_START             500
#define FFP_MSG_BUFFERING_END               501
#define FFP_MSG_BUFFERING_UPDATE            502
#define FFP_MSG_BUFFERING_BYTES_UPDATE      503
#define FFP_MSG_BUFFERING_TIME_UPDATE       504
#define FFP_MSG_SEEK_COMPLETE               600
#define FFP_MSG_PLAYBACK_STATE_CHANGED      700
#define FFP_MSG_TIMED_TEXT                  800
#define FFP_MSG_VIDEO_DECODER_OPEN          900
#define FFP_MSG_OPEN_ERROR                  901

#define FFP_REQ_START                       1001
#define FFP_REQ_PAUSE                       1002
#define FFP_REQ_SEEK                        1003

/**************** msg  define end *****************/

typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    void *obj;
    void (*free_l)(void *obj);
    struct AVMessage *next;
} AVMessage;

typedef struct MessageQueue {
    AVMessage *first_msg, *last_msg;
    int nb_messages;
    int abort_request;
    std::mutex mutex;
    std::condition_variable cond;

    AVMessage *recycle_msg;
    int recycle_count;
    int alloc_count;
} MessageQueue;

class FFMessageQueue {

private:
    MessageQueue *q;

public:

    void init();

    void start();

    void flush();

    void sendMessage1(int what);

    void sendMessage2(int what, int arg1);

    void sendMessage3(int what, int arg1, int arg2);

    int get(AVMessage *msg, int block);

    void remove(int what);

    void abort();

    void destroy();
};


#endif //FFMPEGANDROID_FFMESSAGEQUEUE_H
