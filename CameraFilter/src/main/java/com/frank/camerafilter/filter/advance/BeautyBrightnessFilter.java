package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautyBrightnessFilter extends BaseFilter {

    private int brightness;

    public BeautyBrightnessFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.brightness));
    }

    protected void onInit() {
        super.onInit();
        brightness = GLES30.glGetUniformLocation(getProgramId(), "brightness");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(brightness, 0.3f);
    }

}
