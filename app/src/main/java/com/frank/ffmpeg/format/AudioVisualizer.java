package com.frank.ffmpeg.format;

import android.media.audiofx.Visualizer;
import android.os.Build;

/**
 * Visualizer of Audio frequency
 * Created by frank on 2020/10/20.
 */
public class AudioVisualizer {

    private Visualizer visualizer;

    public void initVisualizer(int audioSession, boolean waveform, boolean fft, Visualizer.OnDataCaptureListener dataCaptureListener) {
        visualizer = new Visualizer(audioSession);
        int captureSize = Visualizer.getCaptureSizeRange()[1];
        int captureRate = Visualizer.getMaxCaptureRate() / 2;

        visualizer.setCaptureSize(captureSize);
        visualizer.setDataCaptureListener(dataCaptureListener, captureRate, waveform, fft);
        visualizer.setScalingMode(Visualizer.SCALING_MODE_NORMALIZED);
        visualizer.setEnabled(true);
    }

    public void releaseVisualizer() {
        if (visualizer != null) {
            int captureRate = Visualizer.getMaxCaptureRate() / 2;
            visualizer.setEnabled(false);
            visualizer.setDataCaptureListener(null, captureRate, false, false);
            visualizer.release();
            visualizer = null;
        }
    }

}
