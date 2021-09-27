/* libSoX lpc-10 format.
 *
 * Copyright 2007 Reuben Thomas <rrt@sc3d.org>
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

#include <lpc10.h>

/* Private data */
typedef struct {
  struct lpc10_encoder_state *encst;
  float speech[LPC10_SAMPLES_PER_FRAME];
  unsigned samples;
  struct lpc10_decoder_state *decst;
} priv_t;

/*
  Write the bits in bits[0] through bits[len-1] to file f, in "packed"
  format.

  bits is expected to be an array of len integer values, where each
  integer is 0 to represent a 0 bit, and any other value represents a
  1 bit. This bit string is written to the file f in the form of
  several 8 bit characters. If len is not a multiple of 8, then the
  last character is padded with 0 bits -- the padding is in the least
  significant bits of the last byte. The 8 bit characters are "filled"
  in order from most significant bit to least significant.
*/
static void write_bits(sox_format_t * ft, INT32 *bits, int len)
{
  int i;
  uint8_t mask; /* The next bit position within the variable "data" to
                   place the next bit. */
  uint8_t data; /* The contents of the next byte to place in the
                   output. */

  /* Fill in the array bits.
   * The first compressed output bit will be the most significant
   * bit of the byte, so initialize mask to 0x80.  The next byte of
   * compressed data is initially 0, and the desired bits will be
   * turned on below.
   */
  mask = 0x80;
  data = 0;

  for (i = 0; i < len; i++) {
    /* Turn on the next bit of output data, if necessary. */
    if (bits[i]) {
      data |= mask;
    }
    /*
     * If the byte data is full, determined by mask becoming 0,
     * then write the byte to the output file, and reinitialize
     * data and mask for the next output byte.  Also add the byte
     * if (i == len-1), because if len is not a multiple of 8,
     * then mask won't yet be 0.  */
    mask >>= 1;
    if ((mask == 0) || (i == len-1)) {
      lsx_writeb(ft, data);
      data = 0;
      mask = 0x80;
    }
  }
}

/*
  Read bits from file f into bits[0] through bits[len-1], in "packed"
  format.

  Read ceiling(len/8) characters from file f, if that many are
  available to read, otherwise read to the end of the file. The first
  character's 8 bits, in order from MSB to LSB, are used to fill
  bits[0] through bits[7]. The second character's bits are used to
  fill bits[8] through bits[15], and so on. If ceiling(len/8)
  characters are available to read, and len is not a multiple of 8,
  then some of the least significant bits of the last character read
  are completely ignored. Every entry of bits[] that is modified is
  changed to either a 0 or a 1.

  The number of bits successfully read is returned, and is always in
  the range 0 to len, inclusive. If it is less than len, it will
  always be a multiple of 8.
*/
static int read_bits(sox_format_t * ft, INT32 *bits, int len)
{
  int i;
  uint8_t c = 0;

  /* Unpack the array bits into coded_frame. */
  for (i = 0; i < len; i++) {
    if (i % 8 == 0) {
      lsx_read_b_buf(ft, &c, (size_t) 1);
      if (lsx_eof(ft)) {
        return (i);
      }
    }
    if (c & (0x80 >> (i & 7))) {
      bits[i] = 1;
    } else {
      bits[i] = 0;
    }
  }
  return (len);
}

static int startread(sox_format_t * ft)
{
  priv_t * lpc = (priv_t *)ft->priv;

  if ((lpc->decst = create_lpc10_decoder_state()) == NULL) {
    fprintf(stderr, "lpc10 could not allocate decoder state");
    return SOX_EOF;
  }
  lpc->samples = LPC10_SAMPLES_PER_FRAME;
  return lsx_check_read_params(ft, 1, 8000., SOX_ENCODING_LPC10, 0, (uint64_t)0, sox_false);
}

static int startwrite(sox_format_t * ft)
{
  priv_t * lpc = (priv_t *)ft->priv;

  if ((lpc->encst = create_lpc10_encoder_state()) == NULL) {
    fprintf(stderr, "lpc10 could not allocate encoder state");
    return SOX_EOF;
  }
  lpc->samples = 0;

  return SOX_SUCCESS;
}

static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
  priv_t * lpc = (priv_t *)ft->priv;
  size_t nread = 0;

  while (nread < len) {
    SOX_SAMPLE_LOCALS;
    /* Read more data if buffer is empty */
    if (lpc->samples == LPC10_SAMPLES_PER_FRAME) {
      INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];

      if (read_bits(ft, bits, LPC10_BITS_IN_COMPRESSED_FRAME) !=
          LPC10_BITS_IN_COMPRESSED_FRAME)
        break;
      lpc10_decode(bits, lpc->speech, lpc->decst);
      lpc->samples = 0;
    }

    while (nread < len && lpc->samples < LPC10_SAMPLES_PER_FRAME)
      buf[nread++] = SOX_FLOAT_32BIT_TO_SAMPLE(lpc->speech[lpc->samples++], ft->clips);
  }

  return nread;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
  priv_t * lpc = (priv_t *)ft->priv;
  size_t nwritten = 0;

  while (len > 0) {
    while (len > 0 && lpc->samples < LPC10_SAMPLES_PER_FRAME) {
      SOX_SAMPLE_LOCALS;
      lpc->speech[lpc->samples++] = SOX_SAMPLE_TO_FLOAT_32BIT(buf[nwritten++], ft->clips);
      len--;
    }

    if (lpc->samples == LPC10_SAMPLES_PER_FRAME) {
      INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];

      lpc10_encode(lpc->speech, bits, lpc->encst);
      write_bits(ft, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
      lpc->samples = 0;
    }
  }

  return nwritten;
}

static int stopread(sox_format_t * ft)
{
  priv_t * lpc = (priv_t *)ft->priv;

  free(lpc->decst);

  return SOX_SUCCESS;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * lpc = (priv_t *)ft->priv;

  free(lpc->encst);

  return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(lpc10)
{
  static char const * const names[] = {"lpc10", "lpc", NULL};
  static sox_rate_t   const write_rates[] = {8000, 0};
  static unsigned     const write_encodings[] = {SOX_ENCODING_LPC10, 0, 0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Low bandwidth, robotic sounding speech compression", names, SOX_FILE_MONO,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    NULL, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
