package com.frank.live.camera2;


import android.util.Size;

public interface Camera2Listener {

    void onCameraOpened(Size previewSize, int displayOrientation);

    void onPreviewFrame(byte[] y, byte[] u, byte[] v);

    void onCameraClosed();

    void onCameraError(Exception e);

}
