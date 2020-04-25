package com.frank.live.util;

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

        a = (input[index] & 0xff000000) >> 24; // a is not used obviously
        R = (input[index] & 0xff0000) >> 16;
        G = (input[index] & 0xff00) >> 8;
        B = (input[index] & 0xff);

        // well known RGB to YUV algorithm
        Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
        U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
        V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

        // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
        // meaning for every 4 Y pixels there are 1 V and 1 U.  Note the sampling is every other
        // pixel AND every other scanLine.
        yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
        if (j % 2 == 0 && index % 2 == 0) {
          yuv420sp[uvIndex++] = (byte) ((V < 0) ? 0 : ((V > 255) ? 255 : V));
          yuv420sp[uvIndex++] = (byte) ((U < 0) ? 0 : ((U > 255) ? 255 : U));
        }

        index++;
      }
    }
    return yuv420sp;
  }

}
