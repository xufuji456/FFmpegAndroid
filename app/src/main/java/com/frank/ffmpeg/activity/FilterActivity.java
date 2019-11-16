package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ToggleButton;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.adapter.HorizontalAdapter;
import com.frank.ffmpeg.listener.OnItemClickListener;
import com.frank.ffmpeg.util.FileUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * 使用ffmpeg进行滤镜
 * Created by frank on 2018/6/5.
 */

public class FilterActivity extends BaseActivity implements SurfaceHolder.Callback{

    //本地视频路径
    private String videoPath = "";

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
    private Button btnSelect;

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
                btnSelect.setVisibility(View.GONE);
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
    int getLayoutId() {
        return R.layout.activity_filter;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initView();
        registerLister();

        hideRunnable = new HideRunnable();
        mHandler.postDelayed(hideRunnable, DELAY_TIME);
    }

    private void initView(){
        surfaceView = getView(R.id.surface_filter);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        videoPlayer = new VideoPlayer();
        btnSound = getView(R.id.btn_sound);

        recyclerView = getView(R.id.recycler_view);
        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
        recyclerView.setLayoutManager(linearLayoutManager);
        List<String> itemList = new ArrayList<>();
        itemList.addAll(Arrays.asList(txtArray));
        horizontalAdapter = new HorizontalAdapter(itemList);
        recyclerView.setAdapter(horizontalAdapter);

        btnSelect = getView(R.id.btn_select_file);
        initViewsWithClick(R.id.btn_select_file);
    }

    //注册监听器
    private void registerLister(){
        horizontalAdapter.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(int position) {
                if(!surfaceCreated)
                    return;
                if (!FileUtil.checkFileExist(videoPath)){
                    showSelectFile();
                    return;
                }
                doFilterPlay(position);
            }
        });

        surfaceView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnSelect.setVisibility(View.VISIBLE);
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

    private void doFilterPlay(final int position) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                //切换播放
                if(isPlaying){
                    videoPlayer.again();
                }
                isPlaying = true;
                videoPlayer.filter(videoPath, surfaceHolder.getSurface(), filters[position]);
            }
        }).start();
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

    @Override
    void onViewClick(View view) {
        if (view.getId() == R.id.btn_select_file) {
            selectFile();
        }
    }

    @Override
    void onSelectedFile(String filePath) {
        videoPath = filePath;
        //选择滤镜模式
        doFilterPlay(6);
        //默认关闭声音
        btnSound.setChecked(true);
    }

}
