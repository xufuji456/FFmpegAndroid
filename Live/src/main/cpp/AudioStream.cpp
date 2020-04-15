
#include <cstring>
#include "AudioStream.h"
#include "PushInterface.h"

AudioStream::AudioStream() {

}

AudioStream::~AudioStream() {
    DELETE(buffer);
    if (audioCodec) {
        faacEncClose(audioCodec);
        audioCodec = 0;
    }
}

void AudioStream::setAudioCallback(AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

void AudioStream::setAudioEncInfo(int samplesInHZ, int channels) {
    mChannels = channels;
    //open faac encoder
    audioCodec = faacEncOpen(static_cast<unsigned long>(samplesInHZ),
                             static_cast<unsigned int>(channels),
                             &inputSamples,
                             &maxOutputBytes);

    //set encoder params
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioCodec);
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->inputFormat = FAAC_INPUT_16BIT;
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec, config);

    //output buffer
    buffer = new u_char[maxOutputBytes];
}

int AudioStream::getInputSamples() {
    return static_cast<int>(inputSamples);
}

RTMPPacket *AudioStream::getAudioTag() {
    u_char *buf;
    u_long len;
    faacEncGetDecoderSpecificInfo(audioCodec, &buf, &len);
    int bodySize = static_cast<int>(2 + len);
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);
    //channel layout: stereo
    packet->m_body[0] = 0xAF;
    if (mChannels == 1) {
        packet->m_body[0] = 0xAE;
    }
    packet->m_body[1] = 0x00;

    memcpy(&packet->m_body[2], buf, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x11;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    return packet;
}

void AudioStream::encodeData(int8_t *data) {
    //encode a frame, and return encoded len
    int byteLen = faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data),
                                static_cast<unsigned int>(inputSamples),
                                buffer,
                                static_cast<unsigned int>(maxOutputBytes));
    if (byteLen > 0) {
        int bodySize = 2 + byteLen;
        RTMPPacket *packet = new RTMPPacket;
        RTMPPacket_Alloc(packet, bodySize);
        //stereo
        packet->m_body[0] = 0xAF;
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE;
        }

        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2], buffer, static_cast<size_t>(byteLen));

        packet->m_hasAbsTimestamp = 0;
        packet->m_nBodySize = bodySize;
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nChannel = 0x11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }
}

