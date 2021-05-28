package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.media.MediaPlayer
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.text.TextUtils
import android.util.Log
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import com.frank.ffmpeg.R
import com.frank.ffmpeg.util.TimeUtil

class AudioPlayActivity : AppCompatActivity() {

    companion object {
        private val TAG = AudioPlayActivity::class.java.simpleName

        private const val MSG_TIME = 123
        private const val MSG_DURATION = 234
    }

    private var txtTitle: TextView? = null
    private var txtArtist: TextView? = null
    private var txtTime: TextView? = null
    private var txtDuration: TextView? = null
    private var audioBar: SeekBar? = null

    private lateinit var audioPlayer:MediaPlayer

    private val mHandler: Handler = @SuppressLint("HandlerLeak")
    object : Handler() {
        override fun handleMessage(msg: Message?) {
            super.handleMessage(msg)
            when (msg?.what) {
                MSG_TIME -> {
                    audioBar?.progress = audioPlayer.currentPosition
                    txtTime?.text = TimeUtil.getVideoTime(audioPlayer.currentPosition.toLong())
                    sendEmptyMessageDelayed(MSG_TIME, 1000)
                }
                MSG_DURATION -> {
                    val duration = msg.obj as Int
                    txtDuration?.text = TimeUtil.getVideoTime(duration.toLong())
                    audioBar?.max = duration
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_audio_play)

        initView()
        initAudioPlayer()
    }

    private fun initView() {
        txtTitle = findViewById(R.id.txt_title)
        txtArtist = findViewById(R.id.txt_artist)
        txtTime = findViewById(R.id.txt_time)
        txtDuration = findViewById(R.id.txt_duration)
        val btnPlay: ImageView = findViewById(R.id.img_play)
        btnPlay.setOnClickListener {
            if (isPlaying()) {
                audioPlayer.pause()
                btnPlay.setImageResource(R.drawable.ic_play)
            } else {
                audioPlayer.start()
                btnPlay.setImageResource(R.drawable.ic_pause)
            }
        }
        audioBar = findViewById(R.id.audio_bar)
        audioBar?.setOnSeekBarChangeListener(object :SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (!fromUser) return
                audioBar?.progress = progress
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                audioPlayer.seekTo(audioBar?.progress!!)
            }
        })
    }

    private fun initAudioPlayer() {
        val path = intent.data?.path
        Log.e(TAG, "path=$path")
        if (TextUtils.isEmpty(path)) return
        audioPlayer = MediaPlayer()
        audioPlayer.setDataSource(path)
        audioPlayer.prepareAsync()
        audioPlayer.setOnPreparedListener {
            Log.e(TAG, "onPrepared...")
            audioPlayer.start()
            val duration = audioPlayer.duration
            mHandler.obtainMessage(MSG_TIME).sendToTarget()
            mHandler.obtainMessage(MSG_DURATION, duration).sendToTarget()
        }
    }

    private fun isPlaying() :Boolean {
        return audioPlayer.isPlaying
    }

    override fun onStop() {
        super.onStop()
        audioPlayer.stop()
        audioPlayer.release()
    }

}
