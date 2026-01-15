package com.frank.next.renderview;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.TextureView;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.frank.next.player.IPlayer;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * render of TextureView
 */
public class TextureRenderView extends TextureView implements IRenderView {

    private static final String TAG = TextureRenderView.class.getSimpleName();

    private MeasureHelper mMeasureHelper;

    public TextureRenderView(Context context) {
        super(context);
        initView();
    }

    public TextureRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    public TextureRenderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView();
    }

    private void initView() {
        mMeasureHelper = new MeasureHelper(this);
        mSurfaceCallback = new SurfaceCallback(this);
        setSurfaceTextureListener(mSurfaceCallback);
    }

    @Override
    public View getView() {
        return this;
    }

    @Override
    public boolean waitForResize() {
        return false;
    }

    @Override
    protected void onDetachedFromWindow() {
        mSurfaceCallback.willDetachFromWindow();
        super.onDetachedFromWindow();
        mSurfaceCallback.didDetachFromWindow();
    }

    @Override
    public void setVideoSize(int videoWidth, int videoHeight) {
        if (videoWidth > 0 && videoHeight > 0) {
            mMeasureHelper.setVideoSize(videoWidth, videoHeight);
            requestLayout();
        }
    }

    @Override
    public void setVideoAspectRatio(int videoSarNum, int videoSarDen) {
        if (videoSarNum > 0 && videoSarDen > 0) {
            mMeasureHelper.setVideoSampleAspectRatio(videoSarNum, videoSarDen);
            requestLayout();
        }
    }

    @Override
    public void setVideoRotation(int degree) {
        mMeasureHelper.setVideoRotation(degree);
        setRotation(degree);
    }

    @Override
    public void setAspectRatio(int aspectRatio) {
        mMeasureHelper.setAspectRatio(aspectRatio);
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mMeasureHelper.doMeasure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(mMeasureHelper.getMeasuredWidth(), mMeasureHelper.getMeasuredHeight());
    }

    public IRenderView.ISurfaceHolder getSurfaceHolder() {
        return new VideoSurfaceHolder(this, mSurfaceCallback.mSurfaceTexture);
    }

    private static final class VideoSurfaceHolder implements IRenderView.ISurfaceHolder {
        private final TextureRenderView mTextureView;
        private final SurfaceTexture mSurfaceTexture;

        public VideoSurfaceHolder(@NonNull TextureRenderView textureView, @Nullable SurfaceTexture surfaceTexture) {
            mTextureView    = textureView;
            mSurfaceTexture = surfaceTexture;
        }

        public void bindPlayer(IPlayer mp) {
            if (mp == null)
                return;
            mp.setSurface(openSurface());
        }

        @NonNull
        @Override
        public IRenderView getRenderView() {
            return mTextureView;
        }

        @Nullable
        @Override
        public SurfaceHolder getSurfaceHolder() {
            return null;
        }

        @Nullable
        @Override
        public SurfaceTexture getSurfaceTexture() {
            return mSurfaceTexture;
        }

        @Nullable
        @Override
        public Surface openSurface() {
            if (mSurfaceTexture == null)
                return null;
            return new Surface(mSurfaceTexture);
        }
    }

    @Override
    public void addRenderCallback(IRenderCallback callback) {
        mSurfaceCallback.addRenderCallback(callback);
    }

    @Override
    public void removeRenderCallback(IRenderCallback callback) {
        mSurfaceCallback.removeRenderCallback(callback);
    }

    private SurfaceCallback mSurfaceCallback;

    private static final class SurfaceCallback implements SurfaceTextureListener {
        private int mWidth;
        private int mHeight;
        private boolean mFormatChanged;
        private SurfaceTexture mSurfaceTexture;
        private boolean mOwnSurfaceTexture    = true;
        private boolean mWillDetachFromWindow = false;
        private boolean mDidDetachFromWindow  = false;

        private final WeakReference<TextureRenderView> mWeakRenderView;
        private final Map<IRenderCallback, Object> mRenderCallbackMap = new ConcurrentHashMap<IRenderCallback, Object>();

        public SurfaceCallback(@NonNull TextureRenderView renderView) {
            mWeakRenderView = new WeakReference<TextureRenderView>(renderView);
        }

        public void setOwnSurfaceTexture(boolean ownSurfaceTexture) {
            mOwnSurfaceTexture = ownSurfaceTexture;
        }

        public void addRenderCallback(@NonNull IRenderCallback callback) {
            mRenderCallbackMap.put(callback, callback);
            ISurfaceHolder surfaceHolder = null;
            if (mSurfaceTexture != null) {
                surfaceHolder = new VideoSurfaceHolder(mWeakRenderView.get(), mSurfaceTexture);
                callback.onSurfaceCreated(surfaceHolder, mWidth, mHeight);
            }

            if (mFormatChanged) {
                if (surfaceHolder == null)
                    surfaceHolder = new VideoSurfaceHolder(mWeakRenderView.get(), mSurfaceTexture);
                callback.onSurfaceChanged(surfaceHolder, 0, mWidth, mHeight);
            }
        }

        public void removeRenderCallback(@NonNull IRenderCallback callback) {
            mRenderCallbackMap.remove(callback);
        }

        @Override
        public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
            mSurfaceTexture = surface;

            ISurfaceHolder surfaceHolder = new VideoSurfaceHolder(mWeakRenderView.get(), surface);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceCreated(surfaceHolder, 0, 0);
            }
        }

        @Override
        public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width, int height) {
            mWidth          = width;
            mHeight         = height;
            mFormatChanged  = true;
            mSurfaceTexture = surface;

            ISurfaceHolder surfaceHolder = new VideoSurfaceHolder(mWeakRenderView.get(), surface);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceChanged(surfaceHolder, 0, width, height);
            }
        }

        @Override
        public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {
        }

        @Override
        public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
            ISurfaceHolder surfaceHolder = new VideoSurfaceHolder(mWeakRenderView.get(), surface);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceDestroyed(surfaceHolder);
            }
            return mOwnSurfaceTexture;
        }

        public void willDetachFromWindow() {
            mWillDetachFromWindow = true;
        }

        public void didDetachFromWindow() {
            mDidDetachFromWindow = true;
        }

        public void releaseSurfaceTexture(SurfaceTexture surfaceTexture) {
            if (surfaceTexture == null) {
                return;
            }
            if (mDidDetachFromWindow) {
                if (surfaceTexture != mSurfaceTexture) {
                    surfaceTexture.release();
                } else if (!mOwnSurfaceTexture) {
                    surfaceTexture.release();
                }
            } else if (mWillDetachFromWindow) {
                if (surfaceTexture != mSurfaceTexture) {
                    surfaceTexture.release();
                } else if (!mOwnSurfaceTexture) {
                    setOwnSurfaceTexture(true);
                }
            } else {
                if (surfaceTexture != mSurfaceTexture) {
                    surfaceTexture.release();
                } else if (!mOwnSurfaceTexture) {
                    setOwnSurfaceTexture(true);
                }
            }
        }
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(TextureRenderView.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(TextureRenderView.class.getName());
    }
}
