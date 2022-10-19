package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautySketchFilter extends BaseFilter {

    private int strengthLocation;
    private int stepOffsetLocation;

    public BeautySketchFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.sketch));
    }

    protected void onInit() {
        super.onInit();
        strengthLocation = GLES30.glGetUniformLocation(getProgramId(), "strength");
        stepOffsetLocation = GLES30.glGetUniformLocation(getProgramId(), "singleStepOffset");
    }

    @Override
    protected void onInitialized() {
        super.onInitialized();
        setFloat(strengthLocation, 0.5f);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
        setFloatVec2(stepOffsetLocation, new float[] {1.0f / width, 1.0f / height});
    }

}
