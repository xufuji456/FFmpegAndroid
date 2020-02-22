
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
    //打开编码器
    mChannels = channels;
    //一次最大能输入编码器的样本数量 (一个样本是16位 2字节)
    //编码后的最大字节数
    audioCodec = faacEncOpen(static_cast<unsigned long>(samplesInHZ),
                             static_cast<unsigned int>(channels),
                             &inputSamples,
                             &maxOutputBytes);

    //设置编码器参数
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioCodec);
    //指定为 mpeg4 标准
    config->mpegVersion = MPEG4;
    //lc 标准
    config->aacObjectType = LOW;
    //16位
    config->inputFormat = FAAC_INPUT_16BIT;
    // 编码出原始数据
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec, config);

    //输出缓冲区 编码后的数据 用这个缓冲区来保存
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
    //双声道
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
    //返回编码后数据字节的长度
    int byteLen = faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data),
                                static_cast<unsigned int>(inputSamples),
                                buffer,
                                static_cast<unsigned int>(maxOutputBytes));
    if (byteLen > 0) {
        int bodySize = 2 + byteLen;
        RTMPPacket *packet = new RTMPPacket;
        RTMPPacket_Alloc(packet, bodySize);
        //双声道
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

