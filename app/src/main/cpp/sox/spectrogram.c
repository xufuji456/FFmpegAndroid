/* libSoX effect: Spectrogram       (c) 2008-9 robs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox_i.h"
#include "fft4g.h"
#include <assert.h>
#include <math.h>
#ifdef HAVE_LIBPNG_PNG_H
#include <libpng/png.h>
#else
#include <png.h>
#endif
#include <zlib.h>

/* For SET_BINARY_MODE: */
#include <fcntl.h>
#ifdef HAVE_IO_H
  #include <io.h>
#endif

#define is_p2(x) !(x & (x - 1))

#define MAX_X_SIZE 200000

#if SSIZE_MAX < UINT32_MAX
#define MAX_Y_SIZE 16384 /* avoid multiplication overflow on 32-bit systems */
#else
#define MAX_Y_SIZE 200000
#endif

typedef enum {
  Window_Hann,
  Window_Hamming,
  Window_Bartlett,
  Window_Rectangular,
  Window_Kaiser,
  Window_Dolph
} win_type_t;

static const lsx_enum_item window_options[] = {
  LSX_ENUM_ITEM(Window_,Hann)
  LSX_ENUM_ITEM(Window_,Hamming)
  LSX_ENUM_ITEM(Window_,Bartlett)
  LSX_ENUM_ITEM(Window_,Rectangular)
  LSX_ENUM_ITEM(Window_,Kaiser)
  LSX_ENUM_ITEM(Window_,Dolph)
  {0, 0}
};

typedef struct {
  /* Parameters */
  double     pixels_per_sec;
  double     window_adjust;
  int        x_size0;
  int        y_size;
  int        Y_size;
  int        dB_range;
  int        gain;
  int        spectrum_points;
  int        perm;
  sox_bool   monochrome;
  sox_bool   light_background;
  sox_bool   high_colour;
  sox_bool   slack_overlap;
  sox_bool   no_axes;
  sox_bool   normalize;
  sox_bool   raw;
  sox_bool   alt_palette;
  sox_bool   truncate;
  win_type_t win_type;
  const char *out_name;
  const char *title;
  const char *comment;
  const char *duration_str;
  const char *start_time_str;
  sox_bool   using_stdout; /* output image to stdout */

  /* Shared work area */
  double     *shared;
  double     **shared_ptr;

  /* Per-channel work area */
  int        WORK;  /* Start of work area is marked by this dummy variable. */
  uint64_t   skip;
  int        dft_size;
  int        step_size;
  int        block_steps;
  int        block_num;
  int        rows;
  int        cols;
  int        read;
  int        x_size;
  int        end;
  int        end_min;
  int        last_end;
  sox_bool   truncated;
  double     *buf;              /* [dft_size] */
  double     *dft_buf;          /* [dft_size] */
  double     *window;           /* [dft_size + 1] */
  double     block_norm;
  double     max;
  double     *magnitudes;       /* [dft_size / 2 + 1] */
  float      *dBfs;
} priv_t;

#define secs(cols) \
  ((double)(cols) * p->step_size * p->block_steps / effp->in_signal.rate)

