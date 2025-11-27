/**
 * Note: error code of next player
 * Date: 2025/11/27
 * Author: frank
 */

#ifndef NEXT_ERROR_CODE_H
#define NEXT_ERROR_CODE_H

/**********************结果成功**********************/
#define RESULT_OK                 0       // 成功

/**********************URL相关**********************/
#define ERROR_URL_INVALID         (-1001) // 无效URL
#define ERROR_URL_TIMESTAMP       (-1002) // 时间戳过期
#define ERROR_URL_AUTH            (-1003) // 鉴权失败

/**********************网络相关**********************/
#define ERROR_NET_DNS             (-2001) // DNS解析失败
#define ERROR_NET_NO_NETWORK      (-2002) // 无网络
#define ERROR_NET_CON_TIMEOUT     (-2003) // 网络连接超时
#define ERROR_NET_READ_TIMEOUT    (-2004) // 网络读取超时
#define ERROR_NET_CHANGED         (-2005) // 网络状态变化，比如WiFi切移动网络
#define ERROR_NET_HTTP401         (-2006) // HTTP请求未授权
#define ERROR_NET_HTTP403         (-2007) // HTTP请求禁止
#define ERROR_NET_HTTP404         (-2008) // HTTP请求不存在
#define ERROR_NET_SERVER_INNER    (-2009) // 服务器内部错误
#define ERROR_NET_NO_MATCH_STREAM (-2010) // 无匹配清晰度的码流
#define ERROR_NET_VIDEO_NOT_FOUND (-2011) // 视频不存在
#define ERROR_NET_REDIRECT        (-2012) // URL重定向

/**********************解析相关**********************/
#define ERROR_PARSE_OPEN          (-3001) // 打开失败
#define ERROR_PARSE_FIND_STREAM   (-3002) // 解析流信息失败
#define ERROR_PARSE_FORMAT        (-3003) // 格式不支持
#define ERROR_PARSE_INVALID_DATA  (-3004) // 无效数据
#define ERROR_PARSE_STREAM_OPEN   (-3005) // 流打开失败
#define ERROR_PARSE_READ_FRAME    (-3006) // 读数据失败
#define ERROR_PARSE_SWITCH_VIDEO  (-3007) // 切视频流失败
#define ERROR_PARSE_SWITCH_AUDIO  (-3008) // 切音频轨失败
#define ERROR_PARSE_SWITCH_SUB    (-3009) // 切字幕轨失败
#define ERROR_PARSE_NOT_INIT      (-3010) // 解析器未初始化
#define ERROR_PARSE_METADATA      (-3011) // 解析元数据出错

/**********************解码相关**********************/
#define ERROR_DECODE_VIDEO_NONE   (-4001) // 视频编码格式不支持
#define ERROR_DECODE_AUDIO_NONE   (-4002) // 音频编码格式不支持
#define ERROR_DECODE_SUB_NONE     (-4003) // 字幕编码格式不支持
#define ERROR_DECODE_VIDEO_OPEN   (-4004) // 视频解码器打开失败
#define ERROR_DECODE_AUDIO_OPEN   (-4005) // 音频解码器打开失败
#define ERROR_DECODE_SUB_OPEN     (-4006) // 字幕解码器打开失败
#define ERROR_DECODE_INVALID      (-4007) // 初始化校验:无效参数
#define ERROR_DECODE_VIDEO_DEC    (-4008) // 视频解码失败
#define ERROR_DECODE_AUDIO_DEC    (-4009) // 音频解码失败
#define ERROR_DECODE_SUB_DEC      (-4010) // 字幕解码失败
#define ERROR_DECODE_NOT_INIT     (-4011) // 解码相关未初始化
#define ERROR_DECODE_BAD_DATA     (-4012) // 数据损坏
#define ERROR_DECODE_MISS_REF     (-4013) // 缺失参考帧
#define ERROR_DECODE_SESSION      (-4014) // session失效，比如前后台切换
#define ERROR_DECODE_PTS_ORDER    (-4015) // pts排序出错
#define ERROR_DECODER_UNSUPPORTED (-4016) // 解码器存在，参数不支持

/**********************渲染相关**********************/
#define ERROR_RENDER_VIDEO_INIT   (-5001) // 渲染器初始化失败
#define ERROR_RENDER_AUDIO_INIT   (-5002) // 音频初始化失败
#define ERROR_RENDER_HANDLE       (-5003) // 渲染处理失败
#define ERROR_RENDER_INPUT        (-5004) // 输入帧处理失败
#define ERROR_RENDER_PIP          (-5005) // 切换画中画失败
#define ERROR_RENDER_FILTER       (-5006) // filter处理失败
#define ERROR_RENDER_HDR          (-5007) // HDR渲染失败
#define ERROR_RENDER_SUBTITLE     (-5008) // 字幕渲染失败
#define ERROR_RENDER_AUDIO_SWR    (-5009) // 音频重采样失败
#define ERROR_RENDER_VIDEO_SWS    (-5010) // 视频像素格式转换失败
#define ERROR_RENDER_VIDEO_SUR    (-5011) // 设置surface失败
#define ERROR_RENDER_VIDEO_CTX    (-5012) // context上下文无效

/**********************解密相关**********************/
#define ERROR_DECRYPT_SECRET_KEY  (-6001) // 获取私钥失败
#define ERROR_DECRYPT_LICENSE     (-6002) // 获取证书失败
#define ERROR_DECRYPT_VERSION_    (-6003) // 版本不匹配
#define ERROR_DECRYPT_VALIDATE    (-6004) // 校验失败
#define ERROR_DECRYPT_FAIL        (-6005) // 解密失败

/**********************预加载相关**********************/
#define ERROR_PRELOAD_FAIL        (-7001) // 预加载失败

/**********************播放相关**********************/
#define ERROR_PLAYER_NOT_INIT     (-8001) // 播放器未初始化
#define ERROR_PLAYER_INIT_FAIL    (-8002) // 播放器初始化失败
#define ERROR_PLAYER_TRY_AGAIN    (-8003) // 重试
#define ERROR_PLAYER_EOF          (-8004) // EOF结束
#define ERROR_PLAYER_STATE        (-8005) // 播放状态不对
#define ERROR_PLAYER_UNSUPPORTED  (-8006) // 不支持操作

/**********************其他错误**********************/
#define ERROR_OTHER_UNKNOWN       (-9999) // 未知错误
#define ERROR_OTHER_OOM           (-9001) // 内存不足
#define ERROR_OTHER_PERMISSION    (-9002) // 没有权限

#endif
