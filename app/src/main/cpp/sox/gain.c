/* libSoX effect: gain/norm/etc.   (c) 2008-9 robs@users.sourceforge.net
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

#define LSX_EFF_ALIAS
#include "sox_i.h"
#include <ctype.h>
#include <string.h>

typedef struct {
  sox_bool      do_equalise, do_balance, do_balance_no_clip, do_limiter;
  sox_bool      do_restore, make_headroom, do_normalise, do_scan;
  double        fixed_gain; /* Valid only in channel 0 */

  double        mult, reclaim, rms, limiter;
  off_t         num_samples;
  sox_sample_t  min, max;
  FILE          * tmp_file;
} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  char const * q;
  for (--argc, ++argv; argc && **argv == '-' && argv[0][1] &&
      !isdigit((unsigned char)argv[0][1]) && argv[0][1] != '.'; --argc, ++argv)
    for (q = &argv[0][1]; *q; ++q) switch (*q) {
      case 'n': p->do_scan = p->do_normalise = sox_true; break;
      case 'e': p->do_scan = p->do_equalise = sox_true; break;
      case 'B': p->do_scan = p->do_balance = sox_true; break;
      case 'b': p->do_scan = p->do_balance_no_clip = sox_true; break;
      case 'r': p->do_scan = p->do_restore = sox_true; break;
      case 'h': p->make_headroom = sox_true; break;
      case 'l': p->do_limiter = sox_true; break;
      default: lsx_fail("invalid option `-%c'", *q); return lsx_usage(effp);
    }
  if ((p->do_equalise + p->do_balance + p->do_balance_no_clip + p->do_restore)/ sox_true > 1) {
    lsx_fail("only one of -e, -B, -b, -r may be given");
    return SOX_EOF;
  }
  if (p->do_normalise && p->do_restore) {
    lsx_fail("only one of -n, -r may be given");
    return SOX_EOF;
  }
  if (p->do_limiter && p->make_headroom) {
    lsx_fail("only one of -l, -h may be given");
    return SOX_EOF;
  }
  do {NUMERIC_PARAMETER(fixed_gain, -HUGE_VAL, HUGE_VAL)} while (0);
  p->fixed_gain = dB_to_linear(p->fixed_gain);
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;

  if (effp->flow == 0) {
    if (p->do_restore) {
      if (!effp->in_signal.mult || *effp->in_signal.mult >= 1) {
        lsx_fail("can't reclaim headroom");
        return SOX_EOF;
      }
      p->reclaim = 1 / *effp->in_signal.mult;
    }
    effp->out_signal.mult = p->make_headroom? &p->fixed_gain : NULL;
    if (!p->do_equalise && !p->do_balance && !p->do_balance_no_clip)
      effp->flows = 1; /* essentially a conditional SOX_EFF_MCHAN */
  }
  p->mult = 0;
  p->max = 1;
  p->min = -1;
  if (p->do_scan) {
    p->tmp_file = lsx_tmpfile();
    if (p->tmp_file == NULL) {
      lsx_fail("can't create temporary file: %s", strerror(errno));
      return SOX_EOF;
    }
  }
  if (p->do_limiter)
    p->limiter = (1 - 1 / p->fixed_gain) * (1. / SOX_SAMPLE_MAX);
  else if (p->fixed_gain == floor(p->fixed_gain) && !p->do_scan)
    effp->out_signal.precision = effp->in_signal.precision;
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len;

  if (p->do_scan) {
    if (fwrite(ibuf, sizeof(*ibuf), *isamp, p->tmp_file) != *isamp) {
      lsx_fail("error writing temporary file: %s", strerror(errno));
      return SOX_EOF;
    }
    if (p->do_balance && !p->do_normalise)
      for (len = *isamp; len; --len, ++ibuf) {
        double d = SOX_SAMPLE_TO_FLOAT_64BIT(*ibuf, effp->clips);
        p->rms += sqr(d);
        ++p->num_samples;
      }
    else if (p->do_balance || p->do_balance_no_clip)
      for (len = *isamp; len; --len, ++ibuf) {
        double d = SOX_SAMPLE_TO_FLOAT_64BIT(*ibuf, effp->clips);
        p->rms += sqr(d);
        ++p->num_samples;
        p->max = max(p->max, *ibuf);
        p->min = min(p->min, *ibuf);
      }
    else for (len = *isamp; len; --len, ++ibuf) {
      p->max = max(p->max, *ibuf);
      p->min = min(p->min, *ibuf);
    }
    *osamp = 0; /* samples not output until drain */
  }
  else {
    double mult = ((priv_t *)(effp - effp->flow)->priv)->fixed_gain;
    len = *isamp = *osamp = min(*isamp, *osamp);
    if (!p->do_limiter) for (; len; --len, ++ibuf)
      *obuf++ = SOX_ROUND_CLIP_COUNT(*ibuf * mult, effp->clips);
    else for (; len; --len, ++ibuf) {
      double d = *ibuf * mult;
      *obuf++ = d < 0 ? 1 / (1 / d - p->limiter) - .5 :
                d > 0 ? 1 / (1 / d + p->limiter) + .5 : 0;
    }
  }
  return SOX_SUCCESS;
}

