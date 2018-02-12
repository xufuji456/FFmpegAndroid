package com.frank.live;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.hardware.Camera;
import android.media.AudioFormat;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.widget.ToggleButton;
import com.frank.live.Push.LivePusher;
import com.frank.live.listener.LiveStateChangeListener;
import com.frank.live.param.AudioParam;
import com.frank.live.param.VideoParam;

/**
 * h264与rtmp实时推流直播
 * Created by frank on 2018/1/28.
 */

public class LiveActivity extends AppCompatActivity implements View.OnClickListener, CompoundButton.OnCheckedChangeListener, LiveStateChangeListener {

    private final static String TAG = LiveActivity.class.getSimpleName();
    private final static int CODE_CAMERA_RECORD = 0x0001;
    private final static String[] permissions = new String[]{Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO};
    private final static String LIVE_URL = "rtmp://192.168.8.115/live/stream";
    private final static int MSG_ERROR = 100;
    private SurfaceHolder surfaceHolder;
    private LivePusher livePusher;
    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if(msg.what == MSG_ERROR){
                String errMsg = (String)msg.obj;
                if(!TextUtils.isEmpty(errMsg)){
                    Toast.makeText(LiveActivity.this, errMsg, Toast.LENGTH_SHORT).show();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_live);

        initView();
        requirePermission();
        initPusher();
    }

    private void initView(){
        findViewById(R.id.btn_swap).setOnClickListener(this);
        ((ToggleButton)findViewById(R.id.btn_live)).setOnCheckedChangeListener(this);
        SurfaceView surface_camera = (SurfaceView) findViewById(R.id.surface_camera);
        surfaceHolder = surface_camera.getHolder();
    }

    private void initPusher() {
        int width = 640;//分辨率设置很重要
        int height = 480;
        int videoBitRate = 400;//kb/s jason-->480kb
        int videoFrameRate = 25;//fps
        VideoParam videoParam = new VideoParam(width, height,
                Camera.CameraInfo.CAMERA_FACING_BACK, videoBitRate, videoFrameRate);
        int sampleRate = 44100;//采样率：Hz
        int channelConfig = AudioFormat.CHANNEL_IN_STEREO;//立体声道
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;//pcm16位
        int numChannels = 2;//声道数
        AudioParam audioParam = new AudioParam(sampleRate, channelConfig, audioFormat, numChannels);
        livePusher = new LivePusher(surfaceHolder, videoParam, audioParam);
    }

    @Override
    public void onClick(View v) {
        if(v.getId() == R.id.btn_swap){
            livePusher.switchCamera();
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if(isChecked){
            livePusher.startPush(LIVE_URL, this);
        }else {
            livePusher.stopPush();
        }
    }

    @Override
    public void onError(String msg) {
        Log.e(TAG, "errMsg=" + msg);
        mHandler.obtainMessage(MSG_ERROR, msg).sendToTarget();
    }

    @TargetApi(Build.VERSION_CODES.M)
    private void requirePermission(){
        requestPermissions(permissions, CODE_CAMERA_RECORD);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if(permissions.length > 0 && grantResults.length == permissions.length){
            for(int i=0; i<permissions.length; i++){
                Log.i(TAG, permissions[i] + ":grantResult=" + grantResults[i]);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        livePusher.release();
    }
}
