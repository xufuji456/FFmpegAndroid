/**
 * Note: Handler of Video Render
 * Date: 2026/6/8
 * Author: frank
 */

#include "VideoRenderHandler.h"

#include <unistd.h>

#include "common/NextConfig.h"
#include "common/NextSpeedMeter.h"
#include "CommonUtil.h"
#include "NextErrorCode.h"
#include "NextLog.h"
#include "NextMessage.h"

#define TAG "ViRenderHandler"

VideoRenderHandler::VideoRenderHandler(sp<VideoDecodeHandler> &videoDecodeHandler,
                                       const sp<PlayerLink> &pLink,
                                       NotifyCallback notifyCb, const char *threadName)
        : BaseThread(threadName),
          mPlayerLink(pLink),
          mVideoDecodeHandler(videoDecodeHandler),
          mNotifyCb(std::move(notifyCb)) {}

VideoRenderHandler::~VideoRenderHandler() {
    mVideoRender.reset();
    mMetaData.reset();
    sws_freeContext(mSwsContext);
}

int VideoRenderHandler::prepare(sp<MetaData> &metadata) {
    mMetaData = metadata;
    this->start();

    return RESULT_OK;
}

#if defined(__ANDROID__)
int VideoRenderHandler::setVideoSurface(ANativeWindow *surface) {
    if (!surface) {
        return ERROR_RENDER_VIDEO_INIT;
    }

    if (!mNativeWindow || mNativeWindow != surface) {
        mNativeWindow = surface;
        if (bPaused && mRenderType == VideoRenderType::VIDEO_RENDER_OPENGL) {
            bForceRefresh = true;
        }
        mSurfaceUpdate.store(true);
    }

    return RESULT_OK;
}
#endif

#if defined(__APPLE__)
UIView *VideoRenderHandler::initWithFrame(int type, CGRect cgrect) {
    std::unique_lock<std::mutex> lock(mLock);

    switch (type) {
    case VIDEO_RENDER_OPENGL: {
        VideoRendererInfo videRendererInfo(VIDEO_RENDER_OPENGL);
        mRenderType = VIDEO_RENDER_OPENGL;
        mVideoRender = VideoRendererFactory::CreateVideoRender(videRendererInfo);
        break;
    }
    case VIDEO_RENDER_METAL: {
        VideoRendererInfo videRendererInfo(VIDEO_RENDER_METAL);
        mRenderType = VIDEO_RENDER_METAL;
        mVideoRender = VideoRendererFactory::CreateVideoRender(videRendererInfo);
        break;
    }
    case VIDEO_RENDER_SAMPLEBUFFER: {
        VideoRendererInfo videRendererInfo(VIDEO_RENDER_SAMPLEBUFFER);
        mRenderType = VIDEO_RENDER_SAMPLEBUFFER;
        mVideoRender = VideoRendererFactory::CreateVideoRenderer(videRendererInfo);
        break;
    }
    default:
        RS_LOGE(TAG, "unknown render type %d .\n", type);
        break;
    }

    if (!mVideoRender) {
        RS_LOGE(TAG, "VideoRender create error\n");
        return nullptr;
    }

    int ret = mVideoRender->init();
    if (ret != RESULT_OK) {
        lock.unlock();
        NotifyListener(MSG_ON_ERROR, ERROR_VIDEO_DISPLAY, ret);
        RS_LOGE(TAG, "init error: %d\n", ret);
        return nullptr;
    }

    ret = mVideoRender->initWithFrame(cgrect);
    if (ret != RESULT_OK) {
        lock.unlock();
        NotifyListener(MSG_ON_ERROR, ERROR_VIDEO_DISPLAY, ret);
        RS_LOGE(TAG, "initWithFrame error: %d\n", ret);
        return nullptr;
    }

    return mVideoRender->getRedRenderView();
}
#endif

