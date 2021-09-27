/* libSoX synth - Synthesizer Effect.
 *
 * Copyright (c) 2001-2009 SoX contributors
 * Copyright (c) Jan 2001  Carsten Borchardt
 *
 * This source code is freely redistributable and may be used for any purpose.
 * This copyright notice must be maintained.  The authors are not responsible
 * for the consequences of using this software.
 *
 * Except for synth types: pluck, tpdf, pinknoise, & brownnoise, and
 *            sweep types: linear, square & exp, which are:
 *
 * Copyright (c) 2006-2013 robs@users.sourceforge.net
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

#include <string.h>
#include <ctype.h>

typedef enum {
  synth_sine,
  synth_square,
  synth_sawtooth,
  synth_triangle,
  synth_trapezium,
  synth_trapetz  = synth_trapezium,   /* Deprecated name for trapezium */
  synth_exp,
                                      /* Tones above, noises below */
  synth_whitenoise,
  synth_noise = synth_whitenoise,     /* Just a handy alias */
  synth_tpdfnoise,
  synth_pinknoise,
  synth_brownnoise,
  synth_pluck
} type_t;

static lsx_enum_item const synth_type[] = {
  LSX_ENUM_ITEM(synth_, sine)
  LSX_ENUM_ITEM(synth_, square)
  LSX_ENUM_ITEM(synth_, sawtooth)
  LSX_ENUM_ITEM(synth_, triangle)
  LSX_ENUM_ITEM(synth_, trapezium)
  LSX_ENUM_ITEM(synth_, trapetz)
  LSX_ENUM_ITEM(synth_, exp)
  LSX_ENUM_ITEM(synth_, whitenoise)
  LSX_ENUM_ITEM(synth_, noise)
  LSX_ENUM_ITEM(synth_, tpdfnoise)
  LSX_ENUM_ITEM(synth_, pinknoise)
  LSX_ENUM_ITEM(synth_, brownnoise)
  LSX_ENUM_ITEM(synth_, pluck)
  {0, 0}
};

typedef enum {synth_create, synth_mix, synth_amod, synth_fmod} combine_t;

static lsx_enum_item const combine_type[] = {
  LSX_ENUM_ITEM(synth_, create)
  LSX_ENUM_ITEM(synth_, mix)
  LSX_ENUM_ITEM(synth_, amod)
  LSX_ENUM_ITEM(synth_, fmod)
  {0, 0}
};



typedef enum {Linear, Square, Exp, Exp_cycle} sweep_t;

typedef struct {
  /* options */
  type_t type;
  combine_t combine;
  double freq, freq2, mult;
  sweep_t sweep;
  double offset, phase;
  double p1, p2, p3; /* Use depends on synth type */

  /* internal stuff */
  double lp_last_out, hp_last_out, hp_last_in, ap_last_out, ap_last_in;
  double cycle_start_time_s, c0, c1, c2, c3, c4, c5, c6;

  double * buffer;
  size_t buffer_len, pos;
} channel_t;



/* Private data for the synthesizer */
typedef struct {
  char *        length_str;
  channel_t *   getopts_channels;
  size_t        getopts_nchannels;
  uint64_t      samples_done;
  uint64_t      samples_to_do;
  channel_t *   channels;
  size_t        number_of_channels;
  sox_bool      no_headroom;
  double        gain;
} priv_t;



static void create_channel(channel_t *  chan)
{
  memset(chan, 0, sizeof(*chan));
  chan->freq2 = chan->freq = 440;
  chan->p3 = chan->p2 = chan->p1 = -1;
}



static void set_default_parameters(channel_t *  chan)
{
  switch (chan->type) {
    case synth_square:    /* p1 is pulse width */
      if (chan->p1 < 0)
        chan->p1 = 0.5;   /* default to 50% duty cycle */
      break;

    case synth_triangle:  /* p1 is position of maximum */
      if (chan->p1 < 0)
        chan->p1 = 0.5;
      break;

    case synth_trapezium:
      /* p1 is length of rising slope,
       * p2 position where falling slope begins
       * p3 position of end of falling slope
       */
      if (chan->p1 < 0) {
        chan->p1 = 0.1;
        chan->p2 = 0.5;
        chan->p3 = 0.6;
      } else if (chan->p2 < 0) { /* try a symmetric waveform */
        if (chan->p1 <= 0.5) {
          chan->p2 = (1 - 2 * chan->p1) / 2;
          chan->p3 = chan->p2 + chan->p1;
        } else {
          /* symetric is not possible, fall back to asymmetrical triangle */
          chan->p2 = chan->p1;
          chan->p3 = 1;
        }
      } else if (chan->p3 < 0)
        chan->p3 = 1;     /* simple falling slope to the end */
      break;

    case synth_exp:
      if (chan->p1 < 0) /* p1 is position of maximum */
        chan->p1 = 0.5;
      if (chan->p2 < 0) /* p2 is amplitude */
        chan->p2 = .5;
      break;

    case synth_pluck:
      if (chan->p1 < 0)
        chan->p1 = .4;
      if (chan->p2 < 0)
        chan->p2 = .2, chan->p3 = .9;

    default: break;
  }
}



#undef NUMERIC_PARAMETER
#define NUMERIC_PARAMETER(p, min, max) { \
char * end_ptr_np; \
double d_np = strtod(argv[argn], &end_ptr_np); \
if (end_ptr_np == argv[argn]) \
  break; \
if (d_np < min || d_np > max || *end_ptr_np != '\0') { \
  lsx_fail("parameter error"); \
  return SOX_EOF; \
} \
chan->p = d_np / 100; /* adjust so abs(parameter) <= 1 */\
if (++argn == argc) \
  break; \
}



