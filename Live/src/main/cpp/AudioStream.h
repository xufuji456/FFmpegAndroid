
#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include "include/rtmp/rtmp.h"
#include "include/faac/faac.h"
#include <sys/types.h>

class AudioStream {
    typedef void (*AudioCallback)(RTMPPacket *packet);

public:
    AudioStream();

    ~AudioStream();

    void setAudioEncInfo(int samplesInHZ, int channels);

    void setAudioCallback(AudioCallback audioCallback);

    int getInputSamples();

    void encodeData(int8_t *data);

    RTMPPacket *getAudioTag();

private:
    AudioCallback audioCallback;
    int mChannels;
    faacEncHandle audioCodec = 0;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0;
};


#endif
