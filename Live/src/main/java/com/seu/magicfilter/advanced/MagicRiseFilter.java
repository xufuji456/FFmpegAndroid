package com.seu.magicfilter.advanced;

import android.opengl.GLES20;

import com.frank.live.R;
import com.seu.magicfilter.base.gpuimage.GPUImageFilter;
import com.seu.magicfilter.utils.MagicFilterType;
import com.seu.magicfilter.utils.OpenGLUtils;


public class MagicRiseFilter extends GPUImageFilter {
    private int[] inputTextureHandles = {-1,-1,-1};
    private int[] inputTextureUniformLocations = {-1,-1,-1};
    
    public MagicRiseFilter(){
        super(MagicFilterType.RISE, R.raw.rise);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        GLES20.glDeleteTextures(inputTextureHandles.length, inputTextureHandles, 0);
        for(int i = 0; i < inputTextureHandles.length; i++)
            inputTextureHandles[i] = -1;
    }

    @Override
    protected void onDrawArraysAfter(){
        for(int i = 0; i < inputTextureHandles.length
                && inputTextureHandles[i] != OpenGLUtils.NO_TEXTURE; i++){
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + (i+3));
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        }
    }

    @Override
    protected void onDrawArraysPre(){
        for(int i = 0; i < inputTextureHandles.length 
                && inputTextureHandles[i] != OpenGLUtils.NO_TEXTURE; i++){
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + (i+3) );
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, inputTextureHandles[i]);
            GLES20.glUniform1i(inputTextureUniformLocations[i], (i+3));
        }
    }

    @Override
    protected void onInit(){
        super.onInit();
        for(int i=0; i < inputTextureUniformLocations.length; i++){
            inputTextureUniformLocations[i] = GLES20.glGetUniformLocation(getProgram(), "inputImageTexture"+(2+i));
        }
    }

    @Override
    protected void onInitialized(){
        super.onInitialized();
        runOnDraw(new Runnable(){
            public void run(){
                inputTextureHandles[0] = OpenGLUtils.loadTexture(getContext(), "filter/blackboard1024.png");
                inputTextureHandles[1] = OpenGLUtils.loadTexture(getContext(), "filter/overlaymap.png");
                inputTextureHandles[2] = OpenGLUtils.loadTexture(getContext(), "filter/risemap.png");
            }
        });
    }
}
