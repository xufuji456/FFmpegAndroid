//
// Created by frank on 2020-01-06.
//

#include <jni.h>
#include <string.h>
#include <malloc.h>
#include <ffmpeg_jni_define.h>
#include <ffmpeg/ffprobe.h>

FFMPEG_FUNC(jstring, handleProbe, jobjectArray commands) {
    int argc = (*env)->GetArrayLength(env, commands);
    char **argv = (char **) malloc(argc * sizeof(char *));
    int i;
    for (i = 0; i < argc; i++) {
        jstring jstr = (jstring) (*env)->GetObjectArrayElement(env, commands, i);
        char *temp = (char *) (*env)->GetStringUTFChars(env, jstr, 0);
        argv[i] = malloc(1024);
        strcpy(argv[i], temp);
        (*env)->ReleaseStringUTFChars(env, jstr, temp);
    }
    //execute ffprobe command
    char *result = ffprobe_run(argc, argv);
    //release memory
    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);

    return (*env)->NewStringUTF(env, result);
}


