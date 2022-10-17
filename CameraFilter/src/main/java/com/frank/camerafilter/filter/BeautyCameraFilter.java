package com.frank.camerafilter.filter;

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES30;

import com.frank.camerafilter.R;
import com.frank.camerafilter.util.OpenGLUtil;

import java.nio.FloatBuffer;

public class BeautyCameraFilter extends BaseFilter {

    private int frameWidth = -1;
    private int frameHeight = -1;
    private int[] frameBuffer = null;
    private int[] frameBufferTexture = null;

    private int textureTransformLocation;
    private float[] textureTransformMatrix;

    public BeautyCameraFilter(Context context) {
        super(OpenGLUtil.readShaderFromSource(context, R.raw.default_vertex),
                OpenGLUtil.readShaderFromSource(context, R.raw.default_fragment));
    }

    protected void onInit() {
        super.onInit();
        textureTransformLocation = GLES30.glGetUniformLocation(getProgramId(), "textureTransform");
    }

    public void setTextureTransformMatrix(float[] matrix) {
        textureTransformMatrix = matrix;
    }

    @Override
    public int onDrawFrame(int textureId) {
        return onDrawFrame(textureId, mVertexBuffer, mTextureBuffer);
    }

    @Override
    public int onDrawFrame(int textureId, FloatBuffer vertexBuffer, FloatBuffer textureBuffer) {
        if (!hasInitialized()) {
            return OpenGLUtil.NOT_INIT;
        }
        GLES30.glUseProgram(getProgramId());
        runPendingOnDrawTask();
        vertexBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributePosition, 2, GLES30.GL_FLOAT, false, 0, vertexBuffer);
        GLES30.glEnableVertexAttribArray(mAttributePosition);
        textureBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributeTextureCoordinate, 2, GLES30.GL_FLOAT, false, 0, textureBuffer);
        GLES30.glEnableVertexAttribArray(mAttributeTextureCoordinate);
        GLES30.glUniformMatrix4fv(textureTransformLocation, 1, false, textureTransformMatrix, 0);

        if (textureId != OpenGLUtil.NO_TEXTURE) {
            GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
            GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId);
            GLES30.glUniform1i(mUniformTexture, 0);
        }

        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
        GLES30.glDisableVertexAttribArray(mAttributePosition);
        GLES30.glDisableVertexAttribArray(mAttributeTextureCoordinate);
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
        return OpenGLUtil.ON_DRAWN;
    }

    public int onDrawToTexture(int textureId) {
        if (!hasInitialized()) {
            return OpenGLUtil.NOT_INIT;
        }
        if (frameBuffer == null) {
            return OpenGLUtil.NO_TEXTURE;
        }
        GLES30.glUseProgram(getProgramId());
        runPendingOnDrawTask();
        GLES30.glViewport(0, 0, frameWidth, frameHeight);
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, frameBuffer[0]);

        mVertexBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributePosition, 2, GLES30.GL_FLOAT, false, 0, mVertexBuffer);
        GLES30.glEnableVertexAttribArray(mAttributePosition);
        mTextureBuffer.position(0);
        GLES30.glVertexAttribPointer(mAttributeTextureCoordinate, 2, GLES30.GL_FLOAT, false, 0, mTextureBuffer);
        GLES30.glEnableVertexAttribArray(mAttributeTextureCoordinate);
        GLES30.glUniformMatrix4fv(textureTransformLocation, 1, false, textureTransformMatrix, 0);

        if (textureId != OpenGLUtil.NO_TEXTURE) {
            GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
            GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId);
            GLES30.glUniform1i(mUniformTexture, 0);
        }

        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
        GLES30.glDisableVertexAttribArray(mAttributePosition);
        GLES30.glDisableVertexAttribArray(mAttributeTextureCoordinate);
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, 0);
        GLES30.glViewport(0, 0, mOutputWidth, mOutputHeight);
        return frameBufferTexture[0];
    }

    @Override
    public void onInputSizeChanged(int width, int height) {
        super.onInputSizeChanged(width, height);

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        destroyFrameBuffer();
    }

    public void initFrameBuffer(int width, int height) {
        if (frameBuffer != null && (frameWidth != width || frameHeight != height))
            destroyFrameBuffer();
        if (frameBuffer == null) {
            frameWidth = width;
            frameHeight = height;
            frameBuffer = new int[1];
            frameBufferTexture = new int[1];

            GLES30.glGenFramebuffers(1, frameBuffer, 0);
            GLES30.glGenTextures(1, frameBufferTexture, 0);
            GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, frameBufferTexture[0]);
            GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
            GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
            GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
            GLES30.glTexParameterf(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);
            GLES30.glTexImage2D(GLES30.GL_TEXTURE_2D, 0, GLES30.GL_RGBA, width, height,
                    0, GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, null);
            GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, frameBuffer[0]);
            GLES30.glFramebufferTexture2D(GLES30.GL_FRAMEBUFFER, GLES30.GL_COLOR_ATTACHMENT0,
                    GLES30.GL_TEXTURE_2D, frameBufferTexture[0], 0);
            GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, 0);
            GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, 0);
        }
    }

    public void destroyFrameBuffer() {
        if (frameBufferTexture != null) {
            GLES30.glDeleteTextures(1, frameBufferTexture, 0);
            frameBufferTexture = null;
        }
        if (frameBuffer != null) {
            GLES30.glDeleteFramebuffers(1, frameBuffer, 0);
            frameBuffer = null;
        }
        frameWidth = -1;
        frameHeight = -1;
    }

}
