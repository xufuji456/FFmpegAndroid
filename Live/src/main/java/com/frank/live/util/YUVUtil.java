package com.frank.live.util;

import android.util.Log;

/**
 * Tool of transforming YUV format
 * Created by frank on 2018/7/1.
 */

public class YUVUtil {

    public static byte[] ARGBtoYUV420SemiPlanar(int[] input, int width, int height) {

        final int frameSize = width * height;
        byte[] yuv420sp = new byte[width * height * 3 / 2];
        int yIndex = 0;
        int uvIndex = frameSize;

        int a, R, G, B, Y, U, V;
        int index = 0;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                a = (input[index] & 0xff000000) >> 24;
                R = (input[index] & 0xff0000) >> 16;
                G = (input[index] & 0xff00) >> 8;
                B = (input[index] & 0xff);

                // RGB to YUV algorithm
                Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

                // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
                // meaning for every 4 Y pixels there are 1 V and 1 U.
                yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : (Math.min(Y, 255)));
                if (j % 2 == 0 && index % 2 == 0) {
                    yuv420sp[uvIndex++] = (byte) ((V < 0) ? 0 : (Math.min(V, 255)));
                    yuv420sp[uvIndex++] = (byte) ((U < 0) ? 0 : (Math.min(U, 255)));
                }

                index++;
            }
        }
        return yuv420sp;
    }

    public static void YUV420pRotate90(byte[] dst, byte[] src, int width, int height) {
        int n = 0;
        int wh = width * height;
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        // y
        for (int j = 0; j < width; j++) {
            for (int i = height - 1; i >= 0; i--) {
                dst[n++] = src[width * i + j];
            }
        }
        // u
        for (int i = 0; i < halfWidth; i++) {
            for (int j = 1; j <= halfHeight; j++) {
                dst[n++] = src[wh + ((halfHeight - j) * halfWidth + i)];
            }
        }
        // v
        for (int i = 0; i < halfWidth; i++) {
            for (int j = 1; j <= halfHeight; j++) {
                dst[n++] = src[wh + wh / 4 + ((halfHeight - j) * halfWidth + i)];
            }
        }
    }

    public static void YUV420pRotate180(byte[] dst, byte[] src, int width, int height) {
        int n = 0;
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        // y
        for (int j = height - 1; j >= 0; j--) {
            for (int i = width; i > 0; i--) {
                dst[n++] = src[width * j + i - 1];
            }
        }
        // u
        int offset = width * height;
        for (int j = halfHeight - 1; j >= 0; j--) {
            for (int i = halfWidth; i > 0; i--) {
                dst[n++] = src[offset + halfWidth * j + i - 1];
            }
        }
        // v
        offset += halfWidth * halfHeight;
        for (int j = halfHeight - 1; j >= 0; j--) {
            for (int i = halfWidth; i > 0; i--) {
                dst[n++] = src[offset + halfWidth * j + i - 1];
            }
        }
    }

}
