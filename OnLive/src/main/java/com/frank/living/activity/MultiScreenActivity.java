package com.frank.living.activity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.PowerManager;
import android.os.Process;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import com.frank.living.R;
import com.frank.living.constant.Constants;
import com.frank.living.listener.OnDoubleClickListener;
import com.frank.living.listener.IjkPlayerListener;
import com.frank.living.widget.IjkVideoView;
import java.util.HashMap;
import java.util.TreeMap;
import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

/**
 * 多路投屏直播
 * Created by xufulong on 2019/01/04
 */
public class MultiScreenActivity extends AppCompatActivity{

    private static final String TAG = MultiScreenActivity.class.getSimpleName();

    private IjkVideoView mVideoView1;
    private IjkVideoView mVideoView2;
    private IjkVideoView mVideoView3;
    private IjkVideoView mVideoView4;

    private View divider1;
    private View divider2;

    private IMediaPlayer ijkPlayer;
    private CustomReceiver customReceiver;
    private String ipAddress;

    //四分屏模式还是全屏模式
    private boolean isMultiScreen;
    //保存客户端ip与通道数对应关系
    private HashMap<String, Integer> clientMap = new HashMap<>();
    //记录每个通道的投屏状态
    private TreeMap<Integer, Boolean> channelMap = new TreeMap<>();

