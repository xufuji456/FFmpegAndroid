package com.frank.ffmpeg.util;

import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * 文件工具类
 * Created by frank on 2018/5/9.
 */

public class FileUtil {

    public static boolean concatFile(String srcFilePath, String appendFilePath, String concatFilePath){
        if(TextUtils.isEmpty(srcFilePath)
                || TextUtils.isEmpty(appendFilePath)
                || TextUtils.isEmpty(concatFilePath)){
            return false;
        }
        File srcFile = new File(srcFilePath);
        if(!srcFile.exists()){
            return false;
        }
        File appendFile = new File(appendFilePath);
        if(!appendFile.exists()){
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
            while ((len = inputStream1.read(data)) > 0){
                outputStream.write(data, 0, len);
            }
            outputStream.flush();
            while ((len = inputStream2.read(data)) > 0){
                outputStream.write(data, 0, len);
            }
            outputStream.flush();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if(inputStream1 != null){
                    inputStream1.close();
                }
                if(inputStream2 != null){
                    inputStream2.close();
                }
                if(outputStream != null){
                    outputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return true;
    }

    /**
     * 判断文件是否存在
     * @param path 文件路径
     * @return 文件是否存在
     */
    public static boolean checkFileExist(String path){
        if (TextUtils.isEmpty(path)) {
            Log.e("FileUtil", path + "is null!");
        }
        File file = new File(path);
        if(!file.exists()){
            Log.e("FileUtil", path + " is not exist!");
        }
        return true;
    }

}
