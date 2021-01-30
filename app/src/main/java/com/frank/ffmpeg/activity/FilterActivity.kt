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
import java.util.Arrays

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
    //is playing or not
    private var isPlaying: Boolean = false
    //the array of filter
    private val filters = arrayOf("lutyuv='u=128:v=128'", "hue='h=60:s=-3'", "edgedetect=low=0.1:high=0.4", "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5", "colorbalance=bs=0.3", "drawbox=x=100:y=100:w=100:h=100:color=red@0.5'", "hflip",
            //adjust the coefficient of sigma to control the blur
            "gblur=sigma=2:steps=1:planes=1:sigmaV=1", "rotate=180*PI/180", "unsharp")
    //vflip is up and down, hflip is left and right
    private val txtArray = arrayOf(FFmpegApplication.getInstance().getString(R.string.filter_sketch), FFmpegApplication.getInstance().getString(R.string.filter_distinct), FFmpegApplication.getInstance().getString(R.string.filter_edge), FFmpegApplication.getInstance().getString(R.string.filter_division), FFmpegApplication.getInstance().getString(R.string.filter_equalize), FFmpegApplication.getInstance().getString(R.string.filter_rectangle), FFmpegApplication.getInstance().getString(R.string.filter_flip), FFmpegApplication.getInstance().getString(R.string.filter_blur), FFmpegApplication.getInstance().getString(R.string.filter_rotate), FFmpegApplication.getInstance().getString(R.string.filter_sharpening))
    private var horizontalAdapter: HorizontalAdapter? = null
    private var recyclerView: RecyclerView? = null
    private var playAudio = true
    private var btnSound: ToggleButton? = null
    private var btnSelect: Button? = null
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
        val itemList = ArrayList(Arrays.asList(*txtArray))
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
        Thread(Runnable {
            if (isPlaying) {
                videoPlayer!!.again()
            }
            isPlaying = true
            videoPlayer!!.filter(videoPath, surfaceHolder!!.surface, filters[position])
        }).start()
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
    }

    override fun onDestroy() {
        super.onDestroy()
        isPlaying = false
        //FIXME
//        videoPlayer?.release()
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
