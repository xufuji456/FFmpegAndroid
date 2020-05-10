package com.frank.ffmpeg.util;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.WindowManager;

public class ScreenUtil {

    private static DisplayMetrics getDisplayMetrics(Context context) {
        WindowManager windowManager = (WindowManager) context.getApplicationContext().getSystemService(Context.WINDOW_SERVICE);
        if (windowManager == null) {
            return null;
        }
        DisplayMetrics displayMetrics = new DisplayMetrics();
        windowManager.getDefaultDisplay().getMetrics(displayMetrics);
        return displayMetrics;
    }

    public static int getScreenWidth(Context context) {
        if (context == null) {
            return 0;
        }
        DisplayMetrics displayMetrics = getDisplayMetrics(context);
        return displayMetrics != null ? displayMetrics.widthPixels : 0;
    }

    public static int getScreenHeight(Context context) {
        if (context == null) {
            return 0;
        }
        DisplayMetrics displayMetrics = getDisplayMetrics(context);
        return displayMetrics != null ? displayMetrics.heightPixels : 0;
    }

    public static int dp2px(Context context, int dpValue) {
        if (context == null) {
            return 0;
        }
        DisplayMetrics displayMetrics = getDisplayMetrics(context);
        float density = displayMetrics != null ? displayMetrics.density : 0;
        return (int) (dpValue * density + 0.5f);
    }

    public static int px2dp(Context context, int pxValue) {
        if (context == null) {
            return 0;
        }
        DisplayMetrics displayMetrics = getDisplayMetrics(context);
        float density = displayMetrics != null ? displayMetrics.density : 0;
        return (int) (pxValue / density + 0.5f);
    }

}
