package com.frank.androidmedia.controller

import android.media.MediaPlayer
import android.view.Surface
import com.frank.androidmedia.listener.PlayerCallback
import java.io.IOException

/**
 * @author xufulong
 * @date 3/18/22 1:53 PM
 * @desc The controller of MediaPlayer
 */
open class MediaPlayerController(playerCallback: PlayerCallback) {

    private var mediaPlayer: MediaPlayer? = null
    private var renderFirstFrame: Boolean = false
    private var playerCallback: PlayerCallback? = null

    init {
        this.playerCallback = playerCallback
    }

    fun initPlayer(filePath :String, surface :Surface) {
        if (mediaPlayer != null) {
            releasePlayer()
        }
        try {
            renderFirstFrame = false
            mediaPlayer = MediaPlayer()
            mediaPlayer!!.setOnPreparedListener {
                mediaPlayer!!.start()
                playerCallback?.onPrepare()
            }
            mediaPlayer!!.setOnErrorListener {
                mp: MediaPlayer?, what: Int, extra: Int ->
                return@setOnErrorListener playerCallback?.onError(what, extra)!!
            }
            mediaPlayer!!.setOnCompletionListener {
                playerCallback?.onCompleteListener()
            }
            mediaPlayer!!.setOnInfoListener {
                mp, what, extra ->  (
                if (what == MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
                    if (!renderFirstFrame) {
                        renderFirstFrame = true
                        playerCallback?.onRenderFirstFrame()
                    }
                })
                return@setOnInfoListener true
            }
            mediaPlayer!!.setDataSource(filePath)
            mediaPlayer!!.setSurface(surface)
            mediaPlayer!!.prepareAsync()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }

    fun currentPosition() : Int {
        if (mediaPlayer == null)
            return 0
        return mediaPlayer!!.currentPosition
    }

    fun duration() : Int {
        if (mediaPlayer == null)
            return 0
        return mediaPlayer!!.duration
    }

    fun seekTo(position :Int) {
        mediaPlayer?.seekTo(position)
    }

    fun togglePlay() {
        if (mediaPlayer == null)
            return

        if (mediaPlayer!!.isPlaying) {
            mediaPlayer!!.pause()
        } else {
            mediaPlayer!!.start()
        }
    }

    fun releasePlayer() {
        if (mediaPlayer != null) {
            mediaPlayer!!.stop()
            mediaPlayer!!.release()
            mediaPlayer = null
        }
    }

}