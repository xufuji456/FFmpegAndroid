package com.frank.next.loader;

public class LibraryLoader {

    private static volatile boolean isLibsLoaded = false;

    private static final Object loadLock = new Object();

    public static void loadLibsOnce() {
        synchronized (loadLock) {
            if (!isLibsLoaded) {
                System.loadLibrary("c++_shared");
                System.loadLibrary("ffmpeg");
                System.loadLibrary("common");
                System.loadLibrary("demux");
                System.loadLibrary("decode");
                System.loadLibrary("render");
                System.loadLibrary("nextplayer");
                isLibsLoaded = true;
            }
        }
    }

}