int VideoRenderHandler::init() {
    std::unique_lock<std::mutex> lock(mLock);

    switch (mPlayerLink->stat.video_dec_type) {
        case OPTION_STR_DECODER_AVCODEC:
            mRenderType = VIDEO_RENDER_OPENGL;
            break;
        case OPTION_STR_DECODER_MEDIACODEC:
            mRenderType = VIDEO_RENDER_MEDIACODEC;
            break;
        case OPTION_STR_DECODER_VTB:
            mRenderType = VIDEO_RENDER_OPENGL;
            break;
        default:
            NEXT_LOGE(TAG, "Unknown video decoder type: %d\n", mPlayerLink->stat.video_dec_type);
            return ERROR_PLAYER_NOT_INIT;
    }

    mVideoRender = VideoRendererFactory::createVideoRender(mRenderType);

    if (!mVideoRender) {
        NEXT_LOGE(TAG, "mVideoRender create error");
        return ERROR_PLAYER_NOT_INIT;
    }

    int ret = mVideoRender->init();
    if (ret != RESULT_OK) {
        lock.unlock();
        notifyListener(MSG_ON_ERROR, ERROR_RENDER_VIDEO_INIT, ret);
        NEXT_LOGE(TAG, "mVideoRender initInternal error");
        return ERROR_PLAYER_NOT_INIT;
    }

    updateVideoMetaData();
    return RESULT_OK;
}

int VideoRenderHandler::updateVideoMetaData() {

    switch (mRenderType) {
        case VIDEO_RENDER_OPENGL:
        case VIDEO_RENDER_METAL:
        case VIDEO_RENDER_SAMPLEBUFFER: {
            TrackInfo track_info = mMetaData->track_info[mMetaData->video_index];
            mVideoFrameMetaData.frame_width     = track_info.width;
            mVideoFrameMetaData.frame_height    = track_info.height;
            mVideoFrameMetaData.view_width      = track_info.width;
            mVideoFrameMetaData.view_height     = track_info.height;
            mVideoFrameMetaData.sar.den         = track_info.sar_den;
            mVideoFrameMetaData.sar.num         = track_info.sar_num;
            mVideoFrameMetaData.aspect_ratio    = ASPECT_RATIO_FILL;
            mVideoFrameMetaData.rotation_mode   = ROTATION_NO;
            mVideoFrameMetaData.color_primaries = AVCOL_PRI_RESERVED0;
            mVideoFrameMetaData.color_trc       = AVCOL_TRC_RESERVED0;
            switch (track_info.color_space) {
                case AVCOL_SPC_BT2020_NCL:
                    mVideoFrameMetaData.color_space = AVCOL_SPC_BT2020_NCL;
                    break;
                case AVCOL_SPC_BT2020_CL:
                    mVideoFrameMetaData.color_space = AVCOL_SPC_BT2020_CL;
                    break;
                case AVCOL_SPC_SMPTE170M:
                case AVCOL_SPC_BT470BG:
                    mVideoFrameMetaData.color_space = AVCOL_SPC_SMPTE170M;
                    break;
                case AVCOL_SPC_SMPTE240M:
                    break;
                case AVCOL_SPC_BT709:
                default:
                    mVideoFrameMetaData.color_space = AVCOL_SPC_BT709;
                    break;
            }
            if (track_info.color_range == AVCOL_RANGE_JPEG) {
                mVideoFrameMetaData.color_range = AVCOL_RANGE_JPEG;
            } else {
                mVideoFrameMetaData.color_range = AVCOL_RANGE_MPEG;
            }
            break;
        }
        case VIDEO_RENDER_MEDIACODEC:
            break;
        default:
            NEXT_LOGE(TAG, "Unknown video render type: ", mRenderType);
            break;
    }

    switch (mPlayerLink->stat.video_dec_type) {
        case OPTION_STR_DECODER_AVCODEC:
            mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_YUV420P;
            break;
        case OPTION_STR_DECODER_MEDIACODEC:
            mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_MEDIACODEC;
            break;
        case OPTION_STR_DECODER_VTB:
            mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_VIDEOTOOLBOX;
            break;
        default:
            mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_UNKNOWN;
            NEXT_LOGE(TAG, "Unknown video decode type: %d\n", mPlayerLink->stat.video_dec_type);
            return ERROR_PLAYER_NOT_INIT;
    }

    if (mGeneralConfig->playerConfig->get()->video_hdr_enable &&
        mPlayerLink->stat.pixel_format == AV_PIX_FMT_YUV420P10LE) {
        mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_YUV420P10LE;
    }

    return RESULT_OK;
}

int VideoRenderHandler::startRender() {
    std::unique_lock<std::mutex> lock(mLock);
    bPaused = false;
    mPlayerLink->video_clock->setClock(mPlayerLink->video_clock->getClock());
    mPlayerLink->video_clock->setPause(false);
    return RESULT_OK;
}

int VideoRenderHandler::pauseRender() {
    std::unique_lock<std::mutex> lock(mLock);
    bPaused = true;
    mPlayerLink->video_clock->setPause(true);
    return RESULT_OK;
}

