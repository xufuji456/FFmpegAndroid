package com.frank.ffmpeg.hardware;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;

/**
 * Extract by MediaExtractor, decode by MediaCodec, and render to Surface
 * Created by frank on 2019/11/16.
 */

public class HardwareDecode {

    private final static String TAG = HardwareDecode.class.getSimpleName();

    private final static long DEQUEUE_TIME = 10 * 1000;
    private final static int SLEEP_TIME = 10;

    private final static int RATIO_1080 = 1080;
    private final static int RATIO_480 = 480;
    private final static int RATIO_240 = 240;

    private Surface mSurface;

    private String mFilePath;

    private VideoDecodeThread videoDecodeThread;

    private OnDataCallback mCallback;

    public interface OnDataCallback {
        void onData(long duration);
    }

    public HardwareDecode(Surface surface, String filePath, OnDataCallback onDataCallback) {
        this.mSurface = surface;
        this.mFilePath = filePath;
        this.mCallback = onDataCallback;
    }

    public void decode() {
        videoDecodeThread = new VideoDecodeThread();
        videoDecodeThread.start();
    }

    public void seekTo(long seekPosition) {
        if (videoDecodeThread != null && !videoDecodeThread.isInterrupted()) {
            videoDecodeThread.seekTo(seekPosition);
        }
    }

    public void setPreviewing(boolean previewing) {
        if (videoDecodeThread != null) {
            videoDecodeThread.setPreviewing(previewing);
        }
    }

    public void release() {
        if (videoDecodeThread != null && !videoDecodeThread.isInterrupted()) {
            videoDecodeThread.interrupt();
            videoDecodeThread.release();
            videoDecodeThread = null;
        }
    }

    private class VideoDecodeThread extends Thread {

        private MediaExtractor mediaExtractor;

        private MediaCodec mediaCodec;

        private boolean isPreviewing;

        void setPreviewing(boolean previewing) {
            this.isPreviewing = previewing;
        }

        void seekTo(long seekPosition) {
            try {
                if (mediaExtractor != null) {
                    mediaExtractor.seekTo(seekPosition, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                }
            } catch (IllegalStateException e) {
                Log.e(TAG, "seekTo error=" + e.toString());
            }
        }

        void release() {
            try {
                if (mediaCodec != null) {
                    mediaCodec.stop();
                    mediaCodec.release();
                }
                if (mediaExtractor != null) {
                    mediaExtractor.release();
                }
            } catch (Exception e) {
                Log.e(TAG, "release error=" + e.toString());
            }
        }

        /**
         * setting the preview resolution according to video aspect ratio
         *
         * @param mediaFormat mediaFormat
         */
        private void setPreviewRatio(MediaFormat mediaFormat) {
            if (mediaFormat == null) {
                return;
            }
            int videoWidth = mediaFormat.getInteger(MediaFormat.KEY_WIDTH);
            int videoHeight = mediaFormat.getInteger(MediaFormat.KEY_HEIGHT);
            int previewRatio;
            if (videoWidth >= RATIO_1080) {
                previewRatio = 10;
            } else if (videoWidth >= RATIO_480) {
                previewRatio = 6;
            } else if (videoWidth >= RATIO_240) {
                previewRatio = 4;
            } else {
                previewRatio = 1;
            }
            int previewWidth = videoWidth / previewRatio;
            int previewHeight = videoHeight / previewRatio;
            Log.e(TAG, "videoWidth=" + videoWidth + "--videoHeight=" + videoHeight
                    + "--previewWidth=" + previewWidth + "--previewHeight=" + previewHeight);
            mediaFormat.setInteger(MediaFormat.KEY_WIDTH, previewWidth);
            mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, previewHeight);
        }

        @Override
        public void run() {
            super.run();

            mediaExtractor = new MediaExtractor();
            MediaFormat mediaFormat = null;
            String mimeType = "";
            try {
                mediaExtractor.setDataSource(mFilePath);
                for (int i = 0; i < mediaExtractor.getTrackCount(); i++) {
                    mediaFormat = mediaExtractor.getTrackFormat(i);
                    mimeType = mediaFormat.getString(MediaFormat.KEY_MIME);
                    if (mimeType != null && mimeType.startsWith("video/")) {
                        mediaExtractor.selectTrack(i);
                        break;
                    }
                }
                if (mediaFormat == null || mimeType == null) {
                    return;
                }
                int width = mediaFormat.getInteger(MediaFormat.KEY_WIDTH);
                int height = mediaFormat.getInteger(MediaFormat.KEY_HEIGHT);
                long duration = mediaFormat.getLong(MediaFormat.KEY_DURATION);
                if (mCallback != null) {
                    mCallback.onData(duration);
                }
                Log.i(TAG, "width=" + width + "--height=" + height + "--duration==" + duration);

                //setting preview resolution
                setPreviewRatio(mediaFormat);
                Log.i(TAG, "mediaFormat=" + mediaFormat.toString());

                //config MediaCodec, and start
                mediaCodec = MediaCodec.createDecoderByType(mimeType);
                mediaCodec.configure(mediaFormat, mSurface, null, 0);
                mediaCodec.start();
                MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
                ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();

                while (!isInterrupted()) {
                    if (!isPreviewing) {
                        SystemClock.sleep(SLEEP_TIME);
                        continue;
                    }
                    //dequeue from input buffer
                    int inputIndex = mediaCodec.dequeueInputBuffer(DEQUEUE_TIME);
                    if (inputIndex >= 0) {
                        ByteBuffer inputBuffer = inputBuffers[inputIndex];
                        int sampleSize = mediaExtractor.readSampleData(inputBuffer, 0);
                        //enqueue to input buffer
                        if (sampleSize < 0) {
                            mediaCodec.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        } else {
                            mediaCodec.queueInputBuffer(inputIndex, 0, sampleSize, mediaExtractor.getSampleTime(), 0);
                            mediaExtractor.advance();
                        }
                    }

                    //dequeue from output buffer
                    int outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, DEQUEUE_TIME);
                    switch (outputIndex) {
                        case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                            Log.i(TAG, "output format changed...");
                            break;
                        case MediaCodec.INFO_TRY_AGAIN_LATER:
                            Log.i(TAG, "try again later...");
                            break;
                        case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                            Log.i(TAG, "output buffer changed...");
                            break;
                        default:
                            //render to surface
                            mediaCodec.releaseOutputBuffer(outputIndex, true);
                            break;
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "setDataSource error=" + e.toString());
            }
        }
    }

}
