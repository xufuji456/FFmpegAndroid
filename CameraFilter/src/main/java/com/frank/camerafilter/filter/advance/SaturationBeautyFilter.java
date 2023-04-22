package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class SaturationBeautyFilter extends BaseFilter {

    private int saturation;

    public SaturationBeautyFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.saturation));
    }

    protected void onInit() {
        super.onInit();
        saturation = GLES30.glGetUniformLocation(getProgramId(), "saturation");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(saturation, 1.8f);
    }

}
