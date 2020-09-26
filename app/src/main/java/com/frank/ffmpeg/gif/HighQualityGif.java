package com.frank.ffmpeg.gif;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * @author frank
 * @date 2020-09-25 20:11
 * @desc Extract frames, and convert to GIF
 */

public class HighQualityGif {

    private final static String TAG = HighQualityGif.class.getSimpleName();

    private final static int TARGET_WIDTH = 320;

    private final static int TARGET_HEIGHT = 180;

    private int mWidth;
    private int mHeight;
    private int mRotateDegree;

    public HighQualityGif(int width, int height, int rotateDegree) {
        mWidth = width;
        mHeight = height;
        mRotateDegree = rotateDegree;
    }

    private int chooseWidth(int width, int height) {
        if (width <= 0 || height <= 0) {
            return TARGET_WIDTH;
        }
        int target;
        if (mRotateDegree == 0 || mRotateDegree == 180) {//landscape
            if (width > TARGET_WIDTH) {
                target = TARGET_WIDTH;
            } else {
                target = width;
            }
        } else {//portrait
            if (height > TARGET_HEIGHT) {
                target = TARGET_HEIGHT;
            } else {
                target = height;
            }
        }
        return target;
    }

    private byte[] generateGif(String filePath, int startTime, int duration, int frameRate) throws IllegalArgumentException {
        if (TextUtils.isEmpty(filePath)) {
            return null;
        }
        String folderPath = Environment.getExternalStorageDirectory() + "/gif_frames/";
        FileUtil.deleteFolder(folderPath);
        int targetWidth = chooseWidth(mWidth, mHeight);
        String[] commandLine = FFmpegUtil.videoToImageWithScale(filePath, startTime, duration, frameRate, targetWidth, folderPath);
        FFmpegCmd.executeSync(commandLine);
        File fileFolder = new File(folderPath);
        if (!fileFolder.exists() || fileFolder.listFiles() == null) {
            return null;
        }
        File[] files = fileFolder.listFiles();

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        BeautyGifEncoder gifEncoder = new BeautyGifEncoder();
        gifEncoder.setFrameRate(10);
        gifEncoder.start(outputStream);

        for (File file:files) {
            Bitmap bitmap = BitmapFactory.decodeFile(file.getAbsolutePath());
            if (bitmap != null) {
                gifEncoder.addFrame(bitmap);
            }
        }
        gifEncoder.finish();
        return outputStream.toByteArray();
    }

    private boolean saveGif(byte[] data, String gifPath) {
        if (data == null || data.length == 0 || TextUtils.isEmpty(gifPath)) {
            return false;
        }
        boolean result = true;
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(gifPath);
            outputStream.write(data);
            outputStream.flush();
        } catch (IOException e) {
            result = false;
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return result;
    }

    /**
     * convert video into GIF
     * @param gifPath gifPath
     * @param filePath filePath
     * @param startTime where starts from the video(Unit: second)
     * @param duration how long you want to convert(Unit: second)
     * @param frameRate how much frames you want in a second
     * @return convert GIF success or not
     */
    public boolean convertGIF(String gifPath, String filePath, int startTime, int duration, int frameRate) {
        byte[] data;
        try {
            data = generateGif(filePath, startTime, duration, frameRate);
        } catch (IllegalArgumentException | OutOfMemoryError e) {
            Log.e(TAG, "generateGif error=" + e.toString());
            return false;
        }
        return saveGif(data, gifPath);
    }

}
