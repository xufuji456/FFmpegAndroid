package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.graphics.Color;
import android.media.MediaMetadataRetriever;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.format.VideoLayout;
import com.frank.ffmpeg.gif.HighQualityGif;
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
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS;

/**
 * video process by FFmpeg command
 * Created by frank on 2018/1/25.
 */
public class VideoHandleActivity extends BaseActivity {

    private final static String TAG = VideoHandleActivity.class.getSimpleName();
    private static final String PATH = Environment.getExternalStorageDirectory().getPath();

    private LinearLayout layoutVideoHandle;
    private LinearLayout layoutProgress;
    private TextView txtProgress;
    private int viewId;
    private FFmpegHandler ffmpegHandler;
    private final static boolean useFFmpegCmd = true;

    private final static int TYPE_IMAGE = 1;
    private final static int TYPE_GIF   = 2;
    private final static int TYPE_TEXT  = 3;
    private final static int waterMarkType = TYPE_IMAGE;

    private String appendPath = PATH + File.separator + "snow.mp4";
    private String outputPath1 = PATH + File.separator + "output1.ts";
    private String outputPath2 = PATH + File.separator + "output2.ts";
    private String listPath = PATH + File.separator + "listFile.txt";

    private boolean isJointing = false;
    private final static boolean convertGifWithFFmpeg = false;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_BEGIN:
                    layoutProgress.setVisibility(View.VISIBLE);
                    layoutVideoHandle.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    layoutProgress.setVisibility(View.GONE);
                    layoutVideoHandle.setVisibility(View.VISIBLE);
                    if (isJointing) {
                        isJointing = false;
                        FileUtil.deleteFile(outputPath1);
                        FileUtil.deleteFile(outputPath2);
                        FileUtil.deleteFile(listPath);
                    }
                    break;
                case MSG_PROGRESS:
                    int progress = msg.arg1;
                    int duration = msg.arg2;
                    if (progress > 0) {
                        txtProgress.setVisibility(View.VISIBLE);
                        String percent = duration > 0 ? "%" : "";
                        String strProgress = progress + percent;
                        txtProgress.setText(strProgress);
                    } else {
                        txtProgress.setVisibility(View.INVISIBLE);
                    }
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
        layoutProgress = getView(R.id.layout_progress);
        txtProgress = getView(R.id.txt_progress);
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
                R.id.btn_speed,
                R.id.btn_flv,
                R.id.btn_thumbnail
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
            case R.id.btn_video_concat://concat video together
                concatVideo(srcFile);
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
                switch (waterMarkType) {
                    case TYPE_IMAGE:// image
                        String photo = PATH + File.separator + "hello.png";
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
                        String textPath = PATH + File.separator + "text.png";
                        boolean result = BitmapUtil.textToPicture(textPath, text, Color.BLUE, 20);
                        Log.i(TAG, "text to picture result=" + result);
                        String textMark = PATH + File.separator + "textMark.mp4";
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, textPath, location, bitRate, offsetXY, textMark);
                        break;
                    default:
                        break;
                }
                break;
            case R.id.btn_generate_gif://convert video into gif
                String video2Gif = PATH + File.separator + "video2Gif.gif";
                int gifStart = 10;
                int gifDuration = 3;
                int width = 320;
                int frameRate = 10;

                if (convertGifWithFFmpeg) {
                    String palettePath = PATH + "/palette.png";
                    FileUtil.deleteFile(palettePath);
                    String[] paletteCmd = FFmpegUtil.generatePalette(srcFile, gifStart, gifDuration,
                            frameRate, width, palettePath);
                    String[] gifCmd = FFmpegUtil.generateGifByPalette(srcFile, palettePath, gifStart, gifDuration,
                            frameRate, width, video2Gif);
                    List<String[]> cmdList = new ArrayList<>();
                    cmdList.add(paletteCmd);
                    cmdList.add(gifCmd);
                    ffmpegHandler.executeFFmpegCmds(cmdList);
                } else {
                    convertGifInHighQuality(video2Gif, srcFile, gifStart, gifDuration, frameRate);
                }
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
                    if (!imageFile.mkdir()) {
                        return;
                    }
                }
                int mStartTime = 10;//start time
                int mDuration = 5;//duration
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
            case R.id.btn_speed://playing speed of video
                String speed = PATH + File.separator + "speed.mp4";
                commandLine = FFmpegUtil.changeSpeed(srcFile, speed, 2f, false);
                break;
            case R.id.btn_flv://rebuild the keyframe index of flv
                if (!".flv".equalsIgnoreCase(FileUtil.getFileSuffix(srcFile))) {
                    Log.e(TAG, "It's not flv file, suffix=" + FileUtil.getFileSuffix(srcFile));
                    return;
                }
                String outputPath = PATH + File.separator + "frame_index.flv";
                commandLine = FFmpegUtil.buildFlvIndex(srcFile, outputPath);
                break;
            case R.id.btn_thumbnail:// insert thumbnail into video
                String thumbSuffix = FileUtil.getFileSuffix(srcFile);
                if (thumbSuffix == null || thumbSuffix.isEmpty()) {
                    return;
                }
                String thumbnailPath = PATH + File.separator + "thumb.jpg";
                String thumbVideoPath = PATH + File.separator + "thumbnailVideo" + thumbSuffix;
                commandLine = FFmpegUtil.insertPicIntoVideo(srcFile, thumbnailPath, thumbVideoPath);
                break;
            default:
                break;
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    /**
     * concat/joint two videos together
     * It's recommended to convert to the same resolution and encoding
     * @param selectedPath the path which is selected
     */
    private void concatVideo(String selectedPath) {
        if (ffmpegHandler == null || selectedPath.isEmpty()) {
            return;
        }
        isJointing = true;
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
        String picturePath = PATH + "/img/";
        if (!FileUtil.checkFileExist(picturePath)) {
            return;
        }
        String tempPath = PATH + "/temp/";
        FileUtil.deleteFolder(tempPath);
        File photoFile = new File(picturePath);
        File[] files = photoFile.listFiles();
        List<String[]> cmdList = new ArrayList<>();
        //the resolution of photo which you want to convert
        String resolution = "640x320";
        for (File file : files) {
            String inputPath = file.getAbsolutePath();
            String outputPath = tempPath + file.getName();
            String[] convertCmd = FFmpegUtil.convertResolution(inputPath, resolution, outputPath);
            cmdList.add(convertCmd);
        }
        String combineVideo = PATH + File.separator + "combineVideo.mp4";
        int frameRate = 2;// suggested synthetic frameRate:1-10
        String[] commandLine = FFmpegUtil.pictureToVideo(tempPath, frameRate, combineVideo);
        cmdList.add(commandLine);
        if (ffmpegHandler != null) {
            ffmpegHandler.executeFFmpegCmds(cmdList);
        }
    }

    private void convertGifInHighQuality(String gifPath, String videoPath, int startTime, int duration, int frameRate) {
        new Thread(() -> {
            mHandler.sendEmptyMessage(MSG_BEGIN);
            int width=0, height=0;
            int rotateDegree = 0;
            try {
                MediaMetadataRetriever retriever = new MediaMetadataRetriever();
                retriever.setDataSource(videoPath);
                String mWidth = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH);
                String mHeight = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT);
                width = Integer.valueOf(mWidth);
                height = Integer.valueOf(mHeight);
                String rotate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION);
                rotateDegree = Integer.valueOf(rotate);
                retriever.release();
                Log.e(TAG, "retrieve width=" + width + "--height=" + height + "--rotate=" + rotate);
            } catch (Exception e) {
                Log.e(TAG, "retrieve error=" + e.toString());
            }
            long start = System.currentTimeMillis();
            HighQualityGif highQualityGif = new HighQualityGif(width, height, rotateDegree);
            boolean result = highQualityGif.convertGIF(gifPath, videoPath, startTime, duration, frameRate);
            Log.e(TAG, "convert gif result=" + result + "--time=" + (System.currentTimeMillis()-start));
            mHandler.sendEmptyMessage(MSG_FINISH);
        }).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }
}
