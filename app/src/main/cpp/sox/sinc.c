/* Effect: sinc filters     Copyright (c) 2008-9 robs@users.sourceforge.net
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
#include "dft_filter.h"
#include <string.h>

typedef struct {
  dft_filter_priv_t  base;
  double             att, beta, phase, Fc0, Fc1, tbw0, tbw1;
  int                num_taps[2];
  sox_bool           round;
} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  dft_filter_priv_t * b = &p->base;
  char * parse_ptr = argv[0];
  int i = 0;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, "+ra:b:p:MILt:n:", NULL, lsx_getopt_flag_none, 1, &optstate);

  b->filter_ptr = &b->filter;
  p->phase = 50;
  p->beta = -1;
  while (i < 2) {
    int c = 1;
    while (c && (c = lsx_getopt(&optstate)) != -1) switch (c) {
      char * parse_ptr2;
      case 'r': p->round = sox_true; break;
      GETOPT_NUMERIC(optstate, 'a', att,  40 , 180)
      GETOPT_NUMERIC(optstate, 'b', beta,  0 , 256)
      GETOPT_NUMERIC(optstate, 'p', phase, 0, 100)
      case 'M': p->phase =  0; break;
      case 'I': p->phase = 25; break;
      case 'L': p->phase = 50; break;
      GETOPT_NUMERIC(optstate, 'n', num_taps[1], 11, 32767)
      case 't': p->tbw1 = lsx_parse_frequency(optstate.arg, &parse_ptr2);
        if (p->tbw1 < 1 || *parse_ptr2) return lsx_usage(effp);
        break;
      default: c = 0;
    }
    if ((p->att && p->beta >= 0) || (p->tbw1 && p->num_taps[1]))
      return lsx_usage(effp);
    if (!i || !p->Fc1)
      p->tbw0 = p->tbw1, p->num_taps[0] = p->num_taps[1];
    if (!i++ && optstate.ind < argc) {
      if (*(parse_ptr = argv[optstate.ind++]) != '-')
        p->Fc0 = lsx_parse_frequency(parse_ptr, &parse_ptr);
      if (*parse_ptr == '-')
        p->Fc1 = lsx_parse_frequency(parse_ptr + 1, &parse_ptr);
    }
  }
  return optstate.ind != argc || p->Fc0 < 0 || p->Fc1 < 0 || *parse_ptr ?
      lsx_usage(effp) : SOX_SUCCESS;
}

static void invert(double * h, int n)
{
  int i;
  for (i = 0; i < n; ++i)
    h[i] = -h[i];
  h[(n - 1) / 2] += 1;
}

static double * lpf(double Fn, double Fc, double tbw, int * num_taps, double att, double * beta, sox_bool round)
{
  int n = *num_taps;
  if ((Fc /= Fn) <= 0 || Fc >= 1) {
    *num_taps = 0;
    return NULL;
  }
  att = att? att : 120;
  lsx_kaiser_params(att, Fc, (tbw? tbw / Fn : .05) * .5, beta, num_taps);
  if (!n) {
    n = *num_taps;
    *num_taps = range_limit(n, 11, 32767);
    if (round)
      *num_taps = 1 + 2 * (int)((int)((*num_taps / 2) * Fc + .5) / Fc + .5);
    lsx_report("num taps = %i (from %i)", *num_taps, n);
  }
  return lsx_make_lpf(*num_taps |= 1, Fc, *beta, 0., 1., sox_false);
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  dft_filter_t * f = p->base.filter_ptr;

  if (!f->num_taps) {
    double Fn = effp->in_signal.rate * .5;
    double * h[2];
    int i, n, post_peak, longer;

    if (p->Fc0 >= Fn || p->Fc1 >= Fn) {
      lsx_fail("filter frequency must be less than sample-rate / 2");
      return SOX_EOF;
    }
    h[0] = lpf(Fn, p->Fc0, p->tbw0, &p->num_taps[0], p->att, &p->beta,p->round);
    h[1] = lpf(Fn, p->Fc1, p->tbw1, &p->num_taps[1], p->att, &p->beta,p->round);
    if (h[0])
      invert(h[0], p->num_taps[0]);

    longer = p->num_taps[1] > p->num_taps[0];
    n = p->num_taps[longer];
    if (h[0] && h[1]) {
      for (i = 0; i < p->num_taps[!longer]; ++i)
        h[longer][i + (n - p->num_taps[!longer])/2] += h[!longer][i];

      if (p->Fc0 < p->Fc1)
        invert(h[longer], n);

      free(h[!longer]);
    }
    if (p->phase != 50)
      lsx_fir_to_phase(&h[longer], &n, &post_peak, p->phase);
    else post_peak = n >> 1;

    if (effp->global_info->plot != sox_plot_off) {
      char title[100];
      sprintf(title, "SoX effect: sinc filter freq=%g-%g",
          p->Fc0, p->Fc1? p->Fc1 : Fn);
      lsx_plot_fir(h[longer], n, effp->in_signal.rate,
          effp->global_info->plot, title, -p->beta * 10 - 25, 5.);
      return SOX_EOF;
    }
    lsx_set_dft_filter(f, h[longer], n, post_peak);
  }
  return lsx_dft_filter_effect_fn()->start(effp);
}

sox_effect_handler_t const * lsx_sinc_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_dft_filter_effect_fn();
  handler.name = "sinc";
  handler.usage = "[-a att|-b beta] [-p phase|-M|-I|-L] [-t tbw|-n taps] [freqHP][-freqLP [-t tbw|-n taps]]";
  handler.getopts = create;
  handler.start = start;
  handler.priv_size = sizeof(priv_t);
  return &handler;
}
