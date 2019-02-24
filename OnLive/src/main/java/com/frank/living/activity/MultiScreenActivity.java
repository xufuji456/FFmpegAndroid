package com.frank.living.activity;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import com.frank.living.R;
import com.frank.living.listener.IjkPlayerListener;
import com.frank.living.widget.IjkVideoView;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class MultiScreenActivity extends AppCompatActivity implements IjkPlayerListener{

    private final static String TAG = MultiScreenActivity.class.getSimpleName();

    private IjkMediaPlayer ijkMediaPlayer;
    private IjkVideoView mVideoView1;
    private IjkVideoView mVideoView2;
    private IjkVideoView mVideoView3;
    private IjkVideoView mVideoView4;

    private final static String url1 = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
    private final static String url2 = "http://ivi.bupt.edu.cn/hls/cctv5phd.m3u8";
    private final static String url3 = "rtmp://58.200.131.2:1935/livetv/hunantv";
    private final static String url4 = "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_multi_screen);

        init();

    }

    private void init(){
        IjkMediaPlayer.loadLibrariesOnce(null);
        IjkMediaPlayer.native_profileBegin("libijkplayer.so");

        mVideoView1 = (IjkVideoView) findViewById(R.id.video_view1);
        mVideoView1.setIjkPlayerListener(this);
        mVideoView1.setVideoPath(url1);
        mVideoView1.start();

        mVideoView2 = (IjkVideoView) findViewById(R.id.video_view2);
        mVideoView2.setVideoPath(url2);
        mVideoView2.start();

        mVideoView3 = (IjkVideoView) findViewById(R.id.video_view3);
        mVideoView3.setVideoPath(url3);
        mVideoView3.start();

        mVideoView4 = (IjkVideoView) findViewById(R.id.video_view4);
        mVideoView4.setVideoPath(url4);
        mVideoView4.start();
    }

    private void initOptions(){
        if (ijkMediaPlayer == null)
            return;
        Log.e(TAG, "initOptions");
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "fast", 1);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "probesize", 200);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "flush_packets", 1);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "packet-buffering", 0);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "start-on-prepared", 1);
        //0：代表关闭  1：代表开启
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 0);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 0);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 0);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "max-buffer-size", 0);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "min-frames", 2);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "max_cached_duration", 30);
        //input buffer:don't limit the input buffer size (useful with realtime streams)
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "infbuf", 1);
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "fflags", "nobuffer");
        //ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "rtsp_transport", "tcp");
        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "analyzedmaxduration", 100);
    }

    @Override
    public void onIjkPlayer(IjkMediaPlayer ijkMediaPlayer) {
        this.ijkMediaPlayer = ijkMediaPlayer;
        initOptions();
    }


    @Override
    protected void onStop() {
        super.onStop();

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
