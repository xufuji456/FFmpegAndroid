package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.graphics.Color
import android.media.MediaMetadataRetriever
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.util.Log
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.StaggeredGridLayoutManager

import com.frank.ffmpeg.FFmpegCmd
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.WaterfallAdapter
import com.frank.ffmpeg.model.VideoLayout
import com.frank.ffmpeg.gif.HighQualityGif
import com.frank.ffmpeg.handler.FFmpegHandler
import com.frank.ffmpeg.tool.JsonParseTool
import com.frank.ffmpeg.util.BitmapUtil
import com.frank.ffmpeg.util.FFmpegUtil
import com.frank.ffmpeg.util.FileUtil

import java.io.File
import java.util.ArrayList

import com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS
import com.frank.ffmpeg.listener.OnItemClickListener

/**
 * video process by FFmpeg command
 * Created by frank on 2018/1/25.
 */
class VideoHandleActivity : BaseActivity() {

    private var layoutVideoHandle: RecyclerView? = null
    private var layoutProgress: LinearLayout? = null
    private var txtProgress: TextView? = null
    private var currentPosition: Int = 0
    private var ffmpegHandler: FFmpegHandler? = null

    private val appendPath = PATH + File.separator + "snow.mp4"
    private val outputPath1 = PATH + File.separator + "output1.ts"
    private val outputPath2 = PATH + File.separator + "output2.ts"
    private val listPath = PATH + File.separator + "listFile.txt"

    private var list :List<String> ?= null
    private var isJointing = false

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            when (msg.what) {
                MSG_BEGIN -> {
                    layoutProgress!!.visibility = View.VISIBLE
                    layoutVideoHandle!!.visibility = View.GONE
                }
                MSG_FINISH -> {
                    layoutProgress!!.visibility = View.GONE
                    layoutVideoHandle!!.visibility = View.VISIBLE
                    if (isJointing) {
                        isJointing = false
                        FileUtil.deleteFile(outputPath1)
                        FileUtil.deleteFile(outputPath2)
                        FileUtil.deleteFile(listPath)
                    }
                    if (!outputPath.isNullOrEmpty() && !this@VideoHandleActivity.isDestroyed) {
                        showToast("Save to:$outputPath")
                        outputPath = ""
                    }
                }
                MSG_PROGRESS -> {
                    val progress = msg.arg1
                    val duration = msg.arg2
                    if (progress > 0) {
                        txtProgress!!.visibility = View.VISIBLE
                        val percent = if (duration > 0) "%" else ""
                        val strProgress = progress.toString() + percent
                        txtProgress!!.text = strProgress
                    } else {
                        txtProgress!!.visibility = View.INVISIBLE
                    }
                }
                else -> {
                }
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_video_handle

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

//        hideActionBar()
        intView()
        ffmpegHandler = FFmpegHandler(mHandler)
    }

