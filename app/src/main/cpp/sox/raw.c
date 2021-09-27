/* libSoX raw I/O
 *
 * Copyright 1991-2007 Lance Norskog And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"
#include "g711.h"

typedef sox_uint16_t sox_uint14_t;
typedef sox_uint16_t sox_uint13_t;
typedef sox_int16_t sox_int14_t;
typedef sox_int16_t sox_int13_t;
#define SOX_ULAW_BYTE_TO_SAMPLE(d,clips)   SOX_SIGNED_16BIT_TO_SAMPLE(sox_ulaw2linear16(d),clips)
#define SOX_ALAW_BYTE_TO_SAMPLE(d,clips)   SOX_SIGNED_16BIT_TO_SAMPLE(sox_alaw2linear16(d),clips)
#define SOX_SAMPLE_TO_ULAW_BYTE(d,c) sox_14linear2ulaw(SOX_SAMPLE_TO_UNSIGNED(14,d,c) - 0x2000)
#define SOX_SAMPLE_TO_ALAW_BYTE(d,c) sox_13linear2alaw(SOX_SAMPLE_TO_UNSIGNED(13,d,c) - 0x1000)

int lsx_rawseek(sox_format_t * ft, uint64_t offset)
{
  return lsx_offset_seek(ft, (off_t)ft->data_start, (off_t)offset);
}

/* Works nicely for starting read and write; lsx_rawstart{read,write}
 * are #defined in sox_i.h */
int lsx_rawstart(sox_format_t * ft, sox_bool default_rate,
                 sox_bool default_channels, sox_bool default_length,
                 sox_encoding_t encoding, unsigned size)
{
  if (default_rate && ft->signal.rate == 0) {
    lsx_warn("`%s': sample rate not specified; trying 8kHz", ft->filename);
    ft->signal.rate = 8000;
  }

  if (default_channels && ft->signal.channels == 0) {
    lsx_warn("`%s': # channels not specified; trying mono", ft->filename);
    ft->signal.channels = 1;
  }

  if (encoding != SOX_ENCODING_UNKNOWN) {
    if (ft->mode == 'r' && ft->encoding.encoding != SOX_ENCODING_UNKNOWN &&
        ft->encoding.encoding != encoding)
      lsx_report("`%s': Format options overriding file-type encoding",
                 ft->filename);
    else
      ft->encoding.encoding = encoding;
  }

  if (size != 0) {
    if (ft->mode == 'r' && ft->encoding.bits_per_sample != 0 &&
        ft->encoding.bits_per_sample != size)
      lsx_report("`%s': Format options overriding file-type sample-size",
                 ft->filename);
    else
      ft->encoding.bits_per_sample = size;
  }

  if (!ft->signal.length && ft->mode == 'r' && default_length &&
      ft->encoding.bits_per_sample)
    ft->signal.length =
        div_bits(lsx_filelength(ft), ft->encoding.bits_per_sample);

  return SOX_SUCCESS;
}

#define READ_SAMPLES_FUNC(type, size, sign, ctype, uctype, cast) \
  static size_t sox_read_ ## sign ## type ## _samples( \
      sox_format_t * ft, sox_sample_t *buf, size_t len) \
  { \
    size_t n, nread; \
    SOX_SAMPLE_LOCALS; \
    ctype *data = lsx_malloc(sizeof(ctype) * len); \
    LSX_USE_VAR(sox_macro_temp_sample), LSX_USE_VAR(sox_macro_temp_double); \
    nread = lsx_read_ ## type ## _buf(ft, (uctype *)data, len); \
    for (n = 0; n < nread; n++) \
      *buf++ = cast(data[n], ft->clips); \
    free(data); \
    return nread; \
  }

