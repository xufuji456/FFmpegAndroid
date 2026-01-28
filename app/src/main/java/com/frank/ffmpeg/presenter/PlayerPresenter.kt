package com.frank.ffmpeg.presenter

import android.annotation.SuppressLint
import android.app.Activity
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.ScaleGestureDetector
import android.view.View
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.RelativeLayout
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import com.frank.next.player.IPlayer
import com.frank.ffmpeg.R
import com.frank.ffmpeg.util.PlayerUtil
import com.frank.ffmpeg.util.TimeUtil
import com.frank.ffmpeg.view.PlayerView
import kotlin.math.max
import kotlin.math.min
import androidx.core.view.isVisible

/**
 * Note: Presenter of player
 * Date: 2026/1/28 08:08
 * Author: frank
 */
class PlayerPresenter {

    companion object {
        const val TAG = "PlayerPresenter"
    }

    // UI
    private lateinit var backView:          ImageView
    private lateinit var textPosition:      TextView
    private lateinit var textDuration:      TextView
    private lateinit var progressSeekBar:   SeekBar
    private lateinit var switchDirection:   ImageView
    private lateinit var playPauseButton:   ImageView
    private lateinit var videoPlayerView:   PlayerView
    private lateinit var videoTopLayout:    RelativeLayout
    private lateinit var videoFrameLayout:  RelativeLayout
    private lateinit var videoBottomLayout: RelativeLayout

    // debug information
    private lateinit var debugInfoLayout:    LinearLayout
    private lateinit var debugInfoNameView:  TextView
    private lateinit var debugContentLayout: View
    private lateinit var debugInfoValueView: TextView

    // scale
    private var scaleFactor    = 1.0f
    private var oldScaleFactor = 1.0f

    private lateinit var mContext:  Activity
    private lateinit var mListener: OnPresenterListener

    interface OnPresenterListener {

        fun onNextUrl(): String

        fun onSwitchOrientation()

    }

    fun initView(view: View, context: Activity) {
        mContext = context

        initDebugInfoView(view)
        initVideoViewScale(view)
        initVideoProgressView(view)
        initVideoControllerView(view)
    }

    fun setOnPresenterListener(listener: OnPresenterListener) {
        mListener = listener
    }

    fun initPlayer(url: String) {
        videoPlayerView.setVideoPath(url)
    }

    fun getPlayerView(): PlayerView {
        return videoPlayerView
    }

    private fun initVideoControllerView(view: View) {
        backView          = view.findViewById(R.id.img_player_back)
        textPosition      = view.findViewById(R.id.tv_player_position)
        textDuration      = view.findViewById(R.id.tv_player_duration)
        videoTopLayout    = view.findViewById(R.id.layout_player_top)
        playPauseButton   = view.findViewById(R.id.img_player_pause)
        switchDirection   = view.findViewById(R.id.img_screen_switch)
        videoPlayerView   = view.findViewById(R.id.view_player_controller)
        videoBottomLayout = view.findViewById(R.id.layout_player_bottom)

        backView.setOnClickListener {
            mContext.finish()
        }
        switchDirection.setOnClickListener {
            mListener.onSwitchOrientation()
        }
        playPauseButton.setOnClickListener {
            if (videoPlayerView.isPlaying()) {
                videoPlayerView.pause()
                playPauseButton.setBackgroundResource(R.drawable.ic_play)
            } else {
                videoPlayerView.start()
                playPauseButton.setBackgroundResource(R.drawable.ic_pause)
            }
        }

        registerPlayerListener()
    }

