/* libSoX effect: Upsample (zero stuff)    (c) 2011 robs@users.sourceforge.net
 *
 * Sometimes filters perform better at higher sampling rates, so e.g.
 *   sox -r 48k input output upsample 4 filter rate 48k vol 4
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

typedef struct {unsigned factor, pos;} priv_t;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  p->factor = 2;
  --argc, ++argv;
  do {NUMERIC_PARAMETER(factor, 1, 256)} while (0);
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  effp->out_signal.rate = effp->in_signal.rate * p->factor;
  return p->factor == 1? SOX_EFF_NULL : SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t ilen = *isamp, olen = *osamp;
  while (sox_true) {
    for (; p->pos && olen; p->pos = (p->pos + 1) % p->factor, --olen)
      *obuf++ = 0;
    if (!ilen || !olen)
      break;
    *obuf++ = *ibuf++;
    --olen, --ilen;
    ++p->pos;
  }
  *isamp -= ilen, *osamp -= olen;
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_upsample_effect_fn(void)
{
  static sox_effect_handler_t handler = {"upsample", "[factor (2)]",
    SOX_EFF_RATE | SOX_EFF_MODIFY, create, start, flow, NULL, NULL, NULL, sizeof(priv_t)};
  return &handler;
}
