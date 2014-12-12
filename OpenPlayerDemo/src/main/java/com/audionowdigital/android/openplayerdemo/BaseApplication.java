package com.audionowdigital.android.openplayerdemo;

import android.app.Application;

public class BaseApplication extends Application {

    public void onCreate() {
        super.onCreate();
    }
    @Override
    public void onTerminate() {
        super.onTerminate();
    }

}
