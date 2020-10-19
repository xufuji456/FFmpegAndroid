package com.frank.ffmpeg.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Point
import android.util.AttributeSet
import android.view.View

import java.util.ArrayList

/**
 * the custom view of audio visualizer
 * Created by frank on 2020/10/19.
 */
class VisualizerView : View {

    private var upShowStyle = ShowStyle.STYLE_HOLLOW_LUMP

    private var waveData: ByteArray? = null
    private var pointList: MutableList<Point>? = null

    private var lumpPaint: Paint? = null
    private var wavePath = Path()


    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
        init()
    }

    private fun init() {
        lumpPaint = Paint()
        lumpPaint!!.isAntiAlias = true
        lumpPaint!!.color = LUMP_COLOR

        lumpPaint!!.strokeWidth = 2f
        lumpPaint!!.style = Paint.Style.STROKE
    }

    fun setWaveData(data: ByteArray) {
        this.waveData = readyData(data)
        genSamplingPoint(data)
        invalidate()
    }


    fun setStyle(upShowStyle: ShowStyle) {
        this.upShowStyle = upShowStyle
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        wavePath.reset()

        for (i in 0 until LUMP_COUNT) {
            if (waveData == null) {
                canvas.drawRect(((LUMP_WIDTH + LUMP_SPACE) * i).toFloat(),
                        (LUMP_MAX_HEIGHT - LUMP_MIN_HEIGHT).toFloat(),
                        ((LUMP_WIDTH + LUMP_SPACE) * i + LUMP_WIDTH).toFloat(),
                        LUMP_MAX_HEIGHT.toFloat(),
                        lumpPaint!!)
                continue
            }

            when (upShowStyle) {
                ShowStyle.STYLE_HOLLOW_LUMP -> drawLump(canvas, i, false)
                ShowStyle.STYLE_WAVE -> drawWave(canvas, i, false)
                else -> {
                }
            }
        }
    }

    private fun drawWave(canvas: Canvas, i: Int, reversal: Boolean) {
        if (pointList == null || pointList!!.size < 2) {
            return
        }
        val ratio = SCALE * if (reversal) -1 else 1
        if (i < pointList!!.size - 2) {
            val point = pointList!![i]
            val nextPoint = pointList!![i + 1]
            val midX = point.x + nextPoint.x shr 1
            if (i == 0) {
                wavePath.moveTo(point.x.toFloat(), LUMP_MAX_HEIGHT - point.y * ratio)
            }
            wavePath.cubicTo(midX.toFloat(), LUMP_MAX_HEIGHT - point.y * ratio,
                    midX.toFloat(), LUMP_MAX_HEIGHT - nextPoint.y * ratio,
                    nextPoint.x.toFloat(), LUMP_MAX_HEIGHT - nextPoint.y * ratio)

            canvas.drawPath(wavePath, lumpPaint!!)
        }
    }

    private fun drawLump(canvas: Canvas, i: Int, reversal: Boolean) {
        val minus = if (reversal) -1 else 1
        val top = LUMP_MAX_HEIGHT - (LUMP_MIN_HEIGHT + waveData!![i] * SCALE) * minus

        canvas.drawRect((LUMP_SIZE * i).toFloat(),
                top,
                (LUMP_SIZE * i + LUMP_WIDTH).toFloat(),
                LUMP_MAX_HEIGHT.toFloat(),
                lumpPaint!!)
    }

    /**
     * generate the waveform of sampling points
     *
     * @param data data
     */
    private fun genSamplingPoint(data: ByteArray) {
        if (upShowStyle != ShowStyle.STYLE_WAVE) {
            return
        }
        if (pointList == null) {
            pointList = ArrayList()
        } else {
            pointList!!.clear()
        }
        pointList!!.add(Point(0, 0))
        var i = WAVE_SAMPLING_INTERVAL
        while (i < LUMP_COUNT) {
            pointList!!.add(Point(LUMP_SIZE * i, waveData!![i].toInt()))
            i += WAVE_SAMPLING_INTERVAL
        }
        pointList!!.add(Point(LUMP_SIZE * LUMP_COUNT, 0))
    }

    enum class ShowStyle {
        STYLE_HOLLOW_LUMP,
        STYLE_WAVE,
        STYLE_NOTHING
    }

    companion object {

        private const val LUMP_COUNT = 128
        private const val LUMP_WIDTH = 6
        private const val LUMP_SPACE = 2
        private const val LUMP_MIN_HEIGHT = LUMP_WIDTH
        private const val LUMP_MAX_HEIGHT = 200
        private const val LUMP_SIZE = LUMP_WIDTH + LUMP_SPACE
        private val LUMP_COLOR = Color.parseColor("#6de6f6")

        private const val WAVE_SAMPLING_INTERVAL = 3

        private const val SCALE = (LUMP_MAX_HEIGHT / LUMP_COUNT).toFloat()

        private fun readyData(fft: ByteArray): ByteArray {
            val newData = ByteArray(LUMP_COUNT)
            var abs: Byte
            for (i in 0 until LUMP_COUNT) {
                abs = kotlin.math.abs(fft[i].toInt()).toByte()
                newData[i] = if (abs < 0) 127 else abs
            }
            return newData
        }
    }
}