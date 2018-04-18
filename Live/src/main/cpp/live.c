#include <jni.h>
#include <string.h>
#include "include/x264/x264.h"
#include <android/log.h>
#include "include/rtmp/rtmp.h"
#include "include/faac/faac.h"
#include <pthread.h>
#include "queue.h"
#include <stdint.h>

#define TAG "FrankLive"
#define LOGI(format, ...) __android_log_print(ANDROID_LOG_INFO, TAG, format, ##__VA_ARGS__)
#define LOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, format, ##__VA_ARGS__)

x264_picture_t picture_in;
x264_picture_t picture_out;
int y_len, uv_len;
x264_t *video_encode_handle;
faacEncHandle *audio_encode_handle;
uint32_t start_time;
pthread_cond_t cond;
pthread_mutex_t mutex;
char *url_path;
int is_pushing = FALSE;
unsigned long inputSamples;
unsigned long maxOutputBytes;
//子线程回调给Java需要用到JavaVM
JavaVM* javaVM;
//调用类
jobject jobject_error;

/***************与Java层对应**************/
//视频编码器打开失败
const int ERROR_VIDEO_ENCODER_OPEN = 0x01;
//视频帧编码失败
const int ERROR_VIDEO_ENCODE = 0x02;
//音频编码器打开失败
const int ERROR_AUDIO_ENCODER_OPEN = 0x03;
//音频帧编码失败
const int ERROR_AUDIO_ENCODE = 0x04;
//RTMP连接失败
const int ERROR_RTMP_CONNECT = 0x05;
//RTMP连接流失败
const int ERROR_RTMP_CONNECT_STREAM = 0x06;
//RTMP发送数据包失败
const int ERROR_RTMP_SEND_PACKAT = 0x07;
/***************与Java层对应**************/

void add_rtmp_packet(RTMPPacket *pPacket);
void add_x264_body(uint8_t *buf, int len);
void add_x264_key_header(unsigned char sps[100], unsigned char pps[100], int len, int pps_len);
void add_aac_body(unsigned char *buf, int len);
void add_aac_header();

//当调用System.loadLibrary时，会回调这个方法
jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved){
    javaVM = vm;
    return JNI_VERSION_1_6;
}

