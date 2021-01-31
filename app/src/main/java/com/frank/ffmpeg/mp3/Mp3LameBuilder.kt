package com.frank.ffmpeg.mp3

class Mp3LameBuilder internal constructor() {


    internal var inSampleRate: Int = 0
    internal var outSampleRate: Int = 0
    internal var outBitrate: Int = 0
    internal var outChannel: Int = 0
    var quality: Int = 0
    internal var vbrQuality: Int = 0
    internal var abrMeanBitrate: Int = 0
    internal var lowPassFreq: Int = 0
    internal var highPassFreq: Int = 0
    internal var scaleInput: Float = 0.toFloat()
    internal var mode: Mode
    internal var vbrMode: VbrMode

    internal var id3tagTitle: String? = null
    internal var id3tagArtist: String? = null
    internal var id3tagAlbum: String? = null
    internal var id3tagComment: String? = null
    internal var id3tagYear: String? = null


    enum class Mode {
        STEREO, JSTEREO, MONO, DEFAULT
    }

    enum class VbrMode {
        VBR_OFF, VBR_RH, VBR_MTRH, VBR_ABR, VBR_DEFAUT
    }

    init {

        this.id3tagTitle = null
        this.id3tagAlbum = null
        this.id3tagArtist = null
        this.id3tagComment = null
        this.id3tagYear = null

        this.inSampleRate = 44100
        this.outSampleRate = 0
        this.outChannel = 2
        this.outBitrate = 128
        this.scaleInput = 1f

        this.quality = 5
        this.mode = Mode.DEFAULT
        this.vbrMode = VbrMode.VBR_OFF
        this.vbrQuality = 5
        this.abrMeanBitrate = 128

        this.lowPassFreq = 0
        this.highPassFreq = 0
    }

    internal fun setQuality(quality: Int): Mp3LameBuilder {
        this.quality = quality
        return this
    }

    internal fun setInSampleRate(inSampleRate: Int): Mp3LameBuilder {
        this.inSampleRate = inSampleRate
        return this
    }

    internal fun setOutSampleRate(outSampleRate: Int): Mp3LameBuilder {
        this.outSampleRate = outSampleRate
        return this
    }

    internal fun setOutBitrate(bitrate: Int): Mp3LameBuilder {
        this.outBitrate = bitrate
        return this
    }

    internal fun setOutChannels(channels: Int): Mp3LameBuilder {
        this.outChannel = channels
        return this
    }

    internal fun setId3tagTitle(title: String): Mp3LameBuilder {
        this.id3tagTitle = title
        return this
    }

    internal fun setId3tagArtist(artist: String): Mp3LameBuilder {
        this.id3tagArtist = artist
        return this
    }

    internal fun setId3tagAlbum(album: String): Mp3LameBuilder {
        this.id3tagAlbum = album
        return this
    }

    internal fun setId3tagComment(comment: String): Mp3LameBuilder {
        this.id3tagComment = comment
        return this
    }

    internal fun setId3tagYear(year: String): Mp3LameBuilder {
        this.id3tagYear = year
        return this
    }

    internal fun setScaleInput(scaleAmount: Float): Mp3LameBuilder {
        this.scaleInput = scaleAmount
        return this
    }

    internal fun setMode(mode: Mode): Mp3LameBuilder {
        this.mode = mode
        return this
    }

    internal fun setVbrMode(mode: VbrMode): Mp3LameBuilder {
        this.vbrMode = mode
        return this
    }

    internal fun setVbrQuality(quality: Int): Mp3LameBuilder {
        this.vbrQuality = quality
        return this
    }

    internal fun setAbrMeanBitrate(bitrate: Int): Mp3LameBuilder {
        this.abrMeanBitrate = bitrate
        return this
    }

    internal fun setLowpassFreqency(freq: Int): Mp3LameBuilder {
        this.lowPassFreq = freq
        return this
    }

    internal fun setHighpassFreqency(freq: Int): Mp3LameBuilder {
        this.highPassFreq = freq
        return this
    }

    internal fun build(): Mp3Lame {
        return Mp3Lame(this)
    }

}