    private fun initVideoProgressView(view: View) {
        progressSeekBar = view.findViewById(R.id.seekbar_player_progress)
        progressSeekBar.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onStartTrackingTouch(p0: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                videoPlayerView.seekTo(seekBar!!.progress)
            }

            override fun onProgressChanged(
                seekBar: SeekBar?,
                progress: Int,
                fromUser: Boolean
            ) {
                if (!fromUser)
                    return
                progressSeekBar.progress = progress
            }
        })

        val handler = Handler(Looper.getMainLooper())
        val updateSeekBar = object : Runnable {
            override fun run() {
                val position = videoPlayerView.getCurrentPosition()
                if (position > 0) {
                    progressSeekBar.progress = position
                }
                textPosition.text = TimeUtil.getVideoTime(position.toLong())
                videoPlayerView.getPlayer()?.let { updateDebugInfo(it) }
                handler.postDelayed(this, 1000)
            }
        }
        handler.post(updateSeekBar)
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun initVideoViewScale(view: View) {
        videoFrameLayout = view.findViewById(R.id.layout_player_root)
        val scaleGestureDetector = ScaleGestureDetector(
            mContext,
            object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    oldScaleFactor = scaleFactor
                    scaleFactor *= detector.scaleFactor
                    scaleFactor = max(1.0f, min(scaleFactor, 5.0f))
                    videoPlayerView.setViewScale(scaleFactor)
                    return true
                }
            })

        videoFrameLayout.setOnTouchListener { _, event ->
            scaleGestureDetector.onTouchEvent(event)
            true
        }
    }

    private fun initDebugInfoView(view: View) {
        debugInfoLayout    = view.findViewById(R.id.layout_realtime_info)
        debugInfoNameView  = view.findViewById(R.id.tv_info_name)
        debugInfoValueView = view.findViewById(R.id.tv_info_value)
        debugContentLayout = view.findViewById(R.id.layout_player_info)
        debugContentLayout.visibility = View.VISIBLE
        debugContentLayout.setOnClickListener {
            debugInfoLayout.visibility = if (debugInfoLayout.isVisible)
                View.INVISIBLE
            else
                View.VISIBLE
        }
    }

    private fun updateDebugInfo(mediaPlayer: IPlayer) {
        val debugInfo = PlayerUtil.getDebugInfo(mediaPlayer, videoPlayerView.getRenderViewType())
        debugInfoNameView.text  = debugInfo.first
        debugInfoValueView.text = debugInfo.second
    }

    private fun registerPlayerListener() {
        videoPlayerView.setOnPreparedListener { mp ->
            Log.i(TAG, "onPrepared, duration=" + mp.duration)
        }
        videoPlayerView.setOnInfoListener { what, extra ->
            handleVideoInfoEvent(what, extra)
        }
        videoPlayerView.setOnPlayingListener {playing ->
            if (playing) {
                playPauseButton.setBackgroundResource(R.drawable.ic_pause)
            } else {
                playPauseButton.setBackgroundResource(R.drawable.ic_play)
            }
        }
        videoPlayerView.setOnVideoSizeChangedListener { width, height ->
            Log.i(TAG, "onVideoSizeChanged, width=$width, height=$height")
        }
        videoPlayerView.setOnErrorListener { what, extra ->
            run {
                Log.i(TAG, "onError, what=$what, extra=$extra")
                return@run true
            }
        }
        videoPlayerView.setOnSeekCompleteListener {
            Log.i(TAG, "onSeekComplete...")
        }
        videoPlayerView.setOnCompleteListener {
            playPauseButton.setBackgroundResource(R.drawable.ic_play)
            handleVideoPlayCompleteEvent()
        }
    }

    private fun handleVideoPlayCompleteEvent() {
        videoPlayerView.seekTo(0)
        val autoPlayNext = true
        val autoLoop = false
        when {
            autoPlayNext -> {
                val nextUrl = mListener.onNextUrl()
                videoPlayerView.switchVideo(nextUrl)
            }

            autoLoop -> {
                videoPlayerView.start()
            }
        }
    }

    private fun handleVideoInfoEvent(what: Int, extra: Int): Boolean {
        when (what) {
            IPlayer.MSG_OPEN_INPUT -> {

            }
            IPlayer.MSG_FIND_STREAM_INFO -> {

            }
            IPlayer.MSG_VIDEO_FIRST_PACKET -> {

            }
            IPlayer.MSG_VIDEO_DECODE_START -> {

            }
            IPlayer.MSG_VIDEO_RENDER_START -> {
                textDuration.text = TimeUtil.getVideoTime(videoPlayerView.getDuration().toLong())
                progressSeekBar.max = videoPlayerView.getDuration()
            }
        }
        return true
    }

}
