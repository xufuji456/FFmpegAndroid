package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
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
import android.widget.Toast;
import android.widget.ToggleButton;

import com.frank.ffmpeg.FFmpegApplication;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.adapter.HorizontalAdapter;
import com.frank.ffmpeg.listener.OnItemClickListener;
import com.frank.ffmpeg.util.FileUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Using ffmpeg to filter
 * Created by frank on 2018/6/5.
 */

public class FilterActivity extends BaseActivity implements SurfaceHolder.Callback {

    private String videoPath = Environment.getExternalStorageDirectory().getPath() + "/beyond.mp4";

    private VideoPlayer videoPlayer;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private boolean surfaceCreated;
    //is playing or not
    private boolean isPlaying;
    //the array of filter
    private String[] filters = new String[]{
            "lutyuv='u=128:v=128'",
            "hue='h=60:s=-3'",
            "edgedetect=low=0.1:high=0.4",
            "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5",
            "colorbalance=bs=0.3",
            "drawbox=x=100:y=100:w=100:h=100:color=red@0.5'",
            "hflip",
            //adjust the coefficient of sigma to control the blur
            "gblur=sigma=2:steps=1:planes=1:sigmaV=1",
            "rotate=180*PI/180",
            "unsharp"
    };
    //vflip is up and down, hflip is left and right
    private String[] txtArray = new String[]{
            FFmpegApplication.getInstance().getString(R.string.filter_sketch),
            FFmpegApplication.getInstance().getString(R.string.filter_distinct),
            FFmpegApplication.getInstance().getString(R.string.filter_edge),
            FFmpegApplication.getInstance().getString(R.string.filter_division),
            FFmpegApplication.getInstance().getString(R.string.filter_equalize),
            FFmpegApplication.getInstance().getString(R.string.filter_rectangle),
            FFmpegApplication.getInstance().getString(R.string.filter_flip),
            FFmpegApplication.getInstance().getString(R.string.filter_blur),
            FFmpegApplication.getInstance().getString(R.string.filter_rotate),
            FFmpegApplication.getInstance().getString(R.string.filter_sharpening)
    };
    private HorizontalAdapter horizontalAdapter;
    private RecyclerView recyclerView;
    private boolean playAudio = true;
    private ToggleButton btnSound;
    private Button btnSelect;

    private final static int MSG_HIDE = 222;
    private final static int DELAY_TIME = 5000;
    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == MSG_HIDE) { //after idle 5s, hide the controller view
                recyclerView.setVisibility(View.GONE);
                btnSound.setVisibility(View.GONE);
                btnSelect.setVisibility(View.GONE);
            }
        }
    };

    private class HideRunnable implements Runnable {
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

    private void initView() {
        surfaceView = getView(R.id.surface_filter);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        videoPlayer = new VideoPlayer();
        btnSound = getView(R.id.btn_sound);

        recyclerView = getView(R.id.recycler_view);
        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
        recyclerView.setLayoutManager(linearLayoutManager);
        List<String> itemList = new ArrayList<>(Arrays.asList(txtArray));
        horizontalAdapter = new HorizontalAdapter(itemList);
        recyclerView.setAdapter(horizontalAdapter);

        btnSelect = getView(R.id.btn_select_file);
        initViewsWithClick(R.id.btn_select_file);
    }

    private void registerLister() {
        horizontalAdapter.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(int position) {
                if (!surfaceCreated)
                    return;
                if (!FileUtil.checkFileExist(videoPath)) {
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
                recyclerView.setVisibility(View.VISIBLE);
                mHandler.postDelayed(hideRunnable, DELAY_TIME);
            }
        });

        btnSound.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setPlayAudio();
            }
        });
    }

    /**
     * switch filter
     * @param position position in the array of filters
     */
    private void doFilterPlay(final int position) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (isPlaying) {
                    videoPlayer.again();
                }
                isPlaying = true;
                videoPlayer.filter(videoPath, surfaceHolder.getSurface(), filters[position]);
            }
        }).start();
    }

    private void setPlayAudio() {
        playAudio = !playAudio;
        videoPlayer.playAudio(playAudio);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        surfaceCreated = true;
        if (FileUtil.checkFileExist(videoPath)) {
            doFilterPlay(4);
            btnSound.setChecked(true);
        } else {
            Toast.makeText(FilterActivity.this, getString(R.string.file_not_found), Toast.LENGTH_SHORT).show();
        }
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
        //FIXME
//        videoPlayer.release();
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
        doFilterPlay(4);
        //sound off by default
        btnSound.setChecked(true);
    }

}
