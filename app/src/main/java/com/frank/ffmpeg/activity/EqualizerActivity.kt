package com.frank.ffmpeg.activity

import android.os.Bundle
import android.os.Environment
import android.util.Pair
import android.view.View
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.frank.ffmpeg.AudioPlayer
import com.frank.ffmpeg.R
import com.frank.ffmpeg.adapter.EqualizerAdapter
import com.frank.ffmpeg.listener.OnSeeBarListener
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
    private val bandsList = arrayListOf(
            65, 92, 131, 185, 262, 370,
            523, 740, 1047, 1480, 2093, 2960,
            4180, 5920, 8372, 11840, 16744, 20000)
    private val minEQLevel = 0
    private var filterThread: Thread? = null
    private var mAudioPlayer: AudioPlayer? = null
    private var equalizerAdapter: EqualizerAdapter? = null
    private var audioPath = Environment.getExternalStorageDirectory().path + "/tiger.mp3"

    override val layoutId: Int
        get() = R.layout.activity_equalizer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
        setupEqualizer()
        doEqualize()
    }

    private fun initView() {
        val equalizerView = findViewById<RecyclerView>(R.id.list_equalizer)
        val layoutManager = LinearLayoutManager(this)
        layoutManager.orientation = RecyclerView.VERTICAL
        equalizerView.layoutManager = layoutManager
        equalizerAdapter = EqualizerAdapter(this, this)
        equalizerView.adapter = equalizerAdapter
    }

    private fun setupEqualizer() {
        val equalizerList = ArrayList<Pair<*, *>>()
        val maxEQLevel = 20
        for (i in 0 until bandsList.size) {
            val centerFreq = bandsList[i].toString() + " Hz"
            val pair = Pair.create(centerFreq, 0)
            equalizerList.add(pair)
        }
        if (equalizerAdapter != null) {
            equalizerAdapter!!.setMaxProgress(maxEQLevel - minEQLevel)
            equalizerAdapter!!.setEqualizerList(equalizerList)
        }
        mAudioPlayer = AudioPlayer()
    }

    private fun doEqualize() {
//        val bandList = arrayListOf<String>()
//        bandList.add("6b=5")
//        bandList.add("8b=4")
//        bandList.add("10b=3")
//        bandList.add("12b=2")
//        bandList.add("14b=1")
//        bandList.add("16b=0")
        val filter = "superequalizer=6b=4:8b=5:10b=5"
        if (filterThread == null) {
            filterThread = Thread(Runnable {
                mAudioPlayer!!.play(audioPath, filter)
            })
            filterThread!!.start()
        } else {
//            mAudioPlayer!!.again(position)
        }
    }

    override fun onProgress(index: Int, progress: Int) {
        doEqualize()
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {
        audioPath = filePath
    }
}
