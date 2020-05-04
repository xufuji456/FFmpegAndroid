package com.frank.living.listener;

import android.view.MotionEvent;
import android.view.View;

/**
 * double click listener
 * Created by xufulong on 2019/2/27.
 */

public class OnDoubleClickListener implements View.OnTouchListener {

    private final static long DOUBLE_TIME = 500;
    private long firstClick;
    private long secondClick;
    private int clickCount;

    private OnDoubleClick onDoubleClick;

    public OnDoubleClickListener(OnDoubleClick onDoubleClick) {
        this.onDoubleClick = onDoubleClick;
    }

    public interface OnDoubleClick {
        void onDouble();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_UP) {
            return v.performClick();
        } else if (event.getAction() == MotionEvent.ACTION_DOWN) {
            clickCount++;
            if (clickCount == 1) {
                firstClick = System.currentTimeMillis();
            } else if (clickCount == 2) {
                secondClick = System.currentTimeMillis();
                if (secondClick - firstClick <= DOUBLE_TIME) {
                    firstClick = 0;
                    clickCount = 1;
                    if (onDoubleClick != null) {
                        onDoubleClick.onDouble();
                    }
                } else {
                    firstClick = secondClick;
                    clickCount = 1;
                }
                secondClick = 0;
            } else {
                clickCount = 0;
                firstClick = 0;
                secondClick = 0;
            }
        }

        return false;
    }

}
