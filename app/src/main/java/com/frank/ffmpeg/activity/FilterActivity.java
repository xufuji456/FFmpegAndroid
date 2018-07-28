package com.frank.ffmpeg.activity;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.adapter.HorizontalAdapter;
import com.frank.ffmpeg.listener.OnItemClickListener;
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
    //是否播放音频
    private boolean playAudio = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_filter);

        initView();
        registerLister();
    }

    private void initView(){
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface_filter);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        videoPlayer = new VideoPlayer();

        RecyclerView recyclerView = (RecyclerView) findViewById(R.id.recycler_view);
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
                final int mPosition = position;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        //切换播放
                        if(isPlaying){
                            videoPlayer.again();
                        }
                        isPlaying = true;
                        videoPlayer.filter(VIDEO_PATH, surfaceHolder.getSurface(), filters[mPosition], playAudio);
                    }
                }).start();
            }
        });
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
