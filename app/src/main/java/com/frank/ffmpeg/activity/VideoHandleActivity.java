package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.format.VideoLayout;
import com.frank.ffmpeg.handler.FFmpegHandler;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;

public class VideoHandleActivity extends BaseActivity {

    private final static String TAG = VideoHandleActivity.class.getSimpleName();
    private static final String PATH = Environment.getExternalStorageDirectory().getPath();

    private ProgressBar progressVideo;
    private LinearLayout layoutVideoHandle;
    private int viewId;
    private FFmpegHandler ffmpegHandler;
    private final static boolean useFFmpegCmd = true;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_BEGIN:
                    progressVideo.setVisibility(View.VISIBLE);
                    layoutVideoHandle.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    progressVideo.setVisibility(View.GONE);
                    layoutVideoHandle.setVisibility(View.VISIBLE);
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_video_handle;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        intView();
        ffmpegHandler = new FFmpegHandler(mHandler);
    }

    private void intView() {
        progressVideo = getView(R.id.progress_video);
        layoutVideoHandle = getView(R.id.layout_video_handle);
        initViewsWithClick(
                R.id.btn_video_transform,
                R.id.btn_video_cut,
                R.id.btn_video_concat,
                R.id.btn_screen_shot,
                R.id.btn_water_mark,
                R.id.btn_generate_gif,
                R.id.btn_screen_record,
                R.id.btn_combine_video,
                R.id.btn_multi_video,
                R.id.btn_reverse_video,
                R.id.btn_denoise_video,
                R.id.btn_to_image,
                R.id.btn_pip,
                R.id.btn_moov
        );
    }

    @Override
    public void onViewClick(View view) {
        viewId = view.getId();
        if (viewId == R.id.btn_combine_video) {
            handlePhoto();
            return;
        }
        selectFile();
    }

    @Override
    void onSelectedFile(String filePath) {
        doHandleVideo(filePath);
    }

    /**
     * 调用ffmpeg处理视频
     *
     * @param srcFile srcFile
     */
    private void doHandleVideo(String srcFile) {
        String[] commandLine = null;
        if (!FileUtil.checkFileExist(srcFile)) {
            return;
        }
        if (!FileUtil.isVideo(srcFile)) {
            showToast(getString(R.string.wrong_video_format));
            return;
        }
        switch (viewId) {
            case R.id.btn_video_transform://视频转码:mp4转flv、wmv, 或者flv、wmv转Mp4
                String transformVideo = PATH + File.separator + "transformVideo.mp4";
                commandLine = FFmpegUtil.transformVideo(srcFile, transformVideo);
                break;
            case R.id.btn_video_cut://视频剪切
                String suffix = FileUtil.getFileSuffix(srcFile);
                if (suffix == null || suffix.isEmpty()) {
                    return;
                }
                String cutVideo = PATH + File.separator + "cutVideo" + suffix;
                int startTime = 0;
                int duration = 20;
                commandLine = FFmpegUtil.cutVideo(srcFile, startTime, duration, cutVideo);
                break;
            case R.id.btn_video_concat://视频合并
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
//                commandLine = FFmpegUtil.concatVideo(srcFile, concatFile.getAbsolutePath(), concatVideo);
                break;
            case R.id.btn_screen_shot://视频截图
                String screenShot = PATH + File.separator + "screenShot.jpg";
                int time = 18;
                commandLine = FFmpegUtil.screenShot(srcFile, time, screenShot);
                break;
            case R.id.btn_water_mark://视频添加水印
                //1、图片
                String photo = PATH + File.separator + "launcher.png";
                String photoMark = PATH + File.separator + "photoMark.mp4";
                String mResolution = "720x1280";
                int bitRate = 1024;
                commandLine = FFmpegUtil.addWaterMark(srcFile, photo, mResolution, bitRate, photoMark);
                //2、文字
//                String text = "Hello,FFmpeg";
//                String textPath = PATH + File.separator + "text.jpg";
//                boolean result = BitmapUtil.textToPicture(textPath, text, this);
//                Log.i(TAG, "text to pitcture result=" + result);
//                String textMark = PATH + File.separator + "textMark.mp4";
//                commandLine = FFmpegUtil.addWaterMark(srcFile, textPath, textMark);
                break;
            case R.id.btn_generate_gif://视频转成gif
                String Video2Gif = PATH + File.separator + "Video2Gif.gif";
                int gifStart = 30;
                int gifDuration = 5;
                String resolution = "720x1280";//240x320、480x640、1080x1920
                int frameRate = 10;
                commandLine = FFmpegUtil.generateGif(srcFile, gifStart, gifDuration,
                        resolution, frameRate, Video2Gif);
                break;
            case R.id.btn_screen_record://屏幕录制
//                String screenRecord = PATH + File.separator + "screenRecord.mp4";
//                String screenSize = "320x240";
//                int recordTime = 10;
//                commandLine = FFmpegUtil.screenRecord(screenSize, recordTime, screenRecord);
                break;
            case R.id.btn_multi_video://视频画面拼接:分辨率、时长、封装格式不一致时，先把视频源转为一致
                String input1 = PATH + File.separator + "input1.mp4";
                String input2 = PATH + File.separator + "input2.mp4";
                String outputFile = PATH + File.separator + "multi.mp4";
                if (!FileUtil.checkFileExist(input1) || !FileUtil.checkFileExist(input2)) {
                    return;
                }
                commandLine = FFmpegUtil.multiVideo(input1, input2, outputFile, VideoLayout.LAYOUT_HORIZONTAL);
                break;
            case R.id.btn_reverse_video://视频反序倒播
                String output = PATH + File.separator + "reverse.mp4";
                commandLine = FFmpegUtil.reverseVideo(srcFile, output);
                break;
            case R.id.btn_denoise_video://视频降噪
                String denoise = PATH + File.separator + "denoise.mp4";
                commandLine = FFmpegUtil.denoiseVideo(srcFile, denoise);
                break;
            case R.id.btn_to_image://视频转图片
                String imagePath = PATH + File.separator + "Video2Image/";//图片保存路径
                File imageFile = new File(imagePath);
                if (!imageFile.exists()) {
                    boolean result = imageFile.mkdir();
                    if (!result) {
                        return;
                    }
                }
                int mStartTime = 10;//开始时间
                int mDuration = 20;//持续时间（注意开始时间+持续时间之和不能大于视频总时长）
                int mFrameRate = 10;//帧率（从视频中每秒抽多少帧）
                commandLine = FFmpegUtil.videoToImage(srcFile, mStartTime, mDuration, mFrameRate, imagePath);
                break;
            case R.id.btn_pip://两个视频合成画中画
                String inputFile1 = PATH + File.separator + "beyond.mp4";
                String inputFile2 = PATH + File.separator + "small_girl.mp4";
                if (!FileUtil.checkFileExist(inputFile1) && !FileUtil.checkFileExist(inputFile2)) {
                    return;
                }
                //x、y坐标点需要根据全屏视频与小视频大小，进行计算
                //比如：全屏视频为320x240，小视频为120x90，那么x=200 y=150
                int x = 200;
                int y = 150;
                String picInPic = PATH + File.separator + "PicInPic.mp4";
                commandLine = FFmpegUtil.picInPicVideo(inputFile1, inputFile2, x, y, picInPic);
                break;
            case R.id.btn_moov://moov前移操作，针对mp4视频moov在mdat后面的情况
                if (!srcFile.endsWith(FileUtil.TYPE_MP4)) {
                    showToast(getString(R.string.tip_not_mp4_video));
                    return;
                }
                String filePath = FileUtil.getFilePath(srcFile);
                String fileName = FileUtil.getFileName(srcFile);
                Log.e(TAG, "moov filePath=" + filePath + "--fileName=" + fileName);
                fileName = "moov_" + fileName;
                String moovPath = filePath + File.separator + fileName;
                if (useFFmpegCmd) {
                    commandLine = FFmpegUtil.moveMoovAhead(srcFile, moovPath);
                } else {
                    long start = System.currentTimeMillis();
                    FFmpegCmd ffmpegCmd = new FFmpegCmd();
                    int result = ffmpegCmd.moveMoovAhead(srcFile, moovPath);
                    Log.e(TAG, "result=" + (result == 0));
                    Log.e(TAG, "move moov use time=" + (System.currentTimeMillis() - start));
                }
                break;
            default:
                break;
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    /**
     * 图片合成视频
     */
    private void handlePhoto() {
        // 图片所在路径，图片命名格式img+number.jpg
        // 这里指定目录为根目录下img文件夹
        String picturePath = PATH + "/img/";
        if (!FileUtil.checkFileExist(picturePath)) {
            return;
        }
        String combineVideo = PATH + File.separator + "combineVideo.mp4";
        int frameRate = 2;// 合成视频帧率建议:1-10  普通视频帧率一般为25
        String[] commandLine = FFmpegUtil.pictureToVideo(picturePath, frameRate, combineVideo);
        if (ffmpegHandler != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }
}
