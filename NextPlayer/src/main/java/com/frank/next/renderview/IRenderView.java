package com.frank.next.renderview;

import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;

import com.frank.next.player.IPlayer;

public interface IRenderView {
    int RENDER_MODE_ASPECT_FIT  = 0; // without clip
    int RENDER_MODE_ASPECT_FILL = 1; // may clip
    int RENDER_MODE_16_9        = 2;
    int RENDER_MODE_4_3         = 3;
    int RENDER_MODE_WRAP        = 4;
    int RENDER_MODE_MATCH       = 5;

    View getView();

    boolean waitForResize();

    void setVideoRotation(int degree);

    void setAspectRatio(int aspectRatio);

    void addRenderCallback(IRenderCallback callback);

    void removeRenderCallback(IRenderCallback callback);

    void setVideoSize(int videoWidth, int videoHeight);

    void setVideoSampleAspectRatio(int videoSarNum, int videoSarDen);

    interface ISurfaceHolder {

        Surface openSurface();

        void bindPlayer(IPlayer mp);

        IRenderView getRenderView();

        SurfaceHolder getSurfaceHolder();

        SurfaceTexture getSurfaceTexture();
    }

    interface IRenderCallback {

        void onSurfaceCreated(ISurfaceHolder holder, int width, int height);

        void onSurfaceChanged(ISurfaceHolder holder, int format, int width, int height);

        void onSurfaceDestroyed(ISurfaceHolder holder);
    }
}
