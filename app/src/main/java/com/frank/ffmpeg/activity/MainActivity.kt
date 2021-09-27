package com.frank.ffmpeg.activity

import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.StaggeredGridLayoutManager

import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.WaterfallAdapter
import com.frank.ffmpeg.listener.OnItemClickListener

/**
 * The main entrance of all Activity
 * Created by frank on 2018/1/23.
 */
class MainActivity : BaseActivity() {

    override val layoutId: Int
        get() = R.layout.activity_main

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
    }

    private fun initView() {
        val list = listOf(
                getString(R.string.audio_handle),
                getString(R.string.video_handle),
                getString(R.string.media_handle),
                getString(R.string.video_push),
                getString(R.string.video_live),
                getString(R.string.video_filter),
                getString(R.string.video_preview),
                getString(R.string.media_probe),
                getString(R.string.audio_effect))

        val viewWaterfall: RecyclerView = findViewById(R.id.list_main_item)
        val layoutManager = StaggeredGridLayoutManager(2, StaggeredGridLayoutManager.VERTICAL)
        viewWaterfall.layoutManager = layoutManager

        val adapter = WaterfallAdapter(list)
        adapter.setOnItemClickListener(object : OnItemClickListener {
            override fun onItemClick(position: Int) {
                doClick(position)
            }
        })
        viewWaterfall.adapter = adapter
    }

    private fun doClick(pos: Int) {
        val intent = Intent()
        when (pos) {
            0 //handle audio
            -> intent.setClass(this@MainActivity, AudioHandleActivity::class.java)
            1 //handle video
            -> intent.setClass(this@MainActivity, VideoHandleActivity::class.java)
            2 //handle media
            -> intent.setClass(this@MainActivity, MediaHandleActivity::class.java)
            3 //pushing
            -> intent.setClass(this@MainActivity, PushActivity::class.java)
            4 //realtime living with rtmp stream
            -> intent.setClass(this@MainActivity, LiveActivity::class.java)
            5 //filter effect
            -> intent.setClass(this@MainActivity, FilterActivity::class.java)
            6 //preview thumbnail
            -> intent.setClass(this@MainActivity, VideoPreviewActivity::class.java)
            7 //probe media format
            -> intent.setClass(this@MainActivity, ProbeFormatActivity::class.java)
            8 //audio effect
            -> intent.setClass(this@MainActivity, AudioEffectActivity::class.java)
//            -> intent.setClass(this@MainActivity, MediaPlayerActivity::class.java)
            else -> {
            }
        }
        startActivity(intent)
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {

    }

}
