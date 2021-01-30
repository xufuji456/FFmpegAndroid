package com.frank.ffmpeg.util

import android.content.Context
import android.util.DisplayMetrics
import android.view.WindowManager

object ScreenUtil {

    private fun getDisplayMetrics(context: Context): DisplayMetrics? {
        val windowManager = context.applicationContext.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val displayMetrics = DisplayMetrics()
        windowManager.defaultDisplay.getMetrics(displayMetrics)
        return displayMetrics
    }

    fun getScreenWidth(context: Context?): Int {
        if (context == null) {
            return 0
        }
        val displayMetrics = getDisplayMetrics(context)
        return displayMetrics?.widthPixels ?: 0
    }

    fun getScreenHeight(context: Context?): Int {
        if (context == null) {
            return 0
        }
        val displayMetrics = getDisplayMetrics(context)
        return displayMetrics?.heightPixels ?: 0
    }

    fun dp2px(context: Context?, dpValue: Int): Int {
        if (context == null) {
            return 0
        }
        val displayMetrics = getDisplayMetrics(context)
        val density: Float = displayMetrics?.density ?: 0F
        return (dpValue * density + 0.5f).toInt()
    }

    fun px2dp(context: Context?, pxValue: Int): Int {
        if (context == null) {
            return 0
        }
        val displayMetrics = getDisplayMetrics(context)
        val density: Float = displayMetrics?.density ?: 0F
        return (pxValue / density + 0.5f).toInt()
    }

}