static const unsigned char alt_palette[] = {
  0x00, 0x00, 0x00,  0x00, 0x00, 0x03,  0x00, 0x01, 0x05,
  0x00, 0x01, 0x08,  0x00, 0x01, 0x0a,  0x00, 0x01, 0x0b,
  0x00, 0x01, 0x0e,  0x01, 0x02, 0x10,  0x01, 0x02, 0x12,
  0x01, 0x02, 0x15,  0x01, 0x02, 0x16,  0x01, 0x02, 0x18,
  0x01, 0x03, 0x1b,  0x01, 0x03, 0x1d,  0x01, 0x03, 0x1f,
  0x01, 0x03, 0x20,  0x01, 0x03, 0x22,  0x01, 0x03, 0x24,
  0x01, 0x03, 0x25,  0x01, 0x03, 0x27,  0x01, 0x03, 0x28,
  0x01, 0x03, 0x2a,  0x01, 0x03, 0x2c,  0x01, 0x03, 0x2e,
  0x01, 0x03, 0x2f,  0x01, 0x03, 0x30,  0x01, 0x03, 0x32,
  0x01, 0x03, 0x34,  0x02, 0x03, 0x36,  0x04, 0x03, 0x38,
  0x05, 0x03, 0x39,  0x07, 0x03, 0x3b,  0x09, 0x03, 0x3d,
  0x0b, 0x03, 0x3f,  0x0e, 0x03, 0x41,  0x0f, 0x02, 0x42,
  0x11, 0x02, 0x44,  0x13, 0x02, 0x46,  0x15, 0x02, 0x48,
  0x17, 0x02, 0x4a,  0x18, 0x02, 0x4b,  0x1a, 0x02, 0x4d,
  0x1d, 0x02, 0x4f,  0x20, 0x02, 0x51,  0x24, 0x02, 0x53,
  0x28, 0x02, 0x55,  0x2b, 0x02, 0x57,  0x30, 0x02, 0x5a,
  0x33, 0x02, 0x5c,  0x37, 0x02, 0x5f,  0x3b, 0x02, 0x61,
  0x3e, 0x02, 0x63,  0x42, 0x02, 0x65,  0x45, 0x02, 0x68,
  0x49, 0x02, 0x6a,  0x4d, 0x02, 0x6c,  0x51, 0x02, 0x6e,
  0x55, 0x02, 0x70,  0x5a, 0x02, 0x72,  0x5f, 0x02, 0x74,
  0x63, 0x02, 0x75,  0x68, 0x02, 0x76,  0x6c, 0x02, 0x78,
  0x70, 0x03, 0x7a,  0x75, 0x03, 0x7c,  0x7a, 0x03, 0x7d,
  0x7e, 0x03, 0x7e,  0x83, 0x03, 0x80,  0x87, 0x03, 0x82,
  0x8c, 0x03, 0x84,  0x90, 0x03, 0x85,  0x93, 0x03, 0x83,
  0x96, 0x03, 0x80,  0x98, 0x03, 0x7e,  0x9b, 0x03, 0x7c,
  0x9e, 0x03, 0x7a,  0xa0, 0x03, 0x78,  0xa3, 0x03, 0x75,
  0xa6, 0x03, 0x73,  0xa9, 0x03, 0x71,  0xab, 0x03, 0x6f,
  0xae, 0x03, 0x6d,  0xb1, 0x03, 0x6a,  0xb3, 0x03, 0x68,
  0xb6, 0x03, 0x66,  0xba, 0x03, 0x62,  0xbc, 0x03, 0x5e,
  0xc0, 0x03, 0x5a,  0xc3, 0x03, 0x56,  0xc7, 0x03, 0x52,
  0xca, 0x03, 0x4e,  0xcd, 0x03, 0x4a,  0xd1, 0x03, 0x46,
  0xd4, 0x03, 0x43,  0xd7, 0x03, 0x3e,  0xdb, 0x03, 0x3a,
  0xde, 0x03, 0x36,  0xe2, 0x03, 0x32,  0xe4, 0x03, 0x2f,
  0xe6, 0x07, 0x2d,  0xe8, 0x0d, 0x2c,  0xea, 0x11, 0x2b,
  0xec, 0x17, 0x2a,  0xed, 0x1b, 0x29,  0xee, 0x20, 0x28,
  0xf0, 0x26, 0x27,  0xf2, 0x2a, 0x26,  0xf4, 0x2f, 0x24,
  0xf5, 0x34, 0x23,  0xf6, 0x39, 0x23,  0xf8, 0x3e, 0x21,
  0xfa, 0x43, 0x20,  0xfc, 0x49, 0x20,  0xfc, 0x4f, 0x22,
  0xfc, 0x56, 0x26,  0xfc, 0x5d, 0x2a,  0xfc, 0x64, 0x2c,
  0xfc, 0x6b, 0x30,  0xfc, 0x72, 0x33,  0xfc, 0x7a, 0x37,
  0xfd, 0x81, 0x3b,  0xfd, 0x88, 0x3e,  0xfd, 0x8f, 0x42,
  0xfd, 0x96, 0x45,  0xfd, 0x9e, 0x49,  0xfd, 0xa5, 0x4d,
  0xfd, 0xac, 0x50,  0xfd, 0xb1, 0x54,  0xfd, 0xb7, 0x58,
  0xfd, 0xbc, 0x5c,  0xfd, 0xc1, 0x61,  0xfd, 0xc6, 0x65,
  0xfd, 0xcb, 0x69,  0xfd, 0xd0, 0x6d,  0xfe, 0xd5, 0x71,
  0xfe, 0xda, 0x76,  0xfe, 0xdf, 0x7a,  0xfe, 0xe4, 0x7e,
  0xfe, 0xe9, 0x82,  0xfe, 0xee, 0x86,  0xfe, 0xf3, 0x8b,
  0xfd, 0xf5, 0x8f,  0xfc, 0xf6, 0x93,  0xfb, 0xf7, 0x98,
  0xfa, 0xf7, 0x9c,  0xf9, 0xf8, 0xa1,  0xf8, 0xf9, 0xa5,
  0xf7, 0xf9, 0xaa,  0xf6, 0xfa, 0xae,  0xf5, 0xfa, 0xb3,
  0xf4, 0xfb, 0xb7,  0xf3, 0xfc, 0xbc,  0xf1, 0xfd, 0xc0,
  0xf0, 0xfd, 0xc5,  0xf0, 0xfe, 0xc9,  0xef, 0xfe, 0xcc,
  0xef, 0xfe, 0xcf,  0xf0, 0xfe, 0xd1,  0xf0, 0xfe, 0xd4,
  0xf0, 0xfe, 0xd6,  0xf0, 0xfe, 0xd8,  0xf0, 0xfe, 0xda,
  0xf1, 0xff, 0xdd,  0xf1, 0xff, 0xdf,  0xf1, 0xff, 0xe1,
  0xf1, 0xff, 0xe4,  0xf1, 0xff, 0xe6,  0xf2, 0xff, 0xe8,
};
#define alt_palette_len (array_length(alt_palette) / 3)