static void start_drain(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  double max = SOX_SAMPLE_MAX, max_peak = 0, max_rms = 0;
  size_t i;

  if (p->do_balance || p->do_balance_no_clip) {
    for (i = 0; i < effp->flows; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      max_rms = max(max_rms, sqrt(q->rms / q->num_samples));
      rewind(q->tmp_file);
    }
    for (i = 0; i < effp->flows; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      double this_rms = sqrt(q->rms / q->num_samples);
      double this_peak = max(q->max / max, q->min / (double)SOX_SAMPLE_MIN);
      q->mult = this_rms != 0? max_rms / this_rms : 1;
      max_peak = max(max_peak, q->mult * this_peak);
      q->mult *= p->fixed_gain;
    }
    if (p->do_normalise || (p->do_balance_no_clip && max_peak > 1))
      for (i = 0; i < effp->flows; ++i) {
        priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
        q->mult /= max_peak;
      }
  } else if (p->do_equalise && !p->do_normalise) {
    for (i = 0; i < effp->flows; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      double this_peak = max(q->max / max, q->min / (double)SOX_SAMPLE_MIN);
      max_peak = max(max_peak, this_peak);
      q->mult = p->fixed_gain / this_peak;
      rewind(q->tmp_file);
    }
    for (i = 0; i < effp->flows; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      q->mult *= max_peak;
    }
  } else {
    p->mult = min(max / p->max, (double)SOX_SAMPLE_MIN / p->min);
    if (p->do_restore) {
      if (p->reclaim > p->mult)
        lsx_report("%.3gdB not reclaimed", linear_to_dB(p->reclaim / p->mult));
      else p->mult = p->reclaim;
    }
    p->mult *= p->fixed_gain;
    rewind(p->tmp_file);
  }
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len;
  int result = SOX_SUCCESS;

  *osamp -= *osamp % effp->in_signal.channels;

  if (p->do_scan) {
    if (!p->mult)
      start_drain(effp);
    len = fread(obuf, sizeof(*obuf), *osamp, p->tmp_file);
    if (len != *osamp && !feof(p->tmp_file)) {
      lsx_fail("error reading temporary file: %s", strerror(errno));
      result = SOX_EOF;
    }
    if (!p->do_limiter) for (*osamp = len; len; --len, ++obuf)
      *obuf = SOX_ROUND_CLIP_COUNT(*obuf * p->mult, effp->clips);
    else for (*osamp = len; len; --len) {
      double d = *obuf * p->mult;
      *obuf++ = d < 0 ? 1 / (1 / d - p->limiter) - .5 :
                d > 0 ? 1 / (1 / d + p->limiter) + .5 : 0;
    }
  }
  else *osamp = 0;
  return result;
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  if (p->do_scan)
    fclose(p->tmp_file); /* auto-deleted by lsx_tmpfile */
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_gain_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "gain", NULL, SOX_EFF_GAIN,
    create, start, flow, drain, stop, NULL, sizeof(priv_t)};
  static char const * lines[] = {
    "[-e|-b|-B|-r] [-n] [-l|-h] [gain-dB]",
    "-e\t Equalise channels: peak to that with max peak;",
    "-B\t Balance channels: rms to that with max rms; no clip protection",
    "-b\t Balance channels: rms to that with max rms; clip protection",
    "\t   Note -Bn = -bn",
    "-r\t Reclaim headroom (as much as possible without clipping); see -h",
    "-n\t Norm file to 0dBfs(output precision); gain-dB, if present, usually <0",
    "-l\t Use simple limiter",
    "-h\t Apply attenuation for headroom for subsequent effects; gain-dB, if",
    "\t   present, is subject to reclaim by a subsequent gain -r",
    "gain-dB\t Apply gain in dB",
  };
  static char * usage;
  handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));
  return &handler;
}

/*------------------ emulation of the old `normalise' effect -----------------*/

static int norm_getopts(sox_effect_t * effp, int argc, char * * argv)
{
  char * argv2[3];
  int argc2 = 2;

  argv2[0] = argv[0], --argc, ++argv;
  argv2[1] = "-n";
  if (argc)
    argv2[argc2++] = *argv, --argc, ++argv;
  return argc? lsx_usage(effp) :
    lsx_gain_effect_fn()->getopts(effp, argc2, argv2);
}

sox_effect_handler_t const * lsx_norm_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_gain_effect_fn();
  handler.name = "norm";
  handler.usage = "[level]";
  handler.getopts = norm_getopts;
  return &handler;
}
