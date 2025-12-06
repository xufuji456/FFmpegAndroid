#include "MixedBuffer.h"

MixedBuffer::MixedBuffer(BufferType type, uint8_t *data, int size, bool own_data)
        : mSize(size),
          mData(data),
          bOwnData(own_data),
          mBufferType(type) {
    InitType(type);
}

MixedBuffer::MixedBuffer(BufferType type, int capacity)
        : mSize(0),
          mData(new uint8_t[capacity]),
          bOwnData(true),
          mBufferType(type) {
    InitType(type);
}

MixedBuffer::~MixedBuffer() {
    if (bOwnData) {
        delete[] mData;
    }
}

void MixedBuffer::InitType(BufferType type) {
    switch (type) {
        case BufferType::BUFFER_VIDEO_FRAME:
            mVideoFrameMetadata = std::make_unique<VideoFrameMetadata>();
            break;
        case BufferType::BUFFER_AUDIO_FRAME:
            mAudioFrameMetadata = std::make_unique<AudioFrameMetadata>();
            break;
        case BufferType::BUFFER_VIDEO_FORMAT:
            mVideoFormatMetadata = std::make_unique<VideoFormatMetadata>();
            break;
        case BufferType::BUFFER_VIDEO_PACKET:
            mVideoPacketMetadata = std::make_unique<VideoPacketMetadata>();
            break;
        case BufferType::BUFFER_AUDIO_PACKET:
            mAudioPacketMetadata = std::make_unique<AudioPacketMetadata>();
            break;
        default:
            break;
    }
}

int MixedBuffer::GetSize() const {
    return mSize;
}

uint8_t *MixedBuffer::GetData() const {
    return mData;
}

BufferType MixedBuffer::GetType() const {
    return mBufferType;
}

uint8_t *MixedBuffer::ObtainData() {
    if (bOwnData && mSize > 0) {
        bOwnData = false;
        return mData;
    }
    return nullptr;
}

VideoFrameMetadata *MixedBuffer::GetVideoFrameMetadata() const {
    return mVideoFrameMetadata.get();
}

AudioFrameMetadata *MixedBuffer::GetAudioFrameMetadata() const {
    return mAudioFrameMetadata.get();
}

VideoPacketMetadata *MixedBuffer::GetVideoPacketMetadata() const {
    return mVideoPacketMetadata.get();
}

AudioPacketMetadata *MixedBuffer::GetAudioPacketMetadata() const {
    return mAudioPacketMetadata.get();
}

VideoFormatMetadata *MixedBuffer::GetVideoFormatMetadata() const {
    return mVideoFormatMetadata.get();
}

void MixedBuffer::UpdateBuffer(uint8_t *data, int size, bool ownData) {
    if (bOwnData) {
        delete[] mData;
    }

    mData    = data;
    mSize    = size;
    bOwnData = ownData;
}
