package com.frank.ffmpeg.view

import android.content.Context
import android.media.AudioManager
import android.media.MediaPlayer
import android.util.AttributeSet
import android.util.Log
import android.view.Gravity
import android.widget.FrameLayout
import com.frank.ffmpeg.FFmpegApplication
import com.frank.next.player.IPlayer
import com.frank.next.player.NextPlayer
import com.frank.next.renderview.IRenderView
import com.frank.next.renderview.IRenderView.IRenderCallback
import com.frank.next.renderview.IRenderView.ISurfaceHolder
import com.frank.next.renderview.SurfaceRenderView
import com.frank.next.renderview.TextureRenderView

/**
 * Note: View Controller layer of player
 * Date: 2026/1/27 20:20
 * Author: frank
 */
class PlayerView : FrameLayout {

    companion object {
        private const val TAG = "PlayerView"

        private const val STATE_ERROR     = -1
        private const val STATE_IDLE      = 0
        private const val STATE_PREPARING = 1
        private const val STATE_PREPARED  = 2
        private const val STATE_PLAYING   = 3
        private const val STATE_PAUSED    = 4
        private const val STATE_COMPLETED = 5

        const val RENDER_TYPE_SURFACE_VIEW = 1
        const val RENDER_TYPE_TEXTURE_VIEW = 2

        private val scaleModeList = intArrayOf(
            IRenderView.RENDER_MODE_ASPECT_FIT,
            IRenderView.RENDER_MODE_ASPECT_FILL,
            IRenderView.RENDER_MODE_WRAP,
            IRenderView.RENDER_MODE_16_9,
            IRenderView.RENDER_MODE_4_3
        )

    }

    private var mVideoWidth       = 0
    private var mVideoHeight      = 0
    private var mSurfaceWidth     = 0
    private var mSurfaceHeight    = 0
    private var mSeekWhenPrepared = 0

    private var mUrl: String? = null
    private var mCurrentState = STATE_IDLE
    private var mAppContext: Context? = null
    private var mRenderView: IRenderView? = null
    private var mStudioPlayer: IPlayer? = null
    private var mSurfaceHolder: ISurfaceHolder? = null
    private val mCurrentAspectRatio = scaleModeList[0]

    private var mOnInfoListener: IPlayer.OnInfoListener? = null
    private var mOnErrorListener: IPlayer.OnErrorListener? = null
    private var mOnPlayingListener:  IPlayer.OnPlayingListener? = null
    private var mOnPreparedListener: IPlayer.OnPreparedListener? = null
    private var mOnCompleteListener: IPlayer.OnCompleteListener? = null
    private var mOnSeekCompleteListener: IPlayer.OnSeekCompleteListener? = null
    private var mOnBufferingUpdateListener: IPlayer.OnBufferUpdateListener? = null
    private var mOnVideoSizeChangedListener: IPlayer.OnVideoSizeChangedListener? = null

