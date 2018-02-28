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

public class GPUImageHueFilter extends GPUImageFilter {

    private float mHue;
    private int mHueLocation;

    public GPUImageHueFilter() {
        this(0.0f);
    }

    public GPUImageHueFilter(final float hue) {
        super(MagicFilterType.HUE, R.raw.hue);
        mHue = hue;
    }

    @Override
    public void onInit() {
        super.onInit();
        mHueLocation = GLES20.glGetUniformLocation(getProgram(), "hueAdjust");
    }

    @Override
    public void onInitialized() {
        super.onInitialized();
        setHue(mHue);
    }

    public void setHue(final float hue) {
        mHue = hue;
        float hueAdjust = (mHue % 360.0f) * (float) Math.PI / 180.0f;
        setFloat(mHueLocation, hueAdjust);
    }
}
