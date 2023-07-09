package com.frank.camerafilter.filter;

import android.opengl.GLES30;

import com.frank.camerafilter.util.OpenGLUtil;
import com.frank.camerafilter.util.Rotation;
import com.frank.camerafilter.util.TextureRotateUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.LinkedList;

public class BaseFilter {

    public final static String NORMAL_VERTEX_SHADER =
            "attribute vec4 position;\n" +
            "attribute vec4 inputTextureCoordinate;\n" +
            "varying vec2 textureCoordinate;\n" +
            "void main() {\n" +
            "   gl_Position = position;\n" +
            "   textureCoordinate = inputTextureCoordinate.xy;\n" +
            "}";

    private final String mVertexShader;
    private final String mFragmentShader;
    private final LinkedList<Runnable> mRunnableDraw;

    protected int mProgramId;
    protected int mInputWidth;
    protected int mInputHeight;
    protected int mOutputWidth;
    protected int mOutputHeight;
    protected int mUniformTexture;
    protected int mAttributePosition;
    protected int mAttributeTextureCoordinate;
    protected boolean mHasInitialized;
    protected FloatBuffer mVertexBuffer;
    protected FloatBuffer mTextureBuffer;

    private int mOverlayTexture;
    private int[] mOverlayTextureId;

    public BaseFilter(String vertexShader, String fragmentShader) {
        mRunnableDraw = new LinkedList<>();
        mVertexShader = vertexShader;
        mFragmentShader = fragmentShader;

        mVertexBuffer = ByteBuffer.allocateDirect(TextureRotateUtil.VERTEX.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mVertexBuffer.put(TextureRotateUtil.VERTEX).position(0);

        mTextureBuffer = ByteBuffer.allocateDirect(TextureRotateUtil.TEXTURE_ROTATE_0.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureBuffer.put(TextureRotateUtil.getRotateTexture(Rotation.NORMAL, false, true))
                .position(0);
    }

    protected void onInit() {
        if (mVertexShader == null || mFragmentShader == null)
            return;
        mProgramId = OpenGLUtil.loadProgram(mVertexShader, mFragmentShader);
        mAttributePosition = GLES30.glGetAttribLocation(mProgramId, "position");
        mUniformTexture = GLES30.glGetUniformLocation(mProgramId, "inputImageTexture");
        mAttributeTextureCoordinate = GLES30.glGetAttribLocation(mProgramId, "inputTextureCoordinate");
    }

    protected void onInitialized() {

    }

    public void init() {
        onInit();
        mHasInitialized = true;
        onInitialized();
    }

    protected void initOverlayTexture() {
        mOverlayTexture = GLES30.glGetUniformLocation(mProgramId, "inputImageTexture2");
        mOverlayTextureId = new int[1];
        GLES30.glGenTextures(1, mOverlayTextureId, 0);
    }

    protected void onDestroy() {

    }

    public void destroy() {
        mHasInitialized = false;
        GLES30.glDeleteProgram(mProgramId);
        onDestroy();
    }

    public void onInputSizeChanged(final int width, final int height) {
        mInputWidth = width;
        mInputHeight = height;
    }

    protected void runPendingOnDrawTask() {
        while (!mRunnableDraw.isEmpty()) {
            mRunnableDraw.removeFirst().run();
        }
    }

    protected void onDrawArrayBefore() {

    }

    protected void onDrawArrayAfter() {

    }

    public int onDrawFrame(final int textureId) {
        return onDrawFrame(textureId, mVertexBuffer, mTextureBuffer);
    }

    public int onDrawFrame(final int textureId, FloatBuffer vertexBuffer, FloatBuffer textureBuffer) {
        if (!mHasInitialized)
            return OpenGLUtil.NOT_INIT;

        GLES30.glUseProgram(mProgramId);
        runPendingOnDrawTask();
        vertexBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributePosition, 2, GLES30.GL_FLOAT, false, 0, vertexBuffer);
        GLES30.glEnableVertexAttribArray(mAttributePosition);
        textureBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributeTextureCoordinate, 2, GLES30.GL_FLOAT, false, 0, textureBuffer);
        GLES30.glEnableVertexAttribArray(mAttributeTextureCoordinate);

        if (textureId != OpenGLUtil.NO_TEXTURE) {
            GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
            GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, textureId);
            GLES30.glUniform1i(mUniformTexture, 0);
        }

        if (mOverlayTextureId != null && mOverlayTextureId[0] != OpenGLUtil.NO_TEXTURE) {
            GLES30.glActiveTexture(GLES30.GL_TEXTURE1);
            GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, mOverlayTextureId[0]);
            GLES30.glUniform1i(mOverlayTexture, 0);
        }

        onDrawArrayBefore();
        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
        GLES30.glDisableVertexAttribArray(mAttributePosition);
        GLES30.glDisableVertexAttribArray(mAttributeTextureCoordinate);
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, 0);
        onDrawArrayAfter();
        return OpenGLUtil.ON_DRAWN;
    }

    public boolean hasInitialized() {
        return mHasInitialized;
    }

    public int getProgramId() {
        return mProgramId;
    }

    protected void runOnDraw(final Runnable runnable) {
        synchronized (mRunnableDraw) {
            mRunnableDraw.addLast(runnable);
        }
    }

    public void setInteger(final int location, final int intVal) {
        runOnDraw(new Runnable() {
            @Override
            public void run() {
                GLES30.glUniform1i(location, intVal);
            }
        });
    }

    public void setFloat(final int location, final float floatVal) {
        runOnDraw(new Runnable() {
            @Override
            public void run() {
                GLES30.glUniform1f(location, floatVal);
            }
        });
    }

    public void setFloatVec2(final int location, final float[] floatArray) {
        runOnDraw(new Runnable() {
            @Override
            public void run() {
                GLES30.glUniform2fv(location, 1, FloatBuffer.wrap(floatArray));
            }
        });
    }

    public void onOutputSizeChanged(final int width, final int height) {
        mOutputWidth = width;
        mOutputHeight = height;
    }

}
