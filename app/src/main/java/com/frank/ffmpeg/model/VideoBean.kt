package com.frank.ffmpeg.model

/**
 * the model of video data
 * Created by frank on 2020/1/7.
 */
class VideoBean {

    //"codec_tag_string": "avc1"
    var videoCodec: String? = null
        get() = if ("[0][0][0][0]" == field) {
            null
        } else field

    //"width": 640
    var width: Int = 0

    //"height": 360
    var height: Int = 0

    //"display_aspect_ratio": "16:9"
    var displayAspectRatio: String? = null

    //"pix_fmt": "yuv420p"
    var pixelFormat: String? = null

    //"profile": "578"
    var profile: String? = null

    //"level": 30
    var level: Int = 0

    //"r_frame_rate": "24000/1001"
    var frameRate: Int = 0

}
