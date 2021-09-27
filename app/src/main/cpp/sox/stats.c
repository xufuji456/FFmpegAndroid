/* libSoX effect: stats   (c) 2009 robs@users.sourceforge.net
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
#include <ctype.h>
#include <string.h>

typedef struct {
  int       scale_bits, hex_bits;
  double    time_constant, scale;

  double    last, sigma_x, sigma_x2, avg_sigma_x2, min_sigma_x2, max_sigma_x2;
  double    min, max, mult, min_run, min_runs, max_run, max_runs;
  off_t     num_samples, tc_samples, min_count, max_count;
  uint32_t  mask;
} priv_t;

static int getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *)effp->priv;
  int c;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, "+x:b:w:s:", NULL, lsx_getopt_flag_none, 1, &optstate);

  p->time_constant = .05;
  p->scale = 1;
  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    GETOPT_NUMERIC(optstate, 'x', hex_bits      ,  2 , 32)
    GETOPT_NUMERIC(optstate, 'b', scale_bits    ,  2 , 32)
    GETOPT_NUMERIC(optstate, 'w', time_constant ,  .01 , 10)
    GETOPT_NUMERIC(optstate, 's', scale         ,  -99, 99)
    default: lsx_fail("invalid option `-%c'", optstate.opt); return lsx_usage(effp);
  }
  if (p->hex_bits)
    p->scale_bits = p->hex_bits;
  return optstate.ind != argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;

  p->last = 0;
  p->mult = exp((-1 / p->time_constant / effp->in_signal.rate));
  p->tc_samples = 5 * p->time_constant * effp->in_signal.rate + .5;
  p->sigma_x = p->sigma_x2 = p->avg_sigma_x2 = p->max_sigma_x2 = 0;
  p->min = p->min_sigma_x2 = 2;
  p->max = -p->min;
  p->num_samples = 0;
  p->mask = 0;
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * ilen, size_t * olen)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len = *ilen = *olen = min(*ilen, *olen);
  memcpy(obuf, ibuf, len * sizeof(*obuf));

  for (; len--; ++ibuf, ++p->num_samples) {
    double d = SOX_SAMPLE_TO_FLOAT_64BIT(*ibuf,);

    if (d < p->min)
      p->min = d, p->min_count = 1, p->min_run = 1, p->min_runs = 0;
    else if (d == p->min) {
      ++p->min_count;
      p->min_run = d == p->last? p->min_run + 1 : 1;
    }
    else if (p->last == p->min)
      p->min_runs += sqr(p->min_run);

    if (d > p->max)
      p->max = d, p->max_count = 1, p->max_run = 1, p->max_runs = 0;
    else if (d == p->max) {
      ++p->max_count;
      p->max_run = d == p->last? p->max_run + 1 : 1;
    }
    else if (p->last == p->max)
      p->max_runs += sqr(p->max_run);

    p->sigma_x += d;
    p->sigma_x2 += sqr(d);
    p->avg_sigma_x2 = p->avg_sigma_x2 * p->mult + (1 - p->mult) * sqr(d);

    if (p->num_samples >= p->tc_samples) {
      if (p->avg_sigma_x2 > p->max_sigma_x2)
        p->max_sigma_x2 = p->avg_sigma_x2;
      if (p->avg_sigma_x2 < p->min_sigma_x2)
        p->min_sigma_x2 = p->avg_sigma_x2;
    }
    p->last = d;
    p->mask |= *ibuf;
  }
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * olen)
{
  priv_t * p = (priv_t *)effp->priv;

  if (p->last == p->min)
    p->min_runs += sqr(p->min_run);
  if (p->last == p->max)
    p->max_runs += sqr(p->max_run);

  (void)obuf, *olen = 0;
  return SOX_SUCCESS;
}

static unsigned bit_depth(uint32_t mask, double min, double max, unsigned * x)
{
  SOX_SAMPLE_LOCALS;
  unsigned result = 32, dummy = 0;

  for (; result && !(mask & 1); --result, mask >>= 1);
  if (x)
    *x = result;
  min = -fmax(fabs(min), fabs(max));
  mask = SOX_FLOAT_64BIT_TO_SAMPLE(min, dummy) << 1;
  for (; result && (mask & SOX_SAMPLE_MIN); --result, mask <<= 1);
  return result;
}

static void output(priv_t const * p, double x)
{
  if (p->scale_bits) {
    unsigned mult = 1 << (p->scale_bits - 1);
    int i;
    x = floor(x * mult + .5);
    i = min(x, mult - 1.);
    if (p->hex_bits)
      if (x < 0) {
        char buf[30];
        sprintf(buf, "%x", -i);
        fprintf(stderr, " %*c%s", 9 - (int)strlen(buf), '-', buf);
      }
      else fprintf(stderr, " %9x", i);
    else fprintf(stderr, " %9i", i);
  }
  else fprintf(stderr, " %9.*f", fabs(p->scale) < 10 ? 6 : 5, p->scale * x);
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;

  if (!effp->flow) {
    double min_runs = 0, max_count = 0, min = 2, max = -2, max_sigma_x = 0, sigma_x = 0, sigma_x2 = 0, min_sigma_x2 = 2, max_sigma_x2 = 0, avg_peak = 0;
    off_t num_samples = 0, min_count = 0, max_runs = 0;
    uint32_t mask = 0;
    unsigned b1, b2, i, n = effp->flows > 1 ? effp->flows : 0;

    for (i = 0; i < effp->flows; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      min = min(min, q->min);
      max = max(max, q->max);
      if (q->num_samples < q->tc_samples)
        q->min_sigma_x2 = q->max_sigma_x2 = q->sigma_x2 / q->num_samples;
      min_sigma_x2 = min(min_sigma_x2, q->min_sigma_x2);
      max_sigma_x2 = max(max_sigma_x2, q->max_sigma_x2);
      sigma_x += q->sigma_x;
      sigma_x2 += q->sigma_x2;
      num_samples += q->num_samples;
      mask |= q->mask;
      if (fabs(q->sigma_x) > fabs(max_sigma_x))
        max_sigma_x = q->sigma_x;
      min_count += q->min_count;
      min_runs += q->min_runs;
      max_count += q->max_count;
      max_runs += q->max_runs;
      avg_peak += max(-q->min, q->max);
    }
    avg_peak /= effp->flows;

    if (!num_samples) {
      lsx_warn("no audio");
      return SOX_SUCCESS;
    }

    if (n == 2)
      fprintf(stderr, "             Overall     Left      Right\n");
    else if (n) {
      fprintf(stderr, "             Overall");
      for (i = 0; i < n; ++i)
        fprintf(stderr, "     Ch%-3i", i + 1);
      fprintf(stderr, "\n");
    }

    fprintf(stderr, "DC offset ");
    output(p, max_sigma_x / p->num_samples);
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      output(p, q->sigma_x / q->num_samples);
    }

    fprintf(stderr, "\nMin level ");
    output(p, min);
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      output(p, q->min);
    }

    fprintf(stderr, "\nMax level ");
    output(p, max);
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      output(p, q->max);
    }

    fprintf(stderr, "\nPk lev dB %10.2f", linear_to_dB(max(-min, max)));
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, "%10.2f", linear_to_dB(max(-q->min, q->max)));
    }

    fprintf(stderr, "\nRMS lev dB%10.2f", linear_to_dB(sqrt(sigma_x2 / num_samples)));
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, "%10.2f", linear_to_dB(sqrt(q->sigma_x2 / q->num_samples)));
    }

    fprintf(stderr, "\nRMS Pk dB %10.2f", linear_to_dB(sqrt(max_sigma_x2)));
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, "%10.2f", linear_to_dB(sqrt(q->max_sigma_x2)));
    }

    fprintf(stderr, "\nRMS Tr dB ");
    if (min_sigma_x2 != 1)
      fprintf(stderr, "%10.2f", linear_to_dB(sqrt(min_sigma_x2)));
    else fprintf(stderr, "         -");
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      if (q->min_sigma_x2 != 1)
        fprintf(stderr, "%10.2f", linear_to_dB(sqrt(q->min_sigma_x2)));
      else fprintf(stderr, "         -");
    }

    if (effp->flows > 1)
      fprintf(stderr, "\nCrest factor       -");
    else fprintf(stderr, "\nCrest factor %7.2f", sigma_x2 ? avg_peak / sqrt(sigma_x2 / num_samples) : 1);
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, "%10.2f", q->sigma_x2? max(-q->min, q->max) / sqrt(q->sigma_x2 / q->num_samples) : 1);
    }

    fprintf(stderr, "\nFlat factor%9.2f", linear_to_dB((min_runs + max_runs) / (min_count + max_count)));
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, " %9.2f", linear_to_dB((q->min_runs + q->max_runs) / (q->min_count + q->max_count)));
    }

    fprintf(stderr, "\nPk count   %9s", lsx_sigfigs3((min_count + max_count) / effp->flows));
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      fprintf(stderr, " %9s", lsx_sigfigs3((double)(q->min_count + q->max_count)));
    }

    b1 = bit_depth(mask, min, max, &b2);
    fprintf(stderr, "\nBit-depth      %2u/%-2u", b1, b2);
    for (i = 0; i < n; ++i) {
      priv_t * q = (priv_t *)(effp - effp->flow + i)->priv;
      b1 = bit_depth(q->mask, q->min, q->max, &b2);
      fprintf(stderr, "     %2u/%-2u", b1, b2);
    }

    fprintf(stderr, "\nNum samples%9s", lsx_sigfigs3((double)p->num_samples));
    fprintf(stderr, "\nLength s   %9.3f", p->num_samples / effp->in_signal.rate);
    fprintf(stderr, "\nScale max ");
    output(p, 1.);
    fprintf(stderr, "\nWindow s   %9.3f", p->time_constant);
    fprintf(stderr, "\n");
  }
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_stats_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "stats", "[-b bits|-x bits|-s scale] [-w window-time]", SOX_EFF_MODIFY,
    getopts, start, flow, drain, stop, NULL, sizeof(priv_t)};
  return &handler;
}