//回调异常给java
void throw_error_to_java(int error_code){
    JNIEnv* env;
    (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
    jclass jclazz = (*env)->GetObjectClass(env, jobject_error);
    jmethodID jmethod = (*env)->GetMethodID(env, jclazz, "errorFromNative", "(I)V");
    (*env)->CallVoidMethod(env, jobject_error, jmethod, error_code);
    (*javaVM)->DetachCurrentThread(javaVM);
}

//推流线程
void *push_thread(void * args){
    //建立RTMP连接
    RTMP* rtmp = RTMP_Alloc();
    if(!rtmp){
        LOGE("RTMP_Alloc fail...");
        goto end;
    }
    RTMP_Init(rtmp);
    RTMP_SetupURL(rtmp, url_path);
    LOGI("url_path=%s", url_path);
    RTMP_EnableWrite(rtmp);
    rtmp->Link.timeout = 10;
    if(!RTMP_Connect(rtmp, NULL)){
        LOGE("RTMP_Connect fail...");
        throw_error_to_java(ERROR_RTMP_CONNECT);
        goto end;
    }
    LOGI("RTMP_Connect success...");
    if(!RTMP_ConnectStream(rtmp, 0)){
        LOGE("RTMP_ConnectStream fail...");
        throw_error_to_java(ERROR_RTMP_CONNECT_STREAM);
        goto end;
    }
    LOGI("RTMP_ConnectStream success...");

    //开始计时
    start_time = RTMP_GetTime();
    is_pushing = TRUE;
    //发送一个ACC HEADER
    add_aac_header();
    //循环推流
    while(is_pushing) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        //从队头去一个RTMP包出来
        RTMPPacket *packet = queue_get_first();
        if(packet){
            queue_delete_first();
            //发送rtmp包，true代表rtmp内部有缓存
            int ret = RTMP_SendPacket(rtmp, packet, TRUE);
            if(!ret){
                LOGE("RTMP_SendPacket fail...");
                RTMPPacket_Free(packet);
                pthread_mutex_unlock(&mutex);
                throw_error_to_java(ERROR_RTMP_SEND_PACKAT);
                goto end;
            }
            RTMPPacket_Free(packet);
        }
        pthread_mutex_unlock(&mutex);
    }
    end:
    LOGI("free all the thing about rtmp...");
    RTMP_Close(rtmp);
    free(rtmp);
    free(url_path);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_frank_live_LiveUtil_native_1start(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = (*env)->GetStringUTFChars(env, url_, 0);
    url_path = malloc(strlen(url) + 1);
    memset(url_path, 0, strlen(url) + 1);
    memcpy(url_path, url, strlen(url));
    //创建队列
    create_queue();
    //初始化互斥锁和条件变量
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_t push_thread_id;
    //创建消费线程推流
    pthread_create(&push_thread_id, NULL, push_thread, NULL);
    (*env)->ReleaseStringUTFChars(env, url_, url);

    jobject_error= (*env)->NewGlobalRef(env, instance);
    return 0;
}

//视频编码器x264参数配置
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_setVideoParam(JNIEnv *env, jobject instance, jint width, jint height,
                                           jint bitRate, jint frameRate) {
    y_len = width * height;
    uv_len = y_len/4;

    x264_param_t param;
    //默认设置
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_csp = X264_CSP_I420;//YUV420SP
    param.i_width = width;
    param.i_height = height;
    param.b_vfr_input = 0;//码率控制不通过timebase和timestamp，通过fps控制
    param.b_repeat_headers = 1;//每帧传入sps和pps，提高视频纠错能力
    param.i_level_idc = 51;//最大level：resolution@frameRate
    param.rc.i_rc_method = X264_RC_CRF;//控制恒定码率 CRF：恒定码率；CQP：恒定质量；ABR：平均码率
    param.rc.i_bitrate = bitRate;//码率
    param.rc.i_vbv_max_bitrate = (int) (bitRate * 1.2);//瞬间最大码率
    param.i_fps_num = (uint32_t) frameRate;//帧率分子
    param.i_fps_den = 1;//帧率分母
    param.i_timebase_num = param.i_fps_den;//时间基分子
    param.i_timebase_den = param.i_fps_num;//时间基分母
    param.i_threads = 1;//编码线程数
    //设置profile档次，"baseline"代表没有B帧
    x264_param_apply_profile(&param, "baseline");
    //初始化图像
    x264_picture_alloc(&picture_in, param.i_csp, param.i_width, param.i_height);
    //打开编码器
    video_encode_handle = x264_encoder_open(&param);
    if(video_encode_handle){
        LOGI("x264_encoder_open success...");
    } else{
        LOGE("x264_encoder_open fail...");
        throw_error_to_java(ERROR_VIDEO_ENCODER_OPEN);
    }
}

//音频编码器FAAC参数配置
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_setAudioParam(JNIEnv *env, jobject instance, jint sampleRate, jint numChannels) {
    inputSamples;
    maxOutputBytes;
    audio_encode_handle = faacEncOpen((unsigned long) sampleRate,
                                      (unsigned int) numChannels, &inputSamples, &maxOutputBytes);
    if(!audio_encode_handle){
        LOGE("faacEncOpen fail...");
        throw_error_to_java(ERROR_AUDIO_ENCODER_OPEN);
        return;
    }
    LOGI("faacEncOpen success...");
    faacEncConfigurationPtr configPtr = faacEncGetCurrentConfiguration(audio_encode_handle);
    configPtr->bandWidth = 0;
    configPtr->mpegVersion = MPEG4;//MPEG版本
    configPtr->outputFormat = 0;//包含ADTS头
    configPtr->useTns = 1;//时域噪音控制
    configPtr->useLfe = 0;
    configPtr->allowMidside = 1;
    configPtr->aacObjectType = LOW;
    configPtr->quantqual = 100;//量化
    configPtr->shortctl = SHORTCTL_NORMAL;
    int result = faacEncSetConfiguration(audio_encode_handle, configPtr);
    if(result){
        LOGI("faacEncSetConfiguration success...");
    } else{
        LOGE("faacEncSetConfiguration fail...");
        throw_error_to_java(ERROR_AUDIO_ENCODER_OPEN);
    }
}

//添加RTMPPacket包到队列中
void add_rtmp_packet(RTMPPacket *pPacket) {
    pthread_mutex_lock(&mutex);
    if(is_pushing){
        queue_append_last(pPacket);
    }
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

//添加SPS和PPS header
void add_x264_key_header(unsigned char sps[100], unsigned char pps[100], int sps_len, int pps_len) {
    int body_size = 16 + sps_len + pps_len;
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    RTMPPacket_Reset(packet);
    unsigned char* body = (unsigned char *) packet->m_body;
    int i = 0;
    body[i++] = 0x17;//VideoHeadType 0-3:FrameType(KeyFrame=1);4-7:CodecId(AVC=7)
    body[i++] = 0x00;//AVC PacketType

    body[i++] = 0x00;//composition type 24bit
    body[i++] = 0x00;
    body[i++] = 0x00;

    body[i++] = 0x01;//AVC decoder configuration record
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];

    body[i++] = 0xFF;

    body[i++] = 0xE1;//sps
    body[i++] = (unsigned char) ((sps_len >> 8) & 0xFF);
    body[i++] = (unsigned char) (sps_len & 0xFF);
    memcpy(&body[i], sps, (size_t) sps_len);
    i += sps_len;

    body[i++] = 0x01;//pps
    body[i++] = (unsigned char) ((pps_len >> 8) & 0xFF);
    body[i++] = (unsigned char) (pps_len & 0xFF);
    memcpy(&body[i], pps, (size_t) pps_len);
    i += pps_len;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    add_rtmp_packet(packet);
}

//添加x264的body
void add_x264_body(uint8_t *buf, int len) {
    if(buf[2] == 0x01){//00 00 01
        buf += 3;
        len -= 3;
    } else if (buf[3] == 0x01){//00 00 00 01
        buf += 4;
        len -= 4;
    }
    int body_size = len + 9;//
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    RTMPPacket_Reset(packet);
    unsigned char *body = (unsigned char *) packet->m_body;
    int type = buf[0] & 0x1F;
    if(type == NAL_SLICE_IDR){//关键帧
        body[0] = 0x17;
    } else{
        body[0] = 0x27;
    }
    body[1] = 0x01;
    body[2] = 0x00;
    body[3] = 0x00;
    body[4] = 0x00;

    body[5] = (unsigned char) ((len >> 24) & 0xFF);
    body[6] = (unsigned char) ((len >> 16) & 0xFF);
    body[7] = (unsigned char) ((len >> 8) & 0xFF);
    body[8] = (unsigned char) (len & 0xFF);

    memcpy(&body[9], buf, (size_t) len);
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nChannel = 0x04;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;

    add_rtmp_packet(packet);
}

//推送视频流
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_pushVideo(JNIEnv *env, jobject instance, jbyteArray data_) {
    //NV21转成YUV420P
    jbyte *nv21_buffer = (*env)->GetByteArrayElements(env, data_, NULL);
    //Y相同，直接拷贝
    memcpy(picture_in.img.plane[0], nv21_buffer, (size_t) y_len);
    jbyte *v_buffer = (jbyte *) picture_in.img.plane[2];
    jbyte *u_buffer = (jbyte *) picture_in.img.plane[1];
    int i;
    //U和V交换
    for(i=0; i<uv_len; i++){
        *(u_buffer+i) = *(nv21_buffer + y_len + 2*i + 1);
        *(v_buffer+i) = *(nv21_buffer + y_len + 2*i);
    }
    x264_nal_t *nal = NULL;
    int nal_num = -1;//NAL unit个数
    //调用h264编码
    if(x264_encoder_encode(video_encode_handle, &nal, &nal_num, &picture_in, & picture_out) < 0){
        LOGE("x264_encoder_encode fail");
        throw_error_to_java(ERROR_VIDEO_ENCODE);
        goto end;
    }
    if(nal_num <= 0){
        LOGE("nal_num <= 0");
        goto end;
    }
    //使用RTMP推流
    //关键帧（I帧）加上SPS和PPS
    int sps_len = 0, pps_len = 0;
    unsigned char sps[100];
    unsigned char pps[100];
    memset(sps, 0, 100);
    memset(pps, 0, 100);
    for (i = 0; i < nal_num; ++i) {
        if(nal[i].i_type == NAL_SPS){//sps
            sps_len = nal[i].i_payload - 4;
            memcpy(sps, nal[i].p_payload + 4, (size_t) sps_len);
        } else if(nal[i].i_type == NAL_PPS){//pps
            pps_len = nal[i].i_payload - 4;
            memcpy(pps, nal[i].p_payload + 4, (size_t) pps_len);
            add_x264_key_header(sps, pps, sps_len, pps_len);
        } else{
            add_x264_body(nal[i].p_payload, nal[i].i_payload);
        }
    }
    end:
    (*env)->ReleaseByteArrayElements(env, data_, nv21_buffer, 0);
}

//添加AAC header
void add_aac_header() {
    unsigned char *ppBuffer;
    unsigned long pSize;
    faacEncGetDecoderSpecificInfo(audio_encode_handle, &ppBuffer, &pSize);
    int body_size = (int) (2 + pSize);
    RTMPPacket* packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    RTMPPacket_Reset(packet);
    unsigned char* body = (unsigned char *) packet->m_body;
    //两字节头
    //soundFormat(4bits):10->aac;soundRate(2bits):3->44kHz;soundSize(1bit):1->pcm_16bit;soundType(1bit):1->stereo
    body[0] = 0xAF;
    //AACPacketType:0表示AAC sequence header
    body[1] = 0x00;

    memcpy(&body[2], ppBuffer, (size_t) pSize);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nChannel = 4;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    add_rtmp_packet(packet);
    free(ppBuffer);
}

//添加AAC body
void add_aac_body(unsigned char *buf, int len) {
    int body_size = 2 + len;
    RTMPPacket* packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    RTMPPacket_Reset(packet);
    unsigned char* body = (unsigned char *) packet->m_body;
    //两字节头
    //soundFormat(4bits):10->aac;soundRate(2bits):3->44kHz;soundSize(1bit):1->pcm_16bit;soundType(1bit):1->stereo
    body[0] = 0xAF;
    //PackageType(StreamType):0->AAC带ADTS头;1->AAC RAW
    body[1] = 0x01;

    memcpy(&body[2], buf, (size_t) len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nChannel = 4;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    add_rtmp_packet(packet);
    free(buf);
}

//推送音频流
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_pushAudio(JNIEnv *env, jobject instance, jbyteArray data_, jint length) {
    jbyte *data = (*env)->GetByteArrayElements(env, data_, NULL);
    int* pcm_buf;
    unsigned char* aac_buf;
    pcm_buf = malloc(inputSamples * sizeof(int));
    aac_buf = malloc(maxOutputBytes * sizeof(unsigned char));
    int count = 0;
    unsigned int buffer_size = (unsigned int) (length / 2);
    unsigned short* buf = (unsigned short*) data;
    while (count < buffer_size){
        int audio_length = (int) inputSamples;
        if((count + audio_length) >= buffer_size){
            audio_length = buffer_size - count;
        }
        int i;
        for(i=0; i<audio_length; i++){
            //每次从实时的pcm音频队列中读出量化位数为8的pcm数据
            int sample_byte = ((int16_t *)buf + count)[i];
            //用8个二进制位来表示一个采样量化点（模数转换）
            pcm_buf[i] = sample_byte << 8;
        }
        count += inputSamples;
        //调用FAAC编码，返回编码字节数
        int bytes_len = faacEncEncode(audio_encode_handle, pcm_buf, (unsigned int) audio_length, aac_buf, maxOutputBytes);
        if(bytes_len <= 0){
//            throw_error_to_java(ERROR_AUDIO_ENCODE);
            LOGE("音频编码失败...");
            continue;
        }
        add_aac_body(aac_buf, bytes_len);
    }
    (*env)->ReleaseByteArrayElements(env, data_, data, 0);
    if(pcm_buf != NULL){
        free(pcm_buf);
    }
}

//停止推流
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_native_1stop(JNIEnv *env, jobject instance) {
    is_pushing = FALSE;
    LOGI("native_1stop");
}

//释放所有资源
JNIEXPORT void JNICALL
Java_com_frank_live_LiveUtil_native_1release(JNIEnv *env, jobject instance) {
    //清除x264的picture缓存
    x264_picture_clean(&picture_in);
    x264_picture_clean(&picture_out);
    //关闭音视频编码器
    x264_encoder_close(video_encode_handle);
    faacEncClose(audio_encode_handle);
    //删除全局引用
    (*env)->DeleteGlobalRef(env, jobject_error);
    (*javaVM)->DestroyJavaVM(javaVM);
    //销毁互斥锁和条件变量
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    //退出线程
    pthread_exit(0);
}