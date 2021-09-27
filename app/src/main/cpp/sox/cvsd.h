/*      libSoX DVMS format module (implementation in cvsd.c)
 *
 *      Copyright (C) 1996-2007 Thomas Sailer and SoX Contributors
 *      Thomas Sailer (sailer@ife.ee.ethz.ch) (HB9JNX/AE4WA)
 *      Swiss Federal Institute of Technology, Electronics Lab
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
 *
 */
#include "sox_i.h"

#define CVSD_ENC_FILTERLEN 16  /* PCM sampling rate */
#define CVSD_DEC_FILTERLEN 48  /* CVSD sampling rate */

typedef struct {
  struct {
    unsigned overload;
    float mla_int;
    float mla_tc0;
    float mla_tc1;
    unsigned phase;
    unsigned phase_inc;
    float v_min, v_max;
  } com;
  union {
    struct {
      /* mirror circular buffer */
      float output_filter[CVSD_DEC_FILTERLEN*2];
      unsigned offset; /* into output_filter; always in first half */
    } dec;
    struct {
      float recon_int;
      /* mirror circular buffer */
      float input_filter[CVSD_ENC_FILTERLEN*2];
      unsigned offset; /* into input_filter; always in first half */
    } enc;
  } c;
  struct {
    unsigned char shreg;
    unsigned mask;
    unsigned cnt;
  } bit;
  unsigned bytes_written;
  unsigned cvsd_rate;
} cvsd_priv_t;

int lsx_cvsdstartread(sox_format_t * ft);
int lsx_cvsdstartwrite(sox_format_t * ft);
size_t lsx_cvsdread(sox_format_t * ft, sox_sample_t *buf, size_t nsamp);
size_t lsx_cvsdwrite(sox_format_t * ft, const sox_sample_t *buf, size_t nsamp);
int lsx_cvsdstopread(sox_format_t * ft);
int lsx_cvsdstopwrite(sox_format_t * ft);

int lsx_dvmsstartread(sox_format_t * ft);
int lsx_dvmsstartwrite(sox_format_t * ft);
int lsx_dvmsstopwrite(sox_format_t * ft);
