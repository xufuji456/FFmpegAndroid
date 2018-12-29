package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.ProgressBar;
import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.format.VideoLayout;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;
import java.io.File;

public class VideoHandleActivity extends AppCompatActivity implements View.OnClickListener{

    private static final String TAG = VideoHandleActivity.class.getSimpleName();
    private static final int MSG_BEGIN = 101;
    private static final int MSG_FINISH = 102;

    private static final String PATH = Environment.getExternalStorageDirectory().getPath();
    private static final String srcFile = PATH + File.separator + "hello.mp4";
    private static final String appendVideo = PATH + File.separator + "test.mp4";
    private ProgressBar progress_video;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case MSG_BEGIN:
                    progress_video.setVisibility(View.VISIBLE);
                    setGone();
                    break;
                case MSG_FINISH:
                    progress_video.setVisibility(View.GONE);
                    setVisible();
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        if (getSupportActionBar() != null){
            getSupportActionBar().hide();
        }
        setContentView(R.layout.activity_video_handle);

        intView();
    }

    private void intView() {
        progress_video = (ProgressBar)findViewById(R.id.progress_video);
        findViewById(R.id.btn_video_transform).setOnClickListener(this);
        findViewById(R.id.btn_video_cut).setOnClickListener(this);
        findViewById(R.id.btn_video_concat).setOnClickListener(this);
        findViewById(R.id.btn_screen_shot).setOnClickListener(this);
        findViewById(R.id.btn_water_mark).setOnClickListener(this);
        findViewById(R.id.btn_generate_gif).setOnClickListener(this);
        findViewById(R.id.btn_screen_record).setOnClickListener(this);
        findViewById(R.id.btn_combine_video).setOnClickListener(this);
        findViewById(R.id.btn_play_video).setOnClickListener(this);
        findViewById(R.id.btn_multi_video).setOnClickListener(this);
        findViewById(R.id.btn_reverse_video).setOnClickListener(this);
        findViewById(R.id.btn_denoise_video).setOnClickListener(this);
        findViewById(R.id.btn_to_image).setOnClickListener(this);
        findViewById(R.id.btn_pip).setOnClickListener(this);
    }

    private void setVisible() {
        findViewById(R.id.btn_video_transform).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_video_cut).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_video_concat).setVisibility(View.GONE);
        findViewById(R.id.btn_screen_shot).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_water_mark).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_generate_gif).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_screen_record).setVisibility(View.GONE);
        findViewById(R.id.btn_combine_video).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_play_video).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_multi_video).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_reverse_video).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_denoise_video).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_to_image).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_pip).setVisibility(View.VISIBLE);
    }

    private void setGone() {
        findViewById(R.id.btn_video_transform).setVisibility(View.GONE);
        findViewById(R.id.btn_video_cut).setVisibility(View.GONE);
        findViewById(R.id.btn_video_concat).setVisibility(View.GONE);
        findViewById(R.id.btn_screen_shot).setVisibility(View.GONE);
        findViewById(R.id.btn_water_mark).setVisibility(View.GONE);
        findViewById(R.id.btn_generate_gif).setVisibility(View.GONE);
        findViewById(R.id.btn_screen_record).setVisibility(View.GONE);
        findViewById(R.id.btn_combine_video).setVisibility(View.GONE);
        findViewById(R.id.btn_play_video).setVisibility(View.GONE);
        findViewById(R.id.btn_multi_video).setVisibility(View.GONE);
        findViewById(R.id.btn_reverse_video).setVisibility(View.GONE);
        findViewById(R.id.btn_denoise_video).setVisibility(View.GONE);
        findViewById(R.id.btn_to_image).setVisibility(View.GONE);
        findViewById(R.id.btn_pip).setVisibility(View.GONE);
    }

    @Override
    public void onClick(View v) {
        int handleType;
        switch (v.getId()){
            case R.id.btn_video_transform:
                handleType = 0;
                break;
            case R.id.btn_video_cut:
                handleType = 1;
                break;
            case R.id.btn_video_concat:
                handleType = 2;
                break;
            case R.id.btn_screen_shot:
                handleType = 3;
                break;
            case R.id.btn_water_mark:
                handleType = 4;
                break;
            case R.id.btn_generate_gif:
                handleType = 5;
                break;
            case R.id.btn_screen_record:
                handleType = 6;
                break;
            case R.id.btn_combine_video:
                handleType = 7;
                break;
            case R.id.btn_play_video:
                handleType = 8;
                break;
            case R.id.btn_multi_video:
                handleType = 9;
                break;
            case R.id.btn_reverse_video:
                handleType = 10;
                break;
            case R.id.btn_denoise_video:
                handleType = 11;
                break;
            case R.id.btn_to_image:
                handleType = 12;
                break;
            case R.id.btn_pip:
                handleType = 13;
                break;
            default:
                handleType = 0;
                break;
        }
        doHandleVideo(handleType);
    }

    /**
     * 调用ffmpeg处理视频
     * @param handleType handleType
     */
    private void doHandleVideo(int handleType){
        String[] commandLine = null;
        switch (handleType){
            case 0://视频转码:mp4转flv、wmv, 或者flv、wmv转Mp4
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                String transformVideo = PATH + File.separator + "transformVideo.flv";
                commandLine = FFmpegUtil.transformVideo(srcFile, transformVideo);
                break;
            case 1://视频剪切
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                String cutVideo = PATH + File.separator + "cutVideo.mp4";
                int startTime = 0;
                int duration = 20;
                commandLine = FFmpegUtil.cutVideo(srcFile, startTime, duration, cutVideo);
                break;
            case 2://视频合并
//                commandLine = FFmpegUtil.toTs(srcFile, ts1);
//                concatStep ++;
//                String concatVideo = PATH + File.separator + "concatVideo.mp4";
//                String appendVideo = PATH + File.separator + "test.mp4";
//                File concatFile = new File(PATH + File.separator + "fileList.txt");
//                try {
//                    FileOutputStream fileOutputStream = new FileOutputStream(concatFile);
//                    fileOutputStream.write(("file \'" + srcFile + "\'").getBytes());
//                    fileOutputStream.write("\n".getBytes());
//                    fileOutputStream.write(("file \'" + appendVideo + "\'").getBytes());
//                    fileOutputStream.flush();
//                    fileOutputStream.close();
//                } catch (Exception e) {
//                    e.printStackTrace();
//                }
//
//                commandLine = FFmpegUtil.concatVideo(srcFile, concatFile.getAbsolutePath(), concatVideo);
                break;
            case 3://视频截图
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                String screenShot = PATH + File.separator + "screenShot.jpg";
                String size = "1080x720";
                commandLine = FFmpegUtil.screenShot(srcFile, size, screenShot);
                break;
            case 4://视频添加水印
                if (!FileUtil.checkFileExist(appendVideo)){
                    return;
                }
                //1、图片
                String photo = PATH + File.separator + "launcher.png";
                String photoMark = PATH + File.separator + "photoMark.mp4";
                commandLine = FFmpegUtil.addWaterMark(appendVideo, photo, photoMark);
                //2、文字
//                String text = "Hello,FFmpeg";
//                String textPath = PATH + File.separator + "text.jpg";
//                boolean result = BitmapUtil.textToPicture(textPath, text, this);
//                Log.i(TAG, "text to pitcture result=" + result);
//                String textMark = PATH + File.separator + "textMark.mp4";
//                commandLine = FFmpegUtil.addWaterMark(appendVideo, textPath, textMark);
                break;
            case 5://视频转成gif
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                String Video2Gif = PATH + File.separator + "Video2Gif.gif";
                int gifStart = 30;
                int gifDuration = 5;
                commandLine = FFmpegUtil.generateGif(srcFile, gifStart, gifDuration, Video2Gif);
                break;
            case 6://屏幕录制
//                String screenRecord = PATH + File.separator + "screenRecord.mp4";
//                String screenSize = "320x240";
//                int recordTime = 10;
//                commandLine = FFmpegUtil.screenRecord(screenSize, recordTime, screenRecord);
                break;
            case 7://图片合成视频
                //图片所在路径，图片命名格式img+number.jpg
                String picturePath = PATH + File.separator + "img/";
                if (!FileUtil.checkFileExist(picturePath)){
                    return;
                }
                String combineVideo = PATH + File.separator + "combineVideo.mp4";
                commandLine = FFmpegUtil.pictureToVideo(picturePath, combineVideo);
                break;
            case 8://视频解码播放
                startActivity(new Intent(VideoHandleActivity.this, VideoPlayerActivity.class));
                return;
            case 9://视频画面拼接:分辨率、时长、封装格式不一致时，先把视频源转为一致
                String input1 = PATH + File.separator + "input1.mp4";
                String input2 = PATH + File.separator + "input2.mp4";
                String outputFile = PATH + File.separator + "multi.mp4";
                if (!FileUtil.checkFileExist(input1) || !FileUtil.checkFileExist(input2)){
                    return;
                }
                commandLine = FFmpegUtil.multiVideo(input1, input2, outputFile, VideoLayout.LAYOUT_HORIZONTAL);
                break;
            case 10://视频反序倒播
                String output = PATH + File.separator + "reverse.mp4";
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                commandLine = FFmpegUtil.reverseVideo(srcFile, output);
                break;
            case 11://视频降噪
                String denoise = PATH + File.separator + "denoise.mp4";
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                commandLine = FFmpegUtil.denoiseVideo(srcFile, denoise);
                break;
            case 12://视频转图片
                String srcFile = PATH + File.separator + "beyond.mp4";
                if (!FileUtil.checkFileExist(srcFile)){
                    return;
                }
                String imagePath = PATH + File.separator + "Video2Image/";//图片保存路径
                File imageFile = new File(imagePath);
                if (!imageFile.exists()){
                    imageFile.mkdir();
                }
                int mStartTime = 10;//开始时间
                int mDuration = 20;//持续时间（注意开始时间+持续时间之和不能大于视频总时长）
                int mFrameRate = 10;//帧率（从视频中每秒抽多少帧）
                commandLine = FFmpegUtil.videoToImage(srcFile, mStartTime, mDuration, mFrameRate, imagePath);
                break;
            case 13://两个视频合成画中画
                String inputFile1 = PATH + File.separator + "beyond.mp4";
                String inputFile2 = PATH + File.separator + "small_girl.mp4";
                if (!FileUtil.checkFileExist(inputFile1) && !FileUtil.checkFileExist(inputFile2)){
                    return;
                }
                //x、y坐标点需要根据全屏视频与小视频大小，进行计算
                //比如：全屏视频为320x240，小视频为120x90，那么x=200 y=150
                int x = 200;
                int y = 150;
                String picInPic = PATH + File.separator + "PicInPic.mp4";
                commandLine = FFmpegUtil.picInPicVideo(inputFile1, inputFile2, x, y, picInPic);
                break;
            default:
                break;
        }
        executeFFmpegCmd(commandLine);
    }

    /**
     * 执行ffmpeg命令行
     * @param commandLine commandLine
     */
    private void executeFFmpegCmd(final String[] commandLine){
        if(commandLine == null){
            return;
        }
        FFmpegCmd.execute(commandLine, new FFmpegCmd.OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle video onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onEnd(int result) {
                Log.i(TAG, "handle video onEnd...");

                mHandler.obtainMessage(MSG_FINISH).sendToTarget();
            }
        });
    }
}