    private fun intView() {
        layoutProgress = getView(R.id.layout_progress)
        txtProgress = getView(R.id.txt_progress)
        list = listOf(
                getString(R.string.video_transform),
                getString(R.string.video_cut),
                getString(R.string.video_concat),
                getString(R.string.video_screen_shot),
                getString(R.string.video_water_mark),
                getString(R.string.video_remove_logo),
                getString(R.string.video_stereo3d),
                getString(R.string.video_to_gif),
                getString(R.string.video_multi),
                getString(R.string.video_reverse),
                getString(R.string.video_denoise),
                getString(R.string.video_image),
                getString(R.string.video_pip),
                getString(R.string.video_speed),
                getString(R.string.video_thumbnail),
                getString(R.string.video_subtitle),
                getString(R.string.video_rotate),
                getString(R.string.video_gray),
                getString(R.string.video_zoom))

        layoutVideoHandle = findViewById(R.id.list_video_item)
        val layoutManager = StaggeredGridLayoutManager(2, StaggeredGridLayoutManager.VERTICAL)
        layoutVideoHandle?.layoutManager = layoutManager

        val adapter = WaterfallAdapter(list)
        adapter.setOnItemClickListener(object : OnItemClickListener {
            override fun onItemClick(position: Int) {
                currentPosition = position
                if (getString(R.string.video_from_photo) == (list as List<String>)[position]) {
                    handlePhoto()
                } else {
                    selectFile()
                }
            }
        })
        layoutVideoHandle?.adapter = adapter
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {
        doHandleVideo(filePath)
    }

    /**
     * Using FFmpeg cmd to handle video
     *
     * @param srcFile srcFile
     */
    private fun doHandleVideo(srcFile: String) {
        var commandLine: Array<String>? = null
        if (!FileUtil.checkFileExist(srcFile)) {
            return
        }
        if (!FileUtil.isVideo(srcFile)
                && (list as List<String>)[currentPosition] != getString(R.string.video_zoom)) {
            showToast(getString(R.string.wrong_video_format))
            return
        }
        val suffix = FileUtil.getFileSuffix(srcFile)
        if (suffix == null || suffix.isEmpty()) {
            return
        }
        when (currentPosition) {
            0 -> { //transform format
                outputPath = PATH + File.separator + "transformVideo.mp4"
                commandLine = FFmpegUtil.transformVideo(srcFile, outputPath)
            }
            1 -> { //cut video
                outputPath = PATH + File.separator + "cutVideo" + suffix
                val startTime = 5.5f
                val duration = 20.0f
                commandLine = FFmpegUtil.cutVideo(srcFile, startTime, duration, outputPath)
            }
            2 -> { //concat video together
                concatVideo(srcFile)
            }
            3 -> { //video snapshot
                outputPath = PATH + File.separator + "screenShot.jpg"
                val time = 10.5f
                commandLine = FFmpegUtil.screenShot(srcFile, time, outputPath)
            }
            4 -> { //add watermark to video
                // the unit of bitRate is kb
                var bitRate = 500
                val retriever = MediaMetadataRetriever()
                retriever.setDataSource(srcFile)
                val mBitRate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE)
                if (mBitRate != null && mBitRate.isNotEmpty()) {
                    val probeBitrate = Integer.valueOf(mBitRate)
                    bitRate = probeBitrate / 1000 / 100 * 100
                }
                retriever.release()
                //1:top left 2:top right 3:bottom left 4:bottom right
                val location = 1
                val offsetXY = 10
                when (waterMarkType) {
                    TYPE_IMAGE// image
                    -> {
                        val photo = PATH + File.separator + "hello.png"
                        outputPath = PATH + File.separator + "photoMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, photo, location, bitRate, offsetXY, outputPath)
                    }
                    TYPE_GIF// gif
                    -> {
                        val gifPath = PATH + File.separator + "ok.gif"
                        outputPath = PATH + File.separator + "gifWaterMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkGif(srcFile, gifPath, location, bitRate, offsetXY, outputPath)
                    }
                    TYPE_TEXT// text
                    -> {
                        val text = "Hello,FFmpeg"
                        val textPath = PATH + File.separator + "text.png"
                        val result = BitmapUtil.textToPicture(textPath, text, Color.BLUE, 20)
                        outputPath = PATH + File.separator + "textMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, textPath, location, bitRate, offsetXY, outputPath)
                    }
                    else -> {
                    }
                }
            }
            5 -> { //Remove logo from video, or use to mosaic video
                outputPath = PATH + File.separator + "removeLogo" + suffix
                val widthL = 64
                val heightL = 40
                commandLine = FFmpegUtil.removeLogo(srcFile, 10, 10, widthL, heightL, outputPath)
            }
            6 -> { // VR video: convert to stereo3d video
                outputPath = PATH + File.separator + "stereo3d" + suffix
                commandLine = FFmpegUtil.videoStereo3D(srcFile, outputPath)
            }
            7 -> { //convert video into gif
                outputPath = PATH + File.separator + "video2Gif.gif"
                val gifStart = 10
                val gifDuration = 3
                val width = 320
                val frameRate = 10

                if (convertGifWithFFmpeg) {
                    val palettePath = "$PATH/palette.png"
                    FileUtil.deleteFile(palettePath)
                    val paletteCmd = FFmpegUtil.generatePalette(srcFile, gifStart, gifDuration,
                            frameRate, width, palettePath)
                    val gifCmd = FFmpegUtil.generateGifByPalette(srcFile, palettePath, gifStart, gifDuration,
                            frameRate, width, outputPath)
                    val cmdList = ArrayList<Array<String>>()
                    cmdList.add(paletteCmd)
                    cmdList.add(gifCmd)
                    ffmpegHandler!!.executeFFmpegCmds(cmdList)
                } else {
                    convertGifInHighQuality(outputPath!!, srcFile, gifStart, gifDuration, frameRate)
                }
            }
            8 -> { //combine video which layout could be horizontal of vertical
                val input1 = PATH + File.separator + "input1.mp4"
                val input2 = PATH + File.separator + "input2.mp4"
                outputPath = PATH + File.separator + "multi.mp4"
                if (!FileUtil.checkFileExist(input1) || !FileUtil.checkFileExist(input2)) {
                    return
                }
                commandLine = FFmpegUtil.multiVideo(input1, input2, outputPath, VideoLayout.LAYOUT_HORIZONTAL)
            }
            9 -> { //video reverse
                outputPath = PATH + File.separator + "reverse.mp4"
                commandLine = FFmpegUtil.reverseVideo(srcFile, outputPath)
            }
            10 -> { //noise reduction of video
                outputPath = PATH + File.separator + "denoise.mp4"
                commandLine = FFmpegUtil.denoiseVideo(srcFile, outputPath)
            }
            11 -> { //convert video to picture
                outputPath = PATH + File.separator + "Video2Image/"
                val imageFile = File(outputPath)
                if (!imageFile.exists()) {
                    if (!imageFile.mkdir()) {
                        return
                    }
                }
                val mStartTime = 10//start time
                val mDuration = 5//duration
                val mFrameRate = 10//frameRate
                commandLine = FFmpegUtil.videoToImage(srcFile, mStartTime, mDuration, mFrameRate, outputPath)
            }
            12 -> { //combine into picture-in-picture video
                val inputFile1 = PATH + File.separator + "beyond.mp4"
                val inputFile2 = PATH + File.separator + "small_girl.mp4"
                if (!FileUtil.checkFileExist(inputFile1) && !FileUtil.checkFileExist(inputFile2)) {
                    return
                }
                //x and y coordinate points need to be calculated according to the size of full video and small video
                //For example: full video is 320x240, small video is 120x90, so x=200 y=150
                val x = 200
                val y = 150
                outputPath = PATH + File.separator + "PicInPic.mp4"
                commandLine = FFmpegUtil.picInPicVideo(inputFile1, inputFile2, x, y, outputPath)
            }
            13 -> { //playing speed of video
                outputPath = PATH + File.separator + "speed.mp4"
                commandLine = FFmpegUtil.changeSpeed(srcFile, outputPath, 2f, false)
            }
            14 -> { // insert thumbnail into video
                val thumbnailPath = PATH + File.separator + "thumb.jpg"
                outputPath = PATH + File.separator + "thumbnailVideo" + suffix
                commandLine = FFmpegUtil.insertPicIntoVideo(srcFile, thumbnailPath, outputPath)
            }
            15 -> { //add subtitle into video
                val subtitlePath = PATH + File.separator + "test.ass"
                outputPath = PATH + File.separator + "subtitle.mkv"
                commandLine = FFmpegUtil.addSubtitleIntoVideo(srcFile, subtitlePath, outputPath)
            }
            16 -> { // set the rotate degree of video
                val rotateDegree = 90
                outputPath = PATH + File.separator + "rotate" + rotateDegree + suffix
                commandLine = FFmpegUtil.rotateVideo(srcFile, rotateDegree, outputPath)
            }
            17 -> { // change video from RGB to gray
                outputPath = PATH + File.separator + "gray" + suffix
                commandLine = FFmpegUtil.toGrayVideo(srcFile, outputPath)
            }
            18 -> { // zoom photo to video
                outputPath = PATH + File.separator + "zoom.mp4"
                val position = 0
                commandLine = FFmpegUtil.photoZoomToVideo(srcFile, position, outputPath)
            }
            else -> {
            }
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler!!.executeFFmpegCmd(commandLine)
        }
    }

