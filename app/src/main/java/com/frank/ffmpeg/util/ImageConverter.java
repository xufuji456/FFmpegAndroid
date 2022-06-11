package com.frank.ffmpeg.util;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class ImageConverter {

    private byte[] yuv;
    private int[] argb;
    private Rect mRect;
    private int[] pixels;

    private final Matrix mMatrix;
    private ByteArrayOutputStream mOutputStream;
    private final static boolean useSystem = false;

    public ImageConverter() {
        mMatrix = new Matrix();
    }

    public Bitmap yuv2bitmap(byte[] data, int width, int height) {
        if (useSystem) {
            return yuv2bitmapSystem(data, width, height);
        } else {
            return yuvToBitmapFormula(data, width, height);
        }
    }

    private Bitmap yuv2bitmapSystem(byte[] data, int width, int height) {
        Bitmap bmp = null;
        try {
            YuvImage image = new YuvImage(data, ImageFormat.NV21, width, height, null);
            if (mRect == null) {
                mRect = new Rect(0, 0, width, height);
            }
            if (mOutputStream == null) {
                mOutputStream = new ByteArrayOutputStream();
            } else {
                mOutputStream.reset();
            }
            image.compressToJpeg(mRect, 100, mOutputStream);
            bmp = BitmapFactory.decodeByteArray(mOutputStream.toByteArray(), 0, mOutputStream.size());
        } catch (Exception ex) {
            Log.e("ImageConverter", "Error:" + ex.getMessage());
        }
        return bmp;
    }

    private int yuvToARGB(int y, int u, int v) {
        int r, g, b;

        r = y + (int) (1.402f * u);
        g = y - (int) (0.344f * v + 0.714f * u);
        b = y + (int) (1.772f * v);
        r = r > 255 ? 255 : Math.max(r, 0);
        g = g > 255 ? 255 : Math.max(g, 0);
        b = b > 255 ? 255 : Math.max(b, 0);
        return 0xff000000 | (r << 16) | (g << 8) | b;
    }

    private Bitmap yuvToBitmapFormula(byte[] data, int width, int height) {
        if (pixels == null) {
            pixels = new int[width * height];
        }
        int size = width * height;
        int offset = size;
        int u, v, y1, y2, y3, y4;

        for (int i = 0, k = 0; i < size; i += 2, k += 2) {
            y1 = data[i] & 0xff;
            y2 = data[i + 1] & 0xff;
            y3 = data[width + i] & 0xff;
            y4 = data[width + i + 1] & 0xff;

            u = data[offset + k] & 0xff;
            v = data[offset + k + 1] & 0xff;
            u = u - 128;
            v = v - 128;

            pixels[i] = yuvToARGB(y1, u, v);
            pixels[i + 1] = yuvToARGB(y2, u, v);
            pixels[width + i] = yuvToARGB(y3, u, v);
            pixels[width + i + 1] = yuvToARGB(y4, u, v);

            if (i != 0 && (i + 2) % width == 0)
                i += width;
        }
        return Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
    }

    public Bitmap rotateBitmap(Bitmap source, float angle) {
        mMatrix.reset();
        mMatrix.postRotate(angle);
        return Bitmap.createBitmap(source, 0, 0, source.getWidth(), source.getHeight(),
                mMatrix, true);
    }

    public Bitmap rotateAndFlipBitmap(Bitmap source, float angle, boolean xFlip, boolean yFlip) {
        mMatrix.reset();
        mMatrix.postRotate(angle);
        mMatrix.postScale(xFlip ? -1 : 1, yFlip ? -1 : 1,
                source.getWidth() / 2f, source.getHeight() / 2f);
        return Bitmap.createBitmap(source, 0, 0,
                source.getWidth(), source.getHeight(), mMatrix, true);
    }


    public byte[] rgbaToYUV420p(Bitmap bitmap, int width, int height) {
        final int frameSize = width * height;
        int index = 0;
        int yIndex = 0;
        int uIndex = frameSize;
        int vIndex = frameSize * 5 / 4;
        int R, G, B, Y, U, V;

        if (argb == null) {
            argb = new int[width * height];
        }
        if (yuv == null) {
            yuv = new byte[width * height * 3 / 2];
        }
        bitmap.getPixels(argb, 0, width, 0, 0, width, height);

        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                R = (argb[index] & 0xff0000) >> 16;
                G = (argb[index] & 0xff00) >> 8;
                B = (argb[index] & 0xff);

                // RGB to YUV algorithm
                Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;

                yuv[yIndex++] = (byte) Y; // -128～127
                if (j % 2 == 0 && i % 2 == 0) {
                    U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                    V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                    yuv[uIndex++] = (byte) U;
                    yuv[vIndex++] = (byte) V;
                }
                index++;
            }
        }
        return yuv;
    }

    public void close() {
        if (mOutputStream != null) {
            try {
                mOutputStream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

}
