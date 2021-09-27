package com.frank.ffmpeg.activity

import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.RadioButton
import android.widget.RadioGroup
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.frank.ffmpeg.AudioPlayer
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.EqualizerAdapter
import com.frank.ffmpeg.listener.OnSeeBarListener
import com.frank.ffmpeg.view.VisualizerView
import java.lang.StringBuilder
import java.util.ArrayList

class EqualizerActivity : BaseActivity(), OnSeeBarListener {

    // unit: Hz  gain:0-20
    /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     |   1b   |   2b   |   3b   |   4b   |   5b   |   6b   |   7b   |   8b   |   9b   |
     |   65   |   92   |   131  |   185  |   262  |   370  |   523  |   740  |  1047  |
     |- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     |   10b  |   11b  |   12b  |   13b  |   14b  |   15b  |   16b  |   17b  |   18b  |
     |   1480 |   2093 |   2960 |   4186 |   5920 |   8372 |  11840 |  16744 |  20000 |
     |- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
    private val bandsList = intArrayOf(
            65, 92, 131, 185, 262, 370,
            523, 740, 1047, 1480, 2093, 2960,
            4180, 5920, 8372, 11840, 16744, 20000)

    private val selectBandList = IntArray(bandsList.size)
    private val minEQLevel = 0
    private var filterThread: Thread? = null
    private var mAudioPlayer: AudioPlayer? = null
    private var visualizerView: VisualizerView? = null
    private var equalizerAdapter: EqualizerAdapter? = null
    private var audioPath = Environment.getExternalStorageDirectory().path + "/know_play.mp3"

    override val layoutId: Int
        get() = R.layout.activity_equalizer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
        setupEqualizer()
        doEqualize()
    }

    private fun initView() {
        visualizerView = findViewById(R.id.visualizer_fft)
        val equalizerView = findViewById<RecyclerView>(R.id.list_equalizer)
        val layoutManager = LinearLayoutManager(this)
        layoutManager.orientation = RecyclerView.VERTICAL
        equalizerView.layoutManager = layoutManager
        equalizerAdapter = EqualizerAdapter(this, this)
        equalizerView.adapter = equalizerAdapter

        val effectEcho: RadioButton = findViewById(R.id.btn_effect_echo)
        val effectFunny: RadioButton = findViewById(R.id.btn_effect_funny)
        val effectTremolo: RadioButton = findViewById(R.id.btn_effect_tremolo)
        val effectLolita: RadioButton = findViewById(R.id.btn_effect_lolita)
        val effectUncle: RadioButton = findViewById(R.id.btn_effect_uncle)
        val effectGroup: RadioGroup = findViewById(R.id.group_audio_effect)
        effectGroup.setOnCheckedChangeListener { group, checkedId ->
            when (checkedId) {
                effectEcho.id -> doAudioEffect(0)
                effectFunny.id -> doAudioEffect(1)
                effectTremolo.id -> doAudioEffect(2)
                effectLolita.id -> doAudioEffect(3)
                effectUncle.id -> doAudioEffect(4)
            }
        }
    }

    private fun setupEqualizer() {
        val equalizerList = ArrayList<Pair<*, *>>()
        val maxEQLevel = 20
        for (element in bandsList) {
            val centerFreq = "$element Hz"
            val pair = Pair.create(centerFreq, 0)
            equalizerList.add(pair)
        }
        if (equalizerAdapter != null) {
            equalizerAdapter!!.setMaxProgress(maxEQLevel - minEQLevel)
            equalizerAdapter!!.setEqualizerList(equalizerList)
        }
        mAudioPlayer = AudioPlayer()
        mAudioPlayer?.setOnFftCallback {
            if (visualizerView != null && it != null) {
                visualizerView!!.post { visualizerView!!.setWaveData(it) }
            }
        }
    }

    private fun doEqualize() {
        doEqualize(0, 0)
    }

    private fun doEqualize(index: Int, progress: Int) {
        if (filterThread == null) {
            val filter = "superequalizer=6b=4:8b=5:10b=5"
            filterThread = Thread(Runnable {
                mAudioPlayer!!.play(audioPath, filter)
            })
            filterThread!!.start()
        } else {
            if (index < 0 || index >= selectBandList.size) return
            selectBandList[index] = progress
            val builder = StringBuilder()
            builder.append("superequalizer=")
            for (i in selectBandList.indices) {
                if (selectBandList[i] > 0) {
                    builder.append(i + 1).append("b=").append(selectBandList[i]).append(":")
                }
            }
            builder.deleteCharAt(builder.length - 1)
            Log.e("Equalizer", "update filter=$builder")
            mAudioPlayer!!.again(builder.toString())
        }
    }

    private fun getAudioEffect(index: Int) :String {
        return when (index) {
            0 -> "aecho=0.8:0.8:1000:0.5"
            1 -> "atempo=2"
            2 -> "tremolo=5:0.9"
            3 -> "asetrate=44100*1.4,aresample=44100,atempo=1/1.4"
            4 -> "asetrate=44100*0.6,aresample=44100,atempo=1/0.6"
            else -> {
                ""
            }
        }
    }

    private fun doAudioEffect(index: Int) {
        var effect = getAudioEffect(index)
        if (effect.isEmpty()) return
        val filter = ",superequalizer=8b=5"
        effect += filter
        if (filterThread == null) {
            filterThread = Thread(Runnable {
                mAudioPlayer!!.play(audioPath, effect)
            })
            filterThread!!.start()
        } else {
            mAudioPlayer!!.again(effect)
        }
    }

    override fun onProgress(index: Int, progress: Int) {
        doEqualize(index, progress)
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {
        audioPath = filePath
    }

    override fun onDestroy() {
        super.onDestroy()
        if (filterThread != null) {
            mAudioPlayer!!.release()
            filterThread?.interrupt()
            filterThread = null
        }
    }
}
