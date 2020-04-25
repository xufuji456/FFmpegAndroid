
package com.frank.live;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Environment;

import androidx.annotation.NonNull;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Spinner;

import com.frank.live.view.SmartCameraView;
import com.seu.magicfilter.utils.MagicFilterType;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class PushActivity extends Activity implements Callback {
    private static String TAG = PushActivity.class.getSimpleName();

    private SmartCameraView mSmartCameraView;

//	MagicFilterType magicType = MagicFilterType.SUNRISE;

    private Button btnMute;

    private boolean isStart = false;

    private boolean is_mute = false;

    private int mDegree;

    private Spinner beautyTypeSelector;

    private ImageView img_photo;

    private boolean takePhoto;

    private final static int videoWidth = 640;
    private final static int videoHeight = 360;
    private final static String[] permissions = new String[]{Manifest.permission.CAMERA};
    private final static int CODE_CAMERA = 1001;

    private final static String[] beautySelector = new String[]{
            LiveApplication.getInstance().getString(R.string.effect_beauty),
            LiveApplication.getInstance().getString(R.string.effect_cool),
            LiveApplication.getInstance().getString(R.string.effect_sunrise),
            LiveApplication.getInstance().getString(R.string.effect_sketch),
            LiveApplication.getInstance().getString(R.string.effect_white),
            LiveApplication.getInstance().getString(R.string.effect_romantic),
            LiveApplication.getInstance().getString(R.string.effect_raw)
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        requestPermissions();

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_push);

        initView();
        initListener();
    }

    @TargetApi(23)
    private void requestPermissions() {
        requestPermissions(permissions, CODE_CAMERA);
    }

    private void initView() {
        //SurfaceView
        mSmartCameraView = findViewById(R.id.gl_surfaceview);
        //beauty type
        beautyTypeSelector = findViewById(R.id.beauty_type_selctor);
        //mute
        btnMute = findViewById(R.id.button_mute);
        //take photo
        img_photo = findViewById(R.id.img_photo);
    }

    private void initListener() {

        ArrayAdapter<String> adapterBeautyType = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, beautySelector);
        adapterBeautyType.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        beautyTypeSelector.setAdapter(adapterBeautyType);
        beautyTypeSelector.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

                switch (position) {
                    case 0:
                        switchCameraFilter(MagicFilterType.BEAUTY);
                        break;
                    case 1:
                        switchCameraFilter(MagicFilterType.COOL);
                        break;
                    case 2:
                        switchCameraFilter(MagicFilterType.SUNRISE);
                        break;
                    case 3:
                        switchCameraFilter(MagicFilterType.SKETCH);
                        break;
                    case 4:
                        switchCameraFilter(MagicFilterType.WHITECAT);
                        break;
                    case 5:
                        switchCameraFilter(MagicFilterType.ROMANCE);
                        break;
                    default:
                        switchCameraFilter(MagicFilterType.NONE);
                        break;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        btnMute.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                is_mute = !is_mute;

                if (is_mute) {
                    btnMute.setText(getString(R.string.voice));
                } else {
                    btnMute.setText(getString(R.string.mute));
                }
            }
        });

        //preview data callback(RGBA)
        mSmartCameraView.setPreviewCallback(new SmartCameraView.PreviewCallback() {
            @Override
            public void onGetRgbaFrame(byte[] data, int width, int height) {

                if (takePhoto) {
                    takePhoto = false;
                    Log.i(TAG, "takePhoto...");
                    doTakePhoto(data, width, height);
                }

            }
        });

        img_photo.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                takePhoto = true;
            }
        });

    }

    private void switchCameraFilter(MagicFilterType type) {
        mSmartCameraView.setFilter(type);
    }

    /**
     * take photo
     *
     * @param data   preview data
     * @param width  the width of photo
     * @param height the height of photo
     */
    private void doTakePhoto(byte[] data, int width, int height) {
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        ByteBuffer buffer = ByteBuffer.wrap(data);
        bitmap.copyPixelsFromBuffer(buffer);

        Log.i(TAG, "doTakePhoto...");
        FileOutputStream fileOutputStream = null;
        String PATH = Environment.getExternalStorageDirectory().getPath();
        String filePath = PATH + File.separator + "hello" + ".jpg";
        try {
            fileOutputStream = new FileOutputStream(filePath);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, fileOutputStream);
            fileOutputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG, "doTakePhoto error=" + e.toString());
        } finally {
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "surfaceCreated..");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "surfaceChanged..");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "Surface Destroyed");
    }

    public void onConfigurationChanged(Configuration newConfig) {
        try {
            super.onConfigurationChanged(newConfig);
            Log.i(TAG, "onConfigurationChanged, start:" + isStart);
            setCameraDisplayOrientation(this, getCameraId());
            mSmartCameraView.setPreviewOrientation(newConfig.orientation, mDegree);
        } catch (Exception ex) {
            Log.e(TAG, "error=" + ex.toString());
        }
    }

    private int getCameraId() {
        return mSmartCameraView.getCameraId();
    }

    public void setPreviewResolution(int width, int height) {
        mSmartCameraView.setPreviewResolution(width, height);
    }

    private void setCameraDisplayOrientation(Activity activity, int cameraId) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info);
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:
                degrees = 270;
                break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;
        } else {
            // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        Log.i(TAG, "curDegree: " + result);
        mDegree = result;
    }

    @Override
    protected void onDestroy() {
        if (isStart) {
            isStart = false;
            if (mSmartCameraView != null) {
                mSmartCameraView.stopCamera();
            }
            Log.i(TAG, "onDestroy StopPublish");
        }
        super.onDestroy();
        finish();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (permissions.length > 0 && grantResults.length > 0) {
            setPreviewResolution(videoWidth, videoHeight);
            if (!mSmartCameraView.startCamera()) {
                Log.e(TAG, "startCamera error...");
            }
        }
    }

}