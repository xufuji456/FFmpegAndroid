package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.content.Intent
import android.net.Uri
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast

import java.io.File
import java.util.ArrayList
import java.util.Locale

import com.frank.ffmpeg.AudioPlayer
import com.frank.ffmpeg.R
import com.frank.ffmpeg.handler.FFmpegHandler
import com.frank.ffmpeg.util.FFmpegUtil
import com.frank.ffmpeg.util.FileUtil

import com.frank.ffmpeg.handler.FFmpegHandler.MSG_INFO
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS
import java.lang.StringBuilder

/**
 * Using ffmpeg command to handle audio
 * Created by frank on 2018/1/23.
 */

class AudioHandleActivity : BaseActivity() {
    private val appendFile = PATH + File.separator + "heart.m4a"

    private var layoutAudioHandle: LinearLayout? = null
    private var layoutProgress: LinearLayout? = null
    private var txtProgress: TextView? = null
    private var viewId: Int = 0
    private var ffmpegHandler: FFmpegHandler? = null

    private val outputPath1 = PATH + File.separator + "output1.mp3"
    private val outputPath2 = PATH + File.separator + "output2.mp3"
    private var isJointing = false
    private var infoBuilder: StringBuilder? = null

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            when (msg.what) {
                MSG_BEGIN -> {
                    layoutProgress!!.visibility = View.VISIBLE
                    layoutAudioHandle!!.visibility = View.GONE
                }
                MSG_FINISH -> {
                    layoutProgress!!.visibility = View.GONE
                    layoutAudioHandle!!.visibility = View.VISIBLE
                    if (isJointing) {
                        isJointing = false
                        FileUtil.deleteFile(outputPath1)
                        FileUtil.deleteFile(outputPath2)
                    }
                    if (infoBuilder != null) {
                        Toast.makeText(this@AudioHandleActivity,
                                infoBuilder.toString(), Toast.LENGTH_LONG).show()
                        infoBuilder = null
                    }
                }
                MSG_PROGRESS -> {
                    val progress = msg.arg1
                    val duration = msg.arg2
                    if (progress > 0) {
                        txtProgress!!.visibility = View.VISIBLE
                        txtProgress!!.text = String.format(Locale.getDefault(), "%d%%", progress)
                    } else {
                        txtProgress!!.visibility = View.GONE
                    }
                }
                MSG_INFO -> {
                    if (infoBuilder == null) infoBuilder = StringBuilder()
                    infoBuilder?.append(msg.obj)
                }
                else -> {
                }
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_audio_handle

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
        ffmpegHandler = FFmpegHandler(mHandler)
    }

    private fun initView() {
        layoutProgress = getView(R.id.layout_progress)
        txtProgress = getView(R.id.txt_progress)
        layoutAudioHandle = getView(R.id.layout_audio_handle)
        initViewsWithClick(
                R.id.btn_transform,
                R.id.btn_cut,
                R.id.btn_concat,
                R.id.btn_mix,
                R.id.btn_play_audio,
                R.id.btn_play_opensl,
                R.id.btn_audio_encode,
                R.id.btn_pcm_concat,
                R.id.btn_audio_speed,
                R.id.btn_audio_echo,
                R.id.btn_audio_tremolo,
                R.id.btn_audio_denoise,
                R.id.btn_audio_equalizer,
                R.id.btn_audio_silence,
                R.id.btn_audio_volume
        )
    }

    override fun onViewClick(view: View) {
        viewId = view.id
        selectFile()
    }

    override fun onSelectedFile(filePath: String) {
        doHandleAudio(filePath)
    }