static int getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *) effp->priv;
  channel_t master, * chan = &master;
  int key = INT_MAX, argn = 0;
  char dummy, * end_ptr;
  const char *n;
  --argc, ++argv;

  if (argc && !strcmp(*argv, "-n")) p->no_headroom = sox_true, ++argv, --argc;

  if (argc > 1 && !strcmp(*argv, "-j") && (
        sscanf(argv[1], "%i %c", &key, &dummy) == 1 || (
          (key = lsx_parse_note(argv[1], &end_ptr)) != INT_MAX &&
          !*end_ptr))) {
    argc -= 2;
    argv += 2;
  }

  /* Get duration if given (if first arg starts with digit) */
  if (argc && (isdigit((int)argv[argn][0]) || argv[argn][0] == '.')) {
    p->length_str = lsx_strdup(argv[argn]);
    /* Do a dummy parse of to see if it will fail */
    n = lsx_parsesamples(0., p->length_str, &p->samples_to_do, 't');
    if (!n || *n)
      return lsx_usage(effp);
    argn++;
  }

  create_channel(chan);
  if (argn < argc) {            /* [off [ph [p1 [p2 [p3]]]]]] */
    do { /* break-able block */
      NUMERIC_PARAMETER(offset,-100, 100)
      NUMERIC_PARAMETER(phase ,   0, 100)
      NUMERIC_PARAMETER(p1,   0, 100)
      NUMERIC_PARAMETER(p2,   0, 100)
      NUMERIC_PARAMETER(p3,   0, 100)
    } while (0);
  }

  while (argn < argc) { /* type [combine] [f1[-f2] [off [ph [p1 [p2 [p3]]]]]] */
    lsx_enum_item const * enum_p = lsx_find_enum_text(argv[argn], synth_type, lsx_find_enum_item_case_sensitive);

    if (enum_p == NULL) {
      lsx_fail("no type given");
      return SOX_EOF;
    }
    p->getopts_channels = lsx_realloc(p->getopts_channels, sizeof(*p->getopts_channels) * (p->getopts_nchannels + 1));
    chan = &p->getopts_channels[p->getopts_nchannels++];
    memcpy(chan, &master, sizeof(*chan));
    chan->type = enum_p->value;
    if (++argn == argc)
      break;

    /* maybe there is a combine-type in next arg */
    enum_p = lsx_find_enum_text(argv[argn], combine_type, lsx_find_enum_item_case_sensitive);
    if (enum_p != NULL) {
      chan->combine = enum_p->value;
      if (++argn == argc)
        break;
    }

    /* read frequencies if given */
    if (!lsx_find_enum_text(argv[argn], synth_type, lsx_find_enum_item_case_sensitive) &&
        argv[argn][0] != '-') {
      static const char sweeps[] = ":+/-";

      chan->freq2 = chan->freq = lsx_parse_frequency_k(argv[argn], &end_ptr, key);
      if (chan->freq < (chan->type == synth_pluck? 27.5 : 0) ||
          (chan->type == synth_pluck && chan->freq > 4220)) {
        lsx_fail("invalid freq");
        return SOX_EOF;
      }
      if (*end_ptr && strchr(sweeps, *end_ptr)) {         /* freq2 given? */
        if (chan->type >= synth_noise) {
          lsx_fail("can't sweep this type");
          return SOX_EOF;
        }
        chan->sweep = strchr(sweeps, *end_ptr) - sweeps;
        chan->freq2 = lsx_parse_frequency_k(end_ptr + 1, &end_ptr, key);
        if (chan->freq2 < 0) {
          lsx_fail("invalid freq2");
          return SOX_EOF;
        }
        if (p->length_str == NULL) {
          lsx_fail("duration must be given when using freq2");
          return SOX_EOF;
        }
      }
      if (*end_ptr) {
        lsx_fail("frequency: invalid trailing character");
        return SOX_EOF;
      }
      if (chan->sweep >= Exp && chan->freq * chan->freq2 == 0) {
        lsx_fail("invalid frequency for exponential sweep");
        return SOX_EOF;
      }

      if (++argn == argc)
        break;
    }

    /* read rest of parameters */
    do { /* break-able block */
      NUMERIC_PARAMETER(offset,-100, 100)
      NUMERIC_PARAMETER(phase ,   0, 100)
      NUMERIC_PARAMETER(p1,   0, 100)
      NUMERIC_PARAMETER(p2,   0, 100)
      NUMERIC_PARAMETER(p3,   0, 100)
    } while (0);
  }

  /* If no channel parameters were given, create one default channel: */
  if (!p->getopts_nchannels) {
    p->getopts_channels = lsx_malloc(sizeof(*p->getopts_channels));
    memcpy(&p->getopts_channels[0], &master, sizeof(channel_t));
    ++p->getopts_nchannels;
  }

  if (!effp->in_signal.channels)
    effp->in_signal.channels = p->getopts_nchannels;

  return SOX_SUCCESS;
}



