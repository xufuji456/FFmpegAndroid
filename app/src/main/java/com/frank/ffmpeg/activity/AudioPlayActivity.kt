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
import com.frank.ffmpeg.handler.FFmpegHandler
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH
import com.frank.ffmpeg.listener.OnLrcListener
import com.frank.ffmpeg.model.MediaBean
import com.frank.ffmpeg.util.FFmpegUtil
import com.frank.ffmpeg.util.TimeUtil
import com.frank.ffmpeg.model.LrcLine
import com.frank.ffmpeg.tool.LrcLineTool
import com.frank.ffmpeg.view.LrcView

class AudioPlayActivity : AppCompatActivity() {

    companion object {
        private val TAG = AudioPlayActivity::class.java.simpleName

        private const val MSG_TIME = 123
        private const val MSG_DURATION = 234
    }

    private var path: String? = null

    private var txtTitle: TextView? = null
    private var txtArtist: TextView? = null
    private var txtTime: TextView? = null
    private var txtDuration: TextView? = null
    private var audioBar: SeekBar? = null
    private var lrcView: LrcView? = null

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
                    lrcView?.seekToTime(audioPlayer.currentPosition.toLong())
                }
                MSG_DURATION -> {
                    val duration = msg.obj as Int
                    txtDuration?.text = TimeUtil.getVideoTime(duration.toLong())
                    audioBar?.max = duration
                }
                MSG_FINISH -> {
                    val result = msg.obj as MediaBean
                    txtTitle?.text = result.audioBean?.title
                    txtArtist?.text = result.audioBean?.artist
                    val lyrics = result.audioBean?.lyrics
                    if (lyrics != null) {
                        val lrcList = arrayListOf<LrcLine>()
                        for (i in lyrics.indices) {
                            Log.e(TAG, "lyrics=_=" + lyrics[i])
                            val line = LrcLineTool.getLrcLine(lyrics[i])
                            if (line != null) lrcList.addAll(line)
                        }
                        LrcLineTool.sortLyrics(lrcList)
                        lrcView?.setLrc(lrcList)
                    }
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_audio_play)

        initView()
        initAudioPlayer()
        initLrc()
    }

    private fun initView() {
        txtTitle = findViewById(R.id.txt_title)
        txtArtist = findViewById(R.id.txt_artist)
        txtTime = findViewById(R.id.txt_time)
        txtDuration = findViewById(R.id.txt_duration)
        lrcView = findViewById(R.id.list_lyrics)
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
                val progress = audioBar?.progress ?: 0
                audioPlayer.seekTo(progress)
                lrcView?.seekToTime(progress.toLong())
            }
        })
        lrcView?.setListener(object :OnLrcListener {
            override fun onLrcSeek(position: Int, lrcLine: LrcLine) {
                audioPlayer.seekTo(lrcLine.startTime.toInt())
                Log.e(TAG, "lrc position=$position--time=${lrcLine.startTime}")
            }
        })
    }

    private fun initAudioPlayer() {
        path = intent.data?.path
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

    private fun initLrc() {
        if (path.isNullOrEmpty()) return
        val ffmpegHandler = FFmpegHandler(mHandler)
        val commandLine = FFmpegUtil.probeFormat(path)
        ffmpegHandler.executeFFprobeCmd(commandLine)
    }

    private fun isPlaying() :Boolean {
        return audioPlayer.isPlaying
    }

    override fun onStop() {
        super.onStop()
        try {
            audioPlayer.stop()
            audioPlayer.release()
        } catch (e: Exception) {
            Log.e(TAG, "release player err=$e")
        }
    }

}