static int getopts(sox_effect_t *effp, int argc, char **argv)
{
  priv_t *p = effp->priv;
  uint64_t dummy;
  const char *next;
  int c;
  lsx_getopt_t optstate;

  lsx_getopt_init(argc, argv, "+S:d:x:X:y:Y:z:Z:q:p:W:w:st:c:AarmnlhTo:",
                  NULL, lsx_getopt_flag_none, 1, &optstate);

  p->dB_range = 120;
  p->spectrum_points = 249;
  p->perm = 1;
  p->out_name = "spectrogram.png";
  p->comment = "Created by SoX";

  while ((c = lsx_getopt(&optstate)) != -1) {
    switch (c) {
      GETOPT_NUMERIC(optstate, 'x', x_size0,        100, MAX_X_SIZE)
      GETOPT_NUMERIC(optstate, 'X', pixels_per_sec,   1, 5000)
      GETOPT_NUMERIC(optstate, 'y', y_size,          64, MAX_Y_SIZE)
      GETOPT_NUMERIC(optstate, 'Y', Y_size,         130, MAX_Y_SIZE)
      GETOPT_NUMERIC(optstate, 'z', dB_range,        20, 180)
      GETOPT_NUMERIC(optstate, 'Z', gain,          -100, 100)
      GETOPT_NUMERIC(optstate, 'q', spectrum_points,  0, p->spectrum_points)
      GETOPT_NUMERIC(optstate, 'p', perm,             1, 6)
      GETOPT_NUMERIC(optstate, 'W', window_adjust,  -10, 10)
      case 'w': p->win_type = lsx_enum_option(c, optstate.arg, window_options);
                break;
      case 's': p->slack_overlap    = sox_true;   break;
      case 'A': p->alt_palette      = sox_true;   break;
      case 'a': p->no_axes          = sox_true;   break;
      case 'r': p->raw              = sox_true;   break;
      case 'm': p->monochrome       = sox_true;   break;
      case 'n': p->normalize        = sox_true;   break;
      case 'l': p->light_background = sox_true;   break;
      case 'h': p->high_colour      = sox_true;   break;
      case 'T': p->truncate         = sox_true;   break;
      case 't': p->title            = optstate.arg; break;
      case 'c': p->comment          = optstate.arg; break;
      case 'o': p->out_name         = optstate.arg; break;
      case 'S':
        next = lsx_parseposition(0, optstate.arg, NULL, 0, 0, '=');
        if (next && !*next) {
          p->start_time_str = lsx_strdup(optstate.arg);
          break;
        }
        return lsx_usage(effp);
      case 'd':
        next = lsx_parsesamples(1e5, optstate.arg, &dummy, 't');
        if (next && !*next) {
          p->duration_str = lsx_strdup(optstate.arg);
          break;
        }
        return lsx_usage(effp);
      default:
        lsx_fail("invalid option `-%c'", optstate.opt);
        return lsx_usage(effp);
    }
  }

  if (!!p->x_size0 + !!p->pixels_per_sec + !!p->duration_str > 2) {
    lsx_fail("only two of -x, -X, -d may be given");
    return SOX_EOF;
  }

  if (p->y_size && p->Y_size) {
    lsx_fail("only one of -y, -Y may be given");
    return SOX_EOF;
  }

  p->gain = -p->gain;
  --p->perm;
  p->spectrum_points += 2;
  if (p->alt_palette)
    p->spectrum_points = min(p->spectrum_points, alt_palette_len);
  p->shared_ptr = &p->shared;

  if (!strcmp(p->out_name, "-")) {
    if (effp->global_info->global_info->stdout_in_use_by) {
      lsx_fail("stdout already in use by `%s'",
               effp->global_info->global_info->stdout_in_use_by);
      return SOX_EOF;
    }
    effp->global_info->global_info->stdout_in_use_by = effp->handler.name;
    p->using_stdout = sox_true;
  }

  return optstate.ind != argc || p->win_type == INT_MAX ?
    lsx_usage(effp) : SOX_SUCCESS;
}

static double make_window(priv_t *p, int end)
{
  double sum = 0;
  double *w = end < 0 ? p->window : p->window + end;
  double beta;
  int n = 1 + p->dft_size - abs(end);
  int i;

  if (end)
    memset(p->window, 0, sizeof(*p->window) * (p->dft_size + 1));

  for (i = 0; i < n; ++i)
    w[i] = 1;

  switch (p->win_type) {
    case Window_Hann:        lsx_apply_hann(w, n); break;
    case Window_Hamming:     lsx_apply_hamming(w, n); break;
    case Window_Bartlett:    lsx_apply_bartlett(w, n); break;
    case Window_Rectangular: break;
    case Window_Kaiser:
      beta = lsx_kaiser_beta((p->dB_range + p->gain) *
                             (1.1 + p->window_adjust / 50), .1);
      lsx_apply_kaiser(w, n, beta);
      break;
    default:
      lsx_apply_dolph(w, n, (p->dB_range + p->gain) *
                      (1.005 + p->window_adjust / 50) + 6);
  }

  for (i = 0; i < p->dft_size; ++i)
    sum += p->window[i];

  /* empirical small window adjustment */
  for (--n, i = 0; i < p->dft_size; ++i)
    p->window[i] *= 2 / sum * sqr((double)n / p->dft_size);

  return sum;
}

static double *rdft_init(size_t n)
{
  double *q = lsx_malloc(2 * (n / 2 + 1) * n * sizeof(*q));
  double *p = q;
  int i, j;

  for (j = 0; j <= n / 2; ++j) {
    for (i = 0; i < n; ++i) {
      *p++ = cos(2 * M_PI * j * i / n);
      *p++ = sin(2 * M_PI * j * i / n);
    }
  }

  return q;
}

#define _ re += in[i] * *q++, im += in[i++] * *q++,
static void rdft_p(const double *q, const double *in, double *out, int n)
{
  int i, j;

  for (j = 0; j <= n / 2; ++j) {
    double re = 0, im = 0;

    for (i = 0; i < (n & ~7);)
      _ _ _ _ _ _ _ _ (void)0;

    while (i < n)
      _ (void)0;

    *out++ += re * re + im * im;
  }
}

