/**
 * Note: message of next player
 * Date: 2025/11/26
 * Author: frank
 */

#ifndef NEXT_MESSAGE_H
#define NEXT_MESSAGE_H

enum PlayerRequest {
    REQUEST_START        = 10001,
    REQUEST_PAUSE        = 10002,
    REQUEST_SEEK         = 10003,
    REQUEST_KERNEL_PAUSE = 10004,
    REQUEST_PLAY_SPEED   = 10005
};

enum PlayerMsg {
    MSG_COMPONENT_OPEN     = 1000,
    MSG_OPEN_INPUT         = 1001,
    MSG_FIND_STREAM_INFO   = 1002,
    MSG_VIDEO_DECODER_OPEN = 1003,
    MSG_VIDEO_FIRST_PACKET = 1004,
    MSG_AUDIO_DECODE_START = 1005,
    MSG_VIDEO_DECODE_START = 1006,
    MSG_ON_PREPARED        = 1007,
    MSG_VIDEO_SIZE_CHANGED = 1008, // arg1 = width, arg2 = height
    MSG_SAR_CHANGED        = 1009, // arg1 = num, arg2 = den
    MSG_ROTATION_CHANGED   = 1010, // arg1 = rotate degree
    MSG_VIDEO_RENDER_START = 1011,
    MSG_AUDIO_RENDER_START = 1012,
    MSG_ON_FLUSH           = 1013,
    MSG_ON_ERROR           = 1014, //arg1 = error msg
    MSG_ON_COMPLETED       = 1015,
    MSG_MEDIA_INFO         = 1016,

    MSG_BUFFER_START       = 2000, // arg1: 1=seek 2=network 3=decode
    MSG_BUFFER_UPDATE      = 2001, // arg1 = progress percent
    MSG_BUFFER_BYTE_UPDATE = 2002, // arg1 = cached data in bytes
    MSG_BUFFER_TIME_UPDATE = 2003, // arg1 = cached duration in ms
    MSG_BUFFER_END         = 2004,

    MSG_VIDEO_SEEK_RENDER_START = 3000,
    MSG_AUDIO_SEEK_RENDER_START = 3001,
    MSG_SEEK_LOOP_START         = 3002, // arg1 = loop count
    MSG_SEEK_COMPLETE           = 3003, // arg1 = seek position
    MSG_ACCURATE_SEEK_COMPLETE  = 3004,
    MSG_PLAY_STATE_CHANGED      = 3005,
    MSG_PLAY_URL_CHANGED        = 3006,
    MSG_SUBTITLE_UPDATE         = 3007

};

#endif
