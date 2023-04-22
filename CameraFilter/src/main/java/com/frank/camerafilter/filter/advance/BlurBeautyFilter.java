package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BlurBeautyFilter extends BaseFilter {

    private int blurSize;

    public BlurBeautyFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.zoomblur));
    }

    protected void onInit() {
        super.onInit();
        blurSize = GLES30.glGetUniformLocation(getProgramId(), "blurSize");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(blurSize, 0.3f);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

}
