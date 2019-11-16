package com.frank.ffmpeg.view;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.SeekBar;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.hardware.HardwareDecode;

/**
 * 视频拖动实时预览的控件
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewBar extends RelativeLayout implements HardwareDecode.OnDataCallback {

    private final static String TAG = VideoPreviewBar.class.getSimpleName();

    private TextureView texturePreView;

    private SeekBar previewBar;

    private HardwareDecode hardwareDecode;

    private long duration;

    private PreviewBarCallback mPreviewBarCallback;

    public VideoPreviewBar(Context context) {
        super(context);
        initView(context);
    }

    public VideoPreviewBar(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        initView(context);
    }

    private void initView(Context context) {
        View view = LayoutInflater.from(context).inflate(R.layout.preview_video, this);
        previewBar = view.findViewById(R.id.preview_bar);
        texturePreView = view.findViewById(R.id.texture_preview);
        setListener();
    }

    private void setPreviewCallback(final String filePath, TextureView texturePreView) {
        texturePreView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                doPreview(filePath, new Surface(surface));
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        });
    }

    private void doPreview(String filePath, Surface surface) {
        if (surface == null || TextUtils.isEmpty(filePath)) {
            return;
        }
        release();
        hardwareDecode = new HardwareDecode(surface, filePath, this);
        hardwareDecode.decode();
    }

    private void setListener() {
        previewBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (!fromUser) {
                    return;
                }
                previewBar.setProgress(progress);
                if (hardwareDecode != null && progress < duration) {
                    hardwareDecode.seekTo(progress);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                if (mPreviewBarCallback != null) {
                    mPreviewBarCallback.onStopTracking(seekBar.getProgress());
                }
            }
        });
    }

    @Override
    public void onData(long duration) {
        Log.i(TAG, "duration=" + duration);
        this.duration = duration;
        previewBar.setMax((int) duration);
    }

    public void init(String videoPath, PreviewBarCallback previewBarCallback) {
        this.mPreviewBarCallback = previewBarCallback;
        setPreviewCallback(videoPath, texturePreView);
    }

    public void release() {
        if (hardwareDecode != null) {
            hardwareDecode.release();
            hardwareDecode = null;
        }
    }

    public interface PreviewBarCallback {
        void onStopTracking(long progress);
    }

}
