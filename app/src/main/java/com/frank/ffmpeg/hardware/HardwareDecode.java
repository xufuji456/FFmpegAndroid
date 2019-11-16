package com.frank.ffmpeg.hardware;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * 使用MediaExtractor抽帧，MediaCodec解码，然后渲染到Surface
 * Created by frank on 2019/11/16.
 */

public class HardwareDecode {

    private final static String TAG = HardwareDecode.class.getSimpleName();

    private final static long DEQUEUE_TIME = 10 * 1000;

    private final static long SLEEP_TIME = 10;

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

    private class VideoDecodeThread extends Thread {

        private MediaExtractor mediaExtractor;

        void seekTo(long seekPosition) {
            try {
                if (mediaExtractor != null) {
                    mediaExtractor.seekTo(seekPosition, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                }
            } catch (IllegalStateException e) {
                Log.e(TAG, "seekTo error=" + e.toString());
            }
        }

        @Override
        public void run() {
            super.run();

            mediaExtractor = new MediaExtractor();
            MediaFormat mediaFormat = null;
            String mimeType = "";
            try {
                mediaExtractor.setDataSource(mFilePath);
                for (int i=0; i<mediaExtractor.getTrackCount(); i++) {
                    mediaFormat = mediaExtractor.getTrackFormat(i);
                    mimeType = mediaFormat.getString(MediaFormat.KEY_MIME);
                    if (mimeType != null &&  mimeType.startsWith("video/")) {
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
                Log.e(TAG, "width=" + width + "--height=" + height + "--duration==" + duration);

                //TODO:重新设置分辨率
                mediaFormat.setInteger(MediaFormat.KEY_WIDTH, width/10);
                mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height/10);
                Log.e(TAG, "mediaFormat=" + mediaFormat.toString());

                //配置MediaCodec，并且start
                MediaCodec mediaCodec = MediaCodec.createDecoderByType(mimeType);
                mediaCodec.configure(mediaFormat, mSurface, null, 0);
                mediaCodec.start();
                long startTime = System.currentTimeMillis();
                MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
                ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();

                boolean isEof = false;

                while (!isInterrupted() && !isEof) {
                    //从缓冲区取出一个缓冲块，如果当前无可用缓冲块，返回inputIndex<0
                    int inputIndex = mediaCodec.dequeueInputBuffer(DEQUEUE_TIME);
                    if (inputIndex >= 0) {
                        ByteBuffer inputBuffer = inputBuffers[inputIndex];
                        int sampleSize = mediaExtractor.readSampleData(inputBuffer, 0);
                        //入队列
                        if (sampleSize < 0) {
                            isEof = true;
                            mediaCodec.queueInputBuffer(inputIndex,0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        } else {
                            mediaCodec.queueInputBuffer(inputIndex, 0, sampleSize, mediaExtractor.getSampleTime(), 0);
                            mediaExtractor.advance();
                        }
                    }

                    //出队列
                    int outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, DEQUEUE_TIME);
                    switch (outputIndex) {
                        case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                            Log.e(TAG, "output format changed...");
                            break;
                        case MediaCodec.INFO_TRY_AGAIN_LATER:
                            Log.i(TAG, "try again later...");
                            break;
                        case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                            Log.e(TAG, "output buffer changed...");
                            break;
                        default:
//                            long time = (System.currentTimeMillis()-startTime);
//                            if (time > 50) {
//                                Log.e(TAG, "pts=" + bufferInfo.presentationTimeUs + "--time=" + time);
//                            }
//                            startTime = System.currentTimeMillis();
//                            while (bufferInfo.presentationTimeUs > (System.currentTimeMillis() - startTime)*1000) {
//                                try {
//                                    Thread.sleep(SLEEP_TIME);
//                                } catch (InterruptedException e) {
//                                    e.printStackTrace();
//                                    Log.e(TAG, "error=" + e.toString());
//                                }
//                            }
                            //渲染到surface
                            mediaCodec.releaseOutputBuffer(outputIndex, true);
                            break;
                    }

                    if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        isEof = true;
                        Log.e(TAG, "is end of stream...");
                    }
                }

                mediaCodec.stop();
                mediaCodec.release();
                mediaExtractor.release();
            } catch (IOException e) {
                Log.e(TAG, "setDataSource error=" + e.toString());
            }

        }
    }

}
