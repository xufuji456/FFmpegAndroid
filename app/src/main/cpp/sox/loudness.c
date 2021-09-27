/* Effect: loudness filter     Copyright (c) 2008 robs@users.sourceforge.net
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
  dft_filter_priv_t base;
  double     delta, start;
  int        n;
} priv_t;

static int create(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *)effp->priv;
  dft_filter_priv_t * b = &p->base;
  b->filter_ptr = &b->filter;
  p->delta = -10;
  p->start = 65;
  p->n = 1023;
  --argc, ++argv;
  do {                    /* break-able block */
    NUMERIC_PARAMETER(delta,-50 , 15) /* FIXME expand range */
    NUMERIC_PARAMETER(start, 50 , 75) /* FIXME expand range */
    NUMERIC_PARAMETER(n    ,127 ,2047)
  } while (0);
  p->n = 2 * p->n + 1;
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static double * make_filter(int n, double start, double delta, double rate)
{
  static const struct {double f, af, lu, tf;} iso226_table[] = {
    {   20,0.532,-31.6,78.5},{   25,0.506,-27.2,68.7},{ 31.5,0.480,-23.0,59.5},
    {   40,0.455,-19.1,51.1},{   50,0.432,-15.9,44.0},{   63,0.409,-13.0,37.5},
    {   80,0.387,-10.3,31.5},{  100,0.367, -8.1,26.5},{  125,0.349, -6.2,22.1},
    {  160,0.330, -4.5,17.9},{  200,0.315, -3.1,14.4},{  250,0.301, -2.0,11.4},
    {  315,0.288, -1.1, 8.6},{  400,0.276, -0.4, 6.2},{  500,0.267,  0.0, 4.4},
    {  630,0.259,  0.3, 3.0},{  800,0.253,  0.5, 2.2},{ 1000,0.250,  0.0, 2.4},
    { 1250,0.246, -2.7, 3.5},{ 1600,0.244, -4.1, 1.7},{ 2000,0.243, -1.0,-1.3},
    { 2500,0.243,  1.7,-4.2},{ 3150,0.243,  2.5,-6.0},{ 4000,0.242,  1.2,-5.4},
    { 5000,0.242, -2.1,-1.5},{ 6300,0.245, -7.1, 6.0},{ 8000,0.254,-11.2,12.6},
    {10000,0.271,-10.7,13.9},{12500,0.301, -3.1,12.3},
  };
  #define LEN (array_length(iso226_table) + 2)
  #define SPL(phon, t) (10 / t.af * log10(4.47e-3 * (pow(10., .025 * (phon)) - \
          1.15) + pow(.4 * pow(10., (t.tf + t.lu) / 10 - 9), t.af)) - t.lu + 94)
  double fs[LEN], spl[LEN], d[LEN], * work, * h;
  int i, work_len;
  
  fs[0] = log(1.);
  spl[0] = delta * .2;
  for (i = 0; i < (int)LEN - 2; ++i) {
    spl[i + 1] = SPL(start + delta, iso226_table[i]) -
                 SPL(start        , iso226_table[i]);
    fs[i + 1] = log(iso226_table[i].f);
  }
  fs[i + 1] = log(100000.);
  spl[i + 1] = spl[0];
  lsx_prepare_spline3(fs, spl, (int)LEN, HUGE_VAL, HUGE_VAL, d);

  for (work_len = 8192; work_len < rate / 2; work_len <<= 1);
  work = lsx_calloc(work_len, sizeof(*work));
  h = lsx_calloc(n, sizeof(*h));

  for (i = 0; i <= work_len / 2; ++i) {
    double f = rate * i / work_len;
    double spl1 = f < 1? spl[0] : lsx_spline3(fs, spl, d, (int)LEN, log(f));
    work[i < work_len / 2 ? 2 * i : 1] = dB_to_linear(spl1);
  }
  lsx_safe_rdft(work_len, -1, work);
  for (i = 0; i < n; ++i)
    h[i] = work[(work_len - n / 2 + i) % work_len] * 2. / work_len;
  lsx_apply_kaiser(h, n, lsx_kaiser_beta(40 + 2./3 * fabs(delta), .1));

  free(work);
  return h;
  #undef SPL
  #undef LEN
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  dft_filter_t * f = p->base.filter_ptr;

  if (p->delta == 0)
    return SOX_EFF_NULL;

  if (!f->num_taps) {
    double * h = make_filter(p->n, p->start, p->delta, effp->in_signal.rate);
    if (effp->global_info->plot != sox_plot_off) {
      char title[100];
      sprintf(title, "SoX effect: loudness %g (%g)", p->delta, p->start);
      lsx_plot_fir(h, p->n, effp->in_signal.rate,
          effp->global_info->plot, title, p->delta - 5, 0.);
      return SOX_EOF;
    }
    lsx_set_dft_filter(f, h, p->n, p->n >> 1);
  }
  return lsx_dft_filter_effect_fn()->start(effp);
}

sox_effect_handler_t const * lsx_loudness_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_dft_filter_effect_fn();
  handler.name = "loudness";
  handler.usage = "[gain [ref]]";
  handler.getopts = create;
  handler.start = start;
  handler.priv_size = sizeof(priv_t);
  return &handler;
}