    /**
     * concat/joint two videos together
     * It's recommended to convert to the same resolution and encoding
     * @param selectedPath the path which is selected
     */
    private fun concatVideo(selectedPath: String) {
        if (ffmpegHandler == null || selectedPath.isEmpty()) {
            return
        }
        isJointing = true
        val targetPath = PATH + File.separator + "jointVideo.mp4"
        val transformCmd1 = FFmpegUtil.transformVideoWithEncode(selectedPath, outputPath1)
        var width = 0
        var height = 0
        //probe width and height of the selected video
        val probeResult = FFmpegCmd.executeProbeSynchronize(FFmpegUtil.probeFormat(selectedPath))
        val mediaBean = JsonParseTool.parseMediaFormat(probeResult)
        if (mediaBean?.videoBean != null) {
            width = mediaBean.videoBean!!.width
            height = mediaBean.videoBean!!.height
            Log.e(TAG, "width=$width--height=$height")
        }
        val transformCmd2 = FFmpegUtil.transformVideoWithEncode(appendPath, width, height, outputPath2)
        val fileList = ArrayList<String>()
        fileList.add(outputPath1)
        fileList.add(outputPath2)
        FileUtil.createListFile(listPath, fileList)
        val jointVideoCmd = FFmpegUtil.jointVideo(listPath, targetPath)
        val commandList = ArrayList<Array<String>>()
        commandList.add(transformCmd1)
        commandList.add(transformCmd2)
        commandList.add(jointVideoCmd)
        ffmpegHandler!!.executeFFmpegCmds(commandList)
    }

