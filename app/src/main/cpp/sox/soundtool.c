/* libSoX SoundTool format handler          (c) 2008 robs@users.sourceforge.net
 * See description in sndtl26.zip on the net.
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

static char const ID1[6] = "SOUND\x1a";
#define text_field_len (size_t)96  /* Includes null-terminator */

static int start_read(sox_format_t * ft)
{
  char id1[sizeof(ID1)], comments[text_field_len + 1];
  uint32_t nsamples;
  uint16_t rate;

  if (lsx_readchars(ft, id1, sizeof(ID1)) ||
      lsx_skipbytes(ft, (size_t) 10) || lsx_readdw(ft, &nsamples) ||
      lsx_readw(ft, &rate) || lsx_skipbytes(ft, (size_t) 6) ||
      lsx_readchars(ft, comments, text_field_len))
    return SOX_EOF;
  if (memcmp(ID1, id1, sizeof(id1))) {
    lsx_fail_errno(ft, SOX_EHDR, "soundtool: can't find SoundTool identifier");
    return SOX_EOF;
  }
  comments[text_field_len] = '\0'; /* Be defensive against incorrect files */
  sox_append_comments(&ft->oob.comments, comments);
  return lsx_check_read_params(ft, 1, (sox_rate_t)rate, SOX_ENCODING_UNSIGNED, 8, (uint64_t)nsamples, sox_true);
}

static int write_header(sox_format_t * ft)
{
  char * comment = lsx_cat_comments(ft->oob.comments);
  char text_buf[text_field_len];
  uint64_t length = ft->olength? ft->olength:ft->signal.length;

  memset(text_buf, 0, sizeof(text_buf));
  strncpy(text_buf, comment, text_field_len - 1);
  free(comment);
  return lsx_writechars(ft, ID1, sizeof(ID1))
      || lsx_writew  (ft, 0)      /* GSound: not used */
      || lsx_writedw (ft, (unsigned) length) /* length of complete sample */
      || lsx_writedw (ft, 0)      /* first byte to play from sample */
      || lsx_writedw (ft, (unsigned) length) /* first byte NOT to play from sample */
      || lsx_writew  (ft, min(65535, (unsigned)(ft->signal.rate + .5)))
      || lsx_writew  (ft, 0)      /* sample size/type */
      || lsx_writew  (ft, 10)     /* speaker driver volume */
      || lsx_writew  (ft, 4)      /* speaker driver DC shift */
      || lsx_writechars(ft, text_buf, sizeof(text_buf))?  SOX_EOF:SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(soundtool)
{
  static char const * const names[] = {"sndt", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_UNSIGNED, 8, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "8-bit linear audio as used by Martin Hepperle's `SoundTool' of 1991/2",
    names, SOX_FILE_LIT_END | SOX_FILE_MONO | SOX_FILE_REWIND,
    start_read, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &handler;
}
