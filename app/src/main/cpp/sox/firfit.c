/* Effect: firfit filter     Copyright (c) 2009 robs@users.sourceforge.net
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

/* This is W.I.P. hence marked SOX_EFF_ALPHA for now.
 * Need to add other interpolation types e.g. linear, bspline, window types,
 * and filter length, maybe phase response type too.
 */

#include "sox_i.h"
#include "dft_filter.h"

typedef struct {
  dft_filter_priv_t base;
  char const         * filename;
  struct {double f, gain;} * knots;
  int        num_knots, n;
} priv_t;

static int create(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *)effp->priv;
  dft_filter_priv_t * b = &p->base;
  b->filter_ptr = &b->filter;
  --argc, ++argv;
  if (argc == 1)
    p->filename = argv[0], --argc;
  p->n = 2047;
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static double * make_filter(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  double * log_freqs, * gains, * d, * work, * h;
  sox_rate_t rate = effp->in_signal.rate;
  int i, work_len;
  
  lsx_valloc(log_freqs , p->num_knots);
  lsx_valloc(gains, p->num_knots);
  lsx_valloc(d  , p->num_knots);
  for (i = 0; i < p->num_knots; ++i) {
    log_freqs[i] = log(max(p->knots[i].f, 1));
    gains[i] = p->knots[i].gain;
  }
  lsx_prepare_spline3(log_freqs, gains, p->num_knots, HUGE_VAL, HUGE_VAL, d);

  for (work_len = 8192; work_len < rate / 2; work_len <<= 1);
  work = lsx_calloc(work_len + 2, sizeof(*work));
  lsx_valloc(h, p->n);

  for (i = 0; i <= work_len; i += 2) {
    double f = rate * 0.5 * i / work_len;
    double spl1 = f < max(p->knots[0].f, 1)? gains[0] : 
                  f > p->knots[p->num_knots - 1].f? gains[p->num_knots - 1] :
                  lsx_spline3(log_freqs, gains, d, p->num_knots, log(f));
    work[i] = dB_to_linear(spl1);
  }
  LSX_PACK(work, work_len);
  lsx_safe_rdft(work_len, -1, work);
  for (i = 0; i < p->n; ++i)
    h[i] = work[(work_len - p->n / 2 + i) % work_len] * 2. / work_len;
  lsx_apply_blackman_nutall(h, p->n);

  free(work);
  return h;
}

static sox_bool read_knots(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  FILE * file = lsx_open_input_file(effp, p->filename, sox_true);
  sox_bool result = sox_false;
  int num_converted = 1;
  char c;

  if (file) {
    lsx_valloc(p->knots, 1);
    while (fscanf(file, " #%*[^\n]%c", &c) >= 0) {
      num_converted = fscanf(file, "%lf %lf",
          &p->knots[p->num_knots].f, &p->knots[p->num_knots].gain);
      if (num_converted == 2) {
        if (p->num_knots && p->knots[p->num_knots].f <= p->knots[p->num_knots - 1].f) {
          lsx_fail("knot frequencies must be strictly increasing");
          break;
        }
        lsx_revalloc(p->knots, ++p->num_knots + 1);
      } else if (num_converted != 0)
        break;
    }
    lsx_report("%i knots", p->num_knots);
    if (feof(file) && num_converted != 1)
      result = sox_true;
    else lsx_fail("error reading knot file `%s', line number %u", p->filename, 1 + p->num_knots);
    if (file != stdin)
      fclose(file);
  }
  return result;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  dft_filter_t * f = p->base.filter_ptr;

  if (!f->num_taps) {
    double * h;
    if (!p->num_knots && !read_knots(effp))
      return SOX_EOF;
    h = make_filter(effp);
    if (effp->global_info->plot != sox_plot_off) {
      lsx_plot_fir(h, p->n, effp->in_signal.rate,
          effp->global_info->plot, "SoX effect: firfit", -30., +30.);
      return SOX_EOF;
    }
    lsx_set_dft_filter(f, h, p->n, p->n >> 1);
  }
  return lsx_dft_filter_effect_fn()->start(effp);
}

sox_effect_handler_t const * lsx_firfit_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_dft_filter_effect_fn();
  handler.name = "firfit";
  handler.usage = "[knots-file]";
  handler.flags |= SOX_EFF_ALPHA;
  handler.getopts = create;
  handler.start = start;
  handler.priv_size = sizeof(priv_t);
  return &handler;
}
