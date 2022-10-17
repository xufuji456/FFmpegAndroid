package com.frank.camerafilter.factory;

import android.content.Context;

import com.frank.camerafilter.filter.advance.BeautyCrayonFilter;
import com.frank.camerafilter.filter.advance.BeautySketchFilter;
import com.frank.camerafilter.filter.BaseFilter;

public class BeautyFilterFactory {

    private static BeautyFilterType filterType = BeautyFilterType.NONE;

    public static BaseFilter getFilter(BeautyFilterType type, Context context) {
        filterType = type;
        switch (type) {
            case SKETCH:
                return new BeautySketchFilter(context);
            case CRAYON:
                return new BeautyCrayonFilter(context);
            default:
                return null;
        }
    }

    public static BeautyFilterType getFilterType() {
        return filterType;
    }

}
