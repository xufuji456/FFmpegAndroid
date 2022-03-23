package com.frank.androidmedia.controller

import android.media.audiofx.AcousticEchoCanceler
import android.media.audiofx.AudioEffect
import android.media.audiofx.AutomaticGainControl
import android.media.audiofx.NoiseSuppressor
import android.util.Log
import java.lang.Exception

/**
 *
 * @author frank
 * @date 2022/3/23
 */
open class AudioProcessController {

    /*************************************************************
                               AudioEffect
                                    |
                ____________________|___________________
               |                    |                   |
       AcousticEchoCanceler  AutomaticGainControl  NoiseSuppressor

     **************************************************************/

    companion object {
        val TAG: String = AudioProcessController::class.java.simpleName
    }

    private var noiseSuppressor: NoiseSuppressor? = null
    private var automaticGainControl: AutomaticGainControl? = null
    private var acousticEchoCanceler: AcousticEchoCanceler? = null

    fun initAEC(audioSessionId: Int): Boolean {
        if (!AcousticEchoCanceler.isAvailable()) {
            Log.e(TAG, "AEC not available...")
            return false
        }
        try {
            acousticEchoCanceler = AcousticEchoCanceler.create(audioSessionId)
        } catch (e: Exception) {
            Log.e(TAG, "init AcousticEchoCanceler error=$e")
            return false
        }
        val result = acousticEchoCanceler?.setEnabled(true)
        if (result != AudioEffect.SUCCESS) {
            acousticEchoCanceler?.release()
            acousticEchoCanceler = null
            return false
        }
        return true
    }

    fun initAGC(audioSessionId: Int): Boolean {
        if (!AutomaticGainControl.isAvailable()) {
            Log.e(TAG, "AGC not available...")
            return false
        }
        try {
            automaticGainControl = AutomaticGainControl.create(audioSessionId)
        } catch (e: Exception) {
            Log.e(TAG, "init AutomaticGainControl error=$e")
            return false
        }
        val result = automaticGainControl?.setEnabled(true)
        if (result != AudioEffect.SUCCESS) {
            automaticGainControl?.release()
            automaticGainControl = null
            return false
        }
        return true
    }

    fun initNS(audioSessionId: Int): Boolean {
        if (!NoiseSuppressor.isAvailable()) {
            Log.e(TAG, "NS not available...")
            return false
        }
        try {
            noiseSuppressor = NoiseSuppressor.create(audioSessionId)
        } catch (e: Exception) {
            Log.e(TAG, "init NoiseSuppressor error=$e")
            return false
        }
        val result = noiseSuppressor?.setEnabled(true)
        if (result != AudioEffect.SUCCESS) {
            noiseSuppressor?.release()
            noiseSuppressor = null
            return false
        }
        return true
    }

    fun release() {
        noiseSuppressor?.release()
        acousticEchoCanceler?.release()
        automaticGainControl?.release()
    }

}