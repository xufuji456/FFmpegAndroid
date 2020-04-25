package com.frank.live;

import android.app.Application;

public class LiveApplication extends Application {

    private static LiveApplication context;

    @Override
    public void onCreate() {
        super.onCreate();
        context = this;
    }

    public static LiveApplication getInstance() {
        return context;
    }

}
