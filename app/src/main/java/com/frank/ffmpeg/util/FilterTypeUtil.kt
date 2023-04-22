package com.frank.ffmpeg.util

import com.frank.camerafilter.factory.BeautyFilterType
import com.frank.ffmpeg.R

/**
 * @author xufulong
 * @date 2022/10/17 5:39 下午
 * @desc
 */
object FilterTypeUtil {

    fun filterTypeToNameId(type: BeautyFilterType): Int {
        return when (type) {
            BeautyFilterType.NONE -> R.string.camera_filter_none
            BeautyFilterType.BRIGHTNESS -> R.string.camera_filter_brightness
            BeautyFilterType.SATURATION -> R.string.camera_filter_saturation
            BeautyFilterType.CONTRAST -> R.string.camera_filter_contrast
            BeautyFilterType.SHARPEN -> R.string.camera_filter_sharpen
            BeautyFilterType.BLUR -> R.string.camera_filter_blur
            BeautyFilterType.HUE -> R.string.camera_filter_hue
            BeautyFilterType.WHITE_BALANCE -> R.string.camera_filter_balance
            BeautyFilterType.SKETCH -> R.string.camera_filter_sketch
            BeautyFilterType.OVERLAY -> R.string.camera_filter_overlay
            BeautyFilterType.BREATH_CIRCLE -> R.string.camera_filter_circle
            else -> R.string.camera_filter_none
        }
    }

}