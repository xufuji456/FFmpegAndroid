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

#include "sox_i.h"
#include "adpcms.h"

static int const ima_steps[89] = { /* ~16-bit precision; 4 bit code */
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
  253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
  1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
  3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
  11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
  32767};

static int const oki_steps[49] = { /* ~12-bit precision; 4 bit code */
  256, 272, 304, 336, 368, 400, 448, 496, 544, 592, 656, 720, 800, 880, 960,
  1056, 1168, 1280, 1408, 1552, 1712, 1888, 2080, 2288, 2512, 2768, 3040, 3344,
  3680, 4048, 4464, 4912, 5392, 5936, 6528, 7184, 7904, 8704, 9568, 10528,
  11584, 12736, 14016, 15408, 16960, 18656, 20512, 22576, 24832};

static int const step_changes[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

/* Creative Labs ~8 bit precision; 4, 3, & 2 bit codes: */
static int const cl4_steps[4] = {0x100, 0x200, 0x400, 0x800};
static int const cl4_changes[8] = {-1, 0, 0, 0, 0, 1, 1, 1};

static int const cl3_steps[5] = {0x100, 0x200, 0x400, 0x800, 0xA00};
static int const cl3_changes[4] = {-1, 0, 0, 1};

static int const cl2_steps[6] = {0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000};
static int const cl2_changes[2] = {-1, 1};

static adpcm_setup_t const setup_table[] = {
  {88, 8, 2, ima_steps, step_changes, ~0},
  {48, 8, 2, oki_steps, step_changes, ~15},
  { 3, 8, 0, cl4_steps, cl4_changes , ~255},
  { 4, 4, 0, cl3_steps, cl3_changes , ~255},
  { 5, 2, 0, cl2_steps, cl2_changes , ~255},
};

void lsx_adpcm_init(adpcm_t * p, int type, int first_sample)
{
  p->setup = setup_table[type];
  p->last_output = first_sample;
  p->step_index = 0;
  p->errors = 0;
}

#define min_sample -0x8000
#define max_sample 0x7fff

int lsx_adpcm_decode(int code, adpcm_t * p)
{
  int s = ((code & (p->setup.sign - 1)) << 1) | 1;
  s = ((p->setup.steps[p->step_index] * s) >> (p->setup.shift + 1)) & p->setup.mask;
  if (code & p->setup.sign)
    s = -s;
  s += p->last_output;
  if (s < min_sample || s > max_sample) {
    int grace = (p->setup.steps[p->step_index] >> (p->setup.shift + 1)) & p->setup.mask;
    if (s < min_sample - grace || s > max_sample + grace) {
      lsx_debug_most("code=%i step=%i grace=%i s=%i",
          code & (2 * p->setup.sign - 1), p->setup.steps[p->step_index], grace, s);
      p->errors++;
    }
    s = s < min_sample? min_sample : max_sample;
  }
  p->step_index += p->setup.changes[code & (p->setup.sign - 1)];
  p->step_index = range_limit(p->step_index, 0, p->setup.max_step_index);
  return p->last_output = s;
}

int lsx_adpcm_encode(int sample, adpcm_t * p)
{
  int delta = sample - p->last_output;
  int sign = 0;
  int code;
  if (delta < 0) {
    sign = p->setup.sign;
    delta = -delta;
  }
  code = (delta << p->setup.shift) / p->setup.steps[p->step_index];
  code = sign | min(code, p->setup.sign - 1);
  lsx_adpcm_decode(code, p); /* Update encoder state */
  return code;
}


/*
 * Format methods
 *
 * Almost like the raw format functions, but cannot be used directly
 * since they require an additional state parameter.
 */

/******************************************************************************
 * Function   : lsx_adpcm_reset
 * Description: Resets the ADPCM codec state.
 * Parameters : state - ADPCM state structure
 *              type - SOX_ENCODING_OKI_ADPCM or SOX_ENCODING_IMA_ADPCM
 * Returns    :
 * Exceptions :
 * Notes      : 1. This function is used for framed ADPCM formats to reset
 *                 the decoder between frames.
 ******************************************************************************/

void lsx_adpcm_reset(adpcm_io_t * state, sox_encoding_t type)
{
  state->file.count = 0;
  state->file.pos = 0;
  state->store.byte = 0;
  state->store.flag = 0;

  lsx_adpcm_init(&state->encoder, (type == SOX_ENCODING_OKI_ADPCM) ? 1 : 0, 0);
}

/******************************************************************************
 * Function   : lsx_adpcm_start
 * Description: Initialises the file parameters and ADPCM codec state.
 * Parameters : ft  - file info structure
 *              state - ADPCM state structure
 *              type - SOX_ENCODING_OKI_ADPCM or SOX_ENCODING_IMA_ADPCM
 * Returns    : int - SOX_SUCCESS
 *                    SOX_EOF
 * Exceptions :
 * Notes      : 1. This function can be used as a startread or
 *                 startwrite method.
 *              2. VOX file format is 4-bit OKI ADPCM that decodes to
 *                 to 12 bit signed linear PCM.
 *              3. Dialogic only supports 6kHz, 8kHz and 11 kHz sampling
 *                 rates but the codecs allows any user specified rate.
 ******************************************************************************/

static int adpcm_start(sox_format_t * ft, adpcm_io_t * state, sox_encoding_t type)
{
  /* setup file info */
  state->file.buf = lsx_malloc(sox_globals.bufsiz);
  state->file.size = sox_globals.bufsiz;
  ft->signal.channels = 1;

  lsx_adpcm_reset(state, type);

  return lsx_rawstart(ft, sox_true, sox_false, sox_true, type, 4);
}

int lsx_adpcm_oki_start(sox_format_t * ft, adpcm_io_t * state)
{
  return adpcm_start(ft, state, SOX_ENCODING_OKI_ADPCM);
}

int lsx_adpcm_ima_start(sox_format_t * ft, adpcm_io_t * state)
{
  return adpcm_start(ft, state, SOX_ENCODING_IMA_ADPCM);
}

/******************************************************************************
 * Function   : lsx_adpcm_read
 * Description: Converts the OKI ADPCM 4-bit samples to 16-bit signed PCM and
 *              then scales the samples to full sox_sample_t range.
 * Parameters : ft     - file info structure
 *              state  - ADPCM state structure
 *              buffer - output buffer
 *              len    - size of output buffer
 * Returns    :        - number of samples returned in buffer
 * Exceptions :
 * Notes      :
 ******************************************************************************/

size_t lsx_adpcm_read(sox_format_t * ft, adpcm_io_t * state, sox_sample_t * buffer, size_t len)
{
  size_t n = 0;
  uint8_t byte;
  int16_t word;

  if (len && state->store.flag) {
    word = lsx_adpcm_decode(state->store.byte, &state->encoder);
    *buffer++ = SOX_SIGNED_16BIT_TO_SAMPLE(word, ft->clips);
    state->store.flag = 0;
    ++n;
  }
  while (n < len && lsx_read_b_buf(ft, &byte, (size_t) 1) == 1) {
    word = lsx_adpcm_decode(byte >> 4, &state->encoder);
    *buffer++ = SOX_SIGNED_16BIT_TO_SAMPLE(word, ft->clips);

    if (++n < len) {
      word = lsx_adpcm_decode(byte, &state->encoder);
      *buffer++ = SOX_SIGNED_16BIT_TO_SAMPLE(word, ft->clips);
      ++n;
    } else {
      state->store.byte = byte;
      state->store.flag = 1;
    }
  }
  return n;
}

/******************************************************************************
 * Function   : stopread
 * Description: Frees the internal buffer allocated in voxstart/imastart.
 * Parameters : ft   - file info structure
 *              state  - ADPCM state structure
 * Returns    : int  - SOX_SUCCESS
 * Exceptions :
 * Notes      :
 ******************************************************************************/

int lsx_adpcm_stopread(sox_format_t * ft UNUSED, adpcm_io_t * state)
{
  if (state->encoder.errors)
    lsx_warn("%s: ADPCM state errors: %u", ft->filename, state->encoder.errors);
  free(state->file.buf);

  return (SOX_SUCCESS);
}


/******************************************************************************
 * Function   : write
 * Description: Converts the supplied buffer to 12 bit linear PCM and encodes
 *              to OKI ADPCM 4-bit samples (packed a two nibbles per byte).
 * Parameters : ft     - file info structure
 *              state  - ADPCM state structure
 *              buffer - output buffer
 *              length - size of output buffer
 * Returns    : int    - SOX_SUCCESS
 *                       SOX_EOF
 * Exceptions :
 * Notes      :
 ******************************************************************************/

size_t lsx_adpcm_write(sox_format_t * ft, adpcm_io_t * state, const sox_sample_t * buffer, size_t length)
{
  size_t count = 0;
  uint8_t byte = state->store.byte;
  uint8_t flag = state->store.flag;
  short word;

  while (count < length) {
    SOX_SAMPLE_LOCALS;
    word = SOX_SAMPLE_TO_SIGNED_16BIT(*buffer++, ft->clips);

    byte <<= 4;
    byte |= lsx_adpcm_encode(word, &state->encoder) & 0x0F;

    flag = !flag;

    if (flag == 0) {
      state->file.buf[state->file.count++] = byte;

      if (state->file.count >= state->file.size) {
        lsx_writebuf(ft, state->file.buf, state->file.count);

        state->file.count = 0;
      }
    }

    count++;
  }

  /* keep last byte across calls */
  state->store.byte = byte;
  state->store.flag = flag;
  return (count);
}

/******************************************************************************
 * Function   : lsx_adpcm_flush
 * Description: Flushes any leftover samples.
 * Parameters : ft   - file info structure
 *              state  - ADPCM state structure
 * Returns    :
 * Exceptions :
 * Notes      : 1. Called directly for writing framed formats
 ******************************************************************************/

void lsx_adpcm_flush(sox_format_t * ft, adpcm_io_t * state)
{
  uint8_t byte = state->store.byte;
  uint8_t flag = state->store.flag;

  /* flush remaining samples */

  if (flag != 0) {
    byte <<= 4;
    state->file.buf[state->file.count++] = byte;
  }
  if (state->file.count > 0)
    lsx_writebuf(ft, state->file.buf, state->file.count);
}

/******************************************************************************
 * Function   : lsx_adpcm_stopwrite
 * Description: Flushes any leftover samples and frees the internal buffer
 *              allocated in voxstart/imastart.
 * Parameters : ft   - file info structure
 *              state  - ADPCM state structure
 * Returns    : int  - SOX_SUCCESS
 * Exceptions :
 * Notes      :
 ******************************************************************************/

int lsx_adpcm_stopwrite(sox_format_t * ft, adpcm_io_t * state)
{
  lsx_adpcm_flush(ft, state);
  free(state->file.buf);
  return (SOX_SUCCESS);
}
