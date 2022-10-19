package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautyColorInvertFilter extends BaseFilter {

    public BeautyColorInvertFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.color_invert));
    }

    protected void onInit() {
        super.onInit();
    }

    protected void onInitialized() {
        super.onInitialized();
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

}