static int start(sox_effect_t *effp)
{
  priv_t *p = effp->priv;
  double actual;
  double duration = 0.0;
  double start_time = 0.0;
  double pixels_per_sec = p->pixels_per_sec;
  uint64_t d;

  memset(&p->WORK, 0, sizeof(*p) - field_offset(priv_t, WORK));

  if (p->duration_str) {
    lsx_parsesamples(effp->in_signal.rate, p->duration_str, &d, 't');
    duration = d / effp->in_signal.rate;
  }

  if (p->start_time_str) {
    uint64_t in_length = effp->in_signal.length != SOX_UNKNOWN_LEN ?
      effp->in_signal.length / effp->in_signal.channels : SOX_UNKNOWN_LEN;

    if (!lsx_parseposition(effp->in_signal.rate, p->start_time_str, &d,
                           0, in_length, '=') || d == SOX_UNKNOWN_LEN) {
      lsx_fail("-S option: audio length is unknown");
      return SOX_EOF;
    }

    start_time = d / effp->in_signal.rate;
    p->skip = d;
  }

  p->x_size = p->x_size0;

  while (sox_true) {
    if (!pixels_per_sec && p->x_size && duration)
      pixels_per_sec = min(5000, p->x_size / duration);
    else if (!p->x_size && pixels_per_sec && duration)
      p->x_size = min(MAX_X_SIZE, (int)(pixels_per_sec * duration + .5));

    if (!duration && effp->in_signal.length != SOX_UNKNOWN_LEN) {
      duration = effp->in_signal.length /
        (effp->in_signal.rate * effp->in_signal.channels);
      duration -= start_time;
      if (duration <= 0)
        duration = 1;
      continue;
    } else if (!p->x_size) {
      p->x_size = 800;
      continue;
    } else if (!pixels_per_sec) {
      pixels_per_sec = 100;
      continue;
    }
    break;
  }

  if (p->y_size) {
    p->dft_size = 2 * (p->y_size - 1);
    if (!is_p2(p->dft_size) && !effp->flow)
      p->shared = rdft_init(p->dft_size);
  } else {
   int y = max(32, (p->Y_size? p->Y_size : 550) / effp->in_signal.channels - 2);
   for (p->dft_size = 128; p->dft_size <= y; p->dft_size <<= 1);
  }

  /* Now that dft_size is set, allocate variable-sized elements of priv_t */
  p->buf        = lsx_calloc(p->dft_size, sizeof(*p->buf));
  p->dft_buf    = lsx_calloc(p->dft_size, sizeof(*p->dft_buf));
  p->window     = lsx_calloc(p->dft_size + 1, sizeof(*p->window));
  p->magnitudes = lsx_calloc(p->dft_size / 2 + 1, sizeof(*p->magnitudes));

  if (is_p2(p->dft_size) && !effp->flow)
    lsx_safe_rdft(p->dft_size, 1, p->dft_buf);

  lsx_debug("duration=%g x_size=%i pixels_per_sec=%g dft_size=%i",
            duration, p->x_size, pixels_per_sec, p->dft_size);

  p->end = p->dft_size;
  p->rows = (p->dft_size >> 1) + 1;
  actual = make_window(p, p->last_end = 0);
  lsx_debug("window_density=%g", actual / p->dft_size);
  p->step_size = (p->slack_overlap ? sqrt(actual * p->dft_size) : actual) + 0.5;
  p->block_steps = effp->in_signal.rate / pixels_per_sec;
  p->step_size =
    p->block_steps / ceil((double)p->block_steps / p->step_size) + 0.5;
  p->block_steps = floor((double)p->block_steps / p->step_size + 0.5);
  p->block_norm = 1.0 / p->block_steps;
  actual = effp->in_signal.rate / p->step_size / p->block_steps;
  if (!effp->flow && actual != pixels_per_sec)
    lsx_report("actual pixels/s = %g", actual);
  lsx_debug("step_size=%i block_steps=%i", p->step_size, p->block_steps);
  p->max = -p->dB_range;
  p->read = (p->step_size - p->dft_size) / 2;

  return SOX_SUCCESS;
}

static int do_column(sox_effect_t *effp)
{
  priv_t *p = effp->priv;
  int i;

  if (p->cols == p->x_size) {
    p->truncated = sox_true;
    if (!effp->flow)
      lsx_report("PNG truncated at %g seconds", secs(p->cols));
    return p->truncate ? SOX_EOF : SOX_SUCCESS;
  }

  ++p->cols;
  p->dBfs = lsx_realloc(p->dBfs, p->cols * p->rows * sizeof(*p->dBfs));

  /* FIXME: allocate in larger steps (for several columns) */
  for (i = 0; i < p->rows; ++i) {
    double dBfs = 10 * log10(p->magnitudes[i] * p->block_norm);
    p->dBfs[(p->cols - 1) * p->rows + i] = dBfs + p->gain;
    p->max = max(dBfs, p->max);
  }

  memset(p->magnitudes, 0, p->rows * sizeof(*p->magnitudes));
  p->block_num = 0;

  return SOX_SUCCESS;
}

static int flow(sox_effect_t *effp,
    const sox_sample_t *ibuf, sox_sample_t *obuf,
    size_t *isamp, size_t *osamp)
{
  priv_t *p = effp->priv;
  size_t len = *isamp = *osamp = min(*isamp, *osamp);
  int i;

  memcpy(obuf, ibuf, len * sizeof(*obuf)); /* Pass on audio unaffected */

  if (p->skip) {
    if (p->skip >= len) {
      p->skip -= len;
      return SOX_SUCCESS;
    }
    ibuf += p->skip;
    len -= p->skip;
    p->skip = 0;
  }

  while (!p->truncated) {
    if (p->read == p->step_size) {
      memmove(p->buf, p->buf + p->step_size,
          (p->dft_size - p->step_size) * sizeof(*p->buf));
      p->read = 0;
    }

    for (; len && p->read < p->step_size; --len, ++p->read, --p->end)
      p->buf[p->dft_size - p->step_size + p->read] =
        SOX_SAMPLE_TO_FLOAT_64BIT(*ibuf++,);

    if (p->read != p->step_size)
      break;

    if ((p->end = max(p->end, p->end_min)) != p->last_end)
      make_window(p, p->last_end = p->end);

    for (i = 0; i < p->dft_size; ++i)
      p->dft_buf[i] = p->buf[i] * p->window[i];

    if (is_p2(p->dft_size)) {
      lsx_safe_rdft(p->dft_size, 1, p->dft_buf);
      p->magnitudes[0] += sqr(p->dft_buf[0]);

      for (i = 1; i < p->dft_size >> 1; ++i)
        p->magnitudes[i] += sqr(p->dft_buf[2*i]) + sqr(p->dft_buf[2*i+1]);

      p->magnitudes[p->dft_size >> 1] += sqr(p->dft_buf[1]);
    }
    else
      rdft_p(*p->shared_ptr, p->dft_buf, p->magnitudes, p->dft_size);

    if (++p->block_num == p->block_steps && do_column(effp) == SOX_EOF)
      return SOX_EOF;
  }

  return SOX_SUCCESS;
}

