package com.frank.ffmpeg.activity

import android.Manifest
import android.media.MediaPlayer
import android.media.audiofx.*
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.*
import androidx.annotation.RequiresApi
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.EqualizerAdapter
import com.frank.ffmpeg.format.AudioVisualizer
import com.frank.ffmpeg.listener.OnSeeBarListener
import com.frank.ffmpeg.view.VisualizerView
import java.io.IOException
import java.util.ArrayList

/**
 * Audio effect: equalizer, enhancer, visualizer
 * Created by frank on 2020/10/20.
 */
class AudioEffectActivity : AppCompatActivity(), OnSeeBarListener {

    companion object {
        private val TAG = AudioEffectActivity::class.java.simpleName

        private val AUDIO_PATH = Environment.getExternalStorageDirectory().path + "/heart.mp3"
    }

    private var mPlayer: MediaPlayer? = null
    private var mEqualizer: Equalizer? = null
    private var mBass: BassBoost? = null
    private var mPresetReverb: PresetReverb? = null
    private val reverbValues = ArrayList<String>()
    private var seekBarList: List<SeekBar>? = ArrayList()
    private var bands: Short = 0
    private var minEQLevel: Short = 0
    private var spinnerStyle: Spinner? = null
    private var spinnerReverb: Spinner? = null
    private var barBassBoost: SeekBar? = null
    private var equalizerAdapter: EqualizerAdapter? = null
    private val enableEqualizer = true
    private var loudnessEnhancer: LoudnessEnhancer? = null
    private var barEnhancer: SeekBar? = null
    private var visualizerView: VisualizerView? = null
    private var mVisualizer: AudioVisualizer? = null

    private val presetReverb = arrayOf("None", "SmallRoom", "MediumRoom", "LargeRoom",
            "MediumHall", "LargeHall", "Plate")

    private val permissions = arrayOf(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS)

    private val onPreparedListener = MediaPlayer.OnPreparedListener {
        setupEqualizer()
        setupPresetStyle()
        // some mobiles throws error here
//        setupReverberation()
        setupBassBoost()
        setLoudnessEnhancer()
        setupVisualizer()

        mPlayer!!.start()
    }

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        checkPermission()
        setContentView(R.layout.activity_audio_effect)
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
        try {
            mPlayer = MediaPlayer()
            mPlayer!!.setDataSource(AUDIO_PATH)
            mPlayer!!.setOnPreparedListener(onPreparedListener)
            mPlayer!!.prepareAsync()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }

    override fun onProgress(index: Int, progress: Int) {
        mEqualizer!!.setBandLevel(index.toShort(), (progress + minEQLevel).toShort())
    }

    private fun setupEqualizer() {
        val equalizerList = ArrayList<Pair<*, *>>()
        mEqualizer = Equalizer(0, mPlayer!!.audioSessionId)
        mEqualizer!!.enabled = enableEqualizer
        // band level: min and max
        minEQLevel = mEqualizer!!.bandLevelRange[0]//min level
        val maxEQLevel = mEqualizer!!.bandLevelRange[1]  // max level
        bands = mEqualizer!!.numberOfBands
        for (i in 0 until bands) {
            val centerFreq = (mEqualizer!!.getCenterFreq(i.toShort()) / 1000).toString() + " Hz"
            val pair = Pair.create(centerFreq, mEqualizer!!.getBandLevel(i.toShort()) - minEQLevel)
            equalizerList.add(pair)
        }
        if (equalizerAdapter != null) {
            equalizerAdapter!!.setMaxProgress(maxEQLevel - minEQLevel)
            equalizerAdapter!!.setEqualizerList(equalizerList)
        }
    }

    private fun setupPresetStyle() {
        for (i in 0 until mEqualizer!!.numberOfPresets) {
            reverbValues.add(mEqualizer!!.getPresetName(i.toShort()))
        }

        spinnerStyle!!.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, reverbValues)
        spinnerStyle!!.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(arg0: AdapterView<*>, arg1: View, arg2: Int, arg3: Long) {
                try {
                    mEqualizer!!.usePreset(arg2.toShort())
                    if (equalizerAdapter != null) {
                        seekBarList = equalizerAdapter!!.getSeekBarList()
                    }
                    if (bands > 0 && seekBarList != null && mEqualizer != null) {
                        for (band in 0 until bands) {
                            seekBarList!![band].progress = mEqualizer!!.getBandLevel(band.toShort()) - minEQLevel
                        }
                    }

                } catch (e: Exception) {
                    Log.e(TAG, "preset style error=$e")
                }

            }
            override fun onNothingSelected(arg0: AdapterView<*>) {}
        }
    }

    private fun setupReverberation() {
        mPresetReverb = PresetReverb(0, mPlayer!!.audioSessionId)
        mPresetReverb!!.enabled = enableEqualizer
        mPlayer!!.attachAuxEffect(mPresetReverb!!.id)
        //sendLevel:0-1
        mPlayer!!.setAuxEffectSendLevel(1.0f)

        spinnerReverb!!.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, presetReverb)
        spinnerReverb!!.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(arg0: AdapterView<*>, arg1: View, arg2: Int, arg3: Long) {
                try {
                    mPresetReverb!!.preset = arg2.toShort()
                } catch (e: Exception) {
                    Log.e(TAG, "preset reverberation error=$e")
                }
            }
            override fun onNothingSelected(arg0: AdapterView<*>) {}
        }
    }

    private fun setupBassBoost() {
        mBass = BassBoost(0, mPlayer!!.audioSessionId)
        mBass!!.enabled = enableEqualizer
        // 0--1000
        barBassBoost!!.max = 1000
        barBassBoost!!.progress = 0
        barBassBoost!!.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                // set the strength of bass boost
                mBass!!.setStrength(progress.toShort())
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}

            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
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
        mVisualizer?.initVisualizer(mPlayer!!.audioSessionId, false, true, object: Visualizer.OnDataCaptureListener{
            override fun onFftDataCapture(visualizer: Visualizer?, fft: ByteArray?, samplingRate: Int) {
                if (visualizerView != null && fft != null) {
                    visualizerView!!.post { visualizerView!!.setWaveData(fft) }
                }
            }

            override fun onWaveFormDataCapture(visualizer: Visualizer?, waveform: ByteArray?, samplingRate: Int) {
            }
        })
    }

    private fun releaseVisualizer() {
        mVisualizer?.releaseVisualizer()
    }

    override fun onDestroy() {
        super.onDestroy()

        mEqualizer?.release()
        mPresetReverb?.release()
        mBass?.release()
        loudnessEnhancer?.release()
        releaseVisualizer()
        mPlayer?.release()
    }

}
