package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.media.MediaMetadataRetriever;
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
import com.frank.ffmpeg.model.MediaBean;
import com.frank.ffmpeg.tool.JsonParseTool;
import com.frank.ffmpeg.util.BitmapUtil;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;

/**
 * video process by FFmpeg command
 * Created by frank on 2018/1/25.
 */
public class VideoHandleActivity extends BaseActivity {

    private final static String TAG = VideoHandleActivity.class.getSimpleName();
    private static final String PATH = Environment.getExternalStorageDirectory().getPath();

    private ProgressBar progressVideo;
    private LinearLayout layoutVideoHandle;
    private int viewId;
    private FFmpegHandler ffmpegHandler;
    private final static boolean useFFmpegCmd = true;

    private final static int TYPE_IMAGE = 1;
    private final static int TYPE_GIF   = 2;
    private final static int TYPE_TEXT  = 3;

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
                R.id.btn_moov,
                R.id.btn_joint
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
     * Using FFmpeg cmd to handle video
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
            case R.id.btn_video_transform://transform format
                String transformVideo = PATH + File.separator + "transformVideo.mp4";
                commandLine = FFmpegUtil.transformVideo(srcFile, transformVideo);
                break;
            case R.id.btn_video_cut://cut video
                String suffix = FileUtil.getFileSuffix(srcFile);
                if (suffix == null || suffix.isEmpty()) {
                    return;
                }
                String cutVideo = PATH + File.separator + "cutVideo" + suffix;
                int startTime = 0;
                int duration = 20;
                commandLine = FFmpegUtil.cutVideo(srcFile, startTime, duration, cutVideo);
                break;
            case R.id.btn_video_concat://concat video
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
            case R.id.btn_screen_shot://video snapshot
                String screenShot = PATH + File.separator + "screenShot.jpg";
                int time = 18;
                commandLine = FFmpegUtil.screenShot(srcFile, time, screenShot);
                break;
            case R.id.btn_water_mark://add watermark to video
                // the unit of bitRate is kb
                int bitRate = 500;
                MediaMetadataRetriever retriever = new MediaMetadataRetriever();
                retriever.setDataSource(srcFile);
                String mBitRate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE);
                if (mBitRate != null && !mBitRate.isEmpty()) {
                    int probeBitrate = Integer.valueOf(mBitRate);
                    bitRate = (probeBitrate/1000/100) * 100;
                }
                //1:top left 2:top right 3:bottom left 4:bottom right
                int location = 2;
                int offsetXY = 5;
                int waterMarkType = 1;
                switch (waterMarkType) {
                    case TYPE_IMAGE:// image
                        String photo = PATH + File.separator + "launcher.png";
                        String photoMark = PATH + File.separator + "photoMark.mp4";
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, photo, location, bitRate, offsetXY, photoMark);
                        break;
                    case TYPE_GIF:// gif
                        String gifPath = PATH + File.separator + "ok.gif";
                        String gifWaterMark = PATH + File.separator + "gifWaterMark.mp4";
                        commandLine = FFmpegUtil.addWaterMarkGif(srcFile, gifPath, location, bitRate, offsetXY, gifWaterMark);
                        break;
                    case TYPE_TEXT:// text
                        String text = "Hello,FFmpeg";
                        String textPath = PATH + File.separator + "text.jpg";
                        boolean result = BitmapUtil.textToPicture(textPath, text, this);
                        Log.i(TAG, "text to picture result=" + result);
                        String textMark = PATH + File.separator + "textMark.mp4";
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, textPath, location, bitRate, offsetXY, textMark);
                        break;
                    default:
                        break;
                }
                break;
            case R.id.btn_generate_gif://convert video into gif
                String Video2Gif = PATH + File.separator + "Video2Gif.gif";
                int gifStart = 30;
                int gifDuration = 5;
                String resolution = "720x1280";//240x320、480x640、1080x1920
                int frameRate = 10;
                commandLine = FFmpegUtil.generateGif(srcFile, gifStart, gifDuration,
                        resolution, frameRate, Video2Gif);
                break;
            case R.id.btn_screen_record://screen recording
