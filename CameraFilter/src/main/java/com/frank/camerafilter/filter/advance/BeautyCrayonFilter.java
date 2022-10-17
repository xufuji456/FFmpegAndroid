package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES20;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautyCrayonFilter extends BaseFilter {

    // 1.0--5.0
    private int mStrengthLocation;

    private int mStepOffsetLocation;

    public BeautyCrayonFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.crayon));
    }

    protected void onInit() {
        super.onInit();
        mStrengthLocation = GLES20.glGetUniformLocation(getProgramId(), "strength");
        mStepOffsetLocation = GLES20.glGetUniformLocation(getProgramId(), "singleStepOffset");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(mStrengthLocation, 2.0f);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
        setFloatVec2(mStepOffsetLocation, new float[] {1.0f / width, 1.0f / height});
    }

    protected void onDestroy() {
        super.onDestroy();
    }

}
