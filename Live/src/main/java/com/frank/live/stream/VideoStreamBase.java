package com.frank.live.stream;

import android.view.SurfaceHolder;

/**
 * Base of VideoStream
 * Created by frank on 2022/01/27.
 */

public abstract class VideoStreamBase {

    public abstract void startLive();

    public abstract void setPreviewDisplay(SurfaceHolder surfaceHolder);

    public abstract void switchCamera();

    public abstract void stopLive();

    public abstract void release();

    public abstract void onPreviewDegreeChanged(int degree);

}
