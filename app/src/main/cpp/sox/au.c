/* libSoX Sun format with header (SunOS 4.1; see /usr/demo/SOUND).
 * Copyright 1991, 1992, 1993 Guido van Rossum And Sundry Contributors.
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Guido van Rossum And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * October 7, 1998 - cbagwell@sprynet.com
 *   G.723 was using incorrect # of bits.  Corrected to 3 and 5 bits.
 *
 * NeXT uses this format also, but has more format codes defined.
 * DEC uses a slight variation and swaps bytes.
 * We support only the common formats, plus
 * CCITT G.721 (32 kbit/s) and G.723 (24/40 kbit/s),
 * courtesy of Sun's public domain implementation.
 */

#include "sox_i.h"
#include "g72x.h"
#include <string.h>

/* Magic numbers used in Sun and NeXT audio files */
static struct {char str[4]; sox_bool reverse_bytes; char const * desc;} id[] = {
  {"\x2e\x73\x6e\x64", MACHINE_IS_LITTLEENDIAN, "big-endian `.snd'"},
  {"\x64\x6e\x73\x2e", MACHINE_IS_BIGENDIAN   , "little-endian `.snd'"},
  {"\x00\x64\x73\x2e", MACHINE_IS_BIGENDIAN   , "little-endian `\\0ds.' (for DEC)"},
  {"\x2e\x73\x64\x00", MACHINE_IS_LITTLEENDIAN, "big-endian `\\0ds.'"},
  {"    ", 0, NULL}
};
#define FIXED_HDR     24
#define SUN_UNSPEC    ~0u         /* Unspecified data size (this is legal) */

typedef enum {
  Unspecified, Mulaw_8, Linear_8, Linear_16, Linear_24, Linear_32, Float,
  Double, Indirect, Nested, Dsp_core, Dsp_data_8, Dsp_data_16, Dsp_data_24,
  Dsp_data_32, Unknown, Display, Mulaw_squelch, Emphasized, Compressed,
  Compressed_emphasized, Dsp_commands, Dsp_commands_samples, Adpcm_g721,
  Adpcm_g722, Adpcm_g723_3, Adpcm_g723_5, Alaw_8, Unknown_other} ft_encoding_t;
static char const * const str[] = {
  "Unspecified", "8-bit mu-law", "8-bit signed linear", "16-bit signed linear",
  "24-bit signed linear", "32-bit signed linear", "Floating-point",
  "Double precision float", "Fragmented sampled data", "Unknown", "DSP program",
  "8-bit fixed-point", "16-bit fixed-point", "24-bit fixed-point",
  "32-bit fixed-point", "Unknown", "Non-audio data", "Mu-law squelch",
  "16-bit linear with emphasis", "16-bit linear with compression",
  "16-bit linear with emphasis and compression", "Music Kit DSP commands",
  "Music Kit DSP samples", "4-bit G.721 ADPCM", "G.722 ADPCM",
  "3-bit G.723 ADPCM", "5-bit G.723 ADPCM", "8-bit a-law", "Unknown"};

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
    case Adpcm_g721  : *size =  4; return SOX_ENCODING_G721; /* read-only */
    case Adpcm_g723_3: *size =  3; return SOX_ENCODING_G723; /* read-only */
    case Adpcm_g723_5: *size =  5; return SOX_ENCODING_G723; /* read-only */
    default:                       return SOX_ENCODING_UNKNOWN;
  }
}

typedef struct {        /* For G72x decoding: */
  struct g72x_state state;
  int (*dec_routine)(int i, int out_coding, struct g72x_state *state_ptr);
  unsigned int in_buffer;
  int in_bits;
} priv_t;

/*
 * Unpack input codes and pass them back as bytes.
 * Returns 1 if there is residual input, returns -1 if eof, else returns 0.
 * (Adapted from Sun's decode.c.)
 */
static int unpack_input(sox_format_t * ft, unsigned char *code)
{
  priv_t * p = (priv_t *) ft->priv;
  unsigned char           in_byte;

  if (p->in_bits < (int)ft->encoding.bits_per_sample) {
    if (lsx_read_b_buf(ft, &in_byte, (size_t) 1) != 1) {
      *code = 0;
      return -1;
    }
    p->in_buffer |= (in_byte << p->in_bits);
    p->in_bits += 8;
  }
  *code = p->in_buffer & ((1 << ft->encoding.bits_per_sample) - 1);
  p->in_buffer >>= ft->encoding.bits_per_sample;
  p->in_bits -= ft->encoding.bits_per_sample;
  return p->in_bits > 0;
}

static size_t dec_read(sox_format_t *ft, sox_sample_t *buf, size_t samp)
{
  priv_t * p = (priv_t *)ft->priv;
  unsigned char code;
  size_t done;

  for (done = 0; samp > 0 && unpack_input(ft, &code) >= 0; ++done, --samp)
    *buf++ = SOX_SIGNED_16BIT_TO_SAMPLE(
        (*p->dec_routine)(code, AUDIO_ENCODING_LINEAR, &p->state),);
  return done;
}

static int startread(sox_format_t * ft)
{
  priv_t * p = (priv_t *) ft->priv;
  char     magic[4];     /* These 6 variables represent a Sun sound */
  uint32_t hdr_size;     /* header on disk.  The uint32_t are written as */
  uint32_t data_size;    /* big-endians.  At least extra bytes (totalling */
  uint32_t ft_encoding;  /* hdr_size - FIXED_HDR) are an "info" field of */
  uint32_t rate;         /* unspecified nature, usually a string.  By */
  uint32_t channels;     /* convention the header size is a multiple of 4. */
  unsigned i, bits_per_sample;
  sox_encoding_t encoding;

  if (lsx_readchars(ft, magic, sizeof(magic)))
    return SOX_EOF;

  for (i = 0; id[i].desc && memcmp(magic, id[i].str, sizeof(magic)); ++i);
  if (!id[i].desc) {
    lsx_fail_errno(ft, SOX_EHDR, "au: can't find Sun/NeXT/DEC identifier");
    return SOX_EOF;
  }
  lsx_report("found %s identifier", id[i].desc);
  ft->encoding.reverse_bytes = id[i].reverse_bytes;

  if (lsx_readdw(ft, &hdr_size) ||
      lsx_readdw(ft, &data_size) ||   /* Can be SUN_UNSPEC */
      lsx_readdw(ft, &ft_encoding) ||
      lsx_readdw(ft, &rate) ||
      lsx_readdw(ft, &channels))
    return SOX_EOF;

  if (hdr_size < FIXED_HDR) {
    lsx_fail_errno(ft, SOX_EHDR, "header size %u is too small", hdr_size);
    return SOX_EOF;
  }
  if (hdr_size < FIXED_HDR + 4)
    lsx_warn("header size %u is too small", hdr_size);

  if (!(encoding = sox_enc(ft_encoding, &bits_per_sample))) {
    int n = min(ft_encoding, Unknown_other);
    lsx_fail_errno(ft, SOX_EFMT, "unsupported encoding `%s' (%#x)", str[n], ft_encoding);
    return SOX_EOF;
  }

  switch (ft_encoding) {
    case Adpcm_g721  : p->dec_routine = g721_decoder   ; break;
    case Adpcm_g723_3: p->dec_routine = g723_24_decoder; break;
    case Adpcm_g723_5: p->dec_routine = g723_40_decoder; break;
  }
  if (p->dec_routine) {
    g72x_init_state(&p->state);
    ft->handler.seek = NULL;
    ft->handler.read = dec_read;
  }

  if (hdr_size > FIXED_HDR) {
    size_t info_size = hdr_size - FIXED_HDR;
    char * buf = lsx_calloc(1, info_size + 1); /* +1 ensures null-terminated */
    if (lsx_readchars(ft, buf, info_size) != SOX_SUCCESS) {
      free(buf);
      return SOX_EOF;
    }
    sox_append_comments(&ft->oob.comments, buf);
    free(buf);
  }
  if (data_size == SUN_UNSPEC)
    data_size = SOX_UNSPEC;
  return lsx_check_read_params(ft, channels, (sox_rate_t)rate, encoding,
      bits_per_sample, div_bits(data_size, bits_per_sample), sox_true);
}

static int write_header(sox_format_t * ft)
{
  char * comment  = lsx_cat_comments(ft->oob.comments);
  size_t len      = strlen(comment) + 1;     /* Write out null-terminated */
  size_t info_len = max(4, (len + 3) & ~3u); /* Minimum & multiple of 4 bytes */
  int i = ft->encoding.reverse_bytes == MACHINE_IS_BIGENDIAN? 2 : 0;
  uint64_t size64 = ft->olength ? ft->olength : ft->signal.length;
  unsigned size = size64 == SOX_UNSPEC
      ? SUN_UNSPEC
      : size64*(ft->encoding.bits_per_sample >> 3) > UINT_MAX
      ? SUN_UNSPEC
      : (unsigned)(size64*(ft->encoding.bits_per_sample >> 3));
  sox_bool error = sox_false
  ||lsx_writechars(ft, id[i].str, sizeof(id[i].str))
  ||lsx_writedw(ft, FIXED_HDR + (unsigned)info_len)
  ||lsx_writedw(ft, size)
  ||lsx_writedw(ft, ft_enc(ft->encoding.bits_per_sample, ft->encoding.encoding))
  ||lsx_writedw(ft, (unsigned)(ft->signal.rate + .5))
  ||lsx_writedw(ft, ft->signal.channels)
  ||lsx_writechars(ft, comment, len)
  ||lsx_padbytes(ft, info_len - len);
  free(comment);
  return error? SOX_EOF: SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(au)
{
  static char const * const names[] = {"au", "snd", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_SIGN2, 8, 16, 24, 32, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "PCM file format used widely on Sun systems",
    names, SOX_FILE_BIG_END | SOX_FILE_REWIND,
    startread, lsx_rawread, NULL,
    write_header, lsx_rawwrite, NULL,
    lsx_rawseek, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
