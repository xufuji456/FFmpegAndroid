package com.frank.ffmpeg.activity

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
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

    private val MANAGE_STORAGE_RC: Int = 100
    override val layoutId: Int
        get() = R.layout.activity_main

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
        getQueryPermission()
    }

    fun getQueryPermission(){
        fun isRPlus() = Build.VERSION.SDK_INT >= Build.VERSION_CODES.R
        if(!isRPlus()) {
           return
        }
        val packageName = this.packageName
        try {
//            "queryPermission".toast(ContentProviderCompat.requireContext())
            val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
            intent.addCategory("android.intent.category.DEFAULT")
            intent.data = Uri.parse("package:$packageName")
            this.startActivityForResult(intent, MANAGE_STORAGE_RC)
        } catch (e: Exception) {
//            "error:".toast(ContentProviderCompat.requireContext())
            val intent = Intent()
            intent.action = Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION
            startActivityForResult(intent, MANAGE_STORAGE_RC)
        }
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
                getString(R.string.audio_effect),
                getString(R.string.camera_filter))

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
            9 //camera filter
            -> intent.setClass(this@MainActivity, CameraFilterActivity::class.java)
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
