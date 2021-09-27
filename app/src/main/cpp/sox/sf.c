/* libSoX file format: IRCAM SoundFile   (c) 2008 robs@users.sourceforge.net
 *
 * See http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/IRCAM/IRCAM.html
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

/* Magic numbers used in IRCAM audio files */
static struct {char str[4]; sox_bool reverse_bytes; char const * desc;} id[] = {
  {"\144\243\001\0", MACHINE_IS_BIGENDIAN   , "little-endian VAX (native)"},
  {"\0\001\243\144", MACHINE_IS_LITTLEENDIAN, "big-endian VAX"},
  {"\144\243\002\0", MACHINE_IS_LITTLEENDIAN, "big-endian Sun (native)"},
  {"\0\002\243\144", MACHINE_IS_BIGENDIAN   , "little-endian Sun"},
  {"\144\243\003\0", MACHINE_IS_BIGENDIAN   , "little-endian MIPS (DEC)"},
  {"\0\003\243\144", MACHINE_IS_LITTLEENDIAN, "big-endian MIPS (SGI)"},
  {"\144\243\004\0", MACHINE_IS_LITTLEENDIAN, "big-endian NeXT"},
  {"    ", 0, NULL}
};
#define FIXED_HDR     1024
#define SF_COMMENT    2        /* code for "comment line" */

typedef enum {Unspecified,
  Linear_8 = 0x00001, Alaw_8 = 0x10001, Mulaw_8 = 0x20001, Linear_16 = 0x00002,
  Linear_24 = 0x00003, Linear_32 = 0x40004, Float = 0x00004, Double = 0x00008
} ft_encoding_t;

static ft_encoding_t ft_enc(unsigned size, sox_encoding_t encoding)
{
  if (encoding == SOX_ENCODING_ULAW  && size ==  8) return Mulaw_8;
  if (encoding == SOX_ENCODING_ALAW  && size ==  8) return Alaw_8;
  if (encoding == SOX_ENCODING_SIGN2 && size ==  8) return Linear_8;
  if (encoding == SOX_ENCODING_SIGN2 && size == 16) return Linear_16;
  if (encoding == SOX_ENCODING_SIGN2 && size == 24) return Linear_24;
  if (encoding == SOX_ENCODING_SIGN2 && size == 32) return Linear_32;
  if (encoding == SOX_ENCODING_FLOAT && size == 32) return Float;
  if (encoding == SOX_ENCODING_FLOAT && size == 64) return Double;
  return Unspecified;
}

static sox_encoding_t sox_enc(uint32_t ft_encoding, unsigned * size)
{
  switch (ft_encoding) {
    case Mulaw_8     : *size =  8; return SOX_ENCODING_ULAW;
    case Alaw_8      : *size =  8; return SOX_ENCODING_ALAW;
    case Linear_8    : *size =  8; return SOX_ENCODING_SIGN2;
    case Linear_16   : *size = 16; return SOX_ENCODING_SIGN2;
    case Linear_24   : *size = 24; return SOX_ENCODING_SIGN2;
    case Linear_32   : *size = 32; return SOX_ENCODING_SIGN2;
    case Float       : *size = 32; return SOX_ENCODING_FLOAT;
    case Double      : *size = 64; return SOX_ENCODING_FLOAT;
    default:                       return SOX_ENCODING_UNKNOWN;
  }
}

static int startread(sox_format_t * ft)
{
  char     magic[4];
  float    rate;
  uint32_t channels, ft_encoding;
  unsigned i, bits_per_sample;
  sox_encoding_t encoding;
  uint16_t code, size;

  if (lsx_readchars(ft, magic, sizeof(magic)))
    return SOX_EOF;

  for (i = 0; id[i].desc && memcmp(magic, id[i].str, sizeof(magic)); ++i);
  if (!id[i].desc) {
    lsx_fail_errno(ft, SOX_EHDR, "sf: can't find IRCAM identifier");
    return SOX_EOF;
  }
  lsx_report("found %s identifier", id[i].desc);
  ft->encoding.reverse_bytes = id[i].reverse_bytes;

  if (lsx_readf(ft, &rate) || lsx_readdw(ft, &channels) || lsx_readdw(ft, &ft_encoding))
    return SOX_EOF;

  if (!(encoding = sox_enc(ft_encoding, &bits_per_sample))) {
    lsx_fail_errno(ft, SOX_EFMT, "sf: unsupported encoding %#x)", ft_encoding);
    return SOX_EOF;
  }
  do {
    if (lsx_readw(ft, &code) || lsx_readw(ft, &size))
      return SOX_EOF;
    if (code == SF_COMMENT) {
      char * buf = lsx_calloc(1, (size_t)size + 1); /* +1 ensures null-terminated */
      if (lsx_readchars(ft, buf, (size_t) size) != SOX_SUCCESS) {
        free(buf);
        return SOX_EOF;
      }
      sox_append_comments(&ft->oob.comments, buf);
      free(buf);
    }
    else if (lsx_skipbytes(ft, (size_t) size))
      return SOX_EOF;
  } while (code);
  if (lsx_skipbytes(ft, FIXED_HDR - (size_t)lsx_tell(ft)))
    return SOX_EOF;

  return lsx_check_read_params(ft, channels, rate, encoding, bits_per_sample, (uint64_t)0, sox_true);
}

static int write_header(sox_format_t * ft)
{
  char * comment  = lsx_cat_comments(ft->oob.comments);
  size_t len      = min(FIXED_HDR - 26, strlen(comment)) + 1; /* null-terminated */
  size_t info_len = max(4, (len + 3) & ~3u); /* Minimum & multiple of 4 bytes */
  int i = ft->encoding.reverse_bytes == MACHINE_IS_BIGENDIAN? 0 : 2;
  sox_bool error  = sox_false
  ||lsx_writechars(ft, id[i].str, sizeof(id[i].str))
  ||lsx_writef(ft, ft->signal.rate)
  ||lsx_writedw(ft, ft->signal.channels)
  ||lsx_writedw(ft, ft_enc(ft->encoding.bits_per_sample, ft->encoding.encoding))
  ||lsx_writew(ft, SF_COMMENT)
  ||lsx_writew(ft, (unsigned) info_len)
  ||lsx_writechars(ft, comment, len)
  ||lsx_padbytes(ft, FIXED_HDR - 20 - len);
  free(comment);
  return error? SOX_EOF: SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(sf)
{
  static char const * const names[] = {"sf", "ircam", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_SIGN2, 8, 16, 24, 32, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Institut de Recherche et Coordination Acoustique/Musique",
    names, SOX_FILE_LIT_END,
    startread, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &handler;
}
