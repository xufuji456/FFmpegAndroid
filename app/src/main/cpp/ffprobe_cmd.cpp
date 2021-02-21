//
// Created by frank on 2020-01-06.
//

#include <jni.h>
#include <string.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ffmpeg_jni_define.h"
#include "ffmpeg/ffprobe.h"

#ifdef __cplusplus
}
#endif

FFPROBE_FUNC(jstring, handleProbe, jobjectArray commands) {
    int argc = env->GetArrayLength(commands);
    char **argv = (char **) malloc(argc * sizeof(char *));
    int i;
    for (i = 0; i < argc; i++) {
        jstring jstr = (jstring) env->GetObjectArrayElement( commands, i);
        char *temp = (char *) env->GetStringUTFChars(jstr, 0);
        argv[i] = static_cast<char *>(malloc(1024));
        strcpy(argv[i], temp);
        env->ReleaseStringUTFChars(jstr, temp);
    }
    //execute ffprobe command
    char *result = ffprobe_run(argc, argv);
    //release memory
    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);

    return env->NewStringUTF(result);
}


