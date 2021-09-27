/* libSoX effect: stereo reverberation
 * Copyright (c) 2007 robs@users.sourceforge.net
 * Filter design based on freeverb by Jezar at Dreampoint.
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

#include "sox_i.h"
#include "fifo.h"

#define lsx_zalloc(var, n) var = lsx_calloc(n, sizeof(*var))
#define filter_advance(p) if (--(p)->ptr < (p)->buffer) (p)->ptr += (p)->size
#define filter_delete(p) free((p)->buffer)

typedef struct {
  size_t  size;
  float   * buffer, * ptr;
  float   store;
} filter_t;

static float comb_process(filter_t * p,  /* gcc -O2 will inline this */
    float const * input, float const * feedback, float const * hf_damping)
{
  float output = *p->ptr;
  p->store = output + (p->store - output) * *hf_damping;
  *p->ptr = *input + p->store * *feedback;
  filter_advance(p);
  return output;
}

static float allpass_process(filter_t * p,  /* gcc -O2 will inline this */
    float const * input)
{
  float output = *p->ptr;
  *p->ptr = *input + output * .5;
  filter_advance(p);
  return output - *input;
}

static const size_t /* Filter delay lengths in samples (44100Hz sample-rate) */
  comb_lengths[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617},
  allpass_lengths[] = {225, 341, 441, 556};
#define stereo_adjust 12

typedef struct {
  filter_t comb   [array_length(comb_lengths)];
  filter_t allpass[array_length(allpass_lengths)];
} filter_array_t;

static void filter_array_create(filter_array_t * p, double rate,
    double scale, double offset)
{
  size_t i;
  double r = rate * (1 / 44100.); /* Compensate for actual sample-rate */

  for (i = 0; i < array_length(comb_lengths); ++i, offset = -offset)
  {
    filter_t * pcomb = &p->comb[i];
    pcomb->size = (size_t)(scale * r * (comb_lengths[i] + stereo_adjust * offset) + .5);
    pcomb->ptr = lsx_zalloc(pcomb->buffer, pcomb->size);
  }
  for (i = 0; i < array_length(allpass_lengths); ++i, offset = -offset)
  {
    filter_t * pallpass = &p->allpass[i];
    pallpass->size = (size_t)(r * (allpass_lengths[i] + stereo_adjust * offset) + .5);
    pallpass->ptr = lsx_zalloc(pallpass->buffer, pallpass->size);
  }
}

static void filter_array_process(filter_array_t * p,
    size_t length, float const * input, float * output,
    float const * feedback, float const * hf_damping, float const * gain)
{
  while (length--) {
    float out = 0, in = *input++;

    size_t i = array_length(comb_lengths) - 1;
    do out += comb_process(p->comb + i, &in, feedback, hf_damping);
    while (i--);

    i = array_length(allpass_lengths) - 1;
    do out = allpass_process(p->allpass + i, &out);
    while (i--);

    *output++ = out * *gain;
  }
}

static void filter_array_delete(filter_array_t * p)
{
  size_t i;

  for (i = 0; i < array_length(allpass_lengths); ++i)
    filter_delete(&p->allpass[i]);
  for (i = 0; i < array_length(comb_lengths); ++i)
    filter_delete(&p->comb[i]);
}

typedef struct {
  float feedback;
  float hf_damping;
  float gain;
  fifo_t input_fifo;
  filter_array_t chan[2];
  float * out[2];
} reverb_t;

static void reverb_create(reverb_t * p, double sample_rate_Hz,
    double wet_gain_dB,
    double room_scale,     /* % */
    double reverberance,   /* % */
    double hf_damping,     /* % */
    double pre_delay_ms,
    double stereo_depth,
    size_t buffer_size,
    float * * out)
{
  size_t i, delay = pre_delay_ms / 1000 * sample_rate_Hz + .5;
  double scale = room_scale / 100 * .9 + .1;
  double depth = stereo_depth / 100;
  double a =  -1 /  log(1 - /**/.3 /**/);           /* Set minimum feedback */
  double b = 100 / (log(1 - /**/.98/**/) * a + 1);  /* Set maximum feedback */

  memset(p, 0, sizeof(*p));
  p->feedback = 1 - exp((reverberance - b) / (a * b));
  p->hf_damping = hf_damping / 100 * .3 + .2;
  p->gain = dB_to_linear(wet_gain_dB) * .015;
  fifo_create(&p->input_fifo, sizeof(float));
  memset(fifo_write(&p->input_fifo, delay, 0), 0, delay * sizeof(float));
  for (i = 0; i <= ceil(depth); ++i) {
    filter_array_create(p->chan + i, sample_rate_Hz, scale, i * depth);
    out[i] = lsx_zalloc(p->out[i], buffer_size);
  }
}

static void reverb_process(reverb_t * p, size_t length)
{
  size_t i;
  for (i = 0; i < 2 && p->out[i]; ++i)
    filter_array_process(p->chan + i, length, (float *) fifo_read_ptr(&p->input_fifo), p->out[i], &p->feedback, &p->hf_damping, &p->gain);
  fifo_read(&p->input_fifo, length, NULL);
}

