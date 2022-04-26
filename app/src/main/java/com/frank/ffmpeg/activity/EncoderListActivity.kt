package com.frank.ffmpeg.activity

import android.media.MediaCodecList
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.frank.ffmpeg.R


class EncoderListActivity : AppCompatActivity() {

    val TAG = "EncoderListActivity"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_encoder_list)
        getEncodeList()
    }

    private fun getEncodeList(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            val list = MediaCodecList(MediaCodecList.REGULAR_CODECS)
            val supportCodes = list.codecInfos
            Log.i(TAG, "解码器列表：")
            for (codec in supportCodes) {
                if (!codec.isEncoder) {
                    val name = codec.name
                    if (name.startsWith("OMX.google")) {
                        Log.i(TAG, "软解->$name")
                    }
                }
            }
            for (codec in supportCodes) {
                if (!codec.isEncoder) {
                    val name = codec.name
                    if (!name.startsWith("OMX.google")) {
                        Log.i(TAG, "硬解->$name")
                    }
                }
            }
            Log.i(TAG, "编码器列表：")
            for (codec in supportCodes) {
                if (codec.isEncoder) {
                    val name = codec.name
                    if (name.startsWith("OMX.google")) {
                        Log.i(TAG, "软编->$name")
                    }
                }
            }
            for (codec in supportCodes) {
                if (codec.isEncoder) {
                    val name = codec.name
                    if (!name.startsWith("OMX.google")) {
                        Log.i(TAG, "硬编->$name")
                    }
                }
            }
        }

    }
}