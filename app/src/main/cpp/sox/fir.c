/* Effect: fir filter from coefs   Copyright (c) 2009 robs@users.sourceforge.net
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
  dft_filter_priv_t  base;
  char const         * filename;
  double             * h;
  int                n;
} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t             * p = (priv_t *)effp->priv;
  dft_filter_priv_t  * b = &p->base;
  double             d;
  char               c;

  b->filter_ptr = &b->filter;
  --argc, ++argv;
  if (!argc)
    p->filename = "-"; /* default to stdin */
  else if (argc == 1)
    p->filename = argv[0], --argc;
  else for (; argc && sscanf(*argv, "%lf%c", &d, &c) == 1; --argc, ++argv) {
    p->n++;
    p->h = lsx_realloc(p->h, p->n * sizeof(*p->h));
    p->h[p->n - 1] = d;
  }
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t        * p = (priv_t *)effp->priv;
  dft_filter_t  * f = p->base.filter_ptr;
  double        d;
  char          c;
  int           i;

  if (!f->num_taps) {
    if (!p->n && p->filename) {
      FILE * file = lsx_open_input_file(effp, p->filename, sox_true);
      if (!file)
        return SOX_EOF;
      while ((i = fscanf(file, " #%*[^\n]%c", &c)) >= 0) {
        if (i >= 1) continue; /* found and skipped a comment */
        if ((i = fscanf(file, "%lf", &d)) > 0) {
          /* found a coefficient value */
          p->n++;
          p->h = lsx_realloc(p->h, p->n * sizeof(*p->h));
          p->h[p->n - 1] = d;
        } else break; /* either EOF, or something went wrong
                         (read or syntax error) */
      }
      if (!feof(file)) {
        lsx_fail("error reading coefficient file");
        if (file != stdin) fclose(file);
        return SOX_EOF;
      }
      if (file != stdin) fclose(file);
    }
    lsx_report("%i coefficients", p->n);
    if (!p->n)
      return SOX_EFF_NULL;
    if (effp->global_info->plot != sox_plot_off) {
      char title[100];
      sprintf(title, "SoX effect: fir (%d coefficients)", p->n);
      lsx_plot_fir(p->h, p->n, effp->in_signal.rate,
          effp->global_info->plot, title, -30., 30.);
      free(p->h);
      return SOX_EOF;
    }
    lsx_set_dft_filter(f, p->h, p->n, p->n >> 1);
  }
  return lsx_dft_filter_effect_fn()->start(effp);
}

sox_effect_handler_t const * lsx_fir_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_dft_filter_effect_fn();
  handler.name = "fir";
  handler.usage = "[coef-file|coefs]";
  handler.getopts = create;
  handler.start = start;
  handler.priv_size = sizeof(priv_t);
  return &handler;
}
