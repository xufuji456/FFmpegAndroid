/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FFTOOLS_FFMPEG_H
#define FFTOOLS_FFMPEG_H

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include "ffmpeg_hw.h"
#include "ffmpeg_common.h"

#define VSYNC_AUTO       -1
#define VSYNC_PASSTHROUGH 0
#define VSYNC_CFR         1
#define VSYNC_VFR         2
#define VSYNC_VSCFR       0xfe
#define VSYNC_DROP        0xff

#define MAX_STREAMS 1024    /* arbitrary sanity check value */

#define ABORT_ON_FLAG_EMPTY_OUTPUT (1 <<  0)

extern const char *const forced_keyframes_const_names[];

extern InputStream **input_streams;
extern int        nb_input_streams;
extern InputFile   **input_files;
extern int        nb_input_files;

extern OutputStream **output_streams;
extern int         nb_output_streams;
extern OutputFile   **output_files;
extern int         nb_output_files;
extern AVIOContext *progress_avio;

extern FilterGraph **filtergraphs;
extern int        nb_filtergraphs;

extern char *vstats_filename;
extern char *sdp_filename;

extern float audio_drift_threshold;
extern float dts_delta_threshold;
extern float dts_error_threshold;

extern int audio_volume;
extern int audio_sync_method;
extern int video_sync_method;
extern float frame_drop_threshold;
extern int do_benchmark;
extern int do_benchmark_all;
extern int do_deinterlace;
extern int do_hex_dump;
extern int do_pkt_dump;
extern int copy_ts;
extern int start_at_zero;
extern int copy_tb;
extern int debug_ts;
extern int exit_on_error;
extern int abort_on_flags;
extern int print_stats;
extern int qp_hist;
extern int stdin_interaction;
extern int frame_bits_per_raw_sample;
extern float max_error_rate;
extern char *videotoolbox_pixfmt;

extern int filter_nbthreads;
extern int filter_complex_nbthreads;
extern int vstats_version;

extern const AVIOInterruptCB int_cb;

extern const OptionDef options[];
extern const HWAccel hwaccels[];
extern AVBufferRef *hw_device_ctx;
#if CONFIG_QSV
extern char *qsv_device;
#endif
extern HWDevice *filter_hw_device;

#ifdef __cplusplus
extern "C" {
#endif
void term_init(void);
void term_exit(void);

void reset_options(OptionsContext *o, int is_input);
void show_usage(void);

void opt_output_file(void *optctx, const char *filename);

void remove_avoptions(AVDictionary **a, AVDictionary *b);
void assert_avoptions(AVDictionary *m);

int guess_input_channel_layout(InputStream *ist);

enum AVPixelFormat choose_pixel_fmt(AVStream *st, AVCodecContext *avctx, AVCodec *codec, enum AVPixelFormat target);
void choose_sample_fmt(AVStream *st, AVCodec *codec);

int configure_filtergraph(FilterGraph *fg);
int configure_output_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out);
void check_filter_outputs(void);
int ist_in_filtergraph(FilterGraph *fg, InputStream *ist);
int filtergraph_is_simple(FilterGraph *fg);
int init_simple_filtergraph(InputStream *ist, OutputStream *ost);
int init_complex_filtergraph(FilterGraph *fg);

void sub2video_update(InputStream *ist, AVSubtitle *sub);

int ifilter_parameters_from_frame(InputFilter *ifilter, const AVFrame *frame);

int ffmpeg_parse_options(int argc, char **argv);

int videotoolbox_init(AVCodecContext *s);
int qsv_init(AVCodecContext *s);
int cuvid_init(AVCodecContext *s);

#ifdef __cplusplus
}
#endif

int run(int argc, char **argv);

enum ProgressState {
    STATE_INIT,
    STATE_RUNNING,
    STATE_FINISH,
    STATE_ERROR,
};

void progress_callback(int position, int duration, int state);
void cancel_task(int cancel);

#endif /* FFTOOLS_FFMPEG_H */
