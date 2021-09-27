/* libSoX file format: CVSD (see cvsd.c)        (c) 2007-8 SoX contributors
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

#include "cvsd.h"

LSX_FORMAT_HANDLER(cvsd)
{
  static char const * const names[] = {"cvsd", "cvs", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_CVSD, 1, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Headerless MIL Std 188 113 Continuously Variable Slope Delta modulation",
    names, SOX_FILE_MONO,
    lsx_cvsdstartread, lsx_cvsdread, lsx_cvsdstopread,
    lsx_cvsdstartwrite, lsx_cvsdwrite, lsx_cvsdstopwrite,
    lsx_rawseek, write_encodings, NULL, sizeof(cvsd_priv_t)
  };
  return &handler;
}

/* libSoX file format: CVU   (c) 2008 robs@users.sourceforge.net
 * Unfiltered, therefore, on decode, use with either filter -4k or rate 8k */

typedef struct {
  double         sample, step, step_mult, step_add;
  unsigned       last_n_bits;
  unsigned char  byte;
  off_t          bit_count;
} priv_t;

static int start(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;

  ft->signal.channels = 1;
  lsx_rawstart(ft, sox_true, sox_false, sox_true, SOX_ENCODING_CVSD, 1);
  p->last_n_bits = 5; /* 101 */
  p->step_mult = exp((-1 / .005 / ft->signal.rate));
  p->step_add = (1 - p->step_mult) * (.1 * SOX_SAMPLE_MAX);
  lsx_debug("step_mult=%g step_add=%f", p->step_mult, p->step_add);
  return SOX_SUCCESS;
}

static void decode(priv_t * p, int bit)
{
  p->last_n_bits = ((p->last_n_bits << 1) | bit) & 7;

  p->step *= p->step_mult;
  if (p->last_n_bits == 0 || p->last_n_bits == 7)
    p->step += p->step_add;

  if (p->last_n_bits & 1)
    p->sample = min(p->step_mult * p->sample + p->step, SOX_SAMPLE_MAX);
  else
    p->sample = max(p->step_mult * p->sample - p->step, SOX_SAMPLE_MIN);
}

static size_t cvsdread(sox_format_t * ft, sox_sample_t * buf, size_t len)
{
  priv_t *p = (priv_t *) ft->priv;
  size_t i;

  for (i = 0; i < len; ++i) {
    if (!(p->bit_count & 7))
      if (lsx_read_b_buf(ft, &p->byte, (size_t)1) != 1)
        break;
    ++p->bit_count;
    decode(p, p->byte & 1);
    p->byte >>= 1;
    *buf++ = floor(p->sample + .5);
  }
  return i;
}

static size_t cvsdwrite(sox_format_t * ft, sox_sample_t const * buf, size_t len)
{
  priv_t *p = (priv_t *) ft->priv;
  size_t i;

  for (i = 0; i < len; ++i) {
    decode(p, *buf++ > p->sample);
    p->byte >>= 1;
    p->byte |= p->last_n_bits << 7;
    if (!(++p->bit_count & 7))
      if (lsx_writeb(ft, p->byte) != SOX_SUCCESS)
        break;
  }
  return len;
}

LSX_FORMAT_HANDLER(cvu)
{
  static char const * const names[] = {"cvu", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_CVSD, 1, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Headerless Continuously Variable Slope Delta modulation (unfiltered)",
    names, SOX_FILE_MONO, start, cvsdread, NULL, start, cvsdwrite, NULL,
    lsx_rawseek, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
