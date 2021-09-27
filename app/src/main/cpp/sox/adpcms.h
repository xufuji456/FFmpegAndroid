/* libSoX ADPCM codecs: IMA, OKI, CL.   (c) 2007-8 robs@users.sourceforge.net
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

typedef struct {
  int max_step_index;
  int sign;
  int shift;
  int const * steps;
  int const * changes;
  int mask;
} adpcm_setup_t;

typedef struct {
  adpcm_setup_t setup;
  int last_output;
  int step_index;
  int errors;
} adpcm_t;

void lsx_adpcm_init(adpcm_t * p, int type, int first_sample);
int lsx_adpcm_decode(int code, adpcm_t * p);
int lsx_adpcm_encode(int sample, adpcm_t * p);

typedef struct {
  adpcm_t encoder;
  struct {
    uint8_t byte;               /* write store */
    uint8_t flag;
  } store;
  sox_fileinfo_t file;
} adpcm_io_t;

/* Format methods */
void lsx_adpcm_reset(adpcm_io_t * state, sox_encoding_t type);
int lsx_adpcm_oki_start(sox_format_t * ft, adpcm_io_t * state);
int lsx_adpcm_ima_start(sox_format_t * ft, adpcm_io_t * state);
size_t lsx_adpcm_read(sox_format_t * ft, adpcm_io_t * state, sox_sample_t *buffer, size_t len);
int lsx_adpcm_stopread(sox_format_t * ft, adpcm_io_t * state);
size_t lsx_adpcm_write(sox_format_t * ft, adpcm_io_t * state, const sox_sample_t *buffer, size_t length);
void lsx_adpcm_flush(sox_format_t * ft, adpcm_io_t * state);
int lsx_adpcm_stopwrite(sox_format_t * ft, adpcm_io_t * state);