static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i, j, k;

  p->samples_done = 0;

  if (p->length_str) {
    if (lsx_parsesamples(effp->in_signal.rate, p->length_str, &p->samples_to_do, 't') == NULL)
      return lsx_usage(effp);
  } else
    p->samples_to_do = effp->in_signal.length != SOX_UNKNOWN_LEN ?
        effp->in_signal.length / effp->in_signal.channels : 0;

  p->number_of_channels = effp->in_signal.channels;
  p->channels = lsx_calloc(p->number_of_channels, sizeof(*p->channels));
  for (i = 0; i < p->number_of_channels; ++i) {
    channel_t *  chan = &p->channels[i];
    *chan = p->getopts_channels[i % p->getopts_nchannels];
    set_default_parameters(chan);
    if (chan->type == synth_pluck) {
      double min, max, frac, p2;

      /* Low pass: */
      double const decay_rate = -2; /* dB / s */
      double const decay_f = min(912, 266 + 106 * log(chan->freq));
      double d = sqr(dB_to_linear(decay_rate / chan->freq));
      d = (d * cos(2 * M_PI * decay_f / effp->in_signal.rate) - 1) / (d - 1);
      chan->c0 = d - sqrt(d * d - 1);
      chan->c1 = 1 - chan->c0;

      /* Single-pole low pass is very rate-dependent: */
      if (effp->in_signal.rate < 44100 || effp->in_signal.rate > 48000) {
        lsx_fail(
          "sample rate for pluck must be 44100-48000; use `rate' to resample");
        return SOX_EOF;
      }
      /* Decay: */
      chan->c1 *= exp(-2e4/ (.05+chan->p1)/ chan->freq/ effp->in_signal.rate);

      /* High pass (DC-block): */
      chan->c2 = exp(-2 * M_PI * 10 / effp->in_signal.rate);
      chan->c3 = (1 + chan->c2) * .5;

      /* All pass (for fractional delay): */
      d = chan->c0 / (chan->c0 + chan->c1);
      chan->buffer_len = effp->in_signal.rate / chan->freq - d;
      frac = effp->in_signal.rate / chan->freq - d - chan->buffer_len;
      chan->c4 = (1 - frac) / (1 + frac);
      chan->pos = 0;

      /* Exitation: */
      chan->buffer = lsx_calloc(chan->buffer_len, sizeof(*chan->buffer));
      for (k = 0, p2 = chan->p2; k < 2 && p2 >= 0; ++k, p2 = chan->p3) {
        double d1 = 0, d2, colour = pow(2., 4 * (p2 - 1));
        int32_t r = p2 * 100 + .5;
        for (j = 0; j < chan->buffer_len; ++j) {
          do d2 = d1 + (chan->phase? DRANQD1:dranqd1(r)) * colour;
          while (fabs(d2) > 1);
          chan->buffer[j] += d2 * (1 - .3 * k);
          d1 = d2 * (colour != 1);
#ifdef TEST_PLUCK
          chan->buffer[j] = sin(2 * M_PI * j / chan->buffer_len);
#endif
        }
      }

      /* In-delay filter graduation: */
      for (j = 0, min = max = 0; j < chan->buffer_len; ++j) {
        double d2, t = (double)j / chan->buffer_len;
        chan->lp_last_out = d2 =
          chan->buffer[j] * chan->c1 + chan->lp_last_out * chan->c0;

        chan->ap_last_out =
          d2 * chan->c4 + chan->ap_last_in - chan->ap_last_out * chan->c4;
        chan->ap_last_in = d2;

        chan->buffer[j] = chan->buffer[j] * (1 - t) + chan->ap_last_out * t;
        min = min(min, chan->buffer[j]);
        max = max(max, chan->buffer[j]);
      }

      /* Normalise: */
      for (j = 0, d = 0; j < chan->buffer_len; ++j) {
        chan->buffer[j] = (2 * chan->buffer[j] - max - min) / (max - min);
        d += sqr(chan->buffer[j]);
      }
      lsx_debug("rms=%f c0=%f c1=%f df=%f d3f=%f c2=%f c3=%f c4=%f frac=%f",
          10 * log(d / chan->buffer_len), chan->c0, chan->c1, decay_f,
          log(chan->c0)/ -2 / M_PI * effp->in_signal.rate,
          chan->c2, chan->c3, chan->c4, frac);
    }
    switch (chan->sweep) {
      case Linear: chan->mult = p->samples_to_do?
          (chan->freq2 - chan->freq) / p->samples_to_do / 2 : 0;
        break;
      case Square: chan->mult = p->samples_to_do?
           sqrt(fabs(chan->freq2 - chan->freq)) / p->samples_to_do / sqrt(3.) : 0;
        if (chan->freq > chan->freq2)
          chan->mult = -chan->mult;
        break;
      case Exp: chan->mult = p->samples_to_do?
          log(chan->freq2 / chan->freq) / p->samples_to_do * effp->in_signal.rate : 1;
        chan->freq /= chan->mult;
        break;
      case Exp_cycle: chan->mult = p->samples_to_do?
          (log(chan->freq2) - log(chan->freq)) / p->samples_to_do : 1;
        break;
    }
    lsx_debug("type=%s, combine=%s, samples_to_do=%" PRIu64 ", f1=%g, f2=%g, "
              "offset=%g, phase=%g, p1=%g, p2=%g, p3=%g mult=%g",
        lsx_find_enum_value(chan->type, synth_type)->text,
        lsx_find_enum_value(chan->combine, combine_type)->text,
        p->samples_to_do, chan->freq, chan->freq2,
        chan->offset, chan->phase, chan->p1, chan->p2, chan->p3, chan->mult);
  }
  p->gain = 1;
  effp->out_signal.mult = p->no_headroom? NULL : &p->gain;
  effp->out_signal.length = p->samples_to_do ?
    p->samples_to_do * effp->out_signal.channels : SOX_UNKNOWN_LEN;
  return SOX_SUCCESS;
}

