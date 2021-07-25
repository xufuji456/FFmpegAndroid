package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.os.Bundle

import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.Toast
import android.widget.ToggleButton

import com.frank.ffmpeg.FFmpegApplication
import com.frank.ffmpeg.R
import com.frank.ffmpeg.VideoPlayer
import com.frank.ffmpeg.adapter.HorizontalAdapter
import com.frank.ffmpeg.listener.OnItemClickListener
import com.frank.ffmpeg.util.FileUtil

import java.util.ArrayList

/**
 * Using ffmpeg to filter
 * Created by frank on 2018/6/5.
 */

class FilterActivity : BaseActivity(), SurfaceHolder.Callback {

    private var videoPath = Environment.getExternalStorageDirectory().path + "/beyond.mp4"

    private var videoPlayer: VideoPlayer? = null
    private var surfaceView: SurfaceView? = null
    private var surfaceHolder: SurfaceHolder? = null
    private var surfaceCreated: Boolean = false
    //vflip is up and down, hflip is left and right
    private val txtArray = arrayOf(
            FFmpegApplication.getInstance().getString(R.string.filter_sketch),
            FFmpegApplication.getInstance().getString(R.string.filter_distinct),
            FFmpegApplication.getInstance().getString(R.string.filter_edge),
            FFmpegApplication.getInstance().getString(R.string.filter_division),
            FFmpegApplication.getInstance().getString(R.string.filter_equalize),
            FFmpegApplication.getInstance().getString(R.string.filter_rectangle),
            FFmpegApplication.getInstance().getString(R.string.filter_flip),
            FFmpegApplication.getInstance().getString(R.string.filter_blur),
            FFmpegApplication.getInstance().getString(R.string.filter_rotate),
            FFmpegApplication.getInstance().getString(R.string.filter_sharpening))
    private var horizontalAdapter: HorizontalAdapter? = null
    private var recyclerView: RecyclerView? = null
    private var playAudio = true
    private var btnSound: ToggleButton? = null
    private var btnSelect: Button? = null
    private var filterThread: Thread? = null

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            if (msg.what == MSG_HIDE) { //after idle 5s, hide the controller view
                recyclerView!!.visibility = View.GONE
                btnSound!!.visibility = View.GONE
                btnSelect!!.visibility = View.GONE
            }
        }
    }

    private var hideRunnable: HideRunnable? = null

    override val layoutId: Int
        get() = R.layout.activity_filter

    private inner class HideRunnable : Runnable {
        override fun run() {
            mHandler.obtainMessage(MSG_HIDE).sendToTarget()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
        registerLister()

        hideRunnable = HideRunnable()
        mHandler.postDelayed(hideRunnable, DELAY_TIME.toLong())
    }

    private fun initView() {
        surfaceView = getView(R.id.surface_filter)
        surfaceHolder = surfaceView!!.holder
        surfaceHolder!!.addCallback(this)
        videoPlayer = VideoPlayer()
        btnSound = getView(R.id.btn_sound)

        recyclerView = getView(R.id.recycler_view)
        val linearLayoutManager = LinearLayoutManager(this)
        linearLayoutManager.orientation = LinearLayoutManager.HORIZONTAL
        recyclerView!!.layoutManager = linearLayoutManager
        val itemList = ArrayList(mutableListOf(*txtArray))
        horizontalAdapter = HorizontalAdapter(itemList)
        recyclerView!!.adapter = horizontalAdapter

        btnSelect = getView(R.id.btn_select_file)
        initViewsWithClick(R.id.btn_select_file)
    }

    private fun registerLister() {
        horizontalAdapter!!.setOnItemClickListener(object : OnItemClickListener {
            override fun onItemClick(position: Int) {
                if (!surfaceCreated)
                    return
                if (!FileUtil.checkFileExist(videoPath)) {
                    showSelectFile()
                    return
                }
                doFilterPlay(position)
            }
        })

        surfaceView!!.setOnClickListener {
            btnSelect!!.visibility = View.VISIBLE
            btnSound!!.visibility = View.VISIBLE
            recyclerView!!.visibility = View.VISIBLE
            mHandler.postDelayed(hideRunnable, DELAY_TIME.toLong())
        }

        btnSound!!.setOnCheckedChangeListener { buttonView, isChecked -> setPlayAudio() }
    }

    /**
     * switch filter
     * @param position position in the array of filters
     */
    private fun doFilterPlay(position: Int) {
        if (filterThread == null) {
            filterThread = Thread(Runnable {
                videoPlayer!!.filter(videoPath, surfaceHolder!!.surface, position)
            })
            filterThread!!.start()
        } else {
            videoPlayer!!.again(position)
        }
    }

    private fun setPlayAudio() {
        playAudio = !playAudio
        videoPlayer!!.playAudio(playAudio)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        surfaceCreated = true
        if (FileUtil.checkFileExist(videoPath)) {
            doFilterPlay(4)
            btnSound!!.isChecked = true
        } else {
            Toast.makeText(this@FilterActivity, getString(R.string.file_not_found), Toast.LENGTH_SHORT).show()
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {

    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceCreated = false
        if (filterThread != null) {
            videoPlayer?.release()
            filterThread?.interrupt()
            filterThread = null
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        videoPlayer = null
        horizontalAdapter = null
    }

    override fun onViewClick(view: View) {
        if (view.id == R.id.btn_select_file) {
            selectFile()
        }
    }

    override fun onSelectedFile(filePath: String) {
        videoPath = filePath
        doFilterPlay(4)
        //sound off by default
        btnSound!!.isChecked = true
    }

    companion object {

        private const val MSG_HIDE = 222
        private const val DELAY_TIME = 5000
    }

}
