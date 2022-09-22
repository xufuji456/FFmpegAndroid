
#include <cstring>
#include "AudioStream.h"
#include "PushInterface.h"

AudioStream::AudioStream() {

}

void AudioStream::setAudioCallback(AudioCallback callback) {
    audioCallback = callback;
}

int AudioStream::setAudioEncInfo(int samplesInHZ, int channels) {
    m_channels = channels;
    //open faac encoder
    m_audioCodec = faacEncOpen(static_cast<unsigned long>(samplesInHZ),
                             static_cast<unsigned int>(channels),
                             &m_inputSamples,
                             &m_maxOutputBytes);
    m_buffer = new u_char[m_maxOutputBytes];

    //set encoder params
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(m_audioCodec);
    config->mpegVersion   = MPEG4;
    config->aacObjectType = LOW;
    config->inputFormat   = FAAC_INPUT_16BIT;
    config->outputFormat  = 0;
    return faacEncSetConfiguration(m_audioCodec, config);
}

int AudioStream::getInputSamples() const {
    return static_cast<int>(m_inputSamples);
}

RTMPPacket *AudioStream::getAudioTag() {
    u_char *buf;
    u_long len;
    faacEncGetDecoderSpecificInfo(m_audioCodec, &buf, &len);
    int bodySize = static_cast<int>(2 + len);
    auto *packet = new RTMPPacket();
    RTMPPacket_Alloc(packet, bodySize);
    //channel layout: stereo
    packet->m_body[0] = 0xAF;
    if (m_channels == 1) {
        packet->m_body[0] = 0xAE;
    }
    packet->m_body[1] = 0x00;

    memcpy(&packet->m_body[2], buf, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize       = bodySize;
    packet->m_packetType      = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel        = 0x11;
    packet->m_headerType      = RTMP_PACKET_SIZE_LARGE;
    return packet;
}

void AudioStream::encodeData(int8_t *data) {
    //encode a frame, and return encoded len
    int byteLen = faacEncEncode(m_audioCodec, reinterpret_cast<int32_t *>(data),
                                static_cast<unsigned int>(m_inputSamples),
                                m_buffer,
                                static_cast<unsigned int>(m_maxOutputBytes));
    if (byteLen > 0) {
        int bodySize = 2 + byteLen;
        auto *packet = new RTMPPacket();
        RTMPPacket_Alloc(packet, bodySize);
        //stereo
        packet->m_body[0] = 0xAF;
        if (m_channels == 1) {
            packet->m_body[0] = 0xAE;
        }

        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2], m_buffer, static_cast<size_t>(byteLen));

        packet->m_hasAbsTimestamp = 0;
        packet->m_nBodySize       = bodySize;
        packet->m_packetType      = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nChannel        = 0x11;
        packet->m_headerType      = RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }
}

AudioStream::~AudioStream() {
    delete m_buffer;
    m_buffer = nullptr;
    if (m_audioCodec) {
        faacEncClose(m_audioCodec);
        m_audioCodec = nullptr;
    }
}

