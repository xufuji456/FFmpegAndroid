package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class HueBeautyFilter extends BaseFilter {

    private int hueAdjust;

    public HueBeautyFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.hue));
    }

    protected void onInit() {
        super.onInit();
        hueAdjust = GLES30.glGetUniformLocation(getProgramId(), "hueAdjust");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(hueAdjust, 3.0f);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

}
