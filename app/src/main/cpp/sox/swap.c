/* libSoX effect: swap pairs of audio channels
 *
 * First version written 01/2012 by Ulrich Klauer.
 * Replaces an older swap effect originally written by Chris Bagwell
 * on March 16, 1999.
 *
 * Copyright 2012 Chris Bagwell and SoX Contributors
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

static int start(sox_effect_t *effp)
{
  return effp->in_signal.channels >= 2 ? SOX_SUCCESS : SOX_EFF_NULL;
}

static int flow(sox_effect_t *effp, const sox_sample_t *ibuf,
    sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
  size_t len = min(*isamp, *osamp);
  size_t channels = effp->in_signal.channels;
  len /= channels;
  *isamp = *osamp = len * channels;

  while (len--) {
    size_t i;
    for (i = 0; i + 1 < channels; i += 2) {
      *obuf++ = ibuf[1];
      *obuf++ = ibuf[0];
      ibuf += 2;
    }
    if (channels % 2)
      *obuf++ = *ibuf++;
  }

  return SOX_SUCCESS;
}

sox_effect_handler_t const *lsx_swap_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "swap", NULL,
    SOX_EFF_MCHAN | SOX_EFF_MODIFY,
    NULL, start, flow, NULL, NULL, NULL,
    0
  };
  return &handler;
}
