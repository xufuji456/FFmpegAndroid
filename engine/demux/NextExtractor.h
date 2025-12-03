#ifndef NEXT_EXTRACTOR_H
#define NEXT_EXTRACTOR_H

#include "ExtractorInterface.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif

class NextExtractor : public ExtractorInterface {
public:

    explicit NextExtractor(NotifyCallback &notifyCb);

    ~NextExtractor() override;

    int Open(const std::string &url, FFmpegOption &opt,
             std::shared_ptr<MetaData> &metadata) override;

    int ReadPacket(AVPacket *pkt) override;

    int Seek(int64_t timestamp, int64_t rel, int seekFlags) override;

    int GetError() override;

    int GetStreamType(int streamIndex) override;

    void SetInterrupt() override;

    void Close() override;

private:
    static int InterruptCallback(void *opaque);

    void NotifyListener(int32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                        void *obj = nullptr, int len = 0);

private:
    NotifyCallback mNotifyCb;
    std::atomic_bool bAbort {false};
    AVFormatContext *mFormatCtx = nullptr;

};

#endif
