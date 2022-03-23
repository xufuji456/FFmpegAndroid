package com.frank.ffmpeg.activity

import android.Manifest
import android.media.MediaPlayer
import android.media.audiofx.*
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.*
import androidx.annotation.RequiresApi
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.frank.androidmedia.controller.AudioEffectController
import com.frank.androidmedia.listener.AudioEffectCallback
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.EqualizerAdapter
import com.frank.ffmpeg.format.AudioVisualizer
import com.frank.ffmpeg.listener.OnSeeBarListener
import com.frank.ffmpeg.util.FileUtil
import com.frank.ffmpeg.view.VisualizerView
import java.io.IOException
import java.util.ArrayList

/**
 * Audio effect: equalizer, enhancer, visualizer, bassBoost
 * Created by frank on 2020/10/20.
 */
class AudioEffectActivity : BaseActivity(), OnSeeBarListener, AudioEffectCallback {

    companion object {
        private val TAG = AudioEffectActivity::class.java.simpleName

        private val audioPath = Environment.getExternalStorageDirectory().path + "/tiger.mp3"
    }

    private var mPlayer: MediaPlayer? = null
    private var spinnerStyle: Spinner? = null
    private var spinnerReverb: Spinner? = null
    private var barBassBoost: SeekBar? = null
    private var equalizerAdapter: EqualizerAdapter? = null
    private var loudnessEnhancer: LoudnessEnhancer? = null
    private var barEnhancer: SeekBar? = null
    private var visualizerView: VisualizerView? = null
    private var mVisualizer: AudioVisualizer? = null

    private var audioEffectController: AudioEffectController? = null

    private val permissions = arrayOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS)

    private val onPreparedListener = MediaPlayer.OnPreparedListener {
        audioEffectController = AudioEffectController(this)
        audioEffectController?.setupEqualizer(mPlayer!!.audioSessionId)
        audioEffectController?.setupPresetStyle(this@AudioEffectActivity, spinnerStyle!!)
        audioEffectController?.setupBassBoost(mPlayer!!.audioSessionId, barBassBoost!!)
        setLoudnessEnhancer()
        setupVisualizer()

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
            mPlayer = MediaPlayer()
            mPlayer!!.setDataSource(audioPath)
            mPlayer!!.setOnPreparedListener(onPreparedListener)
            mPlayer!!.prepareAsync()
        } catch (e: IOException) {
            Log.e("AudioEffect", "play error=$e")
        }
    }

    override fun onProgress(index: Int, progress: Int) {
        audioEffectController?.onEqualizerProgress(index, progress)
    }

    @RequiresApi(Build.VERSION_CODES.KITKAT)
    private fun setLoudnessEnhancer() {
        loudnessEnhancer = LoudnessEnhancer(mPlayer!!.audioSessionId)
        loudnessEnhancer!!.enabled = true
        // Unit: mB
        loudnessEnhancer!!.setTargetGain(500)
        barEnhancer!!.max = 1000
        barEnhancer!!.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    loudnessEnhancer!!.setTargetGain(progress)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
            }
        })
    }

    private fun setupVisualizer() {
        mVisualizer = AudioVisualizer()
        mVisualizer?.initVisualizer(mPlayer!!.audioSessionId, false, true, object : Visualizer.OnDataCaptureListener {
            override fun onFftDataCapture(visualizer: Visualizer?, fft: ByteArray?, samplingRate: Int) {
                if (visualizerView != null && fft != null) {
                    visualizerView!!.post { visualizerView!!.setWaveData(fft) }
                }
            }

            override fun onWaveFormDataCapture(visualizer: Visualizer?, waveform: ByteArray?, samplingRate: Int) {
            }
        })
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {

    }

    private fun releaseVisualizer() {
        mVisualizer?.releaseVisualizer()
    }

    override fun setEqualizerList(maxProgress: Int, equalizerList: ArrayList<Pair<*, *>>) {
        if (equalizerAdapter != null) {
            equalizerAdapter!!.setMaxProgress(maxProgress)
            equalizerAdapter!!.setEqualizerList(equalizerList)
        }
    }

    override fun getSeeBarList(): List<SeekBar>? {
        return equalizerAdapter?.getSeekBarList()
    }

    override fun onDestroy() {
        super.onDestroy()

        audioEffectController?.release()
        loudnessEnhancer?.release()
        releaseVisualizer()
        mPlayer?.release()
    }

}
