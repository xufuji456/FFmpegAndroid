package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class ContrastBeautyFilter extends BaseFilter {

    private int contrast;

    public ContrastBeautyFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.contrast));
    }

    protected void onInit() {
        super.onInit();
        contrast = GLES30.glGetUniformLocation(getProgramId(), "contrast");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(contrast, 1.5f);
    }

}
