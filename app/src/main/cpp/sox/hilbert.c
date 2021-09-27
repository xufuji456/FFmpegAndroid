/* libSoX effect: Hilbert transform filter
 *
 * First version of this effect written 11/2011 by Ulrich Klauer, using maths
 * from "Understanding digital signal processing" by Richard G. Lyons.
 *
 * Copyright 2011 Chris Bagwell and SoX Contributors
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

typedef struct {
  dft_filter_priv_t base;
  double *h;
  int taps;
} priv_t;

static int getopts(sox_effect_t *effp, int argc, char **argv)
{
  lsx_getopt_t optstate;
  int c;
  priv_t *p = (priv_t*)effp->priv;
  dft_filter_priv_t *b = &p->base;

  b->filter_ptr = &b->filter;

  lsx_getopt_init(argc, argv, "+n:", NULL, lsx_getopt_flag_none, 1, &optstate);

  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    GETOPT_NUMERIC(optstate, 'n', taps, 3, 32767)
    default: lsx_fail("invalid option `-%c'", optstate.opt); return lsx_usage(effp);
  }
  if (p->taps && p->taps%2 == 0) {
    lsx_fail("only filters with an odd number of taps are supported");
    return SOX_EOF;
  }
  return optstate.ind != argc ? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t *effp)
{
  priv_t *p = (priv_t*)effp->priv;
  dft_filter_t *f = p->base.filter_ptr;

  if (!f->num_taps) {
    int i;
    if (!p->taps) {
      p->taps = effp->in_signal.rate/76.5 + 2;
      p->taps += 1 - (p->taps%2);
      /* results in a cutoff frequency of about 75 Hz with a Blackman window */
      lsx_debug("choosing number of taps = %d (override with -n)", p->taps);
    }
    lsx_valloc(p->h, p->taps);
    for (i = 0; i < p->taps; i++) {
      int k = -(p->taps/2) + i;
      if (k%2 == 0) {
        p->h[i] = 0.0;
      } else {
        double pk = M_PI * k;
        p->h[i] = (1 - cos(pk))/pk;
      }
    }
    lsx_apply_blackman(p->h, p->taps, .16);

    if (effp->global_info->plot != sox_plot_off) {
      char title[100];
      sprintf(title, "SoX effect: hilbert (%d taps)", p->taps);
      lsx_plot_fir(p->h, p->taps, effp->in_signal.rate,
          effp->global_info->plot, title, -20., 5.);
      free(p->h);
      return SOX_EOF;
    }
    lsx_set_dft_filter(f, p->h, p->taps, p->taps/2);
  }
  return lsx_dft_filter_effect_fn()->start(effp);
}

sox_effect_handler_t const *lsx_hilbert_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_dft_filter_effect_fn();
  handler.name = "hilbert";
  handler.usage = "[-n taps]";
  handler.getopts = getopts;
  handler.start = start;
  handler.priv_size = sizeof(priv_t);
  return &handler;
}
