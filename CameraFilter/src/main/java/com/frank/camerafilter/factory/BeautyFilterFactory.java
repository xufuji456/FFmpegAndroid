package com.frank.camerafilter.factory;

import android.content.Context;

import com.frank.camerafilter.filter.advance.BlurBeautyFilter;
import com.frank.camerafilter.filter.advance.BreathCircleBeautyFilter;
import com.frank.camerafilter.filter.advance.BrightnessBeautyFilter;
import com.frank.camerafilter.filter.advance.ContrastBeautyFilter;
import com.frank.camerafilter.filter.advance.HueBeautyFilter;
import com.frank.camerafilter.filter.advance.OverlayBeautyFilter;
import com.frank.camerafilter.filter.advance.SketchBeautyFilter;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.filter.advance.WhiteBalanceBeautyFilter;
import com.frank.camerafilter.filter.advance.SaturationBeautyFilter;

public class BeautyFilterFactory {

    private static BeautyFilterType filterType = BeautyFilterType.NONE;

    public static BaseFilter getFilter(BeautyFilterType type, Context context) {
        filterType = type;
        switch (type) {
            case BRIGHTNESS:
                return new BrightnessBeautyFilter(context);
            case SATURATION:
                return new SaturationBeautyFilter(context);
            case CONTRAST:
                return new ContrastBeautyFilter(context);
            case SKETCH:
                return new SketchBeautyFilter(context);
            case BLUR:
                return new BlurBeautyFilter(context);
            case HUE:
                return new HueBeautyFilter(context);
            case WHITE_BALANCE:
                return new WhiteBalanceBeautyFilter(context);
            case OVERLAY:
                return new OverlayBeautyFilter(context);
            case BREATH_CIRCLE:
                return new BreathCircleBeautyFilter(context);
            default:
                return null;
        }
    }

    public static BeautyFilterType getFilterType() {
        return filterType;
    }

}
