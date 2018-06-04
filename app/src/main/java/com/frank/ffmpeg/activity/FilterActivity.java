package com.frank.ffmpeg.activity;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import java.io.File;

public class FilterActivity extends AppCompatActivity implements View.OnClickListener, SurfaceHolder.Callback{

    //SD卡根目录
    private final static String PATH = Environment.getExternalStorageDirectory().getPath();
    //本地视频路径
    private final static String VIDEO_PATH = PATH + File.separator + "Beyond.mp4";

    private VideoPlayer videoPlayer;
    private SurfaceHolder surfaceHolder;
    private boolean surfaceCreated;

    private String[] filters = new String[]{
            "lutyuv='u=128:v=128'",
            "hue='h=60:s=-3'",
            "lutrgb='r=0:g=0'",
            "edgedetect=low=0.1:high=0.4",
            "fftfilt=dc_Y=0:weight_Y='exp(-4 * ((Y+X)/(W+H)))'",
            "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5",
            "colorbalance=bs=0.3",
            "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5'",
            "rotate=90",
            "vflip",
            "noise=alls=20:allf=t+u",
            "vignette='PI/4+random(1)*PI/50':eval=frame"
    };
    private int filterType;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_filter);

        initView();
    }

    private void initView(){
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface_filter);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        videoPlayer = new VideoPlayer();

        findViewById(R.id.btn_sketch).setOnClickListener(this);
        findViewById(R.id.btn_hue).setOnClickListener(this);
        findViewById(R.id.btn_lut).setOnClickListener(this);
        findViewById(R.id.btn_edge).setOnClickListener(this);
        findViewById(R.id.btn_blur).setOnClickListener(this);
        findViewById(R.id.btn_grid).setOnClickListener(this);
        findViewById(R.id.btn_balance).setOnClickListener(this);
        findViewById(R.id.btn_box).setOnClickListener(this);
        findViewById(R.id.btn_rotate).setOnClickListener(this);
        findViewById(R.id.btn_flip).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if(!surfaceCreated)
            return;
        switch (v.getId()){
            case R.id.btn_sketch://素描
                filterType = 0;
                break;
            case R.id.btn_hue://hue
                filterType = 1;
                break;
            case R.id.btn_lut://lut
                filterType = 2;
                break;
            case R.id.btn_edge://边缘
                filterType = 3;
                break;
            case R.id.btn_blur://模糊
                filterType = 4;
                break;
            case R.id.btn_grid://九宫格
                filterType = 5;
                break;
            case R.id.btn_balance://色彩平衡
                filterType = 6;
                break;
            case R.id.btn_box://box绘制矩形区域
                filterType = 7;
                break;
            case R.id.btn_rotate://旋转
                filterType = 8;
                break;
            case R.id.btn_flip://翻转
                filterType = 9;
                break;
            default:
                filterType = 0;
                break;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                videoPlayer.filter(VIDEO_PATH, surfaceHolder.getSurface(), filters[filterType]);
            }
        }).start();
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

}