//                String screenRecord = PATH + File.separator + "screenRecord.mp4";
//                String screenSize = "320x240";
//                int recordTime = 10;
//                commandLine = FFmpegUtil.screenRecord(screenSize, recordTime, screenRecord);
                break;
            case R.id.btn_multi_video://combine video which layout could be horizontal of vertical
                String input1 = PATH + File.separator + "input1.mp4";
                String input2 = PATH + File.separator + "input2.mp4";
                String outputFile = PATH + File.separator + "multi.mp4";
                if (!FileUtil.checkFileExist(input1) || !FileUtil.checkFileExist(input2)) {
                    return;
                }
                commandLine = FFmpegUtil.multiVideo(input1, input2, outputFile, VideoLayout.LAYOUT_HORIZONTAL);
                break;
            case R.id.btn_reverse_video://video reverse
                String output = PATH + File.separator + "reverse.mp4";
                commandLine = FFmpegUtil.reverseVideo(srcFile, output);
                break;
            case R.id.btn_denoise_video://noise reduction of video
                String denoise = PATH + File.separator + "denoise.mp4";
                commandLine = FFmpegUtil.denoiseVideo(srcFile, denoise);
                break;
            case R.id.btn_to_image://convert video to picture
                String imagePath = PATH + File.separator + "Video2Image/";
                File imageFile = new File(imagePath);
                if (!imageFile.exists()) {
                    boolean result = imageFile.mkdir();
                    if (!result) {
                        return;
                    }
                }
                int mStartTime = 10;//start time
                int mDuration = 20;//duration
                int mFrameRate = 10;//frameRate
                commandLine = FFmpegUtil.videoToImage(srcFile, mStartTime, mDuration, mFrameRate, imagePath);
                break;
            case R.id.btn_pip://combine into picture-in-picture video
                String inputFile1 = PATH + File.separator + "beyond.mp4";
                String inputFile2 = PATH + File.separator + "small_girl.mp4";
                if (!FileUtil.checkFileExist(inputFile1) && !FileUtil.checkFileExist(inputFile2)) {
                    return;
                }
                //x and y coordinate points need to be calculated according to the size of full video and small video
                //For example: full video is 320x240, small video is 120x90, so x=200 y=150
                int x = 200;
                int y = 150;
                String picInPic = PATH + File.separator + "PicInPic.mp4";
                commandLine = FFmpegUtil.picInPicVideo(inputFile1, inputFile2, x, y, picInPic);
                break;
            case R.id.btn_moov://moov box moves ahead, which is behind mdat box of mp4 video
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
            case R.id.btn_joint:// joint two videos together
                jointVideo(srcFile);
                break;
            default:
                break;
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    /**
     * joint two videos together
     * It's recommended to convert to the same resolution and encoding
     * @param selectedPath the path which is selected
     */
    private void jointVideo(String selectedPath) {
        if (ffmpegHandler == null || selectedPath.isEmpty()) {
            return;
        }
        String appendPath = PATH + File.separator + "snow.mp4";
        String outputPath1 = PATH + File.separator + "output1.ts";
        String outputPath2 = PATH + File.separator + "output2.ts";
        String listPath = PATH + File.separator + "listFile.txt";
        String targetPath = PATH + File.separator + "jointVideo.mp4";
        String[] transformCmd1 = FFmpegUtil.transformVideoWithEncode(selectedPath, outputPath1);
        int width = 0;
        int height = 0;
        //probe width and height of the selected video
        String probeResult = FFmpegCmd.executeProbeSynchronize(FFmpegUtil.probeFormat(selectedPath));
        MediaBean mediaBean = JsonParseTool.parseMediaFormat(probeResult);
        if (mediaBean != null && mediaBean.getVideoBean() != null) {
            width = mediaBean.getVideoBean().getWidth();
            height = mediaBean.getVideoBean().getHeight();
            Log.e(TAG, "width=" + width + "--height=" + height);
        }
        String[] transformCmd2 = FFmpegUtil.transformVideoWithEncode(appendPath, width, height, outputPath2);
        List<String> fileList = new ArrayList<>();
        fileList.add(outputPath1);
        fileList.add(outputPath2);
        FileUtil.createListFile(listPath, fileList);
        String[] jointVideoCmd = FFmpegUtil.jointVideo(listPath, targetPath);
        List<String[]> commandList = new ArrayList<>();
        commandList.add(transformCmd1);
        commandList.add(transformCmd2);
        commandList.add(jointVideoCmd);
        ffmpegHandler.executeFFmpegCmds(commandList);
    }

    /**
     * Combine pictures into video
     */
    private void handlePhoto() {
        // The path of pictures, naming format: img+number.jpg
        // Here assigns the directory is img under the root path
        String picturePath = PATH + "/img/";
        if (!FileUtil.checkFileExist(picturePath)) {
            return;
        }
        String combineVideo = PATH + File.separator + "combineVideo.mp4";
        int frameRate = 2;// suggested synthetic frameRate:1-10
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
