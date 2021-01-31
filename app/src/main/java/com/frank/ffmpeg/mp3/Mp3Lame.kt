package com.frank.ffmpeg.mp3

import com.frank.ffmpeg.AudioPlayer

class Mp3Lame {

    constructor() {
        AudioPlayer.lameInitDefault()
    }

    internal constructor(builder: Mp3LameBuilder) {
        initialize(builder)
    }

    private fun initialize(builder: Mp3LameBuilder) {
        AudioPlayer.lameInit(builder.inSampleRate, builder.outChannel, builder.outSampleRate,
                builder.outBitrate, builder.scaleInput, getIntForMode(builder.mode), getIntForVbrMode(builder.vbrMode), builder.quality, builder.vbrQuality, builder.abrMeanBitrate,
                builder.lowPassFreq, builder.highPassFreq, builder.id3tagTitle, builder.id3tagArtist,
                builder.id3tagAlbum, builder.id3tagYear, builder.id3tagComment)
    }

    fun encode(buffer_l: ShortArray, buffer_r: ShortArray,
               samples: Int, mp3buf: ByteArray): Int {

        return AudioPlayer.lameEncode(buffer_l, buffer_r, samples, mp3buf)
    }

    internal fun encodeBufferInterLeaved(pcm: ShortArray, samples: Int,
                                         mp3buf: ByteArray): Int {
        return AudioPlayer.encodeBufferInterleaved(pcm, samples, mp3buf)
    }

    fun flush(mp3buf: ByteArray): Int {
        return AudioPlayer.lameFlush(mp3buf)
    }

    fun close() {
        AudioPlayer.lameClose()
    }

    private fun getIntForMode(mode: Mp3LameBuilder.Mode): Int {
        return when (mode) {
            Mp3LameBuilder.Mode.STEREO -> 0
            Mp3LameBuilder.Mode.JSTEREO -> 1
            Mp3LameBuilder.Mode.MONO -> 3
            Mp3LameBuilder.Mode.DEFAULT -> 4
        }
    }

    private fun getIntForVbrMode(mode: Mp3LameBuilder.VbrMode): Int {
        return when (mode) {
            Mp3LameBuilder.VbrMode.VBR_OFF -> 0
            Mp3LameBuilder.VbrMode.VBR_RH -> 2
            Mp3LameBuilder.VbrMode.VBR_ABR -> 3
            Mp3LameBuilder.VbrMode.VBR_MTRH -> 4
            Mp3LameBuilder.VbrMode.VBR_DEFAUT -> 6
        }
    }

}
