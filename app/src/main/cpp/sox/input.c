/* libSoX effect: Input audio from a file   (c) 2008 robs@users.sourceforge.net
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
  if (argc != 2 || !(p->file = (sox_format_t *)argv[1]) || p->file->mode != 'r')
    return SOX_EOF;
  return SOX_SUCCESS;
}

static int drain(
    sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;

  /* ensure that *osamp is a multiple of the number of channels. */
  *osamp -= *osamp % effp->out_signal.channels;

  /* Read up to *osamp samples into obuf; store the actual number read
   * back to *osamp */
  *osamp = sox_read(p->file, obuf, *osamp);

  /* sox_read may return a number that is less than was requested; only if
   * 0 samples is returned does it indicate that end-of-file has been reached
   * or an error has occurred */
  if (!*osamp && p->file->sox_errno)
    lsx_fail("%s: %s", p->file->filename, p->file->sox_errstr);
  return *osamp? SOX_SUCCESS : SOX_EOF;
}

sox_effect_handler_t const * lsx_input_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "input", NULL, SOX_EFF_MCHAN | SOX_EFF_LENGTH | SOX_EFF_INTERNAL,
    getopts, NULL, NULL, drain, NULL, NULL, sizeof(priv_t)
  };
  return &handler;
}

