/* libSoX Sounder format handler          (c) 2008 robs@users.sourceforge.net
 * See description in soundr3b.zip on the net.
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

static int start_read(sox_format_t * ft)
{
  uint16_t type, rate;

  if (lsx_readw(ft, &type) || lsx_readw(ft, &rate) || lsx_skipbytes(ft, (size_t) 4))
    return SOX_EOF;
  if (type) {
    lsx_fail_errno(ft, SOX_EHDR, "invalid Sounder header");
    return SOX_EOF;
  }
  return lsx_check_read_params(ft, 1, (sox_rate_t)rate, SOX_ENCODING_UNSIGNED, 8, (uint64_t)0, sox_true);
}

static int write_header(sox_format_t * ft)
{
  return lsx_writew(ft, 0)   /* sample type */
      || lsx_writew(ft, min(65535, (unsigned)(ft->signal.rate + .5)))
      || lsx_writew(ft, 10)  /* speaker driver volume */
      || lsx_writew(ft, 4)?  /* speaker driver DC shift */
      SOX_EOF : SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(sounder)
{
  static char const * const names[] = {"sndr", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_UNSIGNED, 8, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "8-bit linear audio as used by Aaron Wallace's `Sounder' of 1991",
    names, SOX_FILE_LIT_END | SOX_FILE_MONO,
    start_read, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &handler;
}
