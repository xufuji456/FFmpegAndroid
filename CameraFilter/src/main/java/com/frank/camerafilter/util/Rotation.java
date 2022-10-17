package com.frank.camerafilter.util;

public enum Rotation {

    NORMAL, ROTATION_90, ROTATION_180, ROTATION_270;

    public int toInt() {
        switch (this) {
            case NORMAL:
                return 0;
            case ROTATION_90:
                return 90;
            case ROTATION_180:
                return 180;
            case ROTATION_270:
                return 270;
            default:
                throw new IllegalStateException("unknown rotation value...");
        }
    }

    public static Rotation fromInt(int rotation) {
        switch (rotation) {
            case 0:
                return NORMAL;
            case 90:
                return ROTATION_90;
            case 180:
                return ROTATION_180;
            case 270:
                return ROTATION_270;
            default:
                throw new IllegalStateException("unknown rotation=" +rotation);
        }
    }
}
