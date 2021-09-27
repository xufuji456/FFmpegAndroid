/* libSoX file format: null   (c) 2006-8 SoX contributors
 * Based on an original idea by Carsten Borchardt
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

static int startread(sox_format_t * ft)
{
  if (!ft->signal.rate) {
    ft->signal.rate = SOX_DEFAULT_RATE;
    lsx_report("sample rate not specified; using %g", ft->signal.rate);
  }
  ft->signal.precision = ft->encoding.bits_per_sample?
      ft->encoding.bits_per_sample: SOX_SAMPLE_PRECISION;
  /* Default number of channels is application-dependent */
  return SOX_SUCCESS;
}

static size_t read_samples(sox_format_t * ft, sox_sample_t * buf, size_t len)
{
  /* Reading from null generates silence i.e. (sox_sample_t)0. */
  (void)ft;
  memset(buf, 0, sizeof(sox_sample_t) * len);
  return len; /* Return number of samples "read". */
}

static size_t write_samples(
    sox_format_t * ft, sox_sample_t const * buf, size_t len)
{
  /* Writing to null just discards the samples */
  (void)ft, (void)buf;
  return len; /* Return number of samples "written". */
}

LSX_FORMAT_HANDLER(nul)
{
  static const char * const names[] = {"null", NULL};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    NULL, names, SOX_FILE_DEVICE | SOX_FILE_PHONY | SOX_FILE_NOSTDIO,
    startread, read_samples,NULL,NULL, write_samples,NULL,NULL, NULL, NULL, 0
  };
  return &handler;
}
