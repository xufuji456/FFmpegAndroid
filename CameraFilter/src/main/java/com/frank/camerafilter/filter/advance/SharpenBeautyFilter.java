package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class SharpenBeautyFilter extends BaseFilter {
    // -4.0-4.0
    private static final float mSharpness = 2.8f;
    private int mSharpenLocation;
    private int mImageWidthLocation;
    private int mImageHeightLocation;

    public SharpenBeautyFilter(Context context) {
        super(OpenGLUtil.readShaderFromSource(context, R.raw.sharpen_vertex),
                OpenGLUtil.readShaderFromSource(context, R.raw.sharpen_fragment));
    }

    protected void onInit() {
        super.onInit();
        mSharpenLocation = GLES30.glGetUniformLocation(getProgramId(), "sharpen");
        mImageWidthLocation = GLES30.glGetUniformLocation(getProgramId(), "imageWidthFactor");
        mImageHeightLocation = GLES30.glGetUniformLocation(getProgramId(), "imageHeightFactor");
    }

    protected void onInitialized() {
        super.onInitialized();
        setFloat(mSharpenLocation, mSharpness);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
        setFloat(mImageWidthLocation, 1.0f / width);
        setFloat(mImageHeightLocation, 1.0f / height);
    }

}
