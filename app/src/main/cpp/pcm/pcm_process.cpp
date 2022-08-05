//
// Created by xu fulong on 2022/8/5.
//

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>

void pcm_raise_speed(char *input_path, char *output_path)
{
    FILE *input = fopen(input_path, "rb+");
    FILE *output = fopen(output_path, "wb+");
    if (!input && !output) {
        printf("open file fail, msg=%s\n", strerror(errno));
        return;
    }

    int count = 0;
    char *buf = (char*) malloc(sizeof(char) * 4);
    while(!feof(input)) {
        fread(buf, sizeof(char), 4, input);
        if (count % 2 == 0) {
            // L
            fwrite(buf, sizeof(char), 2, output);
            // R
            fwrite(buf + 2, sizeof(char), 2, output);
        }
        count++;
    }

    free(buf);
    fclose(output);
    fclose(input);
}

void pcm_change_volume(char *input_path, char *output_path)
{
    FILE *input = fopen(input_path, "rb+");
    FILE *output = fopen(output_path, "wb+");
    if (!input && !output) {
        printf("open file fail, msg=%s\n", strerror(errno));
        return;
    }

    int count = 0;
    char *buf = (char*) malloc(sizeof(char) * 4);
    while(!feof(input)) {
        fread(buf, sizeof(char), 4, input);
        short *left = (short*) buf;
        *left /= 2;
        short *right = (short*) (buf + 2);
        *right /= 2;
        // L
        fwrite(left, sizeof(short), 1, output);
        // R
        fwrite(right, sizeof(short), 1, output);
        count++;
    }
    printf("resample count=%d\n", count);

    free(buf);
    fclose(output);
    fclose(input);
}

void pcm_split_channel(char *input_path, char *left_path, char *right_path)
{
    FILE *input = fopen(input_path, "rb+");
    FILE *left = fopen(left_path, "wb+");
    FILE *right = fopen(right_path, "wb+");
    if (!input && !left && !right) {
        printf("open file fail, msg=%s\n", strerror(errno));
        return;
    }

    int count = 0;
    char *buf = (char*) malloc(sizeof(char) * 4);
    while(!feof(input)) {
        fread(buf, sizeof(char), 4, input);
        // L
        fwrite(buf, sizeof(char), 2, left);
        // R
        fwrite(buf+2, sizeof(char), 2, right);
        count++;
    }
    printf("resample count=%d\n", count);

    free(buf);
    fclose(left);
    fclose(right);
    fclose(input);
}