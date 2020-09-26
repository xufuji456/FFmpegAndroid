package com.frank.ffmpeg.util;

import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;

/**
 * file tool
 * Created by frank on 2018/5/9.
 */

public class FileUtil {

    private final static String TYPE_MP3 = "mp3";
    private final static String TYPE_AAC = "aac";
    private final static String TYPE_AMR = "amr";
    private final static String TYPE_FLAC = "flac";
    private final static String TYPE_M4A = "m4a";
    private final static String TYPE_WMA = "wma";
    private final static String TYPE_WAV = "wav";
    private final static String TYPE_OGG = "ogg";
    private final static String TYPE_AC3 = "ac3";

    public final static String TYPE_MP4 = "mp4";
    private final static String TYPE_MKV = "mkv";
    private final static String TYPE_WEBM = "webm";
    private final static String TYPE_AVI = "avi";
    private final static String TYPE_WMV = "wmv";
    private final static String TYPE_FLV = "flv";
    private final static String TYPE_TS = "ts";
    private final static String TYPE_M3U8 = "m3u8";
    private final static String TYPE_3GP = "3gp";
    private final static String TYPE_MOV = "mov";
    private final static String TYPE_MPG = "mpg";

    public static boolean concatFile(String srcFilePath, String appendFilePath, String concatFilePath) {
        if (TextUtils.isEmpty(srcFilePath)
                || TextUtils.isEmpty(appendFilePath)
                || TextUtils.isEmpty(concatFilePath)) {
            return false;
        }
        File srcFile = new File(srcFilePath);
        if (!srcFile.exists()) {
            return false;
        }
        File appendFile = new File(appendFilePath);
        if (!appendFile.exists()) {
            return false;
        }
        FileOutputStream outputStream = null;
        FileInputStream inputStream1 = null, inputStream2 = null;
        try {
            inputStream1 = new FileInputStream(srcFile);
            inputStream2 = new FileInputStream(appendFile);
            outputStream = new FileOutputStream(new File(concatFilePath));
            byte[] data = new byte[1024];
            int len;
            while ((len = inputStream1.read(data)) > 0) {
                outputStream.write(data, 0, len);
            }
            outputStream.flush();
            while ((len = inputStream2.read(data)) > 0) {
                outputStream.write(data, 0, len);
            }
            outputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (inputStream1 != null) {
                    inputStream1.close();
                }
                if (inputStream2 != null) {
                    inputStream2.close();
                }
                if (outputStream != null) {
                    outputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return true;
    }

    /**
     * check the file exist or not
     *
     * @param path the path of file
     * @return result of exist or not
     */
    public static boolean checkFileExist(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        File file = new File(path);
        if (!file.exists()) {
            Log.e("FileUtil", path + " is not exist!");
            return false;
        }
        return true;
    }

    public static boolean isAudio(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        path = path.toLowerCase();
        return path.endsWith(TYPE_MP3)
                || path.endsWith(TYPE_AAC)
                || path.endsWith(TYPE_AMR)
                || path.endsWith(TYPE_FLAC)
                || path.endsWith(TYPE_M4A)
                || path.endsWith(TYPE_WMA)
                || path.endsWith(TYPE_WAV)
                || path.endsWith(TYPE_OGG)
                || path.endsWith(TYPE_AC3);
    }

    public static boolean isVideo(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        path = path.toLowerCase();
        return path.endsWith(TYPE_MP4)
                || path.endsWith(TYPE_MKV)
                || path.endsWith(TYPE_WEBM)
                || path.endsWith(TYPE_WMV)
                || path.endsWith(TYPE_AVI)
                || path.endsWith(TYPE_FLV)
                || path.endsWith(TYPE_3GP)
                || path.endsWith(TYPE_TS)
                || path.endsWith(TYPE_M3U8)
                || path.endsWith(TYPE_MOV)
                || path.endsWith(TYPE_MPG);
    }

    public static String getFileSuffix(String fileName) {
        if (TextUtils.isEmpty(fileName) || !fileName.contains(".")) {
            return null;
        }
        return fileName.substring(fileName.lastIndexOf("."));
    }

    public static String getFilePath(String filePath) {
        if (TextUtils.isEmpty(filePath) || !filePath.contains("/")) {
            return null;
        }
        return filePath.substring(0, filePath.lastIndexOf("/"));
    }

    public static String getFileName(String filePath) {
        if (TextUtils.isEmpty(filePath) || !filePath.contains("/")) {
            return null;
        }
        return filePath.substring(filePath.lastIndexOf("/") + 1);
    }

    public static String createListFile(String listPath, List<String> fileList) {
        if ((TextUtils.isEmpty(listPath) || fileList == null || fileList.size() == 0)) {
            return null;
        }
        FileOutputStream outputStream = null;
        try {
            File listFile = new File(listPath);
            if (!listFile.getParentFile().exists()) {
                if (!listFile.mkdirs()) {
                    return null;
                }
            }
            if (!listFile.exists()) {
                if (!listFile.createNewFile()) {
                    return null;
                }
            }
            outputStream = new FileOutputStream(listFile);
            StringBuilder fileBuilder = new StringBuilder();
            for (String file : fileList) {
                fileBuilder
                        .append("file")
                        .append(" ")
                        .append("'")
                        .append(file)
                        .append("'")
                        .append("\n");
            }
            byte[] fileData = fileBuilder.toString().getBytes();
            outputStream.write(fileData, 0, fileData.length);
            outputStream.flush();
            return listFile.getAbsolutePath();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return null;
    }

    public static boolean ensureDir(String fileDir) {
        if (TextUtils.isEmpty(fileDir)) {
            return false;
        }
        File listFile = new File(fileDir);
        if (!listFile.exists()) {
            return listFile.mkdirs();
        }
        return true;
    }

    public static boolean deleteFile(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        File file = new File(path);
        return file.exists() && file.delete();
    }

    public static boolean deleteFolder(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        boolean result = true;
        File tempFile = new File(path);
        if (!tempFile.exists()) {
            return tempFile.mkdir();
        }
        if (tempFile.isDirectory() && tempFile.listFiles() != null) {
            File[] files = tempFile.listFiles();
            for (File file : files) {
                result &= file.delete();
            }
        }
        return result;
    }

}
