package com.frank.ffmpeg.floating;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.VideoView;


public class FloatPlayerView extends FrameLayout {

    private final static String VIDEO_PATH = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
//    private final static String VIDEO_PATH =  "storage/emulated/0/beyond.mp4";

    private VideoView mPlayer;

    public FloatPlayerView(Context context) {
        super(context);
        init(context);
    }

    public FloatPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public FloatPlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    private void init(Context context) {
        mPlayer = new VideoView(context);

        LayoutParams layoutParams = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        layoutParams.gravity = Gravity.CENTER;
        addView(mPlayer, layoutParams);

        mPlayer.setVideoPath(VIDEO_PATH);
        mPlayer.requestFocus();
        mPlayer.start();
    }


    public void onPause() {
        if (mPlayer != null){
            mPlayer.pause();
        }
    }

    public void onResume() {
        if (mPlayer != null){
            mPlayer.resume();
        }
    }

    public void onDestroy(){
        if (mPlayer != null){
            mPlayer.stopPlayback();
            mPlayer = null;
        }
    }
}
