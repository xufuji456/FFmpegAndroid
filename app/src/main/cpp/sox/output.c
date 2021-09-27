/* libSoX effect: Output audio to a file   (c) 2008 robs@users.sourceforge.net
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

typedef struct {sox_format_t * file;} priv_t;

static int getopts(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  if (argc != 2 || !(p->file = (sox_format_t *)argv[1]) || p->file->mode != 'w')
    return SOX_EOF;
  return SOX_SUCCESS;
}

static int flow(sox_effect_t *effp, sox_sample_t const * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  /* Write out *isamp samples */
  size_t len = sox_write(p->file, ibuf, *isamp);

  /* len is the number of samples that were actually written out; if this is
   * different to *isamp, then something has gone wrong--most often, it's
   * out of disc space */
  if (len != *isamp) {
    lsx_fail("%s: %s", p->file->filename, p->file->sox_errstr);
    return SOX_EOF;
  }

  /* Outputting is the last `effect' in the effect chain so always passes
   * 0 samples on to the next effect (as there isn't one!) */
  (void)obuf, *osamp = 0;
  return SOX_SUCCESS; /* All samples output successfully */
}

sox_effect_handler_t const * lsx_output_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "output", NULL, SOX_EFF_MCHAN | SOX_EFF_INTERNAL,
    getopts, NULL, flow, NULL, NULL, NULL, sizeof(priv_t)
  };
  return &handler;
}

