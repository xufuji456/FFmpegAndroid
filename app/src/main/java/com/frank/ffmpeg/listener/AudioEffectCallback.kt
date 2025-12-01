package com.frank.ffmpeg.listener

import android.util.Pair
import android.widget.SeekBar
import java.util.ArrayList

/**
 * The callback of AudioEffect
 *
 * @author frank
 * @date 2022/3/23
 */
interface AudioEffectCallback {

    fun getSeekBarList(): List<SeekBar>?

    fun setEqualizerList(maxProgress: Int, equalizerList: ArrayList<Pair<*, *>>)

    fun onFFTDataCallback(fft: ByteArray?)

}