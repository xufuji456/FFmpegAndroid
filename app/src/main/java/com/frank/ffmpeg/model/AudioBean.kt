package com.frank.ffmpeg.model

/**
 * the model of audio data
 * Created by frank on 2020/1/7.
 */
class AudioBean {

    //"codec_tag_string": "mp4a"
    var audioCodec: String? = null
        get() = if ("[0][0][0][0]" == field) {
            null
        } else field

    //"sample_rate": "44100"
    var sampleRate: Int = 0

    //"channels": 2
    var channels: Int = 0

    //"channel_layout": "stereo"
    var channelLayout: String? = null

    var title: String? = null

    var artist: String? = null

    var album: String? = null

    var albumArtist: String? = null

    var composer: String? = null

    var genre: String? = null

    var lyrics: List<String>? = null

    var lrcLineList: List<LrcLine>? = null

}
