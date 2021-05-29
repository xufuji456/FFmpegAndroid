package com.frank.ffmpeg.tool

import android.text.TextUtils
import android.util.Log

import com.frank.ffmpeg.model.AudioBean
import com.frank.ffmpeg.model.MediaBean
import com.frank.ffmpeg.model.VideoBean

import org.json.JSONObject

/**
 * the tool of parsing json
 * Created by frank on 2020/1/8.
 */
object JsonParseTool {

    private val TAG = JsonParseTool::class.java.simpleName

    private const val TYPE_VIDEO = "video"

    private const val TYPE_AUDIO = "audio"

    fun parseMediaFormat(mediaFormat: String?): MediaBean? {
        if (mediaFormat == null || mediaFormat.isEmpty()) {
            return null
        }
        var mediaBean: MediaBean? = null
        try {
            val jsonMedia = JSONObject(mediaFormat)
            val jsonMediaFormat = jsonMedia.getJSONObject("format")
            mediaBean = MediaBean()
            val streamNum = jsonMediaFormat.optInt("nb_streams")
            mediaBean.streamNum = streamNum
            Log.e(TAG, "streamNum=$streamNum")
            val formatName = jsonMediaFormat.optString("format_name")
            mediaBean.formatName = formatName
            Log.e(TAG, "formatName=$formatName")
            val bitRateStr = jsonMediaFormat.optString("bit_rate")
            if (!TextUtils.isEmpty(bitRateStr)) {
                mediaBean.bitRate = Integer.valueOf(bitRateStr)
            }
            Log.e(TAG, "bitRate=$bitRateStr")
            val sizeStr = jsonMediaFormat.optString("size")
            if (!TextUtils.isEmpty(sizeStr)) {
                mediaBean.size = java.lang.Long.valueOf(sizeStr)
            }
            Log.e(TAG, "size=$sizeStr")
            val durationStr = jsonMediaFormat.optString("duration")
            if (!TextUtils.isEmpty(durationStr)) {
                val duration = java.lang.Float.valueOf(durationStr)
                mediaBean.duration = duration.toLong()
            }

            val jsonMediaStream = jsonMedia.getJSONArray("streams") ?: return mediaBean
            for (index in 0 until jsonMediaStream.length()) {
                val jsonMediaStreamItem = jsonMediaStream.optJSONObject(index) ?: continue
                val codecType = jsonMediaStreamItem.optString("codec_type") ?: continue
                if (codecType == TYPE_VIDEO) {
                    val videoBean = VideoBean()
                    mediaBean.videoBean = videoBean
                    val codecName = jsonMediaStreamItem.optString("codec_tag_string")
                    videoBean.videoCodec = codecName
                    Log.e(TAG, "codecName=$codecName")
                    val width = jsonMediaStreamItem.optInt("width")
                    videoBean.width = width
                    val height = jsonMediaStreamItem.optInt("height")
                    videoBean.height = height
                    Log.e(TAG, "width=$width--height=$height")
                    val aspectRatio = jsonMediaStreamItem.optString("display_aspect_ratio")
                    videoBean.displayAspectRatio = aspectRatio
                    Log.e(TAG, "aspectRatio=$aspectRatio")
                    val pixelFormat = jsonMediaStreamItem.optString("pix_fmt")
                    videoBean.pixelFormat = pixelFormat
                    Log.e(TAG, "pixelFormat=$pixelFormat")
                    val profile = jsonMediaStreamItem.optString("profile")
                    videoBean.profile = profile
                    val level = jsonMediaStreamItem.optInt("level")
                    videoBean.level = level
                    Log.e(TAG, "profile=$profile--level=$level")
                    val frameRateStr = jsonMediaStreamItem.optString("r_frame_rate")
                    if (!TextUtils.isEmpty(frameRateStr)) {
                        val frameRateArray = frameRateStr.split("/".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
                        val frameRate = Math.ceil(java.lang.Double.valueOf(frameRateArray[0]) / java.lang.Double.valueOf(frameRateArray[1]))
                        Log.e(TAG, "frameRate=" + frameRate.toInt())
                        videoBean.frameRate = frameRate.toInt()
                    }
                } else if (codecType == TYPE_AUDIO) {
                    val audioBean = AudioBean()
                    mediaBean.audioBean = audioBean
                    val codecName = jsonMediaStreamItem.optString("codec_tag_string")
                    audioBean.audioCodec = codecName
                    Log.e(TAG, "codecName=$codecName")
                    val sampleRateStr = jsonMediaStreamItem.optString("sample_rate")
                    if (!TextUtils.isEmpty(sampleRateStr)) {
                        audioBean.sampleRate = Integer.valueOf(sampleRateStr)
                    }
                    Log.e(TAG, "sampleRate=$sampleRateStr")
                    val channels = jsonMediaStreamItem.optInt("channels")
                    audioBean.channels = channels
                    Log.e(TAG, "channels=$channels")
                    val channelLayout = jsonMediaStreamItem.optString("channel_layout")
                    audioBean.channelLayout = channelLayout
                    Log.e(TAG, "channelLayout=$channelLayout")
                    val audioTag = jsonMediaFormat.getJSONObject("tags")
                    if (audioTag != null) {
                        parseTag(audioTag, audioBean)
                    }
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "parse error=$e")
        }

        return mediaBean
    }

    fun stringFormat(mediaBean: MediaBean?): String? {
        if (mediaBean == null) {
            return null
        }
        val formatBuilder = StringBuilder()
        formatBuilder.append("duration:").append(mediaBean.duration).append("\n")
        formatBuilder.append("size:").append(mediaBean.size).append("\n")
        formatBuilder.append("bitRate:").append(mediaBean.bitRate).append("\n")
        formatBuilder.append("formatName:").append(mediaBean.formatName).append("\n")
        formatBuilder.append("streamNum:").append(mediaBean.streamNum).append("\n")
        if (mediaBean.videoBean != null) {
            val videoBean = mediaBean.videoBean
            formatBuilder.append("width:").append(videoBean!!.width).append("\n")
            formatBuilder.append("height:").append(videoBean.height).append("\n")
            if (!TextUtils.isEmpty(videoBean.displayAspectRatio)) {
                formatBuilder.append("aspectRatio:").append(videoBean.displayAspectRatio).append("\n")
            }
            formatBuilder.append("pixelFormat:").append(videoBean.pixelFormat).append("\n")
            formatBuilder.append("frameRate:").append(videoBean.frameRate).append("\n")
            if (videoBean.videoCodec != null) {
                formatBuilder.append("videoCodec:").append(videoBean.videoCodec).append("\n")
            }
        }
        if (mediaBean.audioBean != null) {
            val audioBean = mediaBean.audioBean
            formatBuilder.append("sampleRate:").append(audioBean!!.sampleRate).append("\n")
            formatBuilder.append("channels:").append(audioBean.channels).append("\n")
            formatBuilder.append("channelLayout:").append(audioBean.channelLayout).append("\n")
            if (audioBean.audioCodec != null) {
                formatBuilder.append("audioCodec:").append(audioBean.audioCodec).append("\n")
            }
            if (!TextUtils.isEmpty(audioBean.title)) {
                formatBuilder.append("title:").append(audioBean.title).append("\n")
            }
            if (!TextUtils.isEmpty(audioBean.artist)) {
                formatBuilder.append("artist:").append(audioBean.artist).append("\n")
            }
            if (!TextUtils.isEmpty(audioBean.album)) {
                formatBuilder.append("album:").append(audioBean.album).append("\n")
            }
            if (!TextUtils.isEmpty(audioBean.composer)) {
                formatBuilder.append("composer:").append(audioBean.composer).append("\n")
            }
            if (!TextUtils.isEmpty(audioBean.genre)) {
                formatBuilder.append("genre:").append(audioBean.genre).append("\n")
            }
        }
        return formatBuilder.toString()
    }

    private fun parseTag(audioTag: JSONObject, audioBean: AudioBean) {
        val title = audioTag.optString("title")
        audioBean.title = title
        Log.e(TAG, "title=$title")
        val artist = audioTag.optString("artist")
        audioBean.artist = artist
        val album = audioTag.optString("album")
        audioBean.album = album
        val albumArtist = audioTag.optString("album_artist")
        audioBean.albumArtist = albumArtist
        val composer = audioTag.optString("composer")
        audioBean.composer = composer
        val genre = audioTag.optString("genre")
        audioBean.genre = genre
        val lyrics = audioTag.optString("lyrics-eng")
        if (lyrics != null && lyrics.contains("\r\n")) {
            val array = lyrics.split("\r\n")
            audioBean.lyrics = array
        }
    }

}