static int drain(sox_effect_t *effp, sox_sample_t *obuf_, size_t *osamp)
{
  priv_t *p = effp->priv;

  if (!p->truncated) {
    sox_sample_t * ibuf = lsx_calloc(p->dft_size, sizeof(*ibuf));
    sox_sample_t * obuf = lsx_calloc(p->dft_size, sizeof(*obuf));
    size_t isamp = (p->dft_size - p->step_size) / 2;
    int left_over = (isamp + p->read) % p->step_size;

    if (left_over >= p->step_size >> 1)
      isamp += p->step_size - left_over;

    lsx_debug("cols=%i left=%i end=%i", p->cols, p->read, p->end);

    p->end = 0, p->end_min = -p->dft_size;

    if (flow(effp, ibuf, obuf, &isamp, &isamp) == SOX_SUCCESS && p->block_num) {
      p->block_norm *= (double)p->block_steps / p->block_num;
      do_column(effp);
    }

    lsx_debug("flushed cols=%i left=%i end=%i", p->cols, p->read, p->end);
    free(obuf);
    free(ibuf);
  }

  *osamp = 0;

  return SOX_SUCCESS;
}

enum {
  Background,
  Text,
  Labels,
  Grid,
  fixed_palette
};

static unsigned colour(const priv_t *p, double x)
{
  unsigned c = x < -p->dB_range ? 0 : x >= 0 ? p->spectrum_points - 1 :
      1 + (1 + x / p->dB_range) * (p->spectrum_points - 2);
  return fixed_palette + c;
}

static void make_palette(const priv_t *p, png_color *palette)
{
  static const unsigned char black[] = { 0x00, 0x00, 0x00 };
  static const unsigned char dgrey[] = { 0x3f, 0x3f, 0x3f };
  static const unsigned char mgrey[] = { 0x7f, 0x7f, 0x7f };
  static const unsigned char lgrey[] = { 0xbf, 0xbf, 0xbf };
  static const unsigned char white[] = { 0xff, 0xff, 0xff };
  static const unsigned char lbgnd[] = { 0xdd, 0xd8, 0xd0 };
  static const unsigned char mbgnd[] = { 0xdf, 0xdf, 0xdf };
  int i;

  if (p->light_background) {
    memcpy(palette++, p->monochrome ? mbgnd : lbgnd, 3);
    memcpy(palette++, black, 3);
    memcpy(palette++, dgrey, 3);
    memcpy(palette++, dgrey, 3);
  } else {
    memcpy(palette++, black, 3);
    memcpy(palette++, white, 3);
    memcpy(palette++, lgrey, 3);
    memcpy(palette++, mgrey, 3);
  }

  for (i = 0; i < p->spectrum_points; ++i) {
    double c[3];
    double x = (double)i / (p->spectrum_points - 1);
    int at = p->light_background ? p->spectrum_points - 1 - i : i;

    if (p->monochrome) {
      c[2] = c[1] = c[0] = x;
      if (p->high_colour) {
        c[(1 + p->perm) % 3] = x < .4? 0 : 5 / 3. * (x - .4);
        if (p->perm < 3)
          c[(2 + p->perm) % 3] = x < .4? 0 : 5 / 3. * (x - .4);
      }
      palette[at].red  = .5 + 255 * c[0];
      palette[at].green= .5 + 255 * c[1];
      palette[at].blue = .5 + 255 * c[2];
      continue;
    }

    if (p->high_colour) {
      static const int states[3][7] = {
        { 4, 5, 0, 0, 2, 1, 1 },
        { 0, 0, 2, 1, 1, 3, 2 },
        { 4, 1, 1, 3, 0, 0, 2 },
      };
      int j, phase_num = min(7 * x, 6);

      for (j = 0; j < 3; ++j) {
        switch (states[j][phase_num]) {
          case 0: c[j] = 0; break;
          case 1: c[j] = 1; break;
          case 2: c[j] = sin((7 * x - phase_num) * M_PI / 2); break;
          case 3: c[j] = cos((7 * x - phase_num) * M_PI / 2); break;
          case 4: c[j] =      7 * x - phase_num;  break;
          case 5: c[j] = 1 - (7 * x - phase_num); break;
        }
      }
    } else if (p->alt_palette) {
      int n = (double)i / (p->spectrum_points - 1) * (alt_palette_len - 1) + .5;
      c[0] = alt_palette[3 * n + 0] / 255.;
      c[1] = alt_palette[3 * n + 1] / 255.;
      c[2] = alt_palette[3 * n + 2] / 255.;
    } else {
      if      (x < .13) c[0] = 0;
      else if (x < .73) c[0] = 1  * sin((x - .13) / .60 * M_PI / 2);
      else              c[0] = 1;
      if      (x < .60) c[1] = 0;
      else if (x < .91) c[1] = 1  * sin((x - .60) / .31 * M_PI / 2);
      else              c[1] = 1;
      if      (x < .60) c[2] = .5 * sin((x - .00) / .60 * M_PI);
      else if (x < .78) c[2] = 0;
      else              c[2] =          (x - .78) / .22;
    }

    palette[at].red  = .5 + 255 * c[p->perm % 3];
    palette[at].green= .5 + 255 * c[(1 + p->perm + (p->perm % 2)) % 3];
    palette[at].blue = .5 + 255 * c[(2 + p->perm - (p->perm % 2)) % 3];
  }
}

