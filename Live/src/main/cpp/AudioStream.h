
#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include "rtmp/rtmp.h"
#include "faac/faac.h"
#include <sys/types.h>

class AudioStream {
    typedef void (*AudioCallback)(RTMPPacket *packet);

private:
    AudioCallback audioCallback;
    int m_channels;
    faacEncHandle m_audioCodec = 0;
    u_long m_inputSamples;
    u_long m_maxOutputBytes;
    u_char *m_buffer = 0;

public:
    AudioStream();

    ~AudioStream();

    int setAudioEncInfo(int samplesInHZ, int channels);

    void setAudioCallback(AudioCallback audioCallback);

    int getInputSamples() const;

    void encodeData(int8_t *data);

    RTMPPacket *getAudioTag();

};


#endif
