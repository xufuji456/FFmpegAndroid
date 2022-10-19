package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautyWhiteBalanceFilter extends BaseFilter {

    private int tint;
    private int temperature;

    public BeautyWhiteBalanceFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.whitebalance));
    }

    protected void onInit() {
        super.onInit();
        tint = GLES30.glGetUniformLocation(getProgramId(), "tint");
        temperature = GLES30.glGetUniformLocation(getProgramId(), "temperature");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(tint, 0.5f);
        setFloat(temperature, 0.3f);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

}
