//
// Created by frank on 2021/8/16.
//

#ifndef FRANK_VISUALIZER_H
#define FRANK_VISUALIZER_H

#include <math.h>
#include <mutex>

#include "fft.h"
#include "window.h"
#include "fixed_fft.h"

#define MIN_FFT_SIZE 128
#define MAX_FFT_SIZE 1024

#define FIXED_FFT 1

typedef struct
{
    bool convert_to_float;
    int i_channels;
    int i_prev_nb_samples;
    int16_t *p_prev_s16_buff;

    window_param *wind_param;

    uint8_t *data;
    int data_size;
    int nb_samples;
    int8_t *output;
    int out_samples;
} filter_sys_t;


class FrankVisualizer {

private:
    std::mutex mFftLock;

    filter_sys_t *fft_context = nullptr;

public:
    FrankVisualizer();
    ~FrankVisualizer();

    int getOutputSample();

    int8_t* fft_run(uint8_t *input_buffer, int nb_samples);

    int init_visualizer();

    void release_visualizer();
};

#endif //FRANK_VISUALIZER_H
