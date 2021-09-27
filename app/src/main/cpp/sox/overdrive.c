/* libSoX effect: Overdrive            (c) 2008 robs@users.sourceforge.net
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

typedef struct {
  double gain, colour, last_in, last_out, b0, b1, a1;
} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  --argc, ++argv;
  p->gain = p->colour = 20;
  do {
    NUMERIC_PARAMETER(gain, 0, 100)
    NUMERIC_PARAMETER(colour, 0, 100)
  } while (0);
  p->gain = dB_to_linear(p->gain);
  p->colour /= 200;
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;

  if (p->gain == 1)
    return SOX_EFF_NULL;

  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t dummy = 0, len = *isamp = *osamp = min(*isamp, *osamp);
  while (len--) {
    SOX_SAMPLE_LOCALS;
    double d = SOX_SAMPLE_TO_FLOAT_64BIT(*ibuf++, dummy), d0 = d;
    d *= p->gain;
    d += p->colour;
    d = d < -1? -2./3 : d > 1? 2./3 : d - d * d * d * (1./3);
    p->last_out = d - p->last_in + .995 * p->last_out;
    p->last_in = d;
    *obuf++ = SOX_FLOAT_64BIT_TO_SAMPLE(d0 * .5 + p->last_out * .75, dummy);
  }
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_overdrive_effect_fn(void)
{
  static sox_effect_handler_t handler = {"overdrive", "[gain [colour]]",
    SOX_EFF_GAIN, create, start, flow, NULL, NULL, NULL, sizeof(priv_t)};
  return &handler;
}
