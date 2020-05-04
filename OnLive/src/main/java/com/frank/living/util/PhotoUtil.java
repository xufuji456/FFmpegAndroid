package com.frank.living.util;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Looper;
import android.text.TextUtils;
import android.widget.Toast;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * the tool of photo
 * Created by frank on 2019/12/31
 */

public class PhotoUtil {

    public static void savePhoto(Bitmap bitmap, String path, Context context) {
        savePhoto(bitmap, path, context, 100);
    }

    public static void savePhoto(Bitmap bitmap, String path, Context context, int quality) {
        if (bitmap == null || TextUtils.isEmpty(path) || context == null) {
            return;
        }
        if (quality <= 0 || quality > 100) {
            quality = 100;
        }
        FileOutputStream fileOutputStream = null;
        try {
            fileOutputStream = new FileOutputStream(path);
            bitmap.compress(Bitmap.CompressFormat.JPEG, quality, fileOutputStream);
            fileOutputStream.flush();
            if (Looper.myLooper() == Looper.getMainLooper()) {
                Toast.makeText(context.getApplicationContext(), "save success:" + path, Toast.LENGTH_SHORT).show();
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

}