static const Bytef fixed[] = {
  0x78, 0xda, 0x65, 0x54, 0xa1, 0xb6, 0xa5, 0x30, 0x0c, 0x44, 0x56, 0x56,
  0x3e, 0x59, 0xf9, 0x24, 0x72, 0x65, 0x25, 0x32, 0x9f, 0x80, 0x7c, 0x32,
  0x12, 0x59, 0x59, 0x89, 0x44, 0x22, 0x2b, 0xdf, 0x27, 0x3c, 0x79, 0xe5,
  0xca, 0xfd, 0x0c, 0x64, 0xe5, 0x66, 0x92, 0x94, 0xcb, 0x9e, 0x9d, 0x7b,
  0xb8, 0xe4, 0x4c, 0xa7, 0x61, 0x9a, 0x04, 0xa6, 0xe9, 0x81, 0x64, 0x98,
  0x92, 0xc4, 0x44, 0xf4, 0x5e, 0x20, 0xea, 0xf2, 0x53, 0x22, 0x6d, 0xe7,
  0xc9, 0x9f, 0x9f, 0x17, 0x34, 0x4b, 0xa3, 0x98, 0x32, 0xb5, 0x1d, 0x0b,
  0xf9, 0x3c, 0xf3, 0x79, 0xec, 0x5f, 0x96, 0x67, 0xec, 0x8c, 0x29, 0x65,
  0x20, 0xa5, 0x38, 0xe1, 0x0f, 0x10, 0x4a, 0x34, 0x8d, 0x5b, 0x7a, 0x3c,
  0xb9, 0xbf, 0xf7, 0x00, 0x33, 0x34, 0x40, 0x7f, 0xd8, 0x63, 0x97, 0x84,
  0x20, 0x49, 0x72, 0x2e, 0x05, 0x24, 0x55, 0x80, 0xb0, 0x94, 0xd6, 0x53,
  0x0f, 0x80, 0x3d, 0x5c, 0x6b, 0x10, 0x51, 0x41, 0xdc, 0x25, 0xe2, 0x10,
  0x2a, 0xc3, 0x50, 0x9c, 0x89, 0xf6, 0x1e, 0x23, 0xf8, 0x52, 0xbe, 0x5f,
  0xce, 0x73, 0x2d, 0xe5, 0x92, 0x44, 0x6c, 0x7a, 0xf3, 0x6d, 0x79, 0x2a,
  0x3b, 0x8f, 0xfb, 0xe6, 0x7a, 0xb7, 0xe3, 0x9e, 0xf4, 0xa6, 0x9e, 0xf5,
  0xa1, 0x39, 0xc5, 0x70, 0xdb, 0xb7, 0x13, 0x28, 0x87, 0xb5, 0xdb, 0x9b,
  0xd5, 0x59, 0xe2, 0xa3, 0xb5, 0xef, 0xb2, 0x8d, 0xb3, 0x74, 0xb9, 0x24,
  0xbe, 0x96, 0x65, 0x61, 0xb9, 0x2e, 0xf7, 0x26, 0xd0, 0xe7, 0x82, 0x5f,
  0x9c, 0x17, 0xff, 0xe5, 0x92, 0xab, 0x3f, 0xe2, 0x32, 0xf4, 0x87, 0x79,
  0xae, 0x9e, 0x12, 0x39, 0xd9, 0x1b, 0x0c, 0xfe, 0x97, 0xb6, 0x22, 0xee,
  0xab, 0x6a, 0xf6, 0xf3, 0xe7, 0xdc, 0x55, 0x53, 0x1c, 0x5d, 0xf9, 0x3f,
  0xad, 0xf9, 0xde, 0xfa, 0x7a, 0xb5, 0x76, 0x1c, 0x96, 0xa7, 0x1a, 0xd4,
  0x8f, 0xdc, 0xdf, 0xcf, 0x55, 0x34, 0x0e, 0xce, 0x7b, 0x4e, 0xf8, 0x19,
  0xf5, 0xef, 0x69, 0x4c, 0x99, 0x79, 0x1b, 0x79, 0xb4, 0x89, 0x44, 0x37,
  0xdf, 0x04, 0xa4, 0xb1, 0x90, 0x44, 0xe6, 0x01, 0xb1, 0xef, 0xed, 0x3e,
  0x03, 0xe2, 0x93, 0xf3, 0x00, 0xc3, 0xbf, 0x0c, 0x5b, 0x8c, 0x41, 0x2c,
  0x70, 0x1c, 0x60, 0xad, 0xed, 0xf4, 0x1f, 0xfa, 0x94, 0xe2, 0xbf, 0x0c,
  0x87, 0xad, 0x1e, 0x5f, 0x56, 0x07, 0x1c, 0xa1, 0x5e, 0xce, 0x57, 0xaf,
  0x7f, 0x08, 0xa2, 0xc0, 0x1c, 0x0c, 0xbe, 0x1b, 0x3f, 0x2f, 0x39, 0x5f,
  0xd9, 0x66, 0xe6, 0x1e, 0x15, 0x59, 0x29, 0x98, 0x31, 0xaf, 0xa1, 0x34,
  0x7c, 0x1d, 0xf5, 0x9f, 0xe2, 0x34, 0x6b, 0x03, 0xa4, 0x03, 0xa2, 0xb1,
  0x06, 0x08, 0xbd, 0x3e, 0x7a, 0x24, 0xf8, 0x8d, 0x3a, 0xb8, 0xf3, 0x77,
  0x1e, 0x2f, 0xb5, 0x6b, 0x46, 0x0b, 0x10, 0x6f, 0x36, 0xa2, 0xc1, 0xf4,
  0xde, 0x17, 0xd5, 0xaf, 0xd1, 0xf4, 0x66, 0x73, 0x99, 0x8d, 0x47, 0x1a,
  0x3d, 0xaf, 0xc5, 0x44, 0x69, 0xc4, 0x5e, 0x7f, 0xc4, 0x52, 0xf4, 0x51,
  0x3d, 0x95, 0xfb, 0x1b, 0xd0, 0xfd, 0xfd, 0xfa, 0x80, 0xdf, 0x1f, 0xfc,
  0x7d, 0xdc, 0xdf, 0x10, 0xf4, 0xc8, 0x28, 0x5d, 0xc4, 0xb7, 0x62, 0x7f,
  0xd6, 0x59, 0x72, 0x6a, 0xca, 0xbf, 0xfb, 0x9b, 0x1f, 0xe0,
};

