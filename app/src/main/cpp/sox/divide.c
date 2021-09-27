/* libSoX effect: divide   Copyright (c) 2009 robs@users.sourceforge.net
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
 * Needs better handling of when the divisor approaches or is zero; some
 * sort of interpolation of the output values perhaps.
 */

#include "sox_i.h"
#include <string.h>

typedef struct {
  sox_sample_t * last;
} priv_t;

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  p->last = lsx_calloc(effp->in_signal.channels, sizeof(*p->last));
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i, len =  min(*isamp, *osamp) / effp->in_signal.channels;
  *osamp = *isamp = len * effp->in_signal.channels;

  while (len--) {
    double divisor = *obuf++ = *ibuf++;
    if (divisor) {
      double out, mult = 1. / SOX_SAMPLE_TO_FLOAT_64BIT(divisor,);
      for (i = 1; i < effp->in_signal.channels; ++i) {
        out = *ibuf++ * mult;
        p->last[i] = *obuf++ = SOX_ROUND_CLIP_COUNT(out, effp->clips);
      }
    }
    else for (i = 1; i < effp->in_signal.channels; ++i, ++ibuf)
      *obuf++ = p->last[i];
  }
  return SOX_SUCCESS;
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  free(p->last);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_divide_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "divide", NULL, SOX_EFF_MCHAN | SOX_EFF_GAIN | SOX_EFF_ALPHA,
    NULL, start, flow, NULL, stop, NULL, sizeof(priv_t)
  };
  return &handler;
}
