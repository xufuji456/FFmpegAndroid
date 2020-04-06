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
import android.widget.TextView;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.hardware.HardwareDecode;
import com.frank.ffmpeg.util.ScreenUtil;
import com.frank.ffmpeg.util.TimeUtil;

/**
 * the custom view of preview SeekBar
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewBar extends RelativeLayout implements HardwareDecode.OnDataCallback {

    private final static String TAG = VideoPreviewBar.class.getSimpleName();

    private TextureView texturePreView;

    private SeekBar previewBar;

    private TextView txtVideoProgress;

    private TextView txtVideoDuration;

    private HardwareDecode hardwareDecode;

    private PreviewBarCallback mPreviewBarCallback;

    private int duration;

    private int screenWidth;

    private int moveEndPos = 0;

    private int previewHalfWidth;

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
        txtVideoProgress = view.findViewById(R.id.txt_video_progress);
        txtVideoDuration = view.findViewById(R.id.txt_video_duration);
        setListener();
        screenWidth = ScreenUtil.getScreenWidth(context);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (moveEndPos == 0) {
            int previewWidth = texturePreView.getWidth();
            previewHalfWidth = previewWidth / 2;
            int marginEnd = 0;
            MarginLayoutParams layoutParams = (MarginLayoutParams) texturePreView.getLayoutParams();
            if (layoutParams != null) {
                marginEnd = layoutParams.getMarginEnd();
            }
            moveEndPos = screenWidth - previewWidth - marginEnd;
            Log.i(TAG, "previewWidth=" + previewWidth);
        }
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
                    // us to ms
                    hardwareDecode.seekTo(progress * 1000);
                }
                int percent = progress * screenWidth / duration;
                if (percent > previewHalfWidth && percent < moveEndPos && texturePreView != null) {
                    texturePreView.setTranslationX(percent - previewHalfWidth);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                if (texturePreView != null) {
                    texturePreView.setVisibility(VISIBLE);
                }
                if (hardwareDecode != null) {
                    hardwareDecode.setPreviewing(true);
                }
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                if (texturePreView != null) {
                    texturePreView.setVisibility(GONE);
                }
                if (mPreviewBarCallback != null) {
                    mPreviewBarCallback.onStopTracking(seekBar.getProgress());
                }
                if (hardwareDecode != null) {
                    hardwareDecode.setPreviewing(false);
                }
            }
        });
    }

    @Override
    public void onData(long duration) {
        //us to ms
        final int durationMs = (int) (duration / 1000);
        Log.i(TAG, "duration=" + duration);
        this.duration = durationMs;
        post(new Runnable() {
            @Override
            public void run() {
                previewBar.setMax(durationMs);
                txtVideoDuration.setText(TimeUtil.getVideoTime(durationMs));
                texturePreView.setVisibility(GONE);
            }
        });
    }

    private void checkArgument(String videoPath) {
        if (texturePreView == null) {
            throw new IllegalStateException("Must init TextureView first...");
        }
        if (videoPath == null || videoPath.isEmpty()) {
            throw new IllegalStateException("videoPath is empty...");
        }
    }

    public void init(String videoPath, PreviewBarCallback previewBarCallback) {
        checkArgument(videoPath);
        this.mPreviewBarCallback = previewBarCallback;
        doPreview(videoPath, new Surface(texturePreView.getSurfaceTexture()));
    }

    public void initDefault(String videoPath, PreviewBarCallback previewBarCallback) {
        checkArgument(videoPath);
        this.mPreviewBarCallback = previewBarCallback;
        setPreviewCallback(videoPath, texturePreView);
    }

    public void updateProgress(int progress) {
        if (progress >= 0 && progress <= duration) {
            txtVideoProgress.setText(TimeUtil.getVideoTime(progress));
            previewBar.setProgress(progress);
        }
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
