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

import com.frank.ffmpeg.FFmpegCmd
import com.frank.ffmpeg.R
import com.frank.ffmpeg.format.VideoLayout
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

/**
 * video process by FFmpeg command
 * Created by frank on 2018/1/25.
 */
class VideoHandleActivity : BaseActivity() {

    private var layoutVideoHandle: LinearLayout? = null
    private var layoutProgress: LinearLayout? = null
    private var txtProgress: TextView? = null
    private var viewId: Int = 0
    private var ffmpegHandler: FFmpegHandler? = null

    private val appendPath = PATH + File.separator + "snow.mp4"
    private val outputPath1 = PATH + File.separator + "output1.ts"
    private val outputPath2 = PATH + File.separator + "output2.ts"
    private val listPath = PATH + File.separator + "listFile.txt"

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

        hideActionBar()
        intView()
        ffmpegHandler = FFmpegHandler(mHandler)
    }

    private fun intView() {
        layoutProgress = getView(R.id.layout_progress)
        txtProgress = getView(R.id.txt_progress)
        layoutVideoHandle = getView(R.id.layout_video_handle)
        initViewsWithClick(
                R.id.btn_video_transform,
                R.id.btn_video_cut,
                R.id.btn_video_concat,
                R.id.btn_screen_shot,
                R.id.btn_water_mark,
                R.id.btn_generate_gif,
                R.id.btn_screen_record,
                R.id.btn_combine_video,
                R.id.btn_multi_video,
                R.id.btn_reverse_video,
                R.id.btn_denoise_video,
                R.id.btn_to_image,
                R.id.btn_pip,
                R.id.btn_moov,
                R.id.btn_speed,
                R.id.btn_flv,
                R.id.btn_thumbnail,
                R.id.btn_add_subtitle,
                R.id.btn_rotate,
                R.id.btn_gop,
                R.id.btn_remove_logo
        )
    }

    override fun onViewClick(view: View) {
        viewId = view.id
        if (viewId == R.id.btn_combine_video) {
            handlePhoto()
            return
        }
        selectFile()
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
        if (!FileUtil.isVideo(srcFile)) {
            showToast(getString(R.string.wrong_video_format))
            return
        }
        val suffix = FileUtil.getFileSuffix(srcFile)
        if (suffix == null || suffix.isEmpty()) {
            return
        }
        when (viewId) {
            R.id.btn_video_transform//transform format
            -> {
                val transformVideo = PATH + File.separator + "transformVideo.mp4"
                commandLine = FFmpegUtil.transformVideo(srcFile, transformVideo)
            }
            R.id.btn_video_cut//cut video
            -> {
                val cutVideo = PATH + File.separator + "cutVideo" + suffix
                val startTime = 5.5f
                val duration = 20.0f
                commandLine = FFmpegUtil.cutVideo(srcFile, startTime, duration, cutVideo)
            }
            R.id.btn_video_concat//concat video together
            -> concatVideo(srcFile)
            R.id.btn_screen_shot//video snapshot
            -> {
                val screenShot = PATH + File.separator + "screenShot.jpg"
                val time = 10.5f
                commandLine = FFmpegUtil.screenShot(srcFile, time, screenShot)
            }
            R.id.btn_water_mark//add watermark to video
            -> {
                // the unit of bitRate is kb
                var bitRate = 500
                val retriever = MediaMetadataRetriever()
                retriever.setDataSource(srcFile)
                val mBitRate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE)
                if (mBitRate != null && mBitRate.isNotEmpty()) {
                    val probeBitrate = Integer.valueOf(mBitRate)
                    bitRate = probeBitrate / 1000 / 100 * 100
                }
                //1:top left 2:top right 3:bottom left 4:bottom right
                val location = 1
                val offsetXY = 10
                when (waterMarkType) {
                    TYPE_IMAGE// image
                    -> {
                        val photo = PATH + File.separator + "hello.png"
                        val photoMark = PATH + File.separator + "photoMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, photo, location, bitRate, offsetXY, photoMark)
                    }
                    TYPE_GIF// gif
                    -> {
                        val gifPath = PATH + File.separator + "ok.gif"
                        val gifWaterMark = PATH + File.separator + "gifWaterMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkGif(srcFile, gifPath, location, bitRate, offsetXY, gifWaterMark)
                    }
                    TYPE_TEXT// text
                    -> {
                        val text = "Hello,FFmpeg"
                        val textPath = PATH + File.separator + "text.png"
                        val result = BitmapUtil.textToPicture(textPath, text, Color.BLUE, 20)
                        Log.i(TAG, "text to picture result=$result")
                        val textMark = PATH + File.separator + "textMark.mp4"
                        commandLine = FFmpegUtil.addWaterMarkImg(srcFile, textPath, location, bitRate, offsetXY, textMark)
                    }
                    else -> {
                    }
                }
            }
            R.id.btn_remove_logo//Remove logo from video: suppress logo by a simple interpolation
            -> {
                val removeLogoPath = PATH + File.separator + "removeLogo" + suffix
                val widthL = 64
                val heightL = 40
                commandLine = FFmpegUtil.removeLogo(srcFile, 10, 10, widthL, heightL, removeLogoPath)
            }
            R.id.btn_generate_gif//convert video into gif
            -> {
                val video2Gif = PATH + File.separator + "video2Gif.gif"
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
                            frameRate, width, video2Gif)
                    val cmdList = ArrayList<Array<String>>()
                    cmdList.add(paletteCmd)
                    cmdList.add(gifCmd)
                    ffmpegHandler!!.executeFFmpegCmds(cmdList)
                } else {
                    convertGifInHighQuality(video2Gif, srcFile, gifStart, gifDuration, frameRate)
                }
            }
            R.id.btn_multi_video//combine video which layout could be horizontal of vertical
            -> {
                val input1 = PATH + File.separator + "input1.mp4"
                val input2 = PATH + File.separator + "input2.mp4"
                val outputFile = PATH + File.separator + "multi.mp4"
                if (!FileUtil.checkFileExist(input1) || !FileUtil.checkFileExist(input2)) {
                    return
                }
                commandLine = FFmpegUtil.multiVideo(input1, input2, outputFile, VideoLayout.LAYOUT_HORIZONTAL)
            }
            R.id.btn_reverse_video//video reverse
            -> {
                val output = PATH + File.separator + "reverse.mp4"
                commandLine = FFmpegUtil.reverseVideo(srcFile, output)
            }
            R.id.btn_denoise_video//noise reduction of video
            -> {
                val denoise = PATH + File.separator + "denoise.mp4"
                commandLine = FFmpegUtil.denoiseVideo(srcFile, denoise)
            }
            R.id.btn_to_image//convert video to picture
            -> {
                val imagePath = PATH + File.separator + "Video2Image/"
                val imageFile = File(imagePath)
                if (!imageFile.exists()) {
                    if (!imageFile.mkdir()) {
                        return
                    }
                }
                val mStartTime = 10//start time
                val mDuration = 5//duration
                val mFrameRate = 10//frameRate
                commandLine = FFmpegUtil.videoToImage(srcFile, mStartTime, mDuration, mFrameRate, imagePath)
            }
            R.id.btn_pip//combine into picture-in-picture video
            -> {
                val inputFile1 = PATH + File.separator + "beyond.mp4"
                val inputFile2 = PATH + File.separator + "small_girl.mp4"
                if (!FileUtil.checkFileExist(inputFile1) && !FileUtil.checkFileExist(inputFile2)) {
                    return
                }
                //x and y coordinate points need to be calculated according to the size of full video and small video
                //For example: full video is 320x240, small video is 120x90, so x=200 y=150
                val x = 200
                val y = 150
                val picInPic = PATH + File.separator + "PicInPic.mp4"
                commandLine = FFmpegUtil.picInPicVideo(inputFile1, inputFile2, x, y, picInPic)
            }
            R.id.btn_moov//moov box moves ahead, which is behind mdat box of mp4 video
            -> {
                if (!srcFile.endsWith(FileUtil.TYPE_MP4)) {
                    showToast(getString(R.string.tip_not_mp4_video))
                    return
                }
                val filePath = FileUtil.getFilePath(srcFile)
                var fileName = FileUtil.getFileName(srcFile)
                Log.e(TAG, "moov filePath=$filePath--fileName=$fileName")
                fileName = "moov_" + fileName!!
                val moovPath = filePath + File.separator + fileName
                if (useFFmpegCmd) {
                    commandLine = FFmpegUtil.moveMoovAhead(srcFile, moovPath)
                } else {
                    val start = System.currentTimeMillis()
                    val ffmpegCmd = FFmpegCmd()
                    val result = ffmpegCmd.moveMoovAhead(srcFile, moovPath)
                    Log.e(TAG, "result=" + (result == 0))
                    Log.e(TAG, "move moov use time=" + (System.currentTimeMillis() - start))
                }
            }
            R.id.btn_speed//playing speed of video
            -> {
                val speed = PATH + File.separator + "speed.mp4"
                commandLine = FFmpegUtil.changeSpeed(srcFile, speed, 2f, false)
            }
            R.id.btn_flv//rebuild the keyframe index of flv
            -> {
                if (!".flv".equals(FileUtil.getFileSuffix(srcFile)!!, ignoreCase = true)) {
                    Log.e(TAG, "It's not flv file, suffix=" + FileUtil.getFileSuffix(srcFile)!!)
                    return
                }
                val outputPath = PATH + File.separator + "frame_index.flv"
                commandLine = FFmpegUtil.buildFlvIndex(srcFile, outputPath)
            }
            R.id.btn_thumbnail// insert thumbnail into video
            -> {
                val thumbnailPath = PATH + File.separator + "thumb.jpg"
                val thumbVideoPath = PATH + File.separator + "thumbnailVideo" + suffix
                commandLine = FFmpegUtil.insertPicIntoVideo(srcFile, thumbnailPath, thumbVideoPath)
            }
            R.id.btn_add_subtitle//add subtitle into video
            -> {
                val subtitlePath = PATH + File.separator + "test.ass"
                val addSubtitlePath = PATH + File.separator + "subtitle.mkv"
                commandLine = FFmpegUtil.addSubtitleIntoVideo(srcFile, subtitlePath, addSubtitlePath)
            }
            R.id.btn_rotate// set the rotate degree of video
            -> {
                val rotateDegree = 90
                val addSubtitlePath = PATH + File.separator + "rotate" + rotateDegree + suffix
                commandLine = FFmpegUtil.rotateVideo(srcFile, rotateDegree, addSubtitlePath)
            }
            R.id.btn_gop// change the gop(key frame interval) of video
            -> {
                val gop = 30
                val gopPath = PATH + File.separator + "gop" + gop + suffix
                commandLine = FFmpegUtil.changeGOP(srcFile, gop, gopPath)
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
        val resolution = "640x320"
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
        private const val useFFmpegCmd = true

        private const val TYPE_IMAGE = 1
        private const val TYPE_GIF = 2
        private const val TYPE_TEXT = 3
        private const val waterMarkType = TYPE_IMAGE
        private const val convertGifWithFFmpeg = false
    }
}
