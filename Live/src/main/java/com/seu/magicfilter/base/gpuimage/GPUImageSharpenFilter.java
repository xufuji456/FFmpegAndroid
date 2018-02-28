/*
 * Copyright (C) 2012 CyberAgent
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.seu.magicfilter.base.gpuimage;

import android.opengl.GLES20;

import com.frank.live.R;
import com.seu.magicfilter.utils.MagicFilterType;

/**
 * Sharpens the picture. <br>
 * <br>
 * sharpness: from -4.0 to 4.0, with 0.0 as the normal level
 */
public class GPUImageSharpenFilter extends GPUImageFilter {

    private int mSharpnessLocation;
    private float mSharpness;
    private int mImageWidthFactorLocation;
    private int mImageHeightFactorLocation;

    public GPUImageSharpenFilter() {
        this(0.0f);
    }
    
    public GPUImageSharpenFilter(final float sharpness) {
        super(MagicFilterType.SHARPEN, R.raw.vertex_sharpen, R.raw.sharpen);
        mSharpness = sharpness;
    }

    @Override
    public void onInit() {
        super.onInit();
        mSharpnessLocation = GLES20.glGetUniformLocation(getProgram(), "sharpness");
        mImageWidthFactorLocation = GLES20.glGetUniformLocation(getProgram(), "imageWidthFactor");
        mImageHeightFactorLocation = GLES20.glGetUniformLocation(getProgram(), "imageHeightFactor");
        setSharpness(mSharpness);
    }

    @Override
    public void onInputSizeChanged(final int width, final int height) {
        super.onInputSizeChanged(width, height);
        setFloat(mImageWidthFactorLocation, 1.0f / width);
        setFloat(mImageHeightFactorLocation, 1.0f / height);
    }

    public void setSharpness(final float sharpness) {
        mSharpness = sharpness;
        setFloat(mSharpnessLocation, mSharpness);
    }
}