static unsigned char *font;

#define font_x 5
#define font_y 12
#define font_X (font_x + 1)

#define pixel(x,y) pixels[(y) * cols + (x)]
#define print_at(x,y,c,t) print_at_(pixels,cols,x,y,c,t,0)
#define print_up(x,y,c,t) print_at_(pixels,cols,x,y,c,t,1)

static void print_at_(png_byte *pixels, int cols, int x, int y, int c,
                      const char *text, int orientation)
{
  for (; *text; ++text) {
    int pos = ((*text < ' ' || *text > '~'? '~' + 1 : *text) - ' ') * font_y;
    int i, j;

    for (i = 0; i < font_y; ++i) {
      unsigned line = font[pos++];
      for (j = 0; j < font_x; ++j, line <<= 1) {
        if (line & 0x80) {
          switch (orientation) {
            case 0: pixel(x + j, y - i) = c; break;
            case 1: pixel(x + i, y + j) = c; break;
          }
        }
      }
    }

    switch (orientation) {
      case 0: x += font_X; break;
      case 1: y += font_X; break;
    }
  }
}

static int axis(double to, int max_steps, double *limit, char **prefix)
{
  double scale = 1, step = max(1, 10 * to);
  int i, prefix_num = 0;

  if (max_steps) {
    double try;
    double log_10 = HUGE_VAL;
    double min_step = (to *= 10) / max_steps;

    for (i = 5; i; i >>= 1) {
      if ((try = ceil(log10(min_step * i))) <= log_10) {
        step = pow(10., log_10 = try) / i;
        log_10 -= i > 1;
      }
    }

    prefix_num = floor(log_10 / 3);
    scale = pow(10., -3. * prefix_num);
  }

  *prefix = "pnum-kMGTPE" + prefix_num + (prefix_num? 4 : 11);
  *limit = to * scale;

  return step * scale + .5;
}

#define below 48
#define left 58
#define between 37
#define spectrum_width 14
#define right 35

