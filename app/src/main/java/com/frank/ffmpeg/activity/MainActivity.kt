package com.frank.ffmpeg.activity

import android.content.Intent
import android.os.Bundle
import android.view.View

import com.frank.ffmpeg.R

/**
 * The main entrance of all Activity
 * Created by frank on 2018/1/23.
 */
class MainActivity : BaseActivity() {

    override val layoutId: Int
        get() = R.layout.activity_main

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initViewsWithClick(
                R.id.btn_audio,
                R.id.btn_video,
                R.id.btn_media,
                R.id.btn_play,
                R.id.btn_push,
                R.id.btn_live,
                R.id.btn_filter,
                R.id.btn_preview,
                R.id.btn_probe,
                R.id.btn_audio_effect
        )
    }

    override fun onViewClick(view: View) {
        val intent = Intent()
        when (view.id) {
            R.id.btn_audio//handle audio
            -> intent.setClass(this@MainActivity, AudioHandleActivity::class.java)
            R.id.btn_video//handle video
            -> intent.setClass(this@MainActivity, VideoHandleActivity::class.java)
            R.id.btn_media//handle media
            -> intent.setClass(this@MainActivity, MediaHandleActivity::class.java)
            R.id.btn_play//media play
            -> intent.setClass(this@MainActivity, MediaPlayerActivity::class.java)
            R.id.btn_push//pushing
            -> intent.setClass(this@MainActivity, PushActivity::class.java)
            R.id.btn_live//realtime living with rtmp stream
            -> intent.setClass(this@MainActivity, LiveActivity::class.java)
            R.id.btn_filter//filter effect
            -> intent.setClass(this@MainActivity, FilterActivity::class.java)
            R.id.btn_preview//preview thumbnail
            -> intent.setClass(this@MainActivity, VideoPreviewActivity::class.java)
            R.id.btn_probe//probe media format
            -> intent.setClass(this@MainActivity, ProbeFormatActivity::class.java)
            R.id.btn_audio_effect//audio effect
            -> intent.setClass(this@MainActivity, AudioEffectActivity::class.java)
            else -> {
            }
        }
        startActivity(intent)
    }

    override fun onSelectedFile(filePath: String) {

    }

}
