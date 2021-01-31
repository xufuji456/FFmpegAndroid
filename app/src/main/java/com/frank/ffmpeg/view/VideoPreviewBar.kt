package com.frank.ffmpeg.view

import android.content.Context
import android.graphics.SurfaceTexture
import android.text.TextUtils
import android.util.AttributeSet
import android.util.Log
import android.view.*
import android.widget.RelativeLayout
import android.widget.SeekBar
import android.widget.TextView

import com.frank.ffmpeg.R
import com.frank.ffmpeg.hardware.HardwareDecode
import com.frank.ffmpeg.util.ScreenUtil
import com.frank.ffmpeg.util.TimeUtil

/**
 * the custom view of preview SeekBar
 * Created by frank on 2019/11/16.
 */

class VideoPreviewBar : RelativeLayout, HardwareDecode.OnDataCallback {

    private var texturePreView: TextureView? = null

    private var previewBar: SeekBar? = null

    private var txtVideoProgress: TextView? = null

    private var txtVideoDuration: TextView? = null

    private var hardwareDecode: HardwareDecode? = null

    private var mPreviewBarCallback: PreviewBarCallback? = null

    private var duration: Int = 0

    private var screenWidth: Int = 0

    private var moveEndPos = 0

    private var previewHalfWidth: Int = 0

    constructor(context: Context) : super(context) {
        initView(context)
    }

    constructor(context: Context, attributeSet: AttributeSet) : super(context, attributeSet) {
        initView(context)
    }

    private fun initView(context: Context) {
        val view = LayoutInflater.from(context).inflate(R.layout.preview_video, this)
        previewBar = view.findViewById(R.id.preview_bar)
        texturePreView = view.findViewById(R.id.texture_preview)
        txtVideoProgress = view.findViewById(R.id.txt_video_progress)
        txtVideoDuration = view.findViewById(R.id.txt_video_duration)
        setListener()
        screenWidth = ScreenUtil.getScreenWidth(context)
    }

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        super.onLayout(changed, l, t, r, b)
        if (moveEndPos == 0) {
            val previewWidth = texturePreView!!.width
            previewHalfWidth = previewWidth / 2
            val marginEnd: Int
            val layoutParams = texturePreView!!.layoutParams as MarginLayoutParams
            marginEnd = layoutParams.marginEnd
            moveEndPos = screenWidth - previewWidth - marginEnd
            Log.i(TAG, "previewWidth=$previewWidth")
        }
    }

    private fun setPreviewCallback(filePath: String, texturePreView: TextureView) {
        texturePreView.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
                doPreview(filePath, Surface(surface))
            }

            override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {

            }

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                return false
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {

            }
        }
    }

    private fun doPreview(filePath: String, surface: Surface?) {
        if (surface == null || TextUtils.isEmpty(filePath)) {
            return
        }
        release()
        hardwareDecode = HardwareDecode(surface, filePath, this)
        hardwareDecode!!.decode()
    }

    private fun setListener() {
        previewBar!!.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                if (!fromUser) {
                    return
                }
                previewBar!!.progress = progress
                if (hardwareDecode != null && progress < duration) {
                    // us to ms
                    hardwareDecode!!.seekTo((progress * 1000).toLong())
                }
                val percent = progress * screenWidth / duration
                if (percent in (previewHalfWidth + 1) until moveEndPos && texturePreView != null) {
                    texturePreView!!.translationX = (percent - previewHalfWidth).toFloat()
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {
                if (texturePreView != null) {
                    texturePreView!!.visibility = View.VISIBLE
                }
                if (hardwareDecode != null) {
                    hardwareDecode!!.setPreviewing(true)
                }
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                if (texturePreView != null) {
                    texturePreView!!.visibility = View.GONE
                }
                if (mPreviewBarCallback != null) {
                    mPreviewBarCallback!!.onStopTracking(seekBar.progress.toLong())
                }
                if (hardwareDecode != null) {
                    hardwareDecode!!.setPreviewing(false)
                }
            }
        })
    }

    override fun onData(duration: Long) {
        //us to ms
        val durationMs = (duration / 1000).toInt()
        Log.i(TAG, "duration=$duration")
        this.duration = durationMs
        post {
            previewBar!!.max = durationMs
            txtVideoDuration!!.text = TimeUtil.getVideoTime(durationMs.toLong())
            texturePreView!!.visibility = View.GONE
        }
    }

    private fun checkArgument(videoPath: String?) {
        checkNotNull(texturePreView) { "Must init TextureView first..." }
        check(!(videoPath == null || videoPath.isEmpty())) { "videoPath is empty..." }
    }

    fun init(videoPath: String, previewBarCallback: PreviewBarCallback) {
        checkArgument(videoPath)
        this.mPreviewBarCallback = previewBarCallback
        doPreview(videoPath, Surface(texturePreView!!.surfaceTexture))
    }

    fun initDefault(videoPath: String, previewBarCallback: PreviewBarCallback) {
        checkArgument(videoPath)
        this.mPreviewBarCallback = previewBarCallback
        setPreviewCallback(videoPath, texturePreView!!)
    }

    fun updateProgress(progress: Int) {
        if (progress in 0..duration) {
            txtVideoProgress!!.text = TimeUtil.getVideoTime(progress.toLong())
            previewBar!!.progress = progress
        }
    }

    fun release() {
        if (hardwareDecode != null) {
            hardwareDecode!!.release()
            hardwareDecode = null
        }
    }

    interface PreviewBarCallback {
        fun onStopTracking(progress: Long)
    }

    companion object {

        private val TAG = VideoPreviewBar::class.java.simpleName
    }

}
