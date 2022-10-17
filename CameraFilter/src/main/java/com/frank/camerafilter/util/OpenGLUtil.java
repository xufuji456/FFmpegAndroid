package com.frank.camerafilter.util;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES11Ext;
import android.opengl.GLES30;
import android.opengl.GLUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import javax.microedition.khronos.opengles.GL10;

public class OpenGLUtil {

    public final static int ON_DRAWN   = 1;
    public static final int NOT_INIT   = -1;
    public static final int NO_SHADER  = 0;
    public static final int NO_TEXTURE = -1;

    private static Bitmap getBitmapFromAssetFile(Context context, String name) {
        try {
            AssetManager assetManager = context.getResources().getAssets();
            InputStream stream = assetManager.open(name);
            Bitmap bitmap = BitmapFactory.decodeStream(stream);
            stream.close();
            return bitmap;
        } catch (IOException e) {
            return null;
        }
    }

    public static int loadTexture(final Context context, final String name) {
        if (context == null || name == null)
            return NO_TEXTURE;
        final int[] textures = new int[1];
        GLES30.glGenTextures(1, textures, 0);
        if (textures[0] == 0)
            return NO_TEXTURE;
        Bitmap bitmap = getBitmapFromAssetFile(context, name);
        if (bitmap == null)
            return NO_TEXTURE;
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, textures[0]);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_S,     GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_T,     GLES30.GL_CLAMP_TO_EDGE);
        GLUtils.texImage2D(GLES30.GL_TEXTURE_2D, 0, bitmap, 0);
        bitmap.recycle();
        return textures[0];
    }

    private static int loadShader(final String source, final int type) {
        int shader = GLES30.glCreateShader(type);
        GLES30.glShaderSource(shader, source);
        GLES30.glCompileShader(shader);
        int[] compile = new int[1];
        GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, compile, 0);
        if (compile[0] <= 0) {
            Log.e("OpenGlUtil", "Shader compile error=" + GLES30.glGetShaderInfoLog(shader));
            return NO_SHADER;
        }
        return shader;
    }

    public static int loadProgram(final String vertexSource, final String fragmentSource) {
        int vertexShader   = loadShader(vertexSource, GLES30.GL_VERTEX_SHADER);
        int fragmentShader = loadShader(fragmentSource, GLES30.GL_FRAGMENT_SHADER);
        if (vertexShader == NO_SHADER || fragmentShader == NO_SHADER) {
            return 0;
        }
        int programId = GLES30.glCreateProgram();
        GLES30.glAttachShader(programId, vertexShader);
        GLES30.glAttachShader(programId, fragmentShader);
        GLES30.glLinkProgram(programId);
        int[] linked = new int[1];
        GLES30.glGetProgramiv(programId, GLES30.GL_LINK_STATUS, linked, 0);
        if (linked[0] <= 0) {
            programId = 0;
            Log.e("OpenGlUtil", "program link error=" + GLES30.glGetProgramInfoLog(programId));
        }
        GLES30.glDeleteShader(vertexShader);
        GLES30.glDeleteShader(fragmentShader);
        return programId;
    }

    public static int getExternalOESTextureId() {
        int[] textures = new int[1];
        GLES30.glGenTextures(1, textures, 0);
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES30.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES30.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        return textures[0];
    }

    public static String readShaderFromSource(Context context, final int resourceId) {
        String line;
        StringBuilder builder = new StringBuilder();
        InputStream inputStream = context.getResources().openRawResource(resourceId);
        InputStreamReader reader = new InputStreamReader(inputStream);
        BufferedReader bufferedReader = new BufferedReader(reader);
        try {
            while ((line = bufferedReader.readLine()) != null) {
                builder.append(line).append("\n");
            }
        } catch (IOException e) {
            return null;
        } finally {
            try {
                inputStream.close();
                reader.close();
                bufferedReader.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

        }
        return builder.toString();
    }

}
