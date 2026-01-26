package com.frank.ffmpeg.util

import com.frank.next.player.IPlayer
import com.frank.ffmpeg.view.PlayerView.Companion.RENDER_TYPE_SURFACE_VIEW
import com.frank.ffmpeg.view.PlayerView.Companion.RENDER_TYPE_TEXTURE_VIEW

import java.util.Locale

/**
 * Note: debug util of player
 * Date: 2026/1/26 21:21
 * Author: frank
 */

object PlayerUtil {

    private fun formatDuration(duration: Long): String {
        return when {
            duration >= 1000 -> {
                String.format(Locale.US, "%.2f sec", duration.toFloat() / 1000)
            }

            else -> {
                String.format(Locale.US, "%d msec", duration)
            }
        }
    }

    private fun formatSize(bytes: Long): String {
        return when {
            bytes >= 100 * 1024 -> {
                String.format(Locale.US, "%.2f MB", bytes.toFloat() / 1024 / 1024)
            }

            bytes >= 100 -> {
                String.format(Locale.US, "%.1f KB", bytes.toFloat() / 1024)
            }

            else -> {
                String.format(Locale.US, "%d B", bytes)
            }
        }
    }

    fun getDebugInfo(player: IPlayer, renderViewType: Int): Pair<String, String> {
        val videoCodec = player.videoCodecInfo
        val audioCodec = player.audioCodecInfo
        val resolution =
            String.format(Locale.US, "%d * %d", player.videoWidth, player.videoHeight)
        val fps = String.format(
            Locale.ENGLISH,
            "%d / %d / %d",
            player.videoDecodeFrameRate.toInt(),
            player.videoRenderFrameRate.toInt(),
            player.videoFrameRate.toInt()
        )
        val bitRate = String.format(Locale.US, "%.2f kbps", player.bitRate / 1000f)
        val vCache  = String.format(
            Locale.US,
            "%s / %s",
            formatDuration(player.videoCacheTime),
            formatSize(player.getVideoCacheSize()))
        val aCache = String.format(
            Locale.US,
            "%s / %s",
            formatDuration(player.audioCacheTime),
            formatSize(player.getAudioCacheSize())
        )
        val seekTime = player.seekCostTime
        val surfaceType = if (renderViewType == RENDER_TYPE_SURFACE_VIEW) {
            "SurfaceView"
        } else if (renderViewType == RENDER_TYPE_TEXTURE_VIEW) {
            "TextureView"
        } else {
            "Unknown"
        }
        val url = player.playUrl
        val debugInfoName =
            "video_codec\n" + "audio_codec\n" + "resolution\n" + "fps\n" + "bitrate\n" + "v_cache\n" + "a_cache\n" + "seek_time\n" + "surface\n" + "url"
        val debugInfoValue =
            "${videoCodec}\n" + "${audioCodec}\n" + "${resolution}\n" + "${fps}\n" + "${bitRate}\n" + "${vCache}\n" + "${aCache}\n" + "${seekTime}ms\n" + "${surfaceType}\n" + url
        return Pair(debugInfoName, debugInfoValue)
    }
}