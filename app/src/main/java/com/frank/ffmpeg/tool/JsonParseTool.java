package com.frank.ffmpeg.tool;

import android.text.TextUtils;
import android.util.Log;

import com.frank.ffmpeg.model.AudioBean;
import com.frank.ffmpeg.model.MediaBean;
import com.frank.ffmpeg.model.VideoBean;

import org.json.JSONArray;
import org.json.JSONObject;

/**
 * the tool of parsing json
 * Created by frank on 2020/1/8.
 */
public class JsonParseTool {

    private final static String TAG = JsonParseTool.class.getSimpleName();

    private final static String TYPE_VIDEO = "video";

    private final static String TYPE_AUDIO = "audio";

    public static MediaBean parseMediaFormat(String mediaFormat) {
        if (mediaFormat == null || mediaFormat.isEmpty()) {
            return null;
        }
        MediaBean mediaBean = null;
        try {
            JSONObject jsonMedia = new JSONObject(mediaFormat);
            JSONObject jsonMediaFormat = jsonMedia.getJSONObject("format");
            mediaBean = new MediaBean();
            int streamNum = jsonMediaFormat.optInt("nb_streams");
            mediaBean.setStreamNum(streamNum);
            Log.e(TAG, "streamNum=" + streamNum);
            String formatName = jsonMediaFormat.optString("format_name");
            mediaBean.setFormatName(formatName);
            Log.e(TAG, "formatName=" + formatName);
            String bitRateStr = jsonMediaFormat.optString("bit_rate");
            if (!TextUtils.isEmpty(bitRateStr)) {
                mediaBean.setBitRate(Integer.valueOf(bitRateStr));
            }
            Log.e(TAG, "bitRate=" + bitRateStr);
            String sizeStr = jsonMediaFormat.optString("size");
            if (!TextUtils.isEmpty(sizeStr)) {
                mediaBean.setSize(Long.valueOf(sizeStr));
            }
            Log.e(TAG, "size=" + sizeStr);
            String durationStr = jsonMediaFormat.optString("duration");
            if (!TextUtils.isEmpty(durationStr)) {
                float duration = Float.valueOf(durationStr);
                mediaBean.setDuration((long) duration);
            }

            JSONArray jsonMediaStream = jsonMedia.getJSONArray("streams");
            if (jsonMediaStream == null) {
                return mediaBean;
            }
            for (int index = 0; index < jsonMediaStream.length(); index ++) {
                JSONObject jsonMediaStreamItem = jsonMediaStream.optJSONObject(index);
                if (jsonMediaStreamItem == null) continue;
                String codecType = jsonMediaStreamItem.optString("codec_type");
                if (codecType == null) continue;
                if (codecType.equals(TYPE_VIDEO)) {
                    VideoBean videoBean = new VideoBean();
                    mediaBean.setVideoBean(videoBean);
                    String codecName = jsonMediaStreamItem.optString("codec_tag_string");
                    videoBean.setVideoCodec(codecName);
                    Log.e(TAG, "codecName=" + codecName);
                    int width = jsonMediaStreamItem.optInt("width");
                    videoBean.setWidth(width);
                    int height = jsonMediaStreamItem.optInt("height");
                    videoBean.setHeight(height);
                    Log.e(TAG, "width=" + width + "--height=" + height);
                    String aspectRatio = jsonMediaStreamItem.optString("display_aspect_ratio");
                    videoBean.setDisplayAspectRatio(aspectRatio);
                    Log.e(TAG, "aspectRatio=" + aspectRatio);
                    String pixelFormat = jsonMediaStreamItem.optString("pix_fmt");
                    videoBean.setPixelFormat(pixelFormat);
                    Log.e(TAG, "pixelFormat=" +pixelFormat);
                    String profile = jsonMediaStreamItem.optString("profile");
                    videoBean.setProfile(profile);
                    int level = jsonMediaStreamItem.optInt("level");
                    videoBean.setLevel(level);
                    Log.e(TAG, "profile=" + profile + "--level=" + level);
                    String frameRateStr = jsonMediaStreamItem.optString("r_frame_rate");
                    if (!TextUtils.isEmpty(frameRateStr)) {
                        String[] frameRateArray = frameRateStr.split("/");
                        double frameRate = Math.ceil(Double.valueOf(frameRateArray[0]) / Double.valueOf(frameRateArray[1]));
                        Log.e(TAG, "frameRate=" + (int) frameRate);
                        videoBean.setFrameRate((int) frameRate);
                    }
                } else if (codecType.equals(TYPE_AUDIO)) {
                    AudioBean audioBean = new AudioBean();
                    mediaBean.setAudioBean(audioBean);
                    String codecName = jsonMediaStreamItem.optString("codec_tag_string");
                    audioBean.setAudioCodec(codecName);
                    Log.e(TAG, "codecName=" + codecName);
                    String sampleRateStr = jsonMediaStreamItem.optString("sample_rate");
                    if (!TextUtils.isEmpty(sampleRateStr)) {
                        audioBean.setSampleRate(Integer.valueOf(sampleRateStr));
                    }
                    Log.e(TAG, "sampleRate=" + sampleRateStr);
                    int channels = jsonMediaStreamItem.optInt("channels");
                    audioBean.setChannels(channels);
                    Log.e(TAG, "channels=" + channels);
                    String channelLayout = jsonMediaStreamItem.optString("channel_layout");
                    audioBean.setChannelLayout(channelLayout);
                    Log.e(TAG, "channelLayout=" + channelLayout);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "parse error=" + e.toString());
        }
        return mediaBean;
    }

    public static String stringFormat(MediaBean mediaBean) {
        if (mediaBean == null) {
            return null;
        }
        StringBuilder formatBuilder = new StringBuilder();
        formatBuilder.append("duration:").append(mediaBean.getDuration()).append("\n");
        formatBuilder.append("size:").append(mediaBean.getSize()).append("\n");
        formatBuilder.append("bitRate:").append(mediaBean.getBitRate()).append("\n");
        formatBuilder.append("formatName:").append(mediaBean.getFormatName()).append("\n");
        formatBuilder.append("streamNum:").append(mediaBean.getStreamNum()).append("\n");
        if (mediaBean.getVideoBean() != null) {
            VideoBean videoBean = mediaBean.getVideoBean();
            formatBuilder.append("width:").append(videoBean.getWidth()).append("\n");
            formatBuilder.append("height:").append(videoBean.getHeight()).append("\n");
            formatBuilder.append("aspectRatio:").append(videoBean.getDisplayAspectRatio()).append("\n");
            formatBuilder.append("pixelFormat:").append(videoBean.getPixelFormat()).append("\n");
            formatBuilder.append("frameRate:").append(videoBean.getFrameRate()).append("\n");
            if (videoBean.getVideoCodec() != null) {
                formatBuilder.append("videoCodec:").append(videoBean.getVideoCodec()).append("\n");
            }
        }
        if (mediaBean.getAudioBean() != null) {
            AudioBean audioBean = mediaBean.getAudioBean();
            formatBuilder.append("sampleRate:").append(audioBean.getSampleRate()).append("\n");
            formatBuilder.append("channels:").append(audioBean.getChannels()).append("\n");
            formatBuilder.append("channelLayout:").append(audioBean.getChannelLayout()).append("\n");
            if (audioBean.getAudioCodec() != null) {
                formatBuilder.append("audioCodec:").append(audioBean.getAudioCodec()).append("\n");
            }
        }
        return formatBuilder.toString();
    }

}
