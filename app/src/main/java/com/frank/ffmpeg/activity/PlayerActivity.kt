package com.frank.ffmpeg.activity

import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.Bundle
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import com.frank.ffmpeg.R
import com.frank.ffmpeg.presenter.PlayerPresenter

/**
 * Note: Activity of player
 * Date: 2026/1/29 20:20
 * Author: frank
 */
class PlayerActivity : AppCompatActivity() {

    private var isPortrait = true
    private var mPlayIndex = 0
    private var mPlayUrls  = arrayListOf<String>()

    private var mPlayerPresenter: PlayerPresenter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_USER
        setContentView(R.layout.layout_activity_player)
        initData()

        mPlayerPresenter = PlayerPresenter()
        mPlayerPresenter?.initView(window.decorView, this)
        mPlayerPresenter?.initPlayer(mPlayUrls[0])
        mPlayerPresenter?.setOnPresenterListener(object : PlayerPresenter.OnPresenterListener {
            override fun onNextUrl(): String {
                return getNextUrl()
            }

            override fun onSwitchOrientation() {
                switchOrientation()
            }
        })
    }

    private fun initData() {
        mPlayUrls.add("https://d2rkgq4wjjlyz5.cloudfront.net/e07cd0562d1271ef80032680f9ea0102/h265-ordinary-ld.m3u8")
        mPlayUrls.add("https://sf1-cdn-tos.huoshanstatic.com/obj/media-fe/xgplayer_doc_video/mp4/xgplayer-demo-360p.mp4")
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            isPortrait = false
            window.setFlags(
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
            mPlayerPresenter?.getPlayerView()?.requestLayout()
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            isPortrait = true
            val attrs = window.attributes
            attrs.flags = attrs.flags and WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS.inv()
            window.attributes = attrs
            window.clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
            mPlayerPresenter?.getPlayerView()?.requestLayout()
        }
    }

    override fun onResume() {
        super.onResume()
        mPlayerPresenter?.getPlayerView()?.start()
    }

    override fun onPause() {
        super.onPause()
        mPlayerPresenter?.getPlayerView()?.pause()
    }

    override fun onDestroy() {
        super.onDestroy()
        mPlayerPresenter?.getPlayerView()?.release()
    }


    // next video
    private fun getNextUrl(): String {
        mPlayIndex += 1
        if (mPlayIndex > (mPlayUrls.size - 1)) {
            mPlayIndex = 0
        }

        val index = mPlayIndex % mPlayUrls.size
        return mPlayUrls[index]
    }

    // switch orientation
    private fun switchOrientation() {
        requestedOrientation = if (isPortrait) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        }
    }

}
