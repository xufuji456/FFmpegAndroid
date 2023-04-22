package com.frank.camerafilter.factory;

import android.content.Context;

import com.frank.camerafilter.filter.advance.BeautyBlurFilter;
import com.frank.camerafilter.filter.advance.BeautyBreathCircleFilter;
import com.frank.camerafilter.filter.advance.BeautyBrightnessFilter;
import com.frank.camerafilter.filter.advance.BeautyHueFilter;
import com.frank.camerafilter.filter.advance.BeautyOverlayFilter;
import com.frank.camerafilter.filter.advance.BeautySketchFilter;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.filter.advance.BeautyWhiteBalanceFilter;
import com.frank.camerafilter.filter.advance.SaturationBeautyFilter;

public class BeautyFilterFactory {

    private static BeautyFilterType filterType = BeautyFilterType.NONE;

    public static BaseFilter getFilter(BeautyFilterType type, Context context) {
        filterType = type;
        switch (type) {
            case BRIGHTNESS:
                return new BeautyBrightnessFilter(context);
            case SATURATION:
                return new SaturationBeautyFilter(context);
            case SKETCH:
                return new BeautySketchFilter(context);
            case BLUR:
                return new BeautyBlurFilter(context);
            case HUE:
                return new BeautyHueFilter(context);
            case WHITE_BALANCE:
                return new BeautyWhiteBalanceFilter(context);
            case OVERLAY:
                return new BeautyOverlayFilter(context);
            case BREATH_CIRCLE:
                return new BeautyBreathCircleFilter(context);
            default:
                return null;
        }
    }

    public static BeautyFilterType getFilterType() {
        return filterType;
    }

}
