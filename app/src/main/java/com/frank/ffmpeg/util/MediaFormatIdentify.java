package com.frank.ffmpeg.util;

import android.util.Log;

import java.io.FileInputStream;
import java.io.IOException;

/**
 * check media format with identification
 * @author xufulong
 * @date 2023/11/8 08:08 PM
 */
public class MediaFormatIdentify {

    private final static String TAG = "MediaFormatIdentify";

    private final static String TYPE_TS   = "ts";
    private final static String TYPE_MP4  = "mp4";
    private final static String TYPE_3GP  = "3gp";
    private final static String TYPE_MOV  = "mov";
    private final static String TYPE_AVI  = "avi";
    private final static String TYPE_FLV  = "flv";
    private final static String TYPE_F4V  = "f4v";
    private final static String TYPE_MPG  = "mpg";
    private final static String TYPE_WMV  = "wmv";
    private final static String TYPE_MKV  = "mkv";
    private final static String TYPE_WEBM = "webm";
    private final static String TYPE_RMVB = "rm";

    private final static String TYPE_MP3  = "mp3";
    private final static String TYPE_WAV  = "wav";
    private final static String TYPE_M4A  = "m4a";
    private final static String TYPE_OGG  = "ogg";
    private final static String TYPE_AAC  = "aac";
    private final static String TYPE_AMR  = "amr";
    private final static String TYPE_AC3  = "ac3";
    private final static String TYPE_AC4  = "ac4";
    private final static String TYPE_MPEG = "mpeg";
    private final static String TYPE_FLAC = "flac";

    /**
     *
     * @param path path
     * @return prefix of format
     */
    public static String identify(String path) {
        if (path == null || path.isEmpty())
            return null;

        byte[] data = null;
        FileInputStream inputStream = null;
        try {
            inputStream = new FileInputStream(path);
            data = new byte[36];
            int result = inputStream.read(data);
        } catch (IOException e) {
            Log.e(TAG, "read error:" + e);
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        return matchFormat(data);
    }

    private static String matchFormat(byte[] data) {
        if (data == null)
            return null;

        if (data[0] == 0x0B && data[1] == 0x77) {
            return TYPE_AC3;
        }
        if (data[0] == (byte) 0xAC && (data[1] == 0x40 || data[1] == 0x41)) {
            return TYPE_AC4;
        }
        if ((data[0] == 0x61 && data[1] == 0x64 && data[2] == 0x69 && data[3] == 0x66) // ADIF
            || (data[0] == (byte) 0xFF && (data[1] == (byte) 0xF1 || data[1] == (byte) 0xF0))) {// ADTS
            return TYPE_AAC;
        }
        if (data[0] == 0x49 && data[1] == 0x44 && data[2] == 0x33) {
            return TYPE_MP3;
        }
        if (data[0] == 0x4F && data[1] == 0x67 && data[2] == 0x67 && data[3] == 0x53) {
            return TYPE_OGG;
        }
        if (data[8] == 0x57 && data[9] == 0x41 && data[10] == 0x56 && data[11] == 0x45) {
            return TYPE_WAV;
        }
        if (data[2] == 0x41 && data[3] == 0x4D && data[4] == 0x52) {
            return TYPE_AMR;
        }
        if (data[0] == 0x66 && data[1] == 0x4C && data[2] == 0x61 && data[3] == 0x43) {
            return TYPE_FLAC;
        }

        if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
            if (data[3] == (byte) 0xBA) {
                return TYPE_MPG;
            } else {
                return TYPE_MPEG;
            }
        }
        if (data[0] == 0x30 && data[1] == 0x26 && data[2] == (byte) 0xB2 && data[3] == 0x75
                && data[4] == (byte) 0x8E && data[5] == 0x66 && data[6] == (byte) 0xCF && data[7] == 0x11) {
            return TYPE_WMV; // wma
        }
        if (data[4] == 0x66 && data[5] == 0x74 && data[6] == 0x79 && data[7] == 0x70) {
            if (data[8] == 0x69 && data[9] == 0x73 && data[10] == 0x6F && data[11] == 0x6D) {
                return TYPE_MP4;
            } else if (data[8] == 0x4D && data[9] == 0x34 && data[10] == 0x41) {
                return TYPE_M4A;
            } else if (data[8] == 0x71 && data[9] == 0x74) { // qt
                return TYPE_MOV;
            } else if (data[8] == 0x33 && data[9] == 0x67 && data[10] == 0x70) {
                return TYPE_3GP;
            } else if (data[8] == 0x66 && data[9] == 0x34 && data[10] == 0x76) {
                return TYPE_F4V;
            }
        }
        if (data[24] == 0x6D && data[25] == 0x61 && data[26] == 0x74 && data[27] == 0x72
                && data[28] == 0x6F && data[29] == 0x73 && data[30] == 0x6B && data[31] == 0x61) {
            return TYPE_MKV;
        }
        if ((data[24] == 0x77 && data[25] == 0x65 && data[26] == 0x62 && data[27] == 0x6D)
                || (data[31] == 0x77 && data[32] == 0x65 && data[33] == 0x62 && data[34] == 0x6D)) {
            return TYPE_WEBM;
        }
        if (data[0] == 0x47) {
            return TYPE_TS;
        }
        if (data[0] == 0x46 && data[1] == 0x4C && data[2] == 0x56) {
            return TYPE_FLV;
        }
        if (data[8] == 0x41 && data[9] == 0x56 && data[10] == 0x49) {
            return TYPE_AVI;
        }
        if (data[1] == 0x52 && data[2] == 0x4D && data[3] == 0x46) {
            return TYPE_RMVB;
        }
        return null;
    }

    // ac3: 0x0B77
    // ac4: 0xAC40 or 0xAC41
    // mpeg: 000001B3
    // aac: ADIF(61646966) or (0xFFF1 or 0xFFF0)
    // wma: 3026B2758E66CF11 A6D900AA0062CE6C
    // mp3: ID3(494433)
    // ogg: OggS(4F676753)
    // wav: RIFF+4+WAVE(57415645)
    // amr: #!AMR(2321 414D52)
    // flac: fLaC(664C6143)
    // m4a: ftypM4A(66747970 4D3441)

    // ts: 0x47
    // mpg: 000001BA
    // wmv: 3026B2758E66CF11 A6D900AA0062CE6C
    // mp4: ftypisom(66747970 69736F6D)
    // 3gp: ftyp3gp(66747970 336770)
    // mov: ftypqt(66747970 7174)
    // f4v: ftypf4v(66747970 663476)
    // avi: RIFF+4+AVI(415649)
    // flv: FLV(464C56)
    // mkv: 24+matroska(6D6174726F736B61)
    // webm: 24+webm(7765626D)
    // rmvb: 1+RMF(524D46)
}