static int stop(sox_effect_t *effp) /* only called, by end(), on flow 0 */
{
  priv_t     *p        = effp->priv;
  FILE       *file;
  uLong       font_len = 96 * font_y;
  int         chans    = effp->in_signal.channels;
  int         c_rows   = p->rows * chans + chans - 1;
  int         rows     = p->raw? c_rows : below + c_rows + 30 + 20 * !!p->title;
  int         cols     = p->raw? p->cols : left + p->cols + between + spectrum_width + right;
  png_byte   *pixels   = lsx_malloc(cols * rows * sizeof(*pixels));
  png_bytepp  png_rows = lsx_malloc(rows * sizeof(*png_rows));
  png_structp png      = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0,0);
  png_infop   png_info = png_create_info_struct(png);
  png_color   palette[256];
  int         i, j, k;
  int         base;
  int         step;
  int         tick_len = 3 - p->no_axes;
  char        text[200];
  char       *prefix;
  double      limit;
  float       autogain = 0.0;	/* Is changed if the -n flag was supplied */

  free(p->shared);

  if (p->using_stdout) {
    SET_BINARY_MODE(stdout);
    file = stdout;
  } else {
    file = fopen(p->out_name, "wb");
    if (!file) {
      lsx_fail("failed to create `%s': %s", p->out_name, strerror(errno));
      goto error;
    }
  }

  lsx_debug("signal-max=%g", p->max);

  font = lsx_malloc(font_len);
  assert(uncompress(font, &font_len, fixed, sizeof(fixed)) == Z_OK);

  make_palette(p, palette);
  memset(pixels, Background, cols * rows * sizeof(*pixels));

  png_init_io(png, file);
  png_set_PLTE(png, png_info, palette, fixed_palette + p->spectrum_points);
  png_set_IHDR(png, png_info, cols, rows, 8,
      PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  for (j = 0; j < rows; ++j)    /* Put (0,0) at bottom-left of PNG */
    png_rows[rows - 1 - j] = pixels + j * cols;

  /* Spectrogram */

  if (p->normalize)
    /* values are already in dB, so we subtract the maximum value
     * (which will normally be negative) to raise the maximum to 0.0.
     */
    autogain = -p->max;

  for (k = 0; k < chans; ++k) {
    priv_t *q = (effp - effp->flow + k)->priv;

    if (p->normalize) {
      float *fp = q->dBfs;
      for (i = p->rows * p->cols; i > 0; i--)
	*fp++ += autogain;
    }

    base = !p->raw * below + (chans - 1 - k) * (p->rows + 1);

    for (j = 0; j < p->rows; ++j) {
      for (i = 0; i < p->cols; ++i)
        pixel(!p->raw * left + i, base + j) = colour(p, q->dBfs[i*p->rows + j]);

      if (!p->raw && !p->no_axes) /* Y-axis lines */
        pixel(left - 1, base + j) = pixel(left + p->cols, base + j) = Grid;
    }

    if (!p->raw && !p->no_axes)   /* X-axis lines */
      for (i = -1; i <= p->cols; ++i)
        pixel(left + i, base - 1) = pixel(left + i, base + p->rows) = Grid;
  }

  if (!p->raw) {
    if (p->title && (i = strlen(p->title) * font_X) < cols + 1) /* Title */
      print_at((cols - i) / 2, rows - font_y, Text, p->title);

    if (strlen(p->comment) * font_X < cols + 1)     /* Footer comment */
      print_at(1, font_y, Text, p->comment);

    /* X-axis */
    step = axis(secs(p->cols), p->cols / (font_X * 9 / 2), &limit, &prefix);
    sprintf(text, "Time (%.1ss)", prefix);               /* Axis label */
    print_at(left + (p->cols - font_X * strlen(text)) / 2, 24, Text, text);

    for (i = 0; i <= limit; i += step) {
      int x = limit ? (double)i / limit * p->cols + .5 : 0;
      int y;

      for (y = 0; y < tick_len; ++y)                     /* Ticks */
        pixel(left-1+x, below-1-y) = pixel(left-1+x, below+c_rows+y) = Grid;

      if (step == 5 && (i%10))
        continue;

      sprintf(text, "%g", .1 * i);                       /* Tick labels */
      x = left + x - 3 * strlen(text);
      print_at(x, below - 6, Labels, text);
      print_at(x, below + c_rows + 14, Labels, text);
    }

    /* Y-axis */
    step = axis(effp->in_signal.rate / 2,
        (p->rows - 1) / ((font_y * 3 + 1) >> 1), &limit, &prefix);
    sprintf(text, "Frequency (%.1sHz)", prefix);         /* Axis label */
    print_up(10, below + (c_rows - font_X * strlen(text)) / 2, Text, text);

    for (k = 0; k < chans; ++k) {
      base = below + k * (p->rows + 1);

      for (i = 0; i <= limit; i += step) {
        int y = limit ? (double)i / limit * (p->rows - 1) + .5 : 0;
        int x;

        for (x = 0; x < tick_len; ++x)                   /* Ticks */
          pixel(left-1-x, base+y) = pixel(left+p->cols+x, base+y) = Grid;

        if ((step == 5 && (i%10)) || (!i && k && chans > 1))
          continue;

        sprintf(text, i?"%5g":"   DC", .1 * i);          /* Tick labels */
        print_at(left - 4 - font_X * 5, base + y + 5, Labels, text);
        sprintf(text, i?"%g":"DC", .1 * i);
        print_at(left + p->cols + 6, base + y + 5, Labels, text);
      }
    }

    /* Z-axis */
    k = min(400, c_rows);
    base = below + (c_rows - k) / 2;
    print_at(cols - right - 2 - font_X, base - 13, Text, "dBFS");/* Axis label */
    for (j = 0; j < k; ++j) {                            /* Spectrum */
      png_byte b = colour(p, p->dB_range * (j / (k - 1.) - 1));
      for (i = 0; i < spectrum_width; ++i)
        pixel(cols - right - 1 - i, base + j) = b;
    }

    step = 10 * ceil(p->dB_range / 10. * (font_y + 2) / (k - 1));

    for (i = 0; i <= p->dB_range; i += step) {           /* (Tick) labels */
      int y = (double)i / p->dB_range * (k - 1) + .5;
      sprintf(text, "%+i", i - p->gain - p->dB_range - (int)(autogain+0.5));
      print_at(cols - right + 1, base + y + 5, Labels, text);
    }
  }

  free(font);

  png_set_rows(png, png_info, png_rows);
  png_write_png(png, png_info, PNG_TRANSFORM_IDENTITY, NULL);

  if (!p->using_stdout)
    fclose(file);

error:
  png_destroy_write_struct(&png, &png_info);
  free(png_rows);
  free(pixels);
  free(p->dBfs);
  free(p->buf);
  free(p->dft_buf);
  free(p->window);
  free(p->magnitudes);

  return SOX_SUCCESS;
}

static int end(sox_effect_t *effp)
{
  priv_t *p = effp->priv;

  if (effp->flow == 0)
    return stop(effp);

  free(p->dBfs);

  return SOX_SUCCESS;
}

const sox_effect_handler_t *lsx_spectrogram_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "spectrogram",
    0,
    SOX_EFF_MODIFY,
    getopts,
    start,
    flow,
    drain,
    end,
    0,
    sizeof(priv_t)
  };
  static const char *lines[] = {
    "[options]",
    "\t-x num\tX-axis size in pixels; default derived or 800",
    "\t-X num\tX-axis pixels/second; default derived or 100",
    "\t-y num\tY-axis size in pixels (per channel); slow if not 1 + 2^n",
    "\t-Y num\tY-height total (i.e. not per channel); default 550",
    "\t-z num\tZ-axis range in dB; default 120",
    "\t-Z num\tZ-axis maximum in dBFS; default 0",
    "\t-n\tSet Z-axis maximum to the brightest pixel",
    "\t-q num\tZ-axis quantisation (0 - 249); default 249",
    "\t-w name\tWindow: Hann(default)/Hamming/Bartlett/Rectangular/Kaiser/Dolph",
    "\t-W num\tWindow adjust parameter (-10 - 10); applies only to Kaiser/Dolph",
    "\t-s\tSlack overlap of windows",
    "\t-a\tSuppress axis lines",
    "\t-r\tRaw spectrogram; no axes or legends",
    "\t-l\tLight background",
    "\t-m\tMonochrome",
    "\t-h\tHigh colour",
    "\t-p num\tPermute colours (1 - 6); default 1",
    "\t-A\tAlternative, inferior, fixed colour-set (for compatibility only)",
    "\t-t text\tTitle text",
    "\t-c text\tComment text",
    "\t-o text\tOutput file name; default `spectrogram.png'",
    "\t-d time\tAudio duration to fit to X-axis; e.g. 1:00, 48",
    "\t-S position\tStart the spectrogram at the given input position",
  };
  static char *usage;
  handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));
  return &handler;
}
