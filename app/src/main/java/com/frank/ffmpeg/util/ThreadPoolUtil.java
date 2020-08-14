package com.frank.ffmpeg.util;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ThreadPoolUtil {

    public static ExecutorService executeSingleThreadPool(Runnable runnable) {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        executor.submit(runnable);
        return executor;
    }

}
