package com.frank.ffmpeg.format;

import android.media.audiofx.Visualizer;
import android.util.Log;

/**
 * Visualizer of Audio frequency
 * Created by frank on 2020/10/20.
 */
public class AudioVisualizer {

    private Visualizer visualizer;

    public void initVisualizer(int audioSession, boolean waveform, boolean fft, Visualizer.OnDataCaptureListener dataCaptureListener) {
        try {
            visualizer = new Visualizer(audioSession);
            int captureSize = Visualizer.getCaptureSizeRange()[1];
            int captureRate = Visualizer.getMaxCaptureRate() / 2;

            visualizer.setCaptureSize(captureSize);
            visualizer.setDataCaptureListener(dataCaptureListener, captureRate, waveform, fft);
            visualizer.setScalingMode(Visualizer.SCALING_MODE_NORMALIZED);
            visualizer.setEnabled(true);
        } catch (Exception e) {
            Log.e("AudioVisualizer", "initVisualizer error=" + e.toString());
            releaseVisualizer();
        }
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
