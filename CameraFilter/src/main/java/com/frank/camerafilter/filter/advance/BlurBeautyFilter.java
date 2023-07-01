package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BlurBeautyFilter extends BaseFilter {

    private int blurSize;
    private int blurCenter;

    public BlurBeautyFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.zoomblur));
    }

    protected void onInit() {
        super.onInit();
        blurSize = GLES30.glGetUniformLocation(getProgramId(), "blurSize");
        blurCenter = GLES30.glGetUniformLocation(getProgramId(), "blurCenter");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(blurSize, 1.0f);
        setFloatVec2(blurCenter, new float[]{0.5f, 0.5f});
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

}
