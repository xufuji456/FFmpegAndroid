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
            BeautyFilterType.CRAYON -> R.string.camera_filter_crayon
            BeautyFilterType.SKETCH -> R.string.camera_filter_sketch
            else -> R.string.camera_filter_none
        }
    }

}