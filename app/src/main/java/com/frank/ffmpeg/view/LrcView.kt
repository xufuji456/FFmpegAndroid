package com.frank.ffmpeg.view

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Paint.Align
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.annotation.IntDef

import com.frank.ffmpeg.listener.OnLrcListener
import com.frank.ffmpeg.model.LrcLine
import kotlin.math.abs

/**
 * LrcView: display lyrics with playing time, seek and sync
 */

class LrcView(context: Context, attr: AttributeSet) : View(context, attr) {

    private val mPadding = 10

    private val mLrcFontSize = 45

    private var mHighLightRow = 0

    private var mLastMotionY = 0f

    private var currentMillis: Long = 0

    private var mLrcLines: List<LrcLine>? = null

    private var mLrcViewListener: OnLrcListener? = null

    private val mPaint: Paint = Paint(Paint.ANTI_ALIAS_FLAG)

    private val mNormalRowColor = Color.BLACK

    private val mHighLightRowColor = Color.BLUE

    private var mDisplayMode = DISPLAY_MODE_NORMAL

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(MODE_HIGH_LIGHT_NORMAL, MODE_HIGH_LIGHT_KARAOKE)
    annotation class HighLightMode

    @HighLightMode
    private var mode = MODE_HIGH_LIGHT_NORMAL


    init {
        mPaint.textSize = mLrcFontSize.toFloat()
    }

    override fun onDraw(canvas: Canvas) {
        val height = height
        val width = width
        if (mLrcLines == null || mLrcLines!!.isEmpty()) {
            mPaint.color = mHighLightRowColor
            mPaint.textSize = mLrcFontSize.toFloat()
            mPaint.textAlign = Align.CENTER
            canvas.drawText(noLrcTip, (width / 2).toFloat(), (height / 2 - mLrcFontSize).toFloat(), mPaint)
            return
        }

        var rowY: Int
        val rowX = width / 2
        var rowNum: Int = mHighLightRow - 1

        // 1: current highlight lyrics
        val highlightRowY = height / 2 - mLrcFontSize

        if (mode == MODE_HIGH_LIGHT_KARAOKE) {
            // highlight one by one
            drawKaraokeHighLightLrcRow(canvas, width, rowX, highlightRowY)
        } else {
            drawHighLrcRow(canvas, height, rowX, highlightRowY)
        }

        if (mDisplayMode == DISPLAY_MODE_SEEK) {
            mPaint.color = mSeekLineColor
            val mSeekLinePaddingX = 0
            canvas.drawLine(mSeekLinePaddingX.toFloat(), (highlightRowY + mPadding).toFloat(),
                    (width - mSeekLinePaddingX).toFloat(), (highlightRowY + mPadding).toFloat(), mPaint)
            mPaint.color = mSeekLineTextColor
            mPaint.textSize = mSeekLineTextSize.toFloat()
            mPaint.textAlign = Align.LEFT
            canvas.drawText(mLrcLines!![mHighLightRow].timeString!!, 0f, highlightRowY.toFloat(), mPaint)
        }

        // lyrics above the highlight one
        mPaint.color = mNormalRowColor
        mPaint.textSize = mLrcFontSize.toFloat()
        mPaint.textAlign = Align.CENTER
        rowY = highlightRowY - mPadding - mLrcFontSize
        while (rowY > -mLrcFontSize && rowNum >= 0) {
            val text = mLrcLines!![rowNum].content
            canvas.drawText(text!!, rowX.toFloat(), rowY.toFloat(), mPaint)
            rowY -= mPadding + mLrcFontSize
            rowNum--
        }

        // lyrics below the highlight one
        rowNum = mHighLightRow + 1
        rowY = highlightRowY + mPadding + mLrcFontSize
        while (rowY < height && rowNum < mLrcLines!!.size) {
            val text = mLrcLines!![rowNum].content
            canvas.drawText(text!!, rowX.toFloat(), rowY.toFloat(), mPaint)
            rowY += mPadding + mLrcFontSize
            rowNum++
        }
    }

    private fun drawKaraokeHighLightLrcRow(canvas: Canvas, width: Int, rowX: Int, highlightRowY: Int) {
        if (width <= 0 || rowX <= 0 || highlightRowY <= 0) {
            return
        }
        val highLrcLine = mLrcLines!![mHighLightRow]
        val highlightText = highLrcLine.content
        if (highlightText.isNullOrEmpty()) return

        mPaint.color = mNormalRowColor
        mPaint.textSize = mLrcFontSize.toFloat()
        mPaint.textAlign = Align.CENTER
        canvas.drawText(highlightText, rowX.toFloat(), highlightRowY.toFloat(), mPaint)

        val highLineWidth = mPaint.measureText(highlightText).toInt()
        val leftOffset = (width - highLineWidth) / 2
        val start = highLrcLine.startTime
        val end = highLrcLine.endTime
        val highWidth = ((currentMillis - start) * 1.0f / (end - start) * highLineWidth).toInt()
        if (highWidth > 0 && highWidth < Integer.MAX_VALUE) {
            mPaint.color = mHighLightRowColor
            val textBitmap = Bitmap.createBitmap(highWidth, highlightRowY + mPadding, Bitmap.Config.ARGB_8888)
            val textCanvas = Canvas(textBitmap)
            textCanvas.drawText(highlightText, (highLineWidth / 2).toFloat(), highlightRowY.toFloat(), mPaint)
            canvas.drawBitmap(textBitmap, leftOffset.toFloat(), 0f, mPaint)
        }
    }

    private fun drawHighLrcRow(canvas: Canvas, height: Int, rowX: Int, highlightRowY: Int) {
        val highlightText = mLrcLines!![mHighLightRow].content
        mPaint.color = mHighLightRowColor
        mPaint.textSize = mLrcFontSize.toFloat()
        mPaint.textAlign = Align.CENTER
        canvas.drawText(highlightText!!, rowX.toFloat(), highlightRowY.toFloat(), mPaint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (mLrcLines == null || mLrcLines!!.isEmpty()) {
            return super.onTouchEvent(event)
        }
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                mLastMotionY = event.y
                invalidate()
            }
            MotionEvent.ACTION_MOVE -> doSeek(event)
            MotionEvent.ACTION_CANCEL,

            MotionEvent.ACTION_UP -> {
                if (mDisplayMode == DISPLAY_MODE_SEEK) {
                    seekLrc(mHighLightRow, true)
                }
                mDisplayMode = DISPLAY_MODE_NORMAL
                invalidate()
            }
        }
        return true
    }

    /**
     * Moving lyrics, when touch and move the screen
     */
    private fun doSeek(event: MotionEvent) {
        val y = event.y
        val offsetY = y - mLastMotionY
        if (abs(offsetY) < mMinSeekFiredOffset) {
            return
        }
        mDisplayMode = DISPLAY_MODE_SEEK
        val rowOffset = abs(offsetY.toInt() / mLrcFontSize)

        if (offsetY < 0) {
            mHighLightRow += rowOffset
        } else if (offsetY > 0) {
            mHighLightRow -= rowOffset
        }
        mHighLightRow = 0.coerceAtLeast(mHighLightRow)
        mHighLightRow = mHighLightRow.coerceAtMost(mLrcLines!!.size - 1)
        if (rowOffset > 0) {
            mLastMotionY = y
            invalidate()
        }
    }

    fun setListener(listener: OnLrcListener) {
        mLrcViewListener = listener
    }

    fun setLrc(lrcLines: List<LrcLine>) {
        mLrcLines = lrcLines
        invalidate()
    }

    fun setHighLightMode(@HighLightMode mode: Int) {
        this.mode = mode
    }

    private fun seekLrc(position: Int, cb: Boolean) {
        if (mLrcLines == null || position < 0 || position > mLrcLines!!.size) {
            return
        }
        val lrcLine = mLrcLines!![position]
        mHighLightRow = position
        invalidate()
        if (mLrcViewListener != null && cb) {
            mLrcViewListener!!.onLrcSeek(position, lrcLine)
        }
    }

    fun seekToTime(time: Long) {
        if (mLrcLines == null || mLrcLines!!.isEmpty()) {
            return
        }
        if (mDisplayMode != DISPLAY_MODE_NORMAL) {
            return
        }
        currentMillis = time

        for (i in mLrcLines!!.indices) {
            val current = mLrcLines!![i]
            val next = if (i + 1 == mLrcLines!!.size) null else mLrcLines!![i + 1]
            if (time >= current.startTime && next != null && time < next.startTime || time > current.startTime && next == null) {
                seekLrc(i, false)
                return
            }
        }
    }

    companion object {

        private const val mSeekLineTextSize = 25

        private const val mSeekLineColor = Color.RED

        private const val mSeekLineTextColor = Color.BLUE

        /**
         * Mode normal
         */
        const val DISPLAY_MODE_NORMAL = 0
        /**
         * Mode seek
         */
        const val DISPLAY_MODE_SEEK = 1

        /**
         * Minimum seeking distance
         */
        private const val mMinSeekFiredOffset = 10

        private const val noLrcTip = "No lyrics..."

        const val MODE_HIGH_LIGHT_NORMAL = 0

        const val MODE_HIGH_LIGHT_KARAOKE = 1
    }
}
