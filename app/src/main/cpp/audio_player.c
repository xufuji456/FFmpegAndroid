//
// Created by frank on 2018/2/1.
//
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>
//封装格式
#include "libavformat/avformat.h"
//解码
#include "libavcodec/avcodec.h"
//缩放
#include "libswscale/swscale.h"
//重采样
#include "libswresample/swresample.h"
#include <android/log.h>
#include "ffmpeg_jni_define.h"

#define TAG "AudioPlayer"

#define MAX_AUDIO_FRAME_SIZE 48000 * 4

AUDIO_PLAYER_FUNC(void, play, jstring input_jstr) {
	const char* input_cstr = (*env)->GetStringUTFChars(env,input_jstr,NULL);
	LOGI(TAG, "input_cstr=%s", input_cstr);
	//注册组件
	av_register_all();
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	//打开音频文件
	if(avformat_open_input(&pFormatCtx,input_cstr,NULL,NULL) != 0){
		LOGE(TAG, "无法打开音频文件");
		return;
	}
	//获取输入文件信息
	if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
		LOGE(TAG, "无法获取输入文件信息");
		return;
	}
	//获取音频流索引位置
	int i = 0, audio_stream_idx = -1;
	for(; i < pFormatCtx->nb_streams;i++){
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			audio_stream_idx = i;
			break;
		}
	}

	//获取音频解码器
	AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
	AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
	if(codec == NULL){
		LOGE(TAG, "无法获取解码器");
		return;
	}
	//打开解码器
	if(avcodec_open2(codecCtx,codec,NULL) < 0){
		LOGE(TAG, "无法打开解码器");
		return;
	}
	//压缩数据
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	//frame->16bit 44100 PCM 统一音频采样格式与采样率
	SwrContext *swrCtx = swr_alloc();

	//输入的采样格式
	enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
	//输出采样格式16bit PCM
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入采样率
	int in_sample_rate = codecCtx->sample_rate;
	//输出采样率
	int out_sample_rate = in_sample_rate;
	//声道布局（2个声道，默认立体声stereo）
	uint64_t in_ch_layout = codecCtx->channel_layout;
	//输出的声道布局（立体声）
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

	swr_alloc_set_opts(swrCtx,
		  out_ch_layout,out_sample_fmt,out_sample_rate,
		  in_ch_layout,in_sample_fmt,in_sample_rate,
		  0, NULL);
	swr_init(swrCtx);

	//输出的声道个数
	int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

	jclass player_class = (*env)->GetObjectClass(env,thiz);
    if(!player_class){
        LOGE(TAG, "player_class not found...");
    }
	//AudioTrack对象
	jmethodID audio_track_method = (*env)->GetMethodID(env,player_class,"createAudioTrack","(II)Landroid/media/AudioTrack;");
    if(!audio_track_method){
        LOGE(TAG, "audio_track_method not found...");
    }
	jobject audio_track = (*env)->CallObjectMethod(env,thiz,audio_track_method,out_sample_rate,out_channel_nb);

	//调用play方法
	jclass audio_track_class = (*env)->GetObjectClass(env,audio_track);
	jmethodID audio_track_play_mid = (*env)->GetMethodID(env,audio_track_class,"play","()V");
	(*env)->CallVoidMethod(env,audio_track,audio_track_play_mid);

	//获取write()方法
	jmethodID audio_track_write_mid = (*env)->GetMethodID(env,audio_track_class,"write","([BII)I");

	//16bit 44100 PCM 数据
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);

	int got_frame = 0,index = 0, ret;
	//不断读取编码数据
	while(av_read_frame(pFormatCtx,packet) >= 0){
		//解码音频类型的Packet
		if(packet->stream_index == audio_stream_idx){
			//解码
			ret = avcodec_decode_audio4(codecCtx,frame,&got_frame,packet);
			if(ret < 0){
                break;
			}
			//解码一帧成功
			if(got_frame > 0){
				LOGI(TAG, "decode frame count=%d", index++);
                //音频格式转换
                swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data,frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                        frame->nb_samples, out_sample_fmt, 1);

                jbyteArray audio_sample_array = (*env)->NewByteArray(env,out_buffer_size);
                jbyte* sample_byte_array = (*env)->GetByteArrayElements(env,audio_sample_array,NULL);
                //拷贝缓冲数据
                memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
                //释放数组
                (*env)->ReleaseByteArrayElements(env,audio_sample_array,sample_byte_array,0);
                //调用AudioTrack的write方法进行播放
                (*env)->CallIntMethod(env,audio_track,audio_track_write_mid,
                        audio_sample_array,0,out_buffer_size);
                //释放局部引用
                (*env)->DeleteLocalRef(env,audio_sample_array);
                usleep(1000 * 16);
			}
		}
		av_free_packet(packet);
	}
    LOGI(TAG, "decode audio finish");
	av_frame_free(&frame);
	av_free(out_buffer);
	swr_free(&swrCtx);
	avcodec_close(codecCtx);
	avformat_close_input(&pFormatCtx);
	(*env)->ReleaseStringUTFChars(env,input_jstr,input_cstr);

}