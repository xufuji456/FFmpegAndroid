package com.frank.ffmpeg.model

import java.util.ArrayList


class LrcInfo {

    var title: String? = null
    var album: String? = null
    var artist: String? = null
    var author: String? = null
    var creator: String? = null
    var encoder: String? = null
    var version: String? = null
    var offset: Int = 0

    var lrcLineList: ArrayList<LrcLine>? = null
}