    private String url = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_multi_screen);

        parseIntent();
        registerBroadcast();

        initView();
        initListener();
        setupView();
        wakeUp();

    }

    private void initView(){
        mVideoView1 = (IjkVideoView) findViewById(R.id.video_view1);
        mVideoView2 = (IjkVideoView) findViewById(R.id.video_view2);
        mVideoView3 = (IjkVideoView) findViewById(R.id.video_view3);
        mVideoView4 = (IjkVideoView) findViewById(R.id.video_view4);

        divider1 = findViewById(R.id.divider1);
        divider2 = findViewById(R.id.divider2);
    }

    private void initListener(){
        mVideoView1.setOnTouchListener(new OnDoubleClickListener(new OnDoubleClickListener.OnDoubleClick() {
            @Override
            public void onDouble() {
                changeScreenMode(1);
            }
        }));
        mVideoView2.setOnTouchListener(new OnDoubleClickListener(new OnDoubleClickListener.OnDoubleClick() {
            @Override
            public void onDouble() {
                changeScreenMode(2);
            }
        }));
        mVideoView3.setOnTouchListener(new OnDoubleClickListener(new OnDoubleClickListener.OnDoubleClick() {
            @Override
            public void onDouble() {
                changeScreenMode(3);
            }
        }));
        mVideoView4.setOnTouchListener(new OnDoubleClickListener(new OnDoubleClickListener.OnDoubleClick() {
            @Override
            public void onDouble() {
                changeScreenMode(4);
            }
        }));
    }

    private void setupView(){
        //第一路投屏默认全屏
        enterFullScreen(1);
        mVideoView1.setVideoPath(url);
        mVideoView1.setIjkPlayerListener(new IjkPlayerListener() {
            @Override
            public void onIjkPlayer(IjkMediaPlayer ijkMediaPlayer) {
                //设置播放器option
                setOptions(ijkMediaPlayer);
            }
        });
        mVideoView1.setOnPreparedListener(new IMediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(IMediaPlayer iMediaPlayer) {

                ijkPlayer = iMediaPlayer;
            }
        });
        mVideoView1.start();
    }

    /**
     * 外部传进来的数据:URL
     */
    private void parseIntent(){
        for (int i=1; i<=4; i++){
            channelMap.put(i, false);
        }
        clientMap.put(ipAddress, 1);
        channelMap.put(1, true);

        Intent intent = getIntent();
        String url = intent.getStringExtra("url");
        if(!TextUtils.isEmpty(url)){
            this.url = url;
        }
        String ip = intent.getStringExtra("ip");
        if (!TextUtils.isEmpty(ip)){
            ipAddress = ip;
        }
    }

    //唤醒屏幕
    private void wakeUp(){
        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (powerManager == null)
            return;
        PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK
                |PowerManager.ACQUIRE_CAUSES_WAKEUP, TAG);
        wakeLock.acquire(1000);
        wakeLock.release();
    }

    /**
     * 配置播放器参数项
     *
     */
    private void setOptions(IjkMediaPlayer ijkPlayer){
        if (ijkPlayer == null)
            return;
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "fast", 1);//不额外优化
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "probesize", 200);
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "flush_packets", 1);
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "packet-buffering", 0);//是否开启缓冲
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "start-on-prepared", 1);
        //0:代表关闭，1:代表开启
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 0);//开启硬解
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 0);//自动旋屏
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 0);//处理分辨率变化

        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "max-buffer-size", 0);//最大缓存数
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "min-frames", 2);//默认最小帧数2
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "max_cached_duration", 30);//最大缓存时长
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "infbuf", 1);//是否限制输入缓存数
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "fflags", "nobuffer");
        //设置播放前的最大探测时间,分析码流时长:默认1024*1000
        ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "analyzedmaxduration", 100);
        //ijkPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "rtsp_transport", "tcp");//tcp传输数据
    }

    /**
     * 注册广播
     */
    private void registerBroadcast(){
        customReceiver = new CustomReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Constants.ACTION_CLIENT_REMOVE);
        intentFilter.addAction(Constants.ACTION_CLIENT_ADD);
        registerReceiver(customReceiver, intentFilter);
    }

    /**
     * 注销广播
     */
    private void unregisterBroadcast(){
        if(customReceiver != null){
            unregisterReceiver(customReceiver);
            customReceiver = null;
        }
    }

    /**
     * 自定义广播接收器
     */
    private class CustomReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if(action == null)
                return;
            Log.e(TAG, "onReceive=" + action);
            switch (action){
                case Constants.ACTION_CLIENT_REMOVE: //移除投屏
                    int num = intent.getIntExtra("clientNum", 0);
                    if (num == 0){
                        Process.killProcess(Process.myPid());
                    }else if (num > 0){
                        String ipAddress = intent.getStringExtra("ipAddress");
                        int target = clientMap.get(ipAddress);
                        removeClient(target);
                        clientMap.remove(ipAddress);
                        channelMap.put(target, false);
                        //多屏变为单屏时，自动切换为全屏
                        if (num == 1){
                            int castingChannel = getCastingChannel();
                            enterFullScreen(castingChannel);
                        }
                    }
                    break;
                case Constants.ACTION_CLIENT_ADD://增加投屏
                    int clientNum = intent.getIntExtra("clientNum", 0);
                    String otherUrl = intent.getStringExtra("url");
                    String ipAddress = intent.getStringExtra("ip");
                    //选择空闲通道
                    int channel = selectIdleChannel(clientNum);
                    clientMap.put(ipAddress, channel);
                    channelMap.put(channel, true);
                    addClient(channel, otherUrl);
                    //单屏变为两路投屏时，自动切换为多屏模式
                    if (clientNum == 2){
                        exitFullScreen();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    /**
     * 选择空闲通道
     * @param clientNum clientNum
     * @return idleChannel
     */
    private int selectIdleChannel(int clientNum){
        for (int channel = 1; channel < clientNum; channel++){
            if (!channelMap.get(channel)){
                return channel;
            }
        }
        return clientNum;
    }

    /**
     * 获取当前投屏通道
     * @return idleChannel
     */
    private int getCastingChannel(){
        for (int channel = 1; channel <= 4; channel++){
            if (channelMap.get(channel)){
                return channel;
            }
        }
        return 0;
    }

    /**
     * 添加客户端
     * @param target target
     * @param clientUrl clientUrl
     */
    private void addClient(int target, String clientUrl){
        switch (target){
            case 1:
                mVideoView1.setVisibility(View.VISIBLE);
                mVideoView1.setVideoPath(clientUrl);
                mVideoView1.start();
                break;
            case 2:
                mVideoView2.setVisibility(View.VISIBLE);
                mVideoView2.setVideoPath(clientUrl);
                mVideoView2.start();
                break;
            case 3:
                mVideoView3.setVisibility(View.VISIBLE);
                mVideoView3.setVideoPath(clientUrl);
                mVideoView3.start();
                break;
            case 4:
                mVideoView4.setVisibility(View.VISIBLE);
                mVideoView4.setVideoPath(clientUrl);
                mVideoView4.start();
                break;
            default:
                break;
        }
    }

    /**
     * 移除客户端
     * @param target target
     */
    private void removeClient(int target){
        switch (target){
            case 1:
                mVideoView1.stopPlayback();
                mVideoView1.setVisibility(View.GONE);
                break;
            case 2:
                mVideoView2.stopPlayback();
                mVideoView2.setVisibility(View.GONE);
                break;
            case 3:
                mVideoView3.stopPlayback();
                mVideoView3.setVisibility(View.GONE);
                break;
            case 4:
                mVideoView4.stopPlayback();
                mVideoView4.setVisibility(View.GONE);
                break;
            default:
                break;
        }
    }

    /**
     * 隐藏分割线
     */
    private void hideDivider(){
        divider1.setVisibility(View.GONE);
        divider2.setVisibility(View.GONE);
    }

    /**
     * 显示分割线
     */
    private void showDivider(){
        divider1.setVisibility(View.VISIBLE);
        divider2.setVisibility(View.VISIBLE);
    }

    /**
     * 进入全屏模式
     * @param channel channel
     */
    private void enterFullScreen(int channel){
        hideDivider();
        switch (channel){
            case 1:
                mVideoView1.setRenderViewVisible();
                mVideoView2.setRenderViewGone();
                mVideoView3.setRenderViewGone();
                mVideoView4.setRenderViewGone();

                mVideoView1.setVisibility(View.VISIBLE);
                mVideoView2.setVisibility(View.GONE);
                mVideoView3.setVisibility(View.GONE);
                mVideoView4.setVisibility(View.GONE);
                break;
            case 2:
                mVideoView1.setRenderViewGone();
                mVideoView2.setRenderViewVisible();
                mVideoView3.setRenderViewGone();
                mVideoView4.setRenderViewGone();

                mVideoView1.setVisibility(View.GONE);
                mVideoView2.setVisibility(View.VISIBLE);
                mVideoView3.setVisibility(View.GONE);
                mVideoView4.setVisibility(View.GONE);
                break;
            case 3:
                mVideoView1.setRenderViewGone();
                mVideoView2.setRenderViewGone();
                mVideoView3.setRenderViewVisible();
                mVideoView4.setRenderViewGone();

                mVideoView1.setVisibility(View.GONE);
                mVideoView2.setVisibility(View.GONE);
                mVideoView3.setVisibility(View.VISIBLE);
                mVideoView4.setVisibility(View.GONE);
                break;
            case 4:
                mVideoView1.setRenderViewGone();
                mVideoView2.setRenderViewGone();
                mVideoView3.setRenderViewGone();
                mVideoView4.setRenderViewVisible();

                mVideoView1.setVisibility(View.GONE);
                mVideoView2.setVisibility(View.GONE);
                mVideoView3.setVisibility(View.GONE);
                mVideoView4.setVisibility(View.VISIBLE);
                break;
            default:
                break;
        }
    }

    /**
     * 退出全屏模式
     */
    private void exitFullScreen(){
        mVideoView1.setRenderViewVisible();
        mVideoView2.setRenderViewVisible();
        mVideoView3.setRenderViewVisible();
        mVideoView4.setRenderViewVisible();

        showDivider();
        mVideoView1.setVisibility(View.VISIBLE);
        mVideoView2.setVisibility(View.VISIBLE);
        mVideoView3.setVisibility(View.VISIBLE);
        mVideoView4.setVisibility(View.VISIBLE);
    }

/**
 * 切换分屏模式
 * @param channel channel
 */
private void changeScreenMode(int channel){
    isMultiScreen = !isMultiScreen;
    if (isMultiScreen){
        enterFullScreen(channel);
    }else {
        exitFullScreen();
    }
}

    @Override
    protected void onDestroy() {
        super.onDestroy();

        unregisterBroadcast();

        mVideoView1.stopPlayback();
        mVideoView1.release(true);
        mVideoView2.stopPlayback();
        mVideoView2.release(true);
        mVideoView3.stopPlayback();
        mVideoView3.release(true);
        mVideoView4.stopPlayback();
        mVideoView4.release(true);

        IjkMediaPlayer.native_profileEnd();
    }

}

