/* libSoX file format: Psion wve   (c) 2008 robs@users.sourceforge.net
 *
 * See http://filext.com/file-extension/WVE
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
#include <string.h>

static char const ID1[18] = "ALawSoundFile**\0\017\020";
static char const ID2[] = {0,0,0,1,0,0,0,0,0,0}; /* pad & repeat info: ignore */

static int start_read(sox_format_t * ft)
{
  char buf[sizeof(ID1)];
  uint32_t num_samples;

  if (lsx_readchars(ft, buf, sizeof(buf)) || lsx_readdw(ft, &num_samples) ||
      lsx_skipbytes(ft, sizeof(ID2)))
    return SOX_EOF;
  if (memcmp(ID1, buf, sizeof(buf))) {
    lsx_fail_errno(ft, SOX_EHDR, "wve: can't find Psion identifier");
    return SOX_EOF;
  }
  return lsx_check_read_params(ft, 1, 8000., SOX_ENCODING_ALAW, 8, (uint64_t)num_samples, sox_true);
}

static int write_header(sox_format_t * ft)
{
  uint64_t size64 = ft->olength? ft->olength:ft->signal.length;
  unsigned size = size64 > UINT_MAX ? 0 : (unsigned)size64;
  return lsx_writechars(ft, ID1, sizeof(ID1))
      || lsx_writedw(ft, size)
      || lsx_writechars(ft, ID2, sizeof(ID2))? SOX_EOF:SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(wve)
{
  static char const * const names[] = {"wve", NULL};
  static sox_rate_t   const write_rates[] = {8000, 0};
  static unsigned     const write_encodings[] = {SOX_ENCODING_ALAW, 8, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Psion 3 audio format",
    names, SOX_FILE_BIG_END | SOX_FILE_MONO | SOX_FILE_REWIND,
    start_read, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, write_rates, 0
  };
  return &handler;
}