    constructor(context: Context) : super(context) {
        initVideoView(context)
    }

    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        initVideoView(context)
    }

    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int)
            : super(context, attrs, defStyleAttr) {
        initVideoView(context)
    }

    private fun initVideoView(context: Context) {
        mAppContext   = context.applicationContext
        mVideoWidth   = 0
        mVideoHeight  = 0
        mCurrentState = STATE_IDLE
        requestFocus()
        initRender()
    }

    private fun setRenderView(renderView: IRenderView?) {
        if (renderView == null)
            return
        if (mRenderView != null) {
            if (mStudioPlayer != null)
                mStudioPlayer!!.setDisplay(null)
            val renderUIView = mRenderView!!.view
            mRenderView!!.removeRenderCallback(mSurfaceCallback)
            mRenderView = null
            removeView(renderUIView)
        }
        mRenderView = renderView
        renderView.setAspectRatio(mCurrentAspectRatio)
        if (mVideoWidth > 0 && mVideoHeight > 0)
            renderView.setVideoSize(mVideoWidth, mVideoHeight)
        val lp = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, Gravity.CENTER)
        mRenderView!!.view.layoutParams = lp
        addView(mRenderView!!.view)
        mRenderView!!.addRenderCallback(mSurfaceCallback)
    }

    private fun initRender() {
        // use SurfaceView or TextureView
        if (FFmpegApplication.getInstance().enableSurfaceView()) {
            val renderView = SurfaceRenderView(context)
            setRenderView(renderView)
        } else {
            val renderView = TextureRenderView(context)
            if (mStudioPlayer != null) {
                renderView.surfaceHolder.bindPlayer(mStudioPlayer)
                renderView.setVideoSize(mStudioPlayer!!.videoWidth, mStudioPlayer!!.videoHeight)
                renderView.setVideoAspectRatio(
                    mStudioPlayer!!.videoSarNum,
                    mStudioPlayer!!.videoSarDen
                )
                renderView.setAspectRatio(mCurrentAspectRatio)
            }
            setRenderView(renderView)
        }
    }

    private fun bindSurfaceHolder(mp: IPlayer?, holder: ISurfaceHolder?) {
        if (mp == null)
            return
        if (holder == null) {
            mp.setDisplay(null)
            return
        }
        holder.bindPlayer(mp)
    }

    private var mSurfaceCallback: IRenderCallback = object : IRenderCallback {
        override fun onSurfaceCreated(holder: ISurfaceHolder, width: Int, height: Int) {
            mSurfaceHolder = holder
            if (mStudioPlayer != null) {
                bindSurfaceHolder(mStudioPlayer, holder)
            } else {
                openVideo()
            }
        }

        override fun onSurfaceChanged(holder: ISurfaceHolder, format: Int, w: Int, h: Int) {
            mSurfaceWidth  = w
            mSurfaceHeight = h
        }

        override fun onSurfaceDestroyed(holder: ISurfaceHolder) {
            mSurfaceHolder = null
            mStudioPlayer?.setDisplay(null)
        }
    }

    private fun createMediaPlayer(): IPlayer {
        val nextPlayer: IPlayer = NextPlayer()
        nextPlayer.setEnableMediaCodec(FFmpegApplication.getInstance().useMediaCodec())
        nextPlayer.setLiveMode(false)
        return nextPlayer
    }

    private fun openVideo() {
        if (mUrl!!.isEmpty() || mSurfaceHolder == null) {
            return
        }
        release()
        val am = mAppContext!!.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
        try {
            mStudioPlayer = createMediaPlayer()
            // set listener
            mStudioPlayer!!.setOnPreparedListener(mPreparedListener)
            mStudioPlayer!!.setOnVideoSizeChangedListener(mSizeChangedListener)
            mStudioPlayer!!.setOnCompletionListener(mCompleteListener)
            mStudioPlayer!!.setOnErrorListener(mErrorListener)
            mStudioPlayer!!.setOnInfoListener(mInfoListener)
            mStudioPlayer!!.setOnBufferingUpdateListener(mBufferingUpdateListener)
            mStudioPlayer!!.setOnSeekCompleteListener(mSeekCompleteListener)
            // set data source
            mStudioPlayer!!.dataSource = mUrl
            bindSurfaceHolder(mStudioPlayer, mSurfaceHolder)
            mStudioPlayer!!.setScreenOnWhilePlaying(true)
            // prepare
            mStudioPlayer!!.prepareAsync()
            mCurrentState = STATE_PREPARING
        } catch (e: Exception) {
            e.printStackTrace()
            mCurrentState = STATE_ERROR
            mErrorListener.onError(MediaPlayer.MEDIA_ERROR_UNKNOWN, 0)
        }
    }

    private fun isInPlaybackState(): Boolean {
        return (mStudioPlayer != null &&
                mCurrentState != STATE_ERROR &&
                mCurrentState != STATE_IDLE &&
                mCurrentState != STATE_PREPARING)
    }

    /******************************player listener***********************************/

    private val mPreparedListener: IPlayer.OnPreparedListener = object : IPlayer.OnPreparedListener {
        override fun onPrepared(mp: IPlayer) {
            mCurrentState = STATE_PREPARED
            mVideoWidth   = mp.videoWidth
            mVideoHeight  = mp.videoHeight
            mOnPreparedListener?.onPrepared(mStudioPlayer)
            if (mSeekWhenPrepared > 0) {
                seekTo(mSeekWhenPrepared)
                mSeekWhenPrepared = 0
            }
            if (mRenderView != null) {
                mRenderView!!.setVideoSize(mVideoWidth, mVideoHeight)
            }

            start()
        }
    }

    private val mSizeChangedListener = IPlayer.OnVideoSizeChangedListener { width, height ->
        mVideoWidth  = width
        mVideoHeight = height
        if (mVideoWidth != 0 && mVideoHeight != 0) {
            if (mRenderView != null) {
                mRenderView!!.setVideoSize(mVideoWidth, mVideoHeight)
            }
            requestLayout()
        }
        if (mOnVideoSizeChangedListener != null) {
            mOnVideoSizeChangedListener!!.onVideoSizeChanged(width, height)
        }
    }

    private val mInfoListener = IPlayer.OnInfoListener {arg1, arg2 ->
        mOnInfoListener?.onInfo(arg1, arg2)
        when (arg1) {
            IPlayer.MSG_AUDIO_RENDER_START -> Log.i(TAG, "onAudioRenderFirstFrame")
            IPlayer.MSG_VIDEO_RENDER_START -> Log.i(TAG, "onVideoRenderFirstFrame")
            IPlayer.MSG_BUFFER_START       -> Log.i(TAG, "onBufferStart")
            IPlayer.MSG_BUFFER_END         -> Log.i(TAG, "onBufferEnd")
            IPlayer.MSG_ROTATION_CHANGED   -> {
                Log.d(TAG, "onRotationChanged: $arg2")
                mRenderView?.setVideoRotation(arg2)
            }
        }
        true
    }

    private val mErrorListener: IPlayer.OnErrorListener = object : IPlayer.OnErrorListener {
        override fun onError(kernelError: Int, sdkError: Int): Boolean {
            Log.e(TAG, "Error: $kernelError, $sdkError")
            mCurrentState = STATE_ERROR
            mOnErrorListener?.onError(kernelError, sdkError)
            return true
        }
    }

    private val mBufferingUpdateListener = IPlayer.OnBufferUpdateListener { progress ->
        mOnBufferingUpdateListener?.onBufferUpdate(progress)
    }

    private val mSeekCompleteListener = IPlayer.OnSeekCompleteListener { mp: IPlayer? ->
        mOnSeekCompleteListener?.onSeekComplete(mp)
    }

    private val mCompleteListener: IPlayer.OnCompleteListener = object : IPlayer.OnCompleteListener {
        override fun onComplete(mp: IPlayer) {
            mCurrentState = STATE_COMPLETED
            mOnCompleteListener?.onComplete(mStudioPlayer)
        }
    }

    fun setOnPreparedListener(onPreparedListener: IPlayer.OnPreparedListener?) {
        mOnPreparedListener = onPreparedListener
    }

    fun setOnInfoListener(onInfoListener: IPlayer.OnInfoListener?) {
        mOnInfoListener = onInfoListener
    }

    fun setOnBufferUpdateListener(onBufferUpdateListener: IPlayer.OnBufferUpdateListener?) {
        mOnBufferingUpdateListener = onBufferUpdateListener
    }

    fun setOnVideoSizeChangedListener(onVideoSizeChangedListener: IPlayer.OnVideoSizeChangedListener?) {
        mOnVideoSizeChangedListener = onVideoSizeChangedListener
    }

    fun setOnErrorListener(onErrorListener: IPlayer.OnErrorListener?) {
        mOnErrorListener = onErrorListener
    }

    fun setOnSeekCompleteListener(onSeekCompleteListener: IPlayer.OnSeekCompleteListener?) {
        mOnSeekCompleteListener = onSeekCompleteListener
    }

    fun setOnCompleteListener(onCompleteListener: IPlayer.OnCompleteListener?) {
        mOnCompleteListener = onCompleteListener
    }

    fun setOnPlayingListener(onPlayingListener: IPlayer.OnPlayingListener) {
        mOnPlayingListener = onPlayingListener
    }

    /******************************player control***********************************/

    fun setVideoPath(path: String?) {
        mUrl = path
        mSeekWhenPrepared = 0
        openVideo()
    }

    fun switchVideo(path: String?) {
        setVideoPath(path)
    }

    fun start() {
        if (!isInPlaybackState())
            return
        mStudioPlayer!!.start()
        mCurrentState = STATE_PLAYING
        mOnPlayingListener?.onPlaying(true)
    }

    fun pause() {
        if (!isInPlaybackState())
            return
        if (mStudioPlayer!!.isPlaying) {
            mStudioPlayer!!.pause()
            mCurrentState = STATE_PAUSED
            mOnPlayingListener?.onPlaying(false)
        }
    }

    fun seekTo(msec: Int) {
        if (isInPlaybackState()) {
            mStudioPlayer!!.seekTo(msec.toLong())
            mSeekWhenPrepared = 0
        } else {
            mSeekWhenPrepared = msec
        }
    }

    fun stop() {
        if (!isInPlaybackState())
            return
        mStudioPlayer?.stop()
        mCurrentState = STATE_IDLE
    }

    fun release() {
        if (mStudioPlayer != null) {
            mStudioPlayer!!.pause()
            mStudioPlayer!!.release()
            mStudioPlayer = null
        }
        mCurrentState = STATE_IDLE
        val am = mAppContext!!.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        am.abandonAudioFocus(null)
    }

    /******************************get method***********************************/

    fun isPlaying(): Boolean {
        return isInPlaybackState() && mStudioPlayer!!.isPlaying
    }

    fun getCurrentPosition(): Int {
        if (!isInPlaybackState()) {
            return 0
        }
        return mStudioPlayer!!.currentPosition.toInt()
    }

    fun getDuration(): Int {
        if (!isInPlaybackState()) {
            return -1
        }
        return mStudioPlayer!!.duration.toInt()
    }

    fun getPlayer(): IPlayer? {
        return mStudioPlayer
    }

    fun getRenderViewType(): Int {
        return when (mRenderView) {
            is SurfaceRenderView -> RENDER_TYPE_SURFACE_VIEW
            is TextureRenderView -> RENDER_TYPE_TEXTURE_VIEW
            else -> -1
        }
    }

    /******************************set method***********************************/

    fun setViewScale(scale: Float) {
        mRenderView!!.view.scaleX = scale
        mRenderView!!.view.scaleY = scale
    }

    fun setSpeed(speed: Float) {
        mStudioPlayer?.setSpeed(speed)
    }

}
