package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.util.Log
import android.view.View
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.RelativeLayout
import android.widget.TextView

import com.frank.ffmpeg.R
import com.frank.ffmpeg.handler.FFmpegHandler
import com.frank.ffmpeg.handler.FFmpegHandler.*

import com.frank.ffmpeg.model.MediaBean
import com.frank.ffmpeg.tool.JsonParseTool
import com.frank.ffmpeg.util.FFmpegUtil

import com.frank.ffmpeg.util.FileUtil

import com.frank.ffmpeg.metadata.FFmpegMediaRetriever
import java.lang.StringBuilder

/**
 * Using ffprobe to parse media format data
 * Created by frank on 2020/1/7.
 */

class ProbeFormatActivity : BaseActivity() {

    private var txtProbeFormat: TextView? = null
    private var progressProbe: ProgressBar? = null
    private var layoutProbe: RelativeLayout? = null
    private var ffmpegHandler: FFmpegHandler? = null

    private var view: View? = null

    private val MSG_FRAME = 9099

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            when (msg.what) {
                MSG_BEGIN -> {
                    progressProbe!!.visibility = View.VISIBLE
                    layoutProbe!!.visibility = View.GONE
                }
                MSG_FINISH -> {
                    progressProbe!!.visibility = View.GONE
                    layoutProbe!!.visibility = View.VISIBLE
                    val result = msg.obj?: msg.obj
                    if (result != null) {
                        val mediaInfo = JsonParseTool.stringFormat(result as MediaBean)
                        if (!mediaInfo.isNullOrEmpty() && txtProbeFormat != null) {
                            txtProbeFormat!!.text = mediaInfo
                        }
                    }
                }
                MSG_INFO -> {
                    val mediaInfo = msg.obj as String
                    if (!mediaInfo.isNullOrEmpty()) {
                        txtProbeFormat!!.text = mediaInfo
                    }
                }
                MSG_FRAME -> {
                    val bitmap = msg.obj as Bitmap
                    findViewById<ImageView>(R.id.img_frame).setImageBitmap(bitmap)
                }
                else -> {
                }
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_probe

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
        ffmpegHandler = FFmpegHandler(mHandler)
    }

    private fun initView() {
        progressProbe = getView(R.id.progress_probe)
        layoutProbe = getView(R.id.layout_probe)
        initViewsWithClick(R.id.btn_probe_format)
        initViewsWithClick(R.id.btn_retrieve_format)
        txtProbeFormat = getView(R.id.txt_probe_format)
    }

    override fun onViewClick(view: View) {
        this.view = view
        selectFile()
    }

    override fun onSelectedFile(filePath: String) {

        when (view?.id) {
            R.id.btn_probe_format -> {
                doHandleProbe(filePath)
            }
            R.id.btn_retrieve_format -> {
                Thread {retrieveMediaMetadata(filePath)}.start()
            }
            else -> {
            }
        }
    }

    /**
     * use ffprobe to parse video/audio format metadata
     *
     * @param srcFile srcFile
     */
    private fun doHandleProbe(srcFile: String) {
        if (!FileUtil.checkFileExist(srcFile)) {
            return
        }
        val commandLine = FFmpegUtil.probeFormat(srcFile)
        if (ffmpegHandler != null) {
            ffmpegHandler!!.executeFFprobeCmd(commandLine)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        mHandler.removeCallbacksAndMessages(null)
    }

    private fun retrieveMediaMetadata(path :String) {
        if (path.isEmpty()) {
            return
        }

        try {
            val resultBuilder = StringBuilder()
            val retriever = FFmpegMediaRetriever()
            retriever.setDataSource(path)

            val duration = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_DURATION)
            if (duration != null)
                resultBuilder.append("duration:$duration\n")

            val audioCodec = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_AUDIO_CODEC)
            if (audioCodec != null)
                resultBuilder.append("audioCodec:$audioCodec\n")

            val videoCodec = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_VIDEO_CODEC)
            if (videoCodec != null)
                resultBuilder.append("videoCodec:$videoCodec\n")

            val width = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_VIDEO_WIDTH)
            val height = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_VIDEO_HEIGHT)
            if (width != null && height != null)
                resultBuilder.append("resolution:$width x $height\n")

            val frameRate = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_FRAME_RATE)
            if (frameRate != null)
                resultBuilder.append("frameRate:$frameRate\n")

            val sampleRate = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_SAMPLE_RATE)
            if (sampleRate != null)
                resultBuilder.append("sampleRate:$sampleRate\n")

            val channelCount = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_CHANNEL_COUNT)
            if (channelCount != null)
                resultBuilder.append("channelCount:$channelCount\n")

            val channelLayout = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_CHANNEL_LAYOUT)
            if (channelLayout != null)
                resultBuilder.append("channelLayout:$channelLayout\n")

            val pixelFormat = retriever.extractMetadata(FFmpegMediaRetriever.METADATA_KEY_PIXEL_FORMAT)
            if (pixelFormat != null)
                resultBuilder.append("pixelFormat:$pixelFormat\n")

            mHandler.obtainMessage(MSG_INFO, resultBuilder.toString()).sendToTarget()

            // Retrieve frame with timeUs
            val bitmap = retriever.getFrameAtTime(5 * 1000000)
            // Retrieve audio thumbnail, if it has embedded
//            val bitmap = retriever.audioThumbnail
            if (bitmap != null) {
                Log.e("FFmpegRetriever", "bitmap width=${bitmap.width}--height=${bitmap.height}")
                mHandler.obtainMessage(MSG_FRAME, bitmap).sendToTarget()
            }

            retriever.release()
        } catch (e : Exception) {
            Log.e("FFmpegRetriever", "ffmpeg error=$e")
        }
    }

}
