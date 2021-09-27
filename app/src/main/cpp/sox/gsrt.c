/* libSoX file format: Grandstream ring tone (c) 2009 robs@users.sourceforge.net
 *
 * See https://web.archive.org/web/20101128121923/http://grandstream.com/ringtone.html
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
#include <time.h>

#define VERSION_      0x1000000
#define MAX_FILE_SIZE 0x10000
#define HEADER_SIZE   (size_t)512
#define PADDING_SIZE  (size_t)478

static char const id[16] = "ring.bin";

typedef struct {
  char const *    string;
  int             ft_encoding;
  unsigned        bits_per_sample;
  sox_encoding_t  sox_encoding;
} table_t;

static table_t const table[] = {
  {NULL,    0,    8, SOX_ENCODING_ULAW}, 
  {"G726",  2,    0, SOX_ENCODING_UNKNOWN}, 
  {NULL,    3,    0, SOX_ENCODING_GSM}, 
  {NULL,    4,    0, SOX_ENCODING_G723}, 
  {NULL,    8,    8, SOX_ENCODING_ALAW}, 
  {"G722",  9,    0, SOX_ENCODING_UNKNOWN}, 
  {"G728", 15,    2, SOX_ENCODING_UNKNOWN}, 
  {"iLBC", 98,    0, SOX_ENCODING_UNKNOWN}, 
};

static int ft_enc(unsigned bits_per_sample, sox_encoding_t encoding)
{
  size_t i;
  for (i = 0; i < array_length(table); ++i) {
    table_t const * t = &table[i];
    if (t->sox_encoding == encoding && t->bits_per_sample == bits_per_sample)
      return t->ft_encoding;
  }
  return -1; /* Should never get here. */
}

static sox_encoding_t sox_enc(int ft_encoding, unsigned * bits_per_sample)
{
  size_t i;
  for (i = 0; i < array_length(table); ++i) {
    table_t const * t = &table[i];
    if (t->ft_encoding == ft_encoding) {
      *bits_per_sample = t->bits_per_sample;
      if (t->sox_encoding == SOX_ENCODING_UNKNOWN)
        lsx_report("unsupported encoding: %s", t->string);
      return t->sox_encoding;
    }
  }
  *bits_per_sample = 0;
  return SOX_ENCODING_UNKNOWN;
}

static int start_read(sox_format_t * ft)
{
  off_t num_samples;
  char read_id[array_length(id)];
  uint32_t file_size;
  int16_t ft_encoding;
  sox_encoding_t encoding;
  unsigned bits_per_sample;

  lsx_readdw(ft, &file_size);
  num_samples = file_size? file_size * 2 - HEADER_SIZE : SOX_UNSPEC;

  if (file_size >= 2 && ft->seekable) {
    int i, checksum = (file_size >> 16) + file_size;
    for (i = file_size - 2; i; --i) {
      int16_t int16;
      lsx_readsw(ft, &int16);
      checksum += int16;
    }
    if (lsx_seeki(ft, (off_t)sizeof(file_size), SEEK_SET) != 0)
      return SOX_EOF;
    if (checksum & 0xffff)
      lsx_warn("invalid checksum in input file %s", ft->filename);
  }

  lsx_skipbytes(ft, (size_t)(2 + 4 + 6)); /* Checksum, version, time stamp. */

  lsx_readchars(ft, read_id, sizeof(read_id));
  if (memcmp(read_id, id, strlen(id))) {
    lsx_fail_errno(ft, SOX_EHDR, "gsrt: invalid file name in header");
    return SOX_EOF;
  }

  lsx_readsw(ft, &ft_encoding);
  encoding = sox_enc(ft_encoding, &bits_per_sample);
  if (encoding != SOX_ENCODING_ALAW &&
      encoding != SOX_ENCODING_ULAW)
    ft->handler.read = NULL;

  lsx_skipbytes(ft, PADDING_SIZE);

  return lsx_check_read_params(ft, 1, 8000., encoding,
      bits_per_sample, (uint64_t)num_samples, sox_true);
}

static int start_write(sox_format_t * ft)
{
  int i, encoding = ft_enc(ft->encoding.bits_per_sample, ft->encoding.encoding);
  time_t now = sox_globals.repeatable? 0 : time(NULL);
  struct tm const * t = sox_globals.repeatable? gmtime(&now) : localtime(&now);

  int checksum = (VERSION_ >> 16) + VERSION_;
  checksum += t->tm_year + 1900;
  checksum += ((t->tm_mon + 1) << 8) + t->tm_mday;
  checksum += (t->tm_hour << 8) + t->tm_min;
  for (i = sizeof(id) - 2; i >= 0; i -= 2)
    checksum += (id[i] << 8) + id[i + 1];
  checksum += encoding;

  return lsx_writedw(ft, 0)
      || lsx_writesw(ft, -checksum)
      || lsx_writedw(ft, VERSION_)
      || lsx_writesw(ft, t->tm_year + 1900)
      || lsx_writesb(ft, t->tm_mon + 1)
      || lsx_writesb(ft, t->tm_mday)
      || lsx_writesb(ft, t->tm_hour)
      || lsx_writesb(ft, t->tm_min)
      || lsx_writechars(ft, id, sizeof(id))
      || lsx_writesw(ft, encoding)
      || lsx_padbytes(ft, PADDING_SIZE) ? SOX_EOF : SOX_SUCCESS;
}

static size_t write_samples(
    sox_format_t * ft, sox_sample_t const * buf, size_t nsamp)
{
  size_t n = min(nsamp, MAX_FILE_SIZE - (size_t)ft->tell_off);
  if (n != nsamp)
    lsx_warn("audio truncated");
  return lsx_rawwrite(ft, buf, n);
}

static int stop_write(sox_format_t * ft)
{
  long num_samples = ft->tell_off - HEADER_SIZE;

  if (num_samples & 1) {
    sox_sample_t pad = 0;
    lsx_rawwrite(ft, &pad, 1);
  }

  if (ft->seekable) {
    unsigned i, file_size = ft->tell_off >> 1;
    int16_t int16;
    int checksum;
    if (!lsx_seeki(ft, (off_t)sizeof(uint32_t), SEEK_SET)) {
      lsx_readsw(ft, &int16);
      checksum = (file_size >> 16) + file_size - int16;
      if (!lsx_seeki(ft, (off_t)HEADER_SIZE, SEEK_SET)) {
        for (i = (num_samples + 1) >> 1; i; --i) {
          lsx_readsw(ft, &int16);
          checksum += int16;
        }
        if (!lsx_seeki(ft, (off_t)0, SEEK_SET)) {
          lsx_writedw(ft, file_size);
          lsx_writesw(ft, -checksum);
          return SOX_SUCCESS;
        }
      }
    }
  }
  lsx_warn("can't seek in output file `%s'; "
      "length in file header will be unspecified", ft->filename);
  return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(gsrt)
{
  static char const *const names[] = { "gsrt", NULL };
  static sox_rate_t const write_rates[] = { 8000, 0 };
  static unsigned const write_encodings[] = {
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_ULAW, 8, 0,
    0
  };
  static sox_format_handler_t const handler = {
    SOX_LIB_VERSION_CODE, "Grandstream ring tone",
    names, SOX_FILE_BIG_END | SOX_FILE_MONO,
    start_read, lsx_rawread, NULL,
    start_write, write_samples, stop_write,
    lsx_rawseek, write_encodings, write_rates, 0
  };
  return &handler;
}
