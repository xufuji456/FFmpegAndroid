package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.content.IntentFilter
import android.media.AudioFormat
import android.net.ConnectivityManager
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.text.TextUtils
import android.util.Log
import android.view.SurfaceView
import android.view.View
import android.widget.CompoundButton
import android.widget.Toast
import android.widget.ToggleButton

import com.frank.ffmpeg.R
import com.frank.ffmpeg.handler.ConnectionReceiver
import com.frank.ffmpeg.listener.OnNetworkChangeListener
import com.frank.live.camera2.Camera2Helper
import com.frank.live.listener.LiveStateChangeListener
import com.frank.live.param.AudioParam
import com.frank.live.param.VideoParam
import com.frank.live.LivePusherNew

/**
 * Realtime living with rtmp stream
 * Created by frank on 2018/1/28.
 */

open class LiveActivity : BaseActivity(), CompoundButton.OnCheckedChangeListener, LiveStateChangeListener, OnNetworkChangeListener {
    private var textureView: SurfaceView? = null
    private var livePusher: LivePusherNew? = null
    private var isPushing = false
    private var connectionReceiver: ConnectionReceiver? = null

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            if (msg.what == MSG_ERROR) {
                val errMsg = msg.obj as String
                if (!TextUtils.isEmpty(errMsg)) {
                    Toast.makeText(this@LiveActivity, errMsg, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_live

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
        initPusher()
        registerBroadcast(this)
    }

    private fun initView() {
        initViewsWithClick(R.id.btn_swap)
        (findViewById<View>(R.id.btn_live) as ToggleButton).setOnCheckedChangeListener(this)
        (findViewById<View>(R.id.btn_mute) as ToggleButton).setOnCheckedChangeListener(this)
        textureView = getView(R.id.surface_camera)
    }

    private fun initPusher() {
        val width = 640//resolution
        val height = 480
        val videoBitRate = 800000//kb/s
        val videoFrameRate = 10//fps
        val videoParam = VideoParam(width, height,
                Integer.valueOf(Camera2Helper.CAMERA_ID_BACK), videoBitRate, videoFrameRate)
        val sampleRate = 44100//sample rate: Hz
        val channelConfig = AudioFormat.CHANNEL_IN_STEREO
        val audioFormat = AudioFormat.ENCODING_PCM_16BIT
        val numChannels = 2//channel number
        val audioParam = AudioParam(sampleRate, channelConfig, audioFormat, numChannels)
        livePusher = LivePusherNew(this, videoParam, audioParam)
        livePusher!!.setPreviewDisplay(textureView!!.holder)
    }

    private fun registerBroadcast(networkChangeListener: OnNetworkChangeListener) {
        connectionReceiver = ConnectionReceiver(networkChangeListener)
        val intentFilter = IntentFilter()
        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION)
        registerReceiver(connectionReceiver, intentFilter)
    }

    override fun onCheckedChanged(buttonView: CompoundButton, isChecked: Boolean) {
        when (buttonView.id) {
            R.id.btn_live//start or stop living
            -> if (isChecked) {
                livePusher!!.startPush(LIVE_URL, this)
                isPushing = true
            } else {
                livePusher!!.stopPush()
                isPushing = false
            }
            R.id.btn_mute//mute or not
            -> {
                Log.i(TAG, "isChecked=$isChecked")
                livePusher!!.setMute(isChecked)
            }
            else -> {
            }
        }
    }

    override fun onError(msg: String) {
        Log.e(TAG, "errMsg=$msg")
        mHandler.obtainMessage(MSG_ERROR, msg).sendToTarget()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (livePusher != null) {
            if (isPushing) {
                isPushing = false
                livePusher?.stopPush()
            }
            livePusher!!.release()
        }
        if (connectionReceiver != null) {
            unregisterReceiver(connectionReceiver)
        }
    }

    override fun onViewClick(view: View) {
        if (view.id == R.id.btn_swap) {//switch camera
            livePusher!!.switchCamera()
        }
    }

    override fun onSelectedFile(filePath: String) {

    }

    override fun onNetworkChange() {
        Toast.makeText(this, "network is not available", Toast.LENGTH_SHORT).show()
        if (livePusher != null && isPushing) {
            livePusher?.stopPush()
            isPushing = false
        }
    }

    companion object {

        private val TAG = LiveActivity::class.java.simpleName
        private const val LIVE_URL = "rtmp://192.168.31.212/live/stream"
        private const val MSG_ERROR = 100
    }
}