#define elapsed_time_s p->samples_done / effp->in_signal.rate

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf, sox_sample_t * obuf,
    size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *) effp->priv;
  unsigned len = min(*isamp, *osamp) / effp->in_signal.channels;
  unsigned c, done;
  int result = SOX_SUCCESS;

  for (done = 0; done < len && result == SOX_SUCCESS; ++done) {
    for (c = 0; c < effp->in_signal.channels; c++) {
      sox_sample_t synth_input = *ibuf++;
      channel_t *  chan = &p->channels[c];
      double synth_out;              /* [-1, 1] */

      if (chan->type < synth_noise) { /* Need to calculate phase: */
        double phase;            /* [0, 1) */
        switch (chan->sweep) {
          case Linear:
            phase = (chan->freq + p->samples_done * chan->mult) *
                elapsed_time_s;
            break;
          case Square:
            phase = (chan->freq + sign(chan->mult) * 
                sqr(p->samples_done * chan->mult)) * elapsed_time_s;
            break;
          case Exp:
            phase = chan->freq * exp(chan->mult * elapsed_time_s);
            break;
          case Exp_cycle: default: {
            double f = chan->freq * exp(p->samples_done * chan->mult);
            double cycle_elapsed_time_s = elapsed_time_s - chan->cycle_start_time_s;
            if (f * cycle_elapsed_time_s >= 1) {  /* move to next cycle */
              chan->cycle_start_time_s += 1 / f;
              cycle_elapsed_time_s = elapsed_time_s - chan->cycle_start_time_s;
            }
            phase = f * cycle_elapsed_time_s;
            break;
          }
        }
        phase = fmod(phase + chan->phase, 1.0);

        switch (chan->type) {
          case synth_sine:
            synth_out = sin(2 * M_PI * phase);
            break;

          case synth_square:
            /* |_______           | +1
             * |       |          |
             * |_______|__________|  0
             * |       |          |
             * |       |__________| -1
             * |                  |
             * 0       p1          1
             */
            synth_out = -1 + 2 * (phase < chan->p1);
            break;

          case synth_sawtooth:
            /* |           __| +1
             * |        __/  |
             * |_______/_____|  0
             * |  __/        |
             * |_/           | -1
             * |             |
             * 0             1
             */
            synth_out = -1 + 2 * phase;
            break;

          case synth_triangle:
            /* |    .    | +1
             * |   / \   |
             * |__/___\__|  0
             * | /     \ |
             * |/       \| -1
             * |         |
             * 0   p1    1
             */

            if (phase < chan->p1)
              synth_out = -1 + 2 * phase / chan->p1;          /* In rising part of period */
            else
              synth_out = 1 - 2 * (phase - chan->p1) / (1 - chan->p1); /* In falling part */
            break;

          case synth_trapezium:
            /* |    ______             |+1
             * |   /      \            |
             * |__/________\___________| 0
             * | /          \          |
             * |/            \_________|-1
             * |                       |
             * 0   p1    p2   p3       1
             */
            if (phase < chan->p1)       /* In rising part of period */
              synth_out = -1 + 2 * phase / chan->p1;
            else if (phase < chan->p2)  /* In high part of period */
              synth_out = 1;
            else if (phase < chan->p3)  /* In falling part */
              synth_out = 1 - 2 * (phase - chan->p2) / (chan->p3 - chan->p2);
            else                        /* In low part of period */
              synth_out = -1;
            break;

          case synth_exp:
            /* |             |              | +1
             * |            | |             |
             * |          _|   |_           | 0
             * |       __-       -__        |
             * |____---             ---____ | f(p2)
             * |                            |
             * 0             p1             1
             */
            synth_out = dB_to_linear(chan->p2 * -200);  /* 0 ..  1 */
            if (phase < chan->p1)
              synth_out = synth_out * exp(phase * log(1 / synth_out) / chan->p1);
            else
              synth_out = synth_out * exp((1 - phase) * log(1 / synth_out) / (1 - chan->p1));
            synth_out = synth_out * 2 - 1;      /* map 0 .. 1 to -1 .. +1 */
            break;

          default: synth_out = 0;
        }
      } else switch (chan->type) {
        case synth_whitenoise:
          synth_out = DRANQD1;
          break;

        case synth_tpdfnoise:
          synth_out = .5 * (DRANQD1 + DRANQD1);
          break;

        case synth_pinknoise: { /* "Paul Kellet's refined method" */
#define _ .125 / (65536. * 32768.)
          double d = RANQD1;
          chan->c0 = .99886 * chan->c0 + d * (.0555179*_); 
          chan->c1 = .99332 * chan->c1 + d * (.0750759*_); 
          chan->c2 = .96900 * chan->c2 + d * (.1538520*_); 
          chan->c3 = .86650 * chan->c3 + d * (.3104856*_); 
          chan->c4 = .55000 * chan->c4 + d * (.5329522*_); 
          chan->c5 = -.7616 * chan->c5 - d * (.0168980*_); 
          synth_out = chan->c0 + chan->c1 + chan->c2 + chan->c3
                    + chan->c4 + chan->c5 + chan->c6 + d * (.5362*_); 
          chan->c6 = d * (.115926*_); 
          break;
#undef _
        }

        case synth_brownnoise:
          do synth_out = chan->lp_last_out + DRANQD1 * (1. / 16);
          while (fabs(synth_out) > 1);
          chan->lp_last_out = synth_out;
          break;

        case synth_pluck: {
          double d = chan->buffer[chan->pos];

          chan->hp_last_out = 
             (d - chan->hp_last_in) * chan->c3 + chan->hp_last_out * chan->c2;
          chan->hp_last_in = d;
        
          synth_out = range_limit(chan->hp_last_out, -1, 1);

          chan->lp_last_out = d = d * chan->c1 + chan->lp_last_out * chan->c0;

          chan->ap_last_out = chan->buffer[chan->pos] =
            (d - chan->ap_last_out) * chan->c4 + chan->ap_last_in;
          chan->ap_last_in = d;

          chan->pos = chan->pos + 1 == chan->buffer_len? 0 : chan->pos + 1;
          break;
        }

        default: synth_out = 0;
      }

      /* Add offset, but prevent clipping: */
      synth_out = synth_out * (1 - fabs(chan->offset)) + chan->offset;

      switch (chan->combine) {
        case synth_create: synth_out *=  SOX_SAMPLE_MAX; break;
        case synth_mix   : synth_out = (synth_out * SOX_SAMPLE_MAX + synth_input) * .5; break;
        case synth_amod  : synth_out = (synth_out + 1) * synth_input * .5; break;
        case synth_fmod  : synth_out *=  synth_input; break;
      }
      *obuf++ = synth_out < 0? synth_out * p->gain - .5 : synth_out * p->gain + .5;
    }
    if (++p->samples_done == p->samples_to_do)
      result = SOX_EOF;
  }
  *isamp = *osamp = done * effp->in_signal.channels;
  return result;
}



static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  size_t i;

  for (i = 0; i < p->number_of_channels; ++i)
    free(p->channels[i].buffer);
  free(p->channels);
  return SOX_SUCCESS;
}



static int lsx_kill(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  free(p->getopts_channels);
  free(p->length_str);
  return SOX_SUCCESS;
}



const sox_effect_handler_t *lsx_synth_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "synth", "[-j KEY] [-n] [length [offset [phase [p1 [p2 [p3]]]]]]] {type [combine] [[%]freq[k][:|+|/|-[%]freq2[k]] [offset [phase [p1 [p2 [p3]]]]]]}",
    SOX_EFF_MCHAN | SOX_EFF_LENGTH | SOX_EFF_GAIN,
    getopts, start, flow, 0, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