// compute delay of current frame
double VideoRenderHandler::computeDelay(double delay) {
    double diff = 0.0;
    if (!mPlayerLink->audio_clock || !mPlayerLink->video_clock)
        return delay;
    if (getMasterSyncType(mPlayerLink) != CLOCK_VIDEO) {
        diff = mPlayerLink->video_clock->getClock() - getMasterClock(mPlayerLink);
        double syncThreshold =
                std::max(AV_SYNC_THRESHOLD_MIN, std::min(AV_SYNC_THRESHOLD_MAX, delay));
        if (std::abs(diff) < AV_NO_SYNC_THRESHOLD) {
            if (diff <= -syncThreshold) {
                delay = std::max(0.0, delay + diff);
            } else if (diff >= syncThreshold && delay > FRAME_DROP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= syncThreshold) {
                delay *= 2;
            }
        }
    }
    delay /= mPlayerLink->play_rate;
    mPlayerLink->stat.av_diff  = static_cast<float>(diff);
    mPlayerLink->stat.av_delay = static_cast<float>(delay);
    return delay;
}

// compute duration of current frame
double VideoRenderHandler::computeDuration(std::unique_ptr<FrameBuffer> &buffer) const {
    if (buffer->serial == mFrameTick.serial) {
        double duration = static_cast<double>(buffer->pts) / 1000.0 - mFrameTick.pts;
        if (duration <= FLT_EPSILON || duration > MAX_FRAME_DURATION) {
            return mFrameTick.duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

int VideoRenderHandler::readFrame(std::unique_ptr<FrameBuffer> &buffer) {
    if (!mVideoDecodeHandler)
        return ERROR_PARSE_NOT_INIT;
    return mVideoDecodeHandler->getFrame(buffer);
}

int VideoRenderHandler::renderFrame(std::unique_ptr<FrameBuffer> &buffer) {
    if (convertPixelFormat(buffer) != RESULT_OK) {
        return ERROR_RENDER_VIDEO_SWS;
    }

    switch (mRenderType) {
#if defined(__ANDROID__)
        case VIDEO_RENDER_MEDIACODEC: {
            mVideoRenderBufferContext.buffer_index =
                    ((MediaCodecBufferContext *) (buffer->opaque))->buffer_index;
            mVideoRenderBufferContext.buffer_data =
                    ((MediaCodecBufferContext *) (buffer->opaque))->media_codec;
            mVideoRenderBufferContext.decoder_serial =
                    ((MediaCodecBufferContext *) (buffer->opaque))->decoder_serial;
            mVideoRenderBufferContext.decoder =
                    ((MediaCodecBufferContext *) (buffer->opaque))->decoder;
            mVideoRenderBufferContext.opaque =
                    ((MediaCodecBufferContext *) (buffer->opaque))->opaque;
            mVideoRenderBufferContext.release_buffer =
                    [](VideoRenderBufferContext *context, bool render) -> void {
                        MediaCodecBufferContext mediaCodecCtx{};
                        mediaCodecCtx.decoder        = context->decoder;
                        mediaCodecCtx.buffer_index   = context->buffer_index;
                        mediaCodecCtx.media_codec    = context->buffer_data;
                        mediaCodecCtx.decoder_serial = context->decoder_serial;

                        auto *releaseOutputBuffer =
                        (void (*)(MediaCodecBufferContext *context, bool render)) context->opaque;
                        releaseOutputBuffer(&mediaCodecCtx, render);
                    };
            // do rendering video frame
            int ret = mVideoRender->onRender(&mVideoRenderBufferContext, true);

            if (ret != RESULT_OK) {
                notifyListener(MSG_ON_ERROR, ERROR_RENDER_HANDLE, ret);
                NEXT_LOGE(TAG, "onRender error: %d\n", ret);
            }
            if (buffer->opaque) {
                delete (MediaCodecBufferContext *) buffer->opaque;
                buffer->opaque = nullptr;
            }
            break;
        }
#endif
        case VIDEO_RENDER_OPENGL:
        case VIDEO_RENDER_METAL:
        case VIDEO_RENDER_SAMPLEBUFFER: {
            if (buffer->pixel_format == PIXEL_FORMAT_YUV420P10LE) {
                mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_YUV420P10LE;
            } else if (buffer->pixel_format == PIXEL_FORMAT_VIDEOTOOLBOX) {
                mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_VIDEOTOOLBOX;
#if defined(__APPLE__)
                mVideoFrameMetaData.pixel_buffer =
                    (CVPixelBufferRef)((FrameBuffer::VideoToolBufferContext*)(buffer->opaque))->buffer;
#endif
            } else if (buffer->pixel_format == PIXEL_FORMAT_YUV420P ||
                       buffer->pixel_format == PIXEL_FORMAT_YUVJ420P) {
                mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_YUV420P;
            }
#if defined(__APPLE__)
            mVideoFrameMetaData.pixel_buffer =
                (CVPixelBufferRef)((FrameBuffer::VideoToolBufferContext*)(buffer->opaque))->buffer;
#endif
            mVideoFrameMetaData.pitches[0]   = buffer->yBuffer;
            mVideoFrameMetaData.pitches[1]   = buffer->uBuffer;
            mVideoFrameMetaData.pitches[2]   = buffer->vBuffer;
            mVideoFrameMetaData.linesize[0]  = buffer->yStride;
            mVideoFrameMetaData.linesize[1]  = buffer->uStride;
            mVideoFrameMetaData.linesize[2]  = buffer->vStride;
            mVideoFrameMetaData.frame_width  = buffer->width;
            mVideoFrameMetaData.frame_height = buffer->height;
            mVideoFrameMetaData.view_width   = buffer->width;
            mVideoFrameMetaData.view_height  = buffer->height;

            int ret = mVideoRender->onInputFrame(&mVideoFrameMetaData);
            if (ret == RESULT_OK) {
                ret = mVideoRender->onRender();
                if (ret != RESULT_OK) {
                    notifyListener(MSG_ON_ERROR, ERROR_RENDER_HANDLE, ret);
                    NEXT_LOGE(TAG, "onRender error\n");
                }
            } else {
                notifyListener(MSG_ON_ERROR, ERROR_RENDER_INPUT, ret);
                NEXT_LOGE(TAG, "onInputFrame error\n");
            }
#if defined(__APPLE__)
            if (buffer->pixel_fmt == FrameBuffer::PIXEL_FMT_VIDEOTOOLBOX && buffer->opaque &&
                ((FrameBuffer::VideoToolBufferContext *)(buffer->opaque))->buffer) {
                CVBufferRelease((CVPixelBufferRef)((FrameBuffer::VideoToolBufferContext*)(buffer->opaque))->buffer);
                delete (FrameBuffer::VideoToolBufferContext *)buffer->opaque;
                buffer->opaque = nullptr;
            }
#endif
            break;
        }
        case VIDEO_RENDER_UNKNOWN:
        default:
            NEXT_LOGE(TAG, "unknown video render type\n");
            break;
    }
    if (mPlayerLink->last_video_seek_serial == buffer->serial) {
        int lastSeekSerial =
                mPlayerLink->last_video_seek_serial.exchange(-1, std::memory_order_seq_cst);
        if (lastSeekSerial == buffer->serial) {
            mPlayerLink->stat.last_seek_time =
                    (CurrentTimeUs() - mPlayerLink->last_seek_load_start) / 1000;
            NEXT_LOGI(TAG, "video seek complete, cost=%" PRId64 ", serial=%d\n",
                    mPlayerLink->stat.last_seek_time, lastSeekSerial);
            if (getMasterSyncType(mPlayerLink) == CLOCK_VIDEO) {
                notifyListener(MSG_VIDEO_SEEK_RENDER_START, 1);
            } else {
                notifyListener(MSG_VIDEO_SEEK_RENDER_START, 0);
            }
        }
    }
    mPlayerLink->stat.render_rate = mSpeedMeter.add();
    if (!mPlayerLink->first_video_rendered) {
        mPlayerLink->first_video_rendered = true;
        notifyListener(MSG_VIDEO_RENDER_START);
    }
    if (bPaused) {
        while (bPaused && !bAbort && !mPlayerLink->step_to_next_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_20MS));
        }
        if (bAbort) {
            return RESULT_OK;
        }
        if (mPlayerLink->step_to_next_frame) {
            mPlayerLink->step_to_next_frame = false;
        }
    }
    return RESULT_OK;
}

int VideoRenderHandler::convertPixelFormat(std::unique_ptr<FrameBuffer> &buffer) {
    int ret = 0;
    if (buffer->pixel_format == PIXEL_FORMAT_YUV420P10LE &&
        mRenderType != VIDEO_RENDER_SAMPLEBUFFER) {
        auto *srcFrame = reinterpret_cast<AVFrame *>(
                (reinterpret_cast<FFmpegBufferContext *>(buffer->opaque))->av_frame);
        AVFrame *dstFrame = av_frame_alloc();
        if (!dstFrame) {
            return ERROR_RENDER_VIDEO_SWS;
        }
        dstFrame->format  = AV_PIX_FMT_YUV420P;
        dstFrame->width   = buffer->width;
        dstFrame->height  = buffer->height;
        if (av_frame_get_buffer(dstFrame, 32) < 0) {
            av_frame_free(&dstFrame);
            return ERROR_RENDER_VIDEO_SWS;
        }

        mSwsContext = sws_getCachedContext(
                mSwsContext, srcFrame->width, srcFrame->height,
                (AVPixelFormat) srcFrame->format, dstFrame->width, dstFrame->height,
                (AVPixelFormat) dstFrame->format, SWS_BICUBIC, nullptr, nullptr, nullptr);

        if (!mSwsContext ||
            (ret = sws_scale(mSwsContext, srcFrame->data, srcFrame->linesize, 0,
                             srcFrame->height, dstFrame->data, dstFrame->linesize)) < 0) {
            NEXT_LOGE(TAG, "Failed to convert pix_fmt=%d, ret=%d\n", buffer->pixel_format, ret);
            av_frame_unref(dstFrame);
            av_frame_free(&dstFrame);
            return ERROR_RENDER_VIDEO_SWS;
        }

        buffer->width   = dstFrame->width;
        buffer->height  = dstFrame->height;
        buffer->yStride = dstFrame->linesize[0];
        buffer->uStride = dstFrame->linesize[1];
        buffer->vStride = dstFrame->linesize[2];
        buffer->yBuffer = dstFrame->data[0];
        buffer->uBuffer = dstFrame->data[1];
        buffer->vBuffer = dstFrame->data[2];
        buffer->pixel_format = PIXEL_FORMAT_YUV420P;
        mVideoFrameMetaData.pixel_format = PIXEL_FORMAT_YUV420P;

        (reinterpret_cast<FFmpegBufferContext *>(buffer->opaque))
                ->av_frame = reinterpret_cast<void *>(dstFrame);

        av_frame_unref(srcFrame);
        av_frame_free(&srcFrame);
    }
    return RESULT_OK;
}

void VideoRenderHandler::notifyListener(int what, int arg1, int arg2) {
    if (mNotifyCb) {
        mNotifyCb(what, arg1, arg2, nullptr, 0);
    }
}

void VideoRenderHandler::setConfig(const sp<GeneralConfig> &config) {
    std::lock_guard<std::mutex> lock(mLock);
    mGeneralConfig = config;
}

// render thread looping
void VideoRenderHandler::executeTask() {
    int ret = init();
    if (ret != RESULT_OK) {
        notifyListener(MSG_ON_ERROR, ERROR_RENDER_VIDEO_INIT);
        mMetaData.reset();
        NEXT_LOGE(TAG, "VideoRender init error, ret=%d\n", ret);
        return;
    }
    updateVideoMetaData();

#if defined(__APPLE__)
    if (mRenderType == RedRender::VIDEO_RENDER_OPENGL) {
        mVideoRender->attachFilter(RedRender::VIDEO_FILTER_OPENGL, &mVideoFrameMetaData);
    } else if (mRenderType == RedRender::VIDEO_RENDER_METAL) {
        mVideoRender->attachFilter(RedRender::VIDEO_FILTER_METAL, &mVideoFrameMetaData);
    }
#endif

    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();

    double delay          = 0.0;
    double duration       = 0.0;
    double remainTime     = 0.0;
    int64_t lastCheckTime = 0;

    while (!bAbort) {
#if defined(__ANDROID__)
        if (mVideoRender && mRenderType == VIDEO_RENDER_OPENGL) {
            if (mSurfaceUpdate.load()) {
                auto window = mNativeWindow;
                ret = mVideoRender->setSurface(window);
                mSurfaceUpdate = false;
                if (ret != RESULT_OK) {
                    NEXT_LOGE(TAG, "Set surface failed, ret=%d\n", ret);
                }
            }
        }
#endif

        std::unique_ptr<FrameBuffer> frameBuffer;
        ret = readFrame(frameBuffer);
        if (ret == ERROR_PLAYER_EOF) {
            std::unique_lock<std::mutex> lock(mLock);
            if (!bAbort) {
                mCond.wait_for(lock, std::chrono::milliseconds(SLEEP_20MS));
            }
            continue;
        } else if (ret == ERROR_PLAYER_TRY_AGAIN) {
            // getFrame already waited for a while
            continue;
        } else if (ret != RESULT_OK || !frameBuffer) {
            usleep(SLEEP_20MS_CONVERT_US);
            continue;
        }
        if (frameBuffer->serial != mVideoDecodeHandler->getSerial()) {
            continue;
        }
        while (!bAbort) {
#if defined(__ANDROID__)
            {
                if (mSurfaceUpdate.load() && mVideoRender && mNativeWindow
                    && mRenderType == VIDEO_RENDER_OPENGL) {
                    mVideoRender->setSurface(mNativeWindow);
                    mSurfaceUpdate = false;
                }
            }
#endif
            if (frameBuffer->serial != mVideoDecodeHandler->getSerial()) {
                break;
            }

            double time = static_cast<double>(CurrentTimeUs()) / 1000000.0;
            duration    = computeDuration(frameBuffer);
            delay       = computeDelay(duration);

            if (!isnan(mPlayerLink->stat.av_diff)) {
                if (std::abs(mPlayerLink->stat.av_diff) > 1.0 &&
                    CurrentTimeMs() - lastCheckTime > 1000) {
                    lastCheckTime = CurrentTimeMs();
                    NEXT_LOGI(TAG, "av unsync A: %f, V: %f\n",
                            getMasterClock(mPlayerLink), mPlayerLink->video_clock->getClock());
                }
            }

            if (frameBuffer->serial != mSerial) {
                mFrameTick.time = static_cast<double>(CurrentTimeUs()) / 1000000.0;
                mSerial = frameBuffer->serial;
            }

            if (mFrameTick.time <= FLT_EPSILON || time < mFrameTick.time) {
                mFrameTick.time = time;
            }

            if (time < mFrameTick.time + delay && !bForceRefresh) {
                remainTime = std::min(mFrameTick.time + delay - time, REFRESH_RATE);
                if (remainTime * 1000000 > 0) {
                    // TODO: sleep time -> second to microsecond
                    usleep(static_cast<int64_t>(remainTime * 1000000));
                }
                continue;
            }

            mFrameTick.time += delay;
            if (delay > 0 && time - mFrameTick.time > AV_SYNC_THRESHOLD_MAX) {
                mFrameTick.time = time;
            }

            mFrameTick.pts      = static_cast<double>(frameBuffer->pts) / 1000.0;
            mFrameTick.serial   = frameBuffer->serial;
            mFrameTick.duration = duration;

            mPlayerLink->video_clock->setClock(static_cast<double>(frameBuffer->pts) / 1000.0);
            mPlayerLink->video_clock->setClockSerial(frameBuffer->serial);

            if (mVideoDecodeHandler->getQueueSize() > 0) {
                if ((playerConfig->framedrop > 0 ||
                     (/*framedrop &&*/getMasterSyncType(mPlayerLink) != CLOCK_VIDEO)) && time > mFrameTick.time + duration) {
                    NEXT_LOGI(TAG,
                              "video late drop frame pts %f, delay %f, duration %f, "
                              "time %f, mFrameTick.time %f\n",
                              frameBuffer->pts / 1000.0, delay, duration, time, mFrameTick.time);
                    break;
                }
            }

            renderFrame(frameBuffer);
            bForceRefresh = false;
            break;
        }
    }
    if (mVideoRender && (mRenderType == VIDEO_RENDER_OPENGL || mRenderType == VIDEO_RENDER_METAL)) {
        mVideoRender->close();
        mVideoRender->detachAllFilter();
        mVideoRender->releaseContext();
    }
}

int VideoRenderHandler::flush() {
    std::unique_lock<std::mutex> lock(mLock);
    mCond.notify_one();
    return RESULT_OK;
}

int VideoRenderHandler::stop() {
    std::unique_lock<std::mutex> lock(mLock);
    bAbort = true;
    mCond.notify_all();
    return RESULT_OK;
}

void VideoRenderHandler::release() {
    NEXT_LOGD(TAG, "%s release begin\n", __func__ );
    if (mThread.joinable()) {
        mThread.join();
    }
    NEXT_LOGD(TAG, "%s release end\n", __func__ );
}
