
#include <cstring>
#include <jni.h>
#include "VideoStream.h"
#include "include/rtmp/rtmp.h"
#include "PushInterface.h"

VideoStream::VideoStream() {
    pthread_mutex_init(&mutex, 0);
}

VideoStream::~VideoStream() {
    pthread_mutex_destroy(&mutex);
    if (videoCodec) {
        x264_encoder_close(videoCodec);
        videoCodec = 0;
    }
    if (pic_in) {
        x264_picture_clean(pic_in);
        DELETE(pic_in);
    }
}

void VideoStream::setVideoEncInfo(int width, int height, int fps, int bitrate) {
    pthread_mutex_lock(&mutex);
    mWidth = width;
    mHeight = height;
    mFps = fps;
    mBitrate = bitrate;
    ySize = width * height;
    uvSize = ySize / 4;
    if (videoCodec) {
        x264_encoder_close(videoCodec);
        videoCodec = 0;
    }
    if (pic_in) {
        x264_picture_clean(pic_in);
        DELETE(pic_in);
    }

    //setting x264 params
    x264_param_t param;
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_level_idc = 32;
    //input format
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    //no B frame
    param.i_bframe = 0;
    //i_rc_method:bitrate control, CQP(constant quality), CRF(constant bitrate), ABR(average bitrate)
    param.rc.i_rc_method = X264_RC_ABR;
    //bitrate(Kbps)
    param.rc.i_bitrate = bitrate / 1000;
    //max bitrate
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    //unit:kbps
    param.rc.i_vbv_buffer_size = bitrate / 1000;

    //frame rate
    param.i_fps_num = fps;
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;
    //using fps
    param.b_vfr_input = 0;
    //key frame interval(GOP)
    param.i_keyint_max = fps * 2;
    //each key frame attaches sps/pps
    param.b_repeat_headers = 1;
    //thread number
    param.i_threads = 1;

    x264_param_apply_profile(&param, "baseline");
    //open encoder
    videoCodec = x264_encoder_open(&param);
    pic_in = new x264_picture_t;
    x264_picture_alloc(pic_in, X264_CSP_I420, width, height);
    pthread_mutex_unlock(&mutex);
}

void VideoStream::setVideoCallback(VideoCallback videoCallback) {
    this->videoCallback = videoCallback;
}

void VideoStream::encodeData(int8_t *data) {
    pthread_mutex_lock(&mutex);
    //y
    memcpy(pic_in->img.plane[0], data, ySize);
    for (int i = 0; i < uvSize; ++i) {
        //uv
        *(pic_in->img.plane[1] + i) = *(data + ySize + i * 2 + 1);
        *(pic_in->img.plane[2] + i) = *(data + ySize + i * 2);
    }

    x264_nal_t *pp_nal;
    int pi_nal;
    x264_picture_t pic_out;
    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);
    int sps_len = 0;
    int pps_len = 0;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i < pi_nal; ++i) {
        if (pp_nal[i].i_type == NAL_SPS) {
            sps_len = pp_nal[i].i_payload - 4;
            memcpy(sps, pp_nal[i].p_payload + 4, static_cast<size_t>(sps_len));
        } else if (pp_nal[i].i_type == NAL_PPS) {
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, static_cast<size_t>(pps_len));
            sendSpsPps(sps, pps, sps_len, pps_len);
        } else {
            sendFrame(pp_nal[i].i_type, pp_nal[i].p_payload, pp_nal[i].i_payload);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void VideoStream::encodeDataNew(int8_t *y_plane, int8_t *u_plane, int8_t *v_plane) {
    pthread_mutex_lock(&mutex);

    memcpy(pic_in->img.plane[0], y_plane, (size_t) ySize);
    memcpy(pic_in->img.plane[1], u_plane, (size_t) ySize / 4);
    memcpy(pic_in->img.plane[2], v_plane, (size_t) ySize / 4);

    x264_nal_t *pp_nal;
    int pi_nal;
    x264_picture_t pic_out;
    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);
    int sps_len = 0;
    int pps_len = 0;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i < pi_nal; ++i) {
        if (pp_nal[i].i_type == NAL_SPS) {
            sps_len = pp_nal[i].i_payload - 4;
            memcpy(sps, pp_nal[i].p_payload + 4, static_cast<size_t>(sps_len));
        } else if (pp_nal[i].i_type == NAL_PPS) {
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, static_cast<size_t>(pps_len));
            sendSpsPps(sps, pps, sps_len, pps_len);
        } else {
            sendFrame(pp_nal[i].i_type, pp_nal[i].p_payload, pp_nal[i].i_payload);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void VideoStream::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    int bodySize = 13 + sps_len + 3 + pps_len;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);
    int i = 0;
    //start code
    packet->m_body[i++] = 0x17;
    //type
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //version
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];
    packet->m_body[i++] = 0xFF;

    //sps
    packet->m_body[i++] = 0xE1;
    //sps len
    packet->m_body[i++] = (sps_len >> 8) & 0xff;
    packet->m_body[i++] = sps_len & 0xff;
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;

    //pps
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = (pps_len) & 0xff;
    memcpy(&packet->m_body[i], pps, pps_len);

    //video
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    packet->m_nChannel = 10;
    //sps and pps no timestamp
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    videoCallback(packet);
}

void VideoStream::sendFrame(int type, uint8_t *payload, int i_payload) {
    if (payload[2] == 0x00) {
        i_payload -= 4;
        payload += 4;
    } else {
        i_payload -= 3;
        payload += 3;
    }
    int bodySize = 9 + i_payload;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);

    packet->m_body[0] = 0x27;
    if (type == NAL_SLICE_IDR) {
        packet->m_body[0] = 0x17;
        if (DEBUG) {
            LOGI("IDR key frame");
        }
    }
    //packet type
    packet->m_body[1] = 0x01;
    //timestamp
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //packet len
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = (i_payload) & 0xff;

    memcpy(&packet->m_body[9], payload, static_cast<size_t>(i_payload));

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    videoCallback(packet);
}
