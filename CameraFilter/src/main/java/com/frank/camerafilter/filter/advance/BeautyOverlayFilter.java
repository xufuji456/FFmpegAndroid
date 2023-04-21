package com.frank.camerafilter.filter.advance;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.opengl.GLES20;
import android.opengl.GLES30;
import android.opengl.GLUtils;

import com.frank.camerafilter.R;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.util.OpenGLUtil;

public class BeautyOverlayFilter extends BaseFilter {

    private static final int BITMAP_WIDTH  = 512;
    private static final int BITMAP_HEIGHT = 512;

    private Paint paint;
    private Bitmap overlayBitmap;
    private Canvas overlayCanvas;
    private long startTime;

    private final int[] texId = new int[1];
    private final Matrix mMatrix = new Matrix();

    public BeautyOverlayFilter(Context context) {
        super(NORMAL_VERTEX_SHADER, OpenGLUtil.readShaderFromSource(context, R.raw.overlay));
    }

    protected void onInit() {
        super.onInit();

        GLES30.glUseProgram(getProgramId());

        GLES30.glGenTextures(1, texId, 0);
        GLES30.glActiveTexture(GLES30.GL_TEXTURE1);
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, texId[0]);
        GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D,
                GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D,
                GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D,
                GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D,
                GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);

        GLES30.glUniform1i(GLES30.glGetUniformLocation(getProgramId(), "overlayTexture"), 1);
    }

    @Override
    protected void onInitialized() {
        super.onInitialized();

        startTime = System.currentTimeMillis();
        paint = new Paint();
        paint.setTextSize(30);
        paint.setAntiAlias(true);
        paint.setARGB(0xFF, 0xFF, 0xFF, 0xFF);
        paint.setColor(Color.WHITE);
        overlayBitmap = Bitmap.createBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, Bitmap.Config.ARGB_8888);
        overlayCanvas = new Canvas(overlayBitmap);
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);
    }

    private static String addZero(int time) {
        if (time >= 0 && time < 10) {
            return  "0" + time;
        } else if(time >= 10) {
            return "" + time;
        } else {
            return "";
        }
    }
    public static String getVideoTime(long time) {
        if (time <= 0)
            return null;
        time = time / 1000;
        int second, minute=0, hour=0;
        second = (int)time % 60;
        time = time / 60;
        if (time > 0) {
            minute = (int)time % 60;
            hour = (int)time / 60;
        }
        if (hour > 0) {
            return addZero(hour) + ":" + addZero(minute) + ":" + addZero(second);
        } else if (minute > 0){
            return addZero(minute) + ":" + addZero(second);
        } else {
            if ("00".equals(addZero(second))) {
                return "00:01";
            } else {
                return "00:" + addZero(second);
            }
        }
    }

    private Bitmap flipBitmap(Bitmap bitmap) {
        // reuse matrix, so we need to reset
        mMatrix.reset();
        // flip vertical
        mMatrix.postScale(1f, -1f);
        // rotate 90, when in vertical preview with camera
        mMatrix.postRotate(90);
        return Bitmap.createBitmap(
                bitmap, 0, 0,
                bitmap.getWidth(),
                bitmap.getHeight(),
                mMatrix, true);
    }

    private void drawOverlay() {
        long presentationTimeMs = (System.currentTimeMillis() - startTime);
        String text = "time " + getVideoTime(presentationTimeMs);
        overlayBitmap.eraseColor(Color.TRANSPARENT);
        overlayCanvas.drawText(text,  18,  58, paint);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId[0]);
        Bitmap bitmap = flipBitmap(overlayBitmap);
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
        bitmap.recycle();
    }

    @Override
    protected void onDrawArrayBefore() {
        super.onDrawArrayBefore();
        drawOverlay();
    }
}