    /**
     * Combine pictures into video
     */
    private fun handlePhoto() {
        // The path of pictures, naming format: img+number.jpg
        val picturePath = "$PATH/img/"
        if (!FileUtil.checkFileExist(picturePath)) {
            return
        }
        val tempPath = "$PATH/temp/"
        FileUtil.deleteFolder(tempPath)
        val photoFile = File(picturePath)
        val files = photoFile.listFiles()
        val cmdList = ArrayList<Array<String>>()
        //the resolution of photo which you want to convert
        val resolution = "640x480"
        for (file in files) {
            val inputPath = file.absolutePath
            val outputPath = tempPath + file.name
            val convertCmd = FFmpegUtil.convertResolution(inputPath, resolution, outputPath)
            cmdList.add(convertCmd)
        }
        val combineVideo = PATH + File.separator + "combineVideo.mp4"
        val frameRate = 2// suggested synthetic frameRate:1-10
        val commandLine = FFmpegUtil.pictureToVideo(tempPath, frameRate, combineVideo)
        cmdList.add(commandLine)
        if (ffmpegHandler != null) {
            ffmpegHandler!!.executeFFmpegCmds(cmdList)
        }
    }

    private fun convertGifInHighQuality(gifPath: String, videoPath: String, startTime: Int, duration: Int, frameRate: Int) {
        Thread {
            mHandler.sendEmptyMessage(MSG_BEGIN)
            var width = 0
            var height = 0
            var rotateDegree = 0
            try {
                val retriever = MediaMetadataRetriever()
                retriever.setDataSource(videoPath)
                val mWidth = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)
                val mHeight = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)
                width = Integer.valueOf(mWidth)
                height = Integer.valueOf(mHeight)
                val rotate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION)
                rotateDegree = Integer.valueOf(rotate)
                retriever.release()
                Log.e(TAG, "retrieve width=$width--height=$height--rotate=$rotate")
            } catch (e: Exception) {
                Log.e(TAG, "retrieve error=$e")
            }

            val start = System.currentTimeMillis()
            val highQualityGif = HighQualityGif(width, height, rotateDegree)
            val result = highQualityGif.convertGIF(gifPath, videoPath, startTime, duration, frameRate)
            Log.e(TAG, "convert gif result=" + result + "--time=" + (System.currentTimeMillis() - start))
            mHandler.sendEmptyMessage(MSG_FINISH)
        }.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        mHandler.removeCallbacksAndMessages(null)
    }

    companion object {

        private val TAG = VideoHandleActivity::class.java.simpleName
        private val PATH = Environment.getExternalStorageDirectory().path
        private var outputPath :String ?= null

        private const val TYPE_IMAGE = 1
        private const val TYPE_GIF   = 2
        private const val TYPE_TEXT  = 3
        private const val waterMarkType = TYPE_IMAGE
        private const val convertGifWithFFmpeg = false
    }
}
