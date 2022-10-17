package com.frank.camerafilter.util;

public class TextureRotateUtil {

    public final static float[] TEXTURE_ROTATE_0   = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
    };

    public final static float[] TEXTURE_ROTATE_90  = {
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            0.0f, 0.0f
    };

    public final static float[] TEXTURE_ROTATE_180 = {
            1.0f, 0.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
    };

    public final static float[] TEXTURE_ROTATE_270 = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
    };

    public final static float[] VERTEX = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f
    };

    private TextureRotateUtil() {}

    private static float flip(float value) {
        return value == 1.0f ? 0.0f : 1.0f;
    }

    public static float[] getRotateTexture(Rotation rotation, boolean horizontalFlip, boolean verticalFlip) {
        float[] rotateTexture;
        switch (rotation) {
            case ROTATION_90:
                rotateTexture = TEXTURE_ROTATE_90;
                break;
            case ROTATION_180:
                rotateTexture = TEXTURE_ROTATE_180;
                break;
            case ROTATION_270:
                rotateTexture = TEXTURE_ROTATE_270;
                break;
            case NORMAL:
            default:
                rotateTexture = TEXTURE_ROTATE_0;
                break;
        }
        if (horizontalFlip) {
            rotateTexture = new float[] {
                    flip(rotateTexture[0]), rotateTexture[1],
                    flip(rotateTexture[2]), rotateTexture[3],
                    flip(rotateTexture[4]), rotateTexture[5],
                    flip(rotateTexture[6]), rotateTexture[7]
            };
        }
        if (verticalFlip) {
            rotateTexture = new float[] {
                    rotateTexture[0], flip(rotateTexture[1]),
                    rotateTexture[2], flip(rotateTexture[3]),
                    rotateTexture[4], flip(rotateTexture[5]),
                    rotateTexture[6], flip(rotateTexture[7])
            };
        }
        return rotateTexture;
    }

}
