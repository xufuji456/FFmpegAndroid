/* libSoX effect: Downsample
 *
 * First version of this effect written 11/2011 by Ulrich Klauer.
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

typedef struct {
  unsigned int factor;
  unsigned int carry; /* number of samples still to be discarded,
                         carried over from last block */
} priv_t;

static int create(sox_effect_t *effp, int argc, char **argv)
{
  priv_t *p = (priv_t*)effp->priv;
  p->factor = 2;
  --argc, ++argv;
  do { /* break-able block */
    NUMERIC_PARAMETER(factor, 1, 16384)
  } while (0);
  return argc ? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t *effp)
{
  priv_t *p = (priv_t*) effp->priv;
  effp->out_signal.rate = effp->in_signal.rate / p->factor;
  return p->factor == 1 ? SOX_EFF_NULL : SOX_SUCCESS;
}

static int flow(sox_effect_t *effp, const sox_sample_t *ibuf,
    sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
  priv_t *p = (priv_t*)effp->priv;
  size_t ilen = *isamp, olen = *osamp;
  size_t t;

  t = min(p->carry, ilen);
  p->carry -= t;
  ibuf += t; ilen -= t;

  /* NB: either p->carry (usually) or ilen is now zero; hence, a
     non-zero value of ilen implies p->carry == 0, and there is no
     need to test for this in the following while and if. */

  while (ilen >= p->factor && olen) {
    *obuf++ = *ibuf;
    ibuf += p->factor;
    olen--; ilen -= p->factor;
  }

  if (ilen && olen) {
    *obuf++ = *ibuf;
    p->carry = p->factor - ilen;
    olen--; ilen = 0;
  }

  *isamp -= ilen, *osamp -= olen;
  return SOX_SUCCESS;
}

sox_effect_handler_t const *lsx_downsample_effect_fn(void)
{
  static sox_effect_handler_t handler = {"downsample", "[factor (2)]",
    SOX_EFF_RATE | SOX_EFF_MODIFY,
    create, start, flow, NULL, NULL, NULL, sizeof(priv_t)};
  return &handler;
}