static void reverb_delete(reverb_t * p)
{
  size_t i;
  for (i = 0; i < 2 && p->out[i]; ++i) {
    free(p->out[i]);
    filter_array_delete(p->chan + i);
  }
  fifo_delete(&p->input_fifo);
}

/*------------------------------- SoX Wrapper --------------------------------*/

typedef struct {
  double reverberance, hf_damping, pre_delay_ms;
  double stereo_depth, wet_gain_dB, room_scale;
  sox_bool wet_only;

  size_t ichannels, ochannels;
  struct {
    reverb_t reverb;
    float * dry, * wet[2];
  } chan[2];
} priv_t;

static int getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *)effp->priv;
  p->reverberance = p->hf_damping = 50; /* Set non-zero defaults */
  p->stereo_depth = p->room_scale = 100;

  --argc, ++argv;
  p->wet_only = argc && (!strcmp(*argv, "-w") || !strcmp(*argv, "--wet-only"))
    && (--argc, ++argv, sox_true);
  do {  /* break-able block */
    NUMERIC_PARAMETER(reverberance, 0, 100)
    NUMERIC_PARAMETER(hf_damping, 0, 100)
    NUMERIC_PARAMETER(room_scale, 0, 100)
    NUMERIC_PARAMETER(stereo_depth, 0, 100)
    NUMERIC_PARAMETER(pre_delay_ms, 0, 500)
    NUMERIC_PARAMETER(wet_gain_dB, -10, 10)
  } while (0);

  return argc ? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i;

  p->ichannels = p->ochannels = 1;
  effp->out_signal.rate = effp->in_signal.rate;
  if (effp->in_signal.channels > 2 && p->stereo_depth) {
    lsx_warn("stereo-depth not applicable with >2 channels");
    p->stereo_depth = 0;
  }
  if (effp->in_signal.channels == 1 && p->stereo_depth)
    effp->out_signal.channels = p->ochannels = 2;
  else effp->out_signal.channels = effp->in_signal.channels;
  if (effp->in_signal.channels == 2 && p->stereo_depth)
    p->ichannels = p->ochannels = 2;
  else effp->flows = effp->in_signal.channels;
  for (i = 0; i < p->ichannels; ++i) reverb_create(
    &p->chan[i].reverb, effp->in_signal.rate, p->wet_gain_dB, p->room_scale,
    p->reverberance, p->hf_damping, p->pre_delay_ms, p->stereo_depth,
    effp->global_info->global_info->bufsiz / p->ochannels, p->chan[i].wet);

  if (effp->in_signal.mult)
    *effp->in_signal.mult /= !p->wet_only + 2 * dB_to_linear(max(0,p->wet_gain_dB));
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
                sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t c, i, w, len = min(*isamp / p->ichannels, *osamp / p->ochannels);
  SOX_SAMPLE_LOCALS;

  *isamp = len * p->ichannels, *osamp = len * p->ochannels;
  for (c = 0; c < p->ichannels; ++c)
    p->chan[c].dry = fifo_write(&p->chan[c].reverb.input_fifo, len, 0);
  for (i = 0; i < len; ++i) for (c = 0; c < p->ichannels; ++c)
    p->chan[c].dry[i] = SOX_SAMPLE_TO_FLOAT_32BIT(*ibuf++, effp->clips);
  for (c = 0; c < p->ichannels; ++c)
    reverb_process(&p->chan[c].reverb, len);
  if (p->ichannels == 2) for (i = 0; i < len; ++i) for (w = 0; w < 2; ++w) {
    float out = (1 - p->wet_only) * p->chan[w].dry[i] +
      .5 * (p->chan[0].wet[w][i] + p->chan[1].wet[w][i]);
    *obuf++ = SOX_FLOAT_32BIT_TO_SAMPLE(out, effp->clips);
  }
  else for (i = 0; i < len; ++i) for (w = 0; w < p->ochannels; ++w) {
    float out = (1 - p->wet_only) * p->chan[0].dry[i] + p->chan[0].wet[w][i];
    *obuf++ = SOX_FLOAT_32BIT_TO_SAMPLE(out, effp->clips);
  }
  return SOX_SUCCESS;
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i;
  for (i = 0; i < p->ichannels; ++i)
    reverb_delete(&p->chan[i].reverb);
  return SOX_SUCCESS;
}

sox_effect_handler_t const *lsx_reverb_effect_fn(void)
{
  static sox_effect_handler_t handler = {"reverb",
    "[-w|--wet-only]"
    " [reverberance (50%)"
    " [HF-damping (50%)"
    " [room-scale (100%)"
    " [stereo-depth (100%)"
    " [pre-delay (0ms)"
    " [wet-gain (0dB)"
    "]]]]]]",
    SOX_EFF_MCHAN, getopts, start, flow, NULL, stop, NULL, sizeof(priv_t)
  };
  return &handler;
}
