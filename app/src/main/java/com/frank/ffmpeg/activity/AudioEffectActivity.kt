package com.frank.ffmpeg.activity

import android.Manifest
import android.media.MediaPlayer
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.*
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.frank.ffmpeg.controller.AudioEffectController
import com.frank.ffmpeg.listener.AudioEffectCallback
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.EqualizerAdapter
import com.frank.ffmpeg.listener.OnSeekBarListener
import com.frank.ffmpeg.util.FileUtil
import com.frank.ffmpeg.view.VisualizerView
import java.io.IOException
import java.util.ArrayList

/**
 * Audio effect: equalizer, enhancer, visualizer, bassBoost
 * Created by frank on 2020/10/20.
 */
class AudioEffectActivity : BaseActivity(), OnSeekBarListener, AudioEffectCallback {

    companion object {

        private var audioPath = Environment.getExternalStorageDirectory().path + "/tiger.mp3"
    }

    private var mPlayer: MediaPlayer? = null
    private var spinnerStyle: Spinner? = null
    private var spinnerReverb: Spinner? = null
    private var barBassBoost: SeekBar? = null
    private var equalizerAdapter: EqualizerAdapter? = null
    private var barEnhancer: SeekBar? = null
    private var visualizerView: VisualizerView? = null

    private var mAudioEffectController: AudioEffectController? = null

    private val permissions = arrayOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS)

    private val onPreparedListener = MediaPlayer.OnPreparedListener {
        mAudioEffectController = AudioEffectController(this).apply {
            setupEqualizer(mPlayer!!.audioSessionId)
            setupPresetStyle(this@AudioEffectActivity, spinnerStyle!!)
            setupBassBoost(mPlayer!!.audioSessionId, barBassBoost!!)
            setLoudnessEnhancer(mPlayer!!.audioSessionId, barEnhancer!!)
            setupVisualizer(mPlayer!!.audioSessionId)
        }

        mPlayer!!.start()
    }

    override val layoutId: Int
        get() = R.layout.activity_audio_effect

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        checkPermission()
        initView()
        initPlayer()
    }

    private fun initView() {
        spinnerStyle   = findViewById(R.id.spinner_style)
        spinnerReverb  = findViewById(R.id.spinner_reverb)
        barBassBoost   = findViewById(R.id.bar_bassboost)
        barEnhancer    = findViewById(R.id.bar_enhancer)
        visualizerView = findViewById(R.id.visualizer_view)
        val equalizerView = findViewById<RecyclerView>(R.id.equalizer_view)
        val layoutManager = LinearLayoutManager(this)
        layoutManager.orientation = RecyclerView.VERTICAL
        equalizerView.layoutManager = layoutManager
        equalizerAdapter = EqualizerAdapter(this, this)
        equalizerView.adapter = equalizerAdapter
    }

    private fun checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 456)
        }
    }

    private fun initPlayer() {
        if (!FileUtil.checkFileExist(audioPath)) {
            visualizerView?.postDelayed(Runnable { showSelectFile() }, 500)
            return
        }
        try {
            mPlayer = MediaPlayer().apply {
                setDataSource(audioPath)
                setOnPreparedListener(onPreparedListener)
                prepareAsync()
            }
        } catch (e: IOException) {
            Log.e("AudioEffect", "play error=$e")
        }
    }

    override fun onProgress(index: Int, progress: Int) {
        mAudioEffectController?.onEqualizerProgress(index, progress)
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {
        audioPath = filePath
        initPlayer()
    }

    override fun setEqualizerList(maxProgress: Int, equalizerList: ArrayList<Pair<*, *>>) {
        equalizerAdapter?.let {
            it.setMaxProgress(maxProgress)
            it.setEqualizerList(equalizerList)
        }
    }

    override fun getSeekBarList(): List<SeekBar>? {
        return equalizerAdapter?.getSeekBarList()
    }

    override fun onFFTDataCallback(fft: ByteArray?) {
        if (fft != null && visualizerView != null) {
            visualizerView!!.post { visualizerView!!.setWaveData(fft) }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        mAudioEffectController?.release()
        mPlayer?.release()
    }

}
