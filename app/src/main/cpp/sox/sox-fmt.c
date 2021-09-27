/* libSoX file format: SoX native   (c) 2008 robs@users.sourceforge.net
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

static char const magic[2][4] = {".SoX", "XoS."};
#define FIXED_HDR     (4 + 8 + 8 + 4 + 4) /* Without magic */

static int startread(sox_format_t * ft)
{
  char     magic_[sizeof(magic[0])];
  uint32_t headers_bytes, num_channels, comments_bytes;
  uint64_t num_samples;
  double   rate;

  if (lsx_readdw(ft, (uint32_t *)&magic_))
    return SOX_EOF;

  if (memcmp(magic[MACHINE_IS_BIGENDIAN], magic_, sizeof(magic_))) {
    if (memcmp(magic[MACHINE_IS_LITTLEENDIAN], magic_, sizeof(magic_))) {
      lsx_fail_errno(ft, SOX_EHDR, "can't find sox file format identifier");
      return SOX_EOF;
    }
    ft->encoding.reverse_bytes = !ft->encoding.reverse_bytes;
    lsx_report("file is opposite endian");
  }
  if (lsx_readdw(ft, &headers_bytes) ||
      lsx_readqw(ft, &num_samples) ||
      lsx_readdf(ft, &rate) ||
      lsx_readdw(ft, &num_channels) ||
      lsx_readdw(ft, &comments_bytes))
    return SOX_EOF;

  if (((headers_bytes + 4) & 7) ||
      comments_bytes > 0x40000000 || /* max 1 GB */
      headers_bytes < FIXED_HDR + comments_bytes ||
      (num_channels > 65535)) /* Reserve top 16 bits */ {
    lsx_fail_errno(ft, SOX_EHDR, "invalid sox file format header");
    return SOX_EOF;
  }

  if (comments_bytes) {
    char * buf = lsx_calloc(1, (size_t)comments_bytes + 1); /* ensure nul-terminated */
    if (lsx_readchars(ft, buf, (size_t)comments_bytes) != SOX_SUCCESS) {
      free(buf);
      return SOX_EOF;
    }
    sox_append_comments(&ft->oob.comments, buf);
    free(buf);
  }
  
  /* Consume any bytes after the comments and before the start of the audio
   * block.  These may include comment padding up to a multiple of 8 bytes,
   * and further header information that might be defined in future. */
  lsx_seeki(ft, (off_t)(headers_bytes - FIXED_HDR - comments_bytes), SEEK_CUR);

  return lsx_check_read_params(
      ft, num_channels, rate, SOX_ENCODING_SIGN2, 32, num_samples, sox_true);
}

static int write_header(sox_format_t * ft)
{
  char * comments  = lsx_cat_comments(ft->oob.comments);
  size_t comments_len = strlen(comments);
  size_t comments_bytes = (comments_len + 7) & ~7u; /* Multiple of 8 bytes */
  uint64_t size   = ft->olength? ft->olength : ft->signal.length;
  int error;
  uint32_t header;
  memcpy(&header, magic[MACHINE_IS_BIGENDIAN], sizeof(header));
  error = 0
  ||lsx_writedw(ft, header)
  ||lsx_writedw(ft, FIXED_HDR + (unsigned)comments_bytes)
  ||lsx_writeqw(ft, size)
  ||lsx_writedf(ft, ft->signal.rate)
  ||lsx_writedw(ft, ft->signal.channels)
  ||lsx_writedw(ft, (unsigned)comments_len)
  ||lsx_writechars(ft, comments, comments_len)
  ||lsx_padbytes(ft, comments_bytes - comments_len);
  free(comments);
  return error? SOX_EOF: SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(sox)
{
  static char const * const names[] = {"sox", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_SIGN2, 32, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "SoX native intermediate format", names, SOX_FILE_REWIND, 
    startread, lsx_rawread, NULL, write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &handler;
}
