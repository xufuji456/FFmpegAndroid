package com.frank.ffmpeg.util;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * bitmap tool
 * Created by frank on 2018/1/24.
 */

public class BitmapUtil {

    private final static int TEXT_SIZE = 16;
    private final static int TEXT_COLOR = Color.RED;

    /**
     * convert text to bitmap
     *
     * @param text text
     * @return bitmap of teh text
     */
    private static Bitmap textToBitmap(String text) {
        Paint paint = new Paint();
        paint.setTextSize(TEXT_SIZE);
        paint.setTextAlign(Paint.Align.LEFT);
        paint.setColor(TEXT_COLOR);
        paint.setDither(true);
        paint.setAntiAlias(true);
        Paint.FontMetricsInt fm = paint.getFontMetricsInt();
        int width = (int)paint.measureText(text);
        int height = fm.descent - fm.ascent;

        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        canvas.drawText(text, 0, fm.leading - fm.ascent, paint);
        canvas.save();
        return bitmap;
    }

    /**
     * convert text to picture
     *
     * @param filePath filePath
     * @param text     text
     * @return result of generating picture
     */
    public static boolean textToPicture(String filePath, String text) {
        Bitmap bitmap = textToBitmap(text);
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(filePath);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, outputStream);
            outputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                if (outputStream != null) {
                    outputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return true;
    }

    /**
     * delete file
     *
     * @param filePath filePath
     * @return result of deletion
     */
    public static boolean deleteTextFile(String filePath) {
        File file = new File(filePath);
        return file.exists() && file.delete();
    }

}
