/* libSoX file format: HTK   (c) 2008 robs@users.sourceforge.net
 *
 * See http://labrosa.ee.columbia.edu/doc/HTKBook21/HTKBook.html
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

typedef enum {
  Waveform, Lpc, Lprefc, Lpcepstra, Lpdelcep, Irefc,
  Mfcc, Fbank, Melspec, User, Discrete, Unknown} kind_t;
static char const * const str[] = {
  "Sampled waveform", "Linear prediction filter", "Linear prediction",
  "LPC cepstral", "LPC cepstra plus delta", "LPC reflection coef in",
  "Mel-frequency cepstral", "Log mel-filter bank", "Linear mel-filter bank",
  "User defined sample", "Vector quantised data", "Unknown"};

static int start_read(sox_format_t * ft)
{
  uint32_t period_100ns, num_samples;
  uint16_t bytes_per_sample, parmKind;

  if (lsx_readdw(ft, &num_samples     ) ||
      lsx_readdw(ft, &period_100ns    ) ||
      lsx_readw (ft, &bytes_per_sample) ||
      lsx_readw (ft, &parmKind        )) return SOX_EOF;
  if (parmKind != Waveform) {
    int n = min(parmKind & 077, Unknown);
    lsx_fail_errno(ft, SOX_EFMT, "unsupported HTK type `%s' (0%o)", str[n], parmKind);
    return SOX_EOF;
  }
  return lsx_check_read_params(ft, 1, 1e7 / period_100ns, SOX_ENCODING_SIGN2,
      (unsigned)bytes_per_sample << 3, (uint64_t)num_samples, sox_true);
}

static int write_header(sox_format_t * ft)
{
  double period_100ns = 1e7 / ft->signal.rate;
  uint64_t len = ft->olength? ft->olength:ft->signal.length;

  if (len > UINT_MAX)
  {
    lsx_warn("length greater than 32 bits - cannot fit actual length in header");
    len = UINT_MAX;
  }
  if (!ft->olength && floor(period_100ns) != period_100ns)
    lsx_warn("rounding sample period %f (x 100ns) to nearest integer", period_100ns);
  return lsx_writedw(ft, (unsigned)len)
      || lsx_writedw(ft, (unsigned)(period_100ns + .5))
      || lsx_writew(ft, ft->encoding.bits_per_sample >> 3)
      || lsx_writew(ft, Waveform) ? SOX_EOF : SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(htk)
{
  static char const * const names[] = {"htk", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_SIGN2, 16, 0, 0};
  static sox_format_handler_t handler = {
    SOX_LIB_VERSION_CODE,
    "PCM format used for Hidden Markov Model speech processing",
    names, SOX_FILE_BIG_END | SOX_FILE_MONO | SOX_FILE_REWIND,
    start_read, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &handler;
}
