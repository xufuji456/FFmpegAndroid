package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ToggleButton;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.adapter.HorizontalAdapter;
import com.frank.ffmpeg.listener.OnItemClickListener;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FilterActivity extends AppCompatActivity implements SurfaceHolder.Callback{

    //SD卡根目录
    private final static String PATH = Environment.getExternalStorageDirectory().getPath();
    //本地视频路径
    private final static String VIDEO_PATH = PATH + File.separator + "Beyond.mp4";

    private VideoPlayer videoPlayer;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    //surface是否已经创建
    private boolean surfaceCreated;
    //是否正在播放
    private boolean isPlaying;
    //滤镜数组
    private String[] filters = new String[]{
            "lutyuv='u=128:v=128'",
            "hue='h=60:s=-3'",
            "lutrgb='r=0:g=0'",
            "edgedetect=low=0.1:high=0.4",
            "boxblur=2:1",
            "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5",
            "colorbalance=bs=0.3",
            "drawbox=x=100:y=100:w=100:h=100:color=red@0.5'",
            "vflip",
            "unsharp"
    };
    private String[] txtArray = new String[]{
            "素描",
            "鲜明",//hue
            "暖蓝",
            "边缘",
            "模糊",
            "九宫格",
            "均衡",
            "矩形",
            "翻转",//vflip上下翻转,hflip是左右翻转
            "锐化"
    };
    private HorizontalAdapter horizontalAdapter;
    private RecyclerView recyclerView;
    //是否播放音频
    private boolean playAudio = true;
    private ToggleButton btnSound;

    private final static int MSG_HIDE = 222;
    private final static int DELAY_TIME = 5000;
    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == MSG_HIDE){//无操作5s后隐藏滤镜操作栏
                recyclerView.setVisibility(View.GONE);
                btnSound.setVisibility(View.GONE);
            }
        }
    };

    private class HideRunnable implements Runnable{
        @Override
        public void run() {
            mHandler.obtainMessage(MSG_HIDE).sendToTarget();
        }
    }
    private HideRunnable hideRunnable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_filter);

        initView();
        registerLister();

        hideRunnable = new HideRunnable();
        mHandler.postDelayed(hideRunnable, DELAY_TIME);
    }

    private void initView(){
        surfaceView = (SurfaceView) findViewById(R.id.surface_filter);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        videoPlayer = new VideoPlayer();
        btnSound = (ToggleButton) findViewById(R.id.btn_sound);

        recyclerView = (RecyclerView) findViewById(R.id.recycler_view);
        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
        recyclerView.setLayoutManager(linearLayoutManager);
        List<String> itemList = new ArrayList<>();
        itemList.addAll(Arrays.asList(txtArray));
        horizontalAdapter = new HorizontalAdapter(itemList);
        recyclerView.setAdapter(horizontalAdapter);
    }

    //注册监听器
    private void registerLister(){
        horizontalAdapter.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(int position) {
                if(!surfaceCreated)
                    return;
                if (!FileUtil.checkFileExist(VIDEO_PATH)){
                    return;
                }

                final int mPosition = position;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        //切换播放
                        if(isPlaying){
                            videoPlayer.again();
                        }
                        isPlaying = true;
                        videoPlayer.filter(VIDEO_PATH, surfaceHolder.getSurface(), filters[mPosition]);
                    }
                }).start();
            }
        });

        surfaceView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnSound.setVisibility(View.VISIBLE);
                recyclerView.setVisibility(View.VISIBLE);//按下SurfaceView，弹出滤镜操作栏
                mHandler.postDelayed(hideRunnable, DELAY_TIME);//5s后发消息通知隐藏滤镜操作栏
            }
        });

        btnSound.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setPlayAudio();
            }
        });
    }

    //设置是否静音
    private void setPlayAudio(){
        playAudio = !playAudio;
        videoPlayer.playAudio(playAudio);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        surfaceCreated = true;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        surfaceCreated = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        isPlaying = false;
        videoPlayer.release();
        videoPlayer = null;
        horizontalAdapter = null;
    }

}
