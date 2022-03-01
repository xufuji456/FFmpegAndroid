/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#ifndef MEDIA_RETRIEVER_H
#define MEDIA_RETRIEVER_H

#include <Mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ffmpeg_media_retriever.h"

#ifdef __cplusplus
}
#endif

class MediaRetriever
{
	State* state;
public:
    MediaRetriever();
    ~MediaRetriever();
    int setDataSource(const char* dataSourceUrl);
    int setDataSource(int fd, int64_t offset, int64_t length);
    const char* extractMetadata(const char* key);
    int getFrameAtTime(int64_t timeUs, int option, AVPacket *pkt);
    int getScaledFrameAtTime(int64_t timeUs, int option, AVPacket *pkt, int width, int height);
	int getAudioThumbnail(AVPacket *pkt);
    int setNativeWindow(ANativeWindow* native_window);

private:
    Mutex mLock;
};

#endif // MEDIA_RETRIEVER_H