READ_SAMPLES_FUNC(b, 1, u, uint8_t, uint8_t, SOX_UNSIGNED_8BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(b, 1, s, int8_t, uint8_t, SOX_SIGNED_8BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(b, 1, ulaw, uint8_t, uint8_t, SOX_ULAW_BYTE_TO_SAMPLE)
READ_SAMPLES_FUNC(b, 1, alaw, uint8_t, uint8_t, SOX_ALAW_BYTE_TO_SAMPLE)
READ_SAMPLES_FUNC(w, 2, u, uint16_t, uint16_t, SOX_UNSIGNED_16BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(w, 2, s, int16_t, uint16_t, SOX_SIGNED_16BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(3, 3, u, sox_uint24_t, sox_uint24_t, SOX_UNSIGNED_24BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(3, 3, s, sox_int24_t, sox_uint24_t, SOX_SIGNED_24BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(dw, 4, u, uint32_t, uint32_t, SOX_UNSIGNED_32BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(dw, 4, s, int32_t, uint32_t, SOX_SIGNED_32BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(f, sizeof(float), su, float, float, SOX_FLOAT_32BIT_TO_SAMPLE)
READ_SAMPLES_FUNC(df, sizeof(double), su, double, double, SOX_FLOAT_64BIT_TO_SAMPLE)

#define WRITE_SAMPLES_FUNC(type, size, sign, ctype, uctype, cast) \
  static size_t sox_write_ ## sign ## type ## _samples( \
      sox_format_t * ft, sox_sample_t const * buf, size_t len) \
  { \
    SOX_SAMPLE_LOCALS; \
    size_t n, nwritten; \
    ctype *data = lsx_malloc(sizeof(ctype) * len); \
    LSX_USE_VAR(sox_macro_temp_sample), LSX_USE_VAR(sox_macro_temp_double); \
    for (n = 0; n < len; n++) \
      data[n] = cast(buf[n], ft->clips); \
    nwritten = lsx_write_ ## type ## _buf(ft, (uctype *)data, len); \
    free(data); \
    return nwritten; \
  }


WRITE_SAMPLES_FUNC(b, 1, u, uint8_t, uint8_t, SOX_SAMPLE_TO_UNSIGNED_8BIT) 
WRITE_SAMPLES_FUNC(b, 1, s, int8_t, uint8_t, SOX_SAMPLE_TO_SIGNED_8BIT)
WRITE_SAMPLES_FUNC(b, 1, ulaw, uint8_t, uint8_t, SOX_SAMPLE_TO_ULAW_BYTE) 
WRITE_SAMPLES_FUNC(b, 1, alaw, uint8_t, uint8_t, SOX_SAMPLE_TO_ALAW_BYTE)
WRITE_SAMPLES_FUNC(w, 2, u, uint16_t, uint16_t, SOX_SAMPLE_TO_UNSIGNED_16BIT) 
WRITE_SAMPLES_FUNC(w, 2, s, int16_t, uint16_t, SOX_SAMPLE_TO_SIGNED_16BIT)
WRITE_SAMPLES_FUNC(3, 3, u, sox_uint24_t, sox_uint24_t, SOX_SAMPLE_TO_UNSIGNED_24BIT) 
WRITE_SAMPLES_FUNC(3, 3, s, sox_int24_t, sox_uint24_t, SOX_SAMPLE_TO_SIGNED_24BIT)
WRITE_SAMPLES_FUNC(dw, 4, u, uint32_t, uint32_t, SOX_SAMPLE_TO_UNSIGNED_32BIT) 
WRITE_SAMPLES_FUNC(dw, 4, s, int32_t, uint32_t, SOX_SAMPLE_TO_SIGNED_32BIT)
WRITE_SAMPLES_FUNC(f, sizeof(float), su, float, float, SOX_SAMPLE_TO_FLOAT_32BIT) 
WRITE_SAMPLES_FUNC(df, sizeof (double), su, double, double, SOX_SAMPLE_TO_FLOAT_64BIT)

#define GET_FORMAT(type) \
static ft_##type##_fn * type##_fn(sox_format_t * ft) { \
  switch (ft->encoding.bits_per_sample) { \
    case 8: \
      switch (ft->encoding.encoding) { \
        case SOX_ENCODING_SIGN2: return sox_##type##_sb_samples; \
        case SOX_ENCODING_UNSIGNED: return sox_##type##_ub_samples; \
        case SOX_ENCODING_ULAW: return sox_##type##_ulawb_samples; \
        case SOX_ENCODING_ALAW: return sox_##type##_alawb_samples; \
        default: break; } \
      break; \
    case 16: \
      switch (ft->encoding.encoding) { \
        case SOX_ENCODING_SIGN2: return sox_##type##_sw_samples; \
        case SOX_ENCODING_UNSIGNED: return sox_##type##_uw_samples; \
        default: break; } \
      break; \
    case 24: \
      switch (ft->encoding.encoding) { \
        case SOX_ENCODING_SIGN2:    return sox_##type##_s3_samples; \
        case SOX_ENCODING_UNSIGNED: return sox_##type##_u3_samples; \
        default: break; } \
      break; \
    case 32: \
      switch (ft->encoding.encoding) { \
        case SOX_ENCODING_SIGN2: return sox_##type##_sdw_samples; \
        case SOX_ENCODING_UNSIGNED: return sox_##type##_udw_samples; \
        case SOX_ENCODING_FLOAT: return sox_##type##_suf_samples; \
        default: break; } \
      break; \
    case 64: \
      switch (ft->encoding.encoding) { \
        case SOX_ENCODING_FLOAT: return sox_##type##_sudf_samples; \
        default: break; } \
      break; \
    default: \
      lsx_fail_errno(ft, SOX_EFMT, "this handler does not support this data size"); \
      return NULL; } \
  lsx_fail_errno(ft, SOX_EFMT, "this encoding is not supported for this data size"); \
  return NULL; }

typedef size_t(ft_read_fn)
  (sox_format_t * ft, sox_sample_t * buf, size_t len);

GET_FORMAT(read)

/* Read a stream of some type into SoX's internal buffer format. */
size_t lsx_rawread(sox_format_t * ft, sox_sample_t * buf, size_t nsamp)
{
  ft_read_fn * read_buf = read_fn(ft);

  if (read_buf && nsamp)
    return read_buf(ft, buf, nsamp);
  return 0;
}

typedef size_t(ft_write_fn)
  (sox_format_t * ft, sox_sample_t const * buf, size_t len);

GET_FORMAT(write)

/* Writes SoX's internal buffer format to buffer of various data types. */
size_t lsx_rawwrite(
    sox_format_t * ft, sox_sample_t const * buf, size_t nsamp)
{
  ft_write_fn * write_buf = write_fn(ft);

  if (write_buf && nsamp)
    return write_buf(ft, buf, nsamp);
  return 0;
}
