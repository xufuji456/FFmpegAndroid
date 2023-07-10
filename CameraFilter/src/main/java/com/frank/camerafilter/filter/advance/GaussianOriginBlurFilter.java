package com.frank.camerafilter.filter.advance;


import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

/**
 * @author xufulong
 * @date 2023/7/9 9:06 PM
 */

public class GaussianOriginBlurFilter extends BaseFilter {

    private float blurSize = 1.5f;

    private int blurRadius;
    private int blurCenter;
    private int aspectRatio;

    private int textureWidthOffset;

    private int textureHeightOffset;

    public GaussianOriginBlurFilter(Context context) {
        super(OpenGLUtil.readShaderFromSource(context, R.raw.vert_gaussian_blur),
                OpenGLUtil.readShaderFromSource(context, R.raw.frag_gaussian_blur));
    }

    protected void onInit() {
        super.onInit();
        blurRadius          = GLES30.glGetUniformLocation(getProgramId(), "blurRadius");
        blurCenter          = GLES30.glGetUniformLocation(getProgramId(), "blurCenter");
        aspectRatio         = GLES30.glGetUniformLocation(getProgramId(), "aspectRatio");
        textureWidthOffset  = GLES30.glGetUniformLocation(getProgramId(), "textureWidthOffset");
        textureHeightOffset = GLES30.glGetUniformLocation(getProgramId(), "textureHeightOffset");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(blurRadius, 1.0f);
        setFloatVec2(blurCenter, new float[]{0.5f, 0.5f});
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
        float ratio = (float) (height / width);
        setFloat(aspectRatio, ratio);
        setFloat(textureWidthOffset, blurSize / width);
        setFloat(textureHeightOffset, blurSize / height);
    }

    public void setBlurSize(float blurSize) {
        this.blurSize = blurSize;
    }
}