    /**
     * Using ffmpeg cmd to handle audio
     *
     * @param srcFile srcFile
     */
    private fun doHandleAudio(srcFile: String) {
        var commandLine: Array<String>? = null
        if (!FileUtil.checkFileExist(srcFile)) {
            return
        }
        if (!FileUtil.isAudio(srcFile)) {
            showToast(getString(R.string.wrong_audio_format))
            return
        }
        when (viewId) {
            R.id.btn_transform -> if (useFFmpeg) { //use FFmpeg to transform
                val transformFile = PATH + File.separator + "transformAudio.mp3"
                commandLine = FFmpegUtil.transformAudio(srcFile, transformFile)
            } else { //use MediaCodec and libmp3lame to transform
                Thread {
                    val transformPath = PATH + File.separator + "transformAudio.mp3"
                    try {
                        mHandler.obtainMessage(MSG_BEGIN).sendToTarget()
                        val clazz = Class.forName("com.frank.mp3.Mp3Converter")
                        val instance = clazz.newInstance()
                        val method = clazz.getDeclaredMethod("convertToMp3", String::class.java, String::class.java)
                        method.invoke(instance, srcFile, transformPath)
                        mHandler.obtainMessage(MSG_FINISH).sendToTarget()
                    } catch (e: Exception) {
                        Log.e("AudioHandleActivity", "convert mp3 error=" + e.message)
                    }
                }.start()
            }
            R.id.btn_cut//cut audio, it's best not include special characters
            -> {
                val suffix = FileUtil.getFileSuffix(srcFile)
                if (suffix == null || suffix.isEmpty()) {
                    return
                }
                val cutFile = PATH + File.separator + "cutAudio" + suffix
                commandLine = FFmpegUtil.cutAudio(srcFile, 10.5f, 15.0f, cutFile)
            }
            R.id.btn_concat//concat audio
            -> {
                if (!FileUtil.checkFileExist(appendFile)) {
                    return
                }
                concatAudio(srcFile)
                return
            }
            R.id.btn_mix//mix audio
            -> {
                if (!FileUtil.checkFileExist(appendFile)) {
                    return
                }
                val mixSuffix = FileUtil.getFileSuffix(srcFile)
                if (mixSuffix == null || mixSuffix.isEmpty()) {
                    return
                }
                commandLine = if (mixAudio) {
                    val mixAudio = PATH + File.separator + "mix" + mixSuffix
                    FFmpegUtil.mixAudio(srcFile, appendFile, mixAudio)
                } else {
                    val mergeAudio = PATH + File.separator + "merge" + mixSuffix
                    FFmpegUtil.mergeAudio(srcFile, appendFile, mergeAudio)
                }
            }
            R.id.btn_play_audio//use AudioTrack to play audio
            -> {
                val audioIntent = Intent(this@AudioHandleActivity, AudioPlayActivity::class.java)
                audioIntent.data = Uri.parse(srcFile)
                startActivity(audioIntent)
                return
            }
            R.id.btn_play_opensl//use OpenSL ES to play audio
            -> {
                Thread { AudioPlayer().playAudio(srcFile) }.start()
                return
            }
            R.id.btn_audio_encode//audio encode
            -> {
                val pcmFile = PATH + File.separator + "raw.pcm"
                val wavFile = PATH + File.separator + "convert.mp3"
                //sample rate, normal is 8000/16000/44100
                val sampleRate = 44100
                //channel num of pcm
                val channel = 2
                commandLine = FFmpegUtil.encodeAudio(pcmFile, wavFile, sampleRate, channel)
            }
            R.id.btn_pcm_concat//concat PCM streams
            -> {
                val srcPCM = PATH + File.separator + "audio.pcm"
                val appendPCM = PATH + File.separator + "audio.pcm"
                val concatPCM = PATH + File.separator + "concat.pcm"
                if (!FileUtil.checkFileExist(srcPCM) || !FileUtil.checkFileExist(appendPCM)) {
                    return
                }

                mHandler.obtainMessage(MSG_BEGIN).sendToTarget()
                FileUtil.concatFile(srcPCM, appendPCM, concatPCM)
                mHandler.obtainMessage(MSG_FINISH).sendToTarget()
                return
            }
            R.id.btn_audio_speed//change audio speed
            -> {
                val speed = 2.0f // funny effect, range from 0.5 to 100.0
                val speedPath = PATH + File.separator + "speed.mp3"
                commandLine = FFmpegUtil.changeAudioSpeed(srcFile, speedPath, speed)
            }
            R.id.btn_audio_echo // setting echo effect
            -> {
                val echo = 1000 // echo effect, range from 0 to 90000
                val echoPath = PATH + File.separator + "echo.mp3"
                commandLine = FFmpegUtil.audioEcho(srcFile, echo, echoPath)
            }
            R.id.btn_audio_tremolo //tremolo effect
            -> {
                val frequency = 5 // range from 0.1 to 20000.0
                val depth = 0.9f // range from 0 to 1
                val tremoloPath = PATH + File.separator + "tremolo.mp3"
                commandLine = FFmpegUtil.audioTremolo(srcFile, frequency, depth, tremoloPath)
            }
            R.id.btn_audio_denoise //audio denoise
            -> {
                val noisePath = PATH + File.separator + "denoise.mp3"
                commandLine = FFmpegUtil.audioDenoise(srcFile, noisePath)
            }
            R.id.btn_audio_equalizer // equalizer plus
            -> {
                // key:band  value:gain=[0-20]
                val bandList = arrayListOf<String>()
                bandList.add("6b=5")
                bandList.add("8b=4")
                bandList.add("10b=3")
                bandList.add("12b=2")
                bandList.add("14b=1")
                bandList.add("16b=0")
                val equalizePath = PATH + File.separator + "equalize.mp3"
                commandLine = FFmpegUtil.audioEqualizer(srcFile, bandList, equalizePath)
            }
            R.id.btn_audio_silence //silence detect
            -> {
                commandLine = FFmpegUtil.audioSilenceDetect(srcFile)
            }
            R.id.btn_audio_volume // modify volume
            -> {
                val volume = 0.5f // 0.0-1.0
                val volumePath = PATH + File.separator + "volume.mp3"
                commandLine = FFmpegUtil.audioVolume(srcFile, volume, volumePath)
            }
            else -> {
            }
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler!!.executeFFmpegCmd(commandLine)
        }
    }

    private fun concatAudio(selectedPath: String) {
        if (ffmpegHandler == null || selectedPath.isEmpty() || appendFile.isEmpty()) {
            return
        }
        isJointing = true
        val targetPath = PATH + File.separator + "concatAudio.mp3"
        val transformCmd1 = FFmpegUtil.transformAudio(selectedPath, "libmp3lame", outputPath1)
        val transformCmd2 = FFmpegUtil.transformAudio(appendFile, "libmp3lame", outputPath2)
        val fileList = ArrayList<String>()
        fileList.add(outputPath1)
        fileList.add(outputPath2)
        val jointVideoCmd = FFmpegUtil.concatAudio(fileList, targetPath)
        val commandList = ArrayList<Array<String>>()
        commandList.add(transformCmd1)
        commandList.add(transformCmd2)
        commandList.add(jointVideoCmd)
        ffmpegHandler!!.executeFFmpegCmds(commandList)
    }

    override fun onDestroy() {
        super.onDestroy()
        mHandler.removeCallbacksAndMessages(null)
    }

    companion object {

        private val PATH = Environment.getExternalStorageDirectory().path

        private const val useFFmpeg = true

        private const val mixAudio = true
    }

}
