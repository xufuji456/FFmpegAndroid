package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.hardware.Camera;
import android.media.AudioFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.widget.ToggleButton;
import com.frank.ffmpeg.R;
import com.frank.live.Push.LivePusher;
import com.frank.live.listener.LiveStateChangeListener;
import com.frank.live.param.AudioParam;
import com.frank.live.param.VideoParam;

/**
 * h264与rtmp实时推流直播
 * Created by frank on 2018/1/28.
 */

public class LiveActivity extends BaseActivity implements CompoundButton.OnCheckedChangeListener, LiveStateChangeListener {

    private final static String TAG = LiveActivity.class.getSimpleName();
    private final static String LIVE_URL = "rtmp://192.168.1.3/live/stream";
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
    int getLayoutId() {
        return R.layout.activity_live;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initView();
        initPusher();
    }

    private void initView(){
        initViewsWithClick(R.id.btn_swap);
        ((ToggleButton)findViewById(R.id.btn_live)).setOnCheckedChangeListener(this);
        ((ToggleButton)findViewById(R.id.btn_mute)).setOnCheckedChangeListener(this);
        SurfaceView surface_camera = getView(R.id.surface_camera);
        surfaceHolder = surface_camera.getHolder();
    }

    private void initPusher() {
        int width = 640;//分辨率设置
        int height = 480;
        int videoBitRate = 400;//kb/s
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
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        switch (buttonView.getId()){
            case R.id.btn_live://开始/停止直播
                if(isChecked){
                    livePusher.startPush(LIVE_URL, this);
                }else {
                    livePusher.stopPush();
                }
                break;
            case R.id.btn_mute://设置静音
                Log.i(TAG, "isChecked=" + isChecked);
                livePusher.setMute(isChecked);
                break;
            default:
                break;
        }
    }

    @Override
    public void onError(String msg) {
        Log.e(TAG, "errMsg=" + msg);
        mHandler.obtainMessage(MSG_ERROR, msg).sendToTarget();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (livePusher != null) {
            livePusher.release();
        }
    }

    @Override
    void onViewClick(View view) {
        if(view.getId() == R.id.btn_swap){//切换摄像头
            livePusher.switchCamera();
        }
    }

    @Override
    void onSelectedFile(String filePath) {

    }
}
