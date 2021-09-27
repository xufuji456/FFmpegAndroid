/* libSoX Effect: Adjust the audio speed (pitch and tempo together)
 * (c) 2006,8 robs@users.sourceforge.net
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
 *
 *
 * Adjustment is given as the ratio of the new speed to the old speed, or as
 * a number of cents (100ths of a semitone) to change.  Speed change is
 * actually performed by whichever resampling effect is in effect.
 */

#include "sox_i.h"
#include <string.h>

typedef struct {
  double factor;
} priv_t;

static int getopts(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *) effp->priv;
  sox_bool is_cents = sox_false;

  --argc, ++argv;
  if (argc == 1) {
    char c, dummy;
    int scanned = sscanf(*argv, "%lf%c %c", &p->factor, &c, &dummy);
    if (scanned == 1 || (scanned == 2 && c == 'c')) {
      is_cents |= scanned == 2;
      if (is_cents || p->factor > 0) {
        p->factor = is_cents? pow(2., p->factor / 1200) : p->factor;
        return SOX_SUCCESS;
      }
    }
  }
  return lsx_usage(effp);
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;

  if (p->factor == 1)
    return SOX_EFF_NULL;

  effp->out_signal.rate = effp->in_signal.rate * p->factor;

  effp->out_signal.length = effp->in_signal.length;
    /* audio length if measured in samples doesn't change */

  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_speed_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "speed", "factor[c]",
    SOX_EFF_MCHAN | SOX_EFF_RATE | SOX_EFF_LENGTH | SOX_EFF_MODIFY,
    getopts, start, lsx_flow_copy, 0, 0, 0, sizeof(priv_t)};
  return &handler;
}
