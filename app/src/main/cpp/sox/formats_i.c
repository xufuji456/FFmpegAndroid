/* Implements a libSoX internal interface for use in implementing file formats.
 * All public functions & data are prefixed with lsx_ .
 *
 * (c) 2005-8 Chris Bagwell and SoX contributors
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
#include <sys/stat.h>
#include <stdarg.h>

void lsx_fail_errno(sox_format_t * ft, int sox_errno, const char *fmt, ...)
{
  va_list args;

  ft->sox_errno = sox_errno;

  va_start(args, fmt);
#ifdef HAVE_VSNPRINTF
  vsnprintf(ft->sox_errstr, sizeof(ft->sox_errstr), fmt, args);
#else
  vsprintf(ft->sox_errstr, fmt, args);
#endif
  va_end(args);
  ft->sox_errstr[255] = '\0';
}

void lsx_set_signal_defaults(sox_format_t * ft)
{
  if (!ft->signal.rate     ) ft->signal.rate      = SOX_DEFAULT_RATE;
  if (!ft->signal.precision) ft->signal.precision = SOX_DEFAULT_PRECISION;
  if (!ft->signal.channels ) ft->signal.channels  = SOX_DEFAULT_CHANNELS;

  if (!ft->encoding.bits_per_sample)
    ft->encoding.bits_per_sample = ft->signal.precision;
  if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
    ft->encoding.encoding = SOX_ENCODING_SIGN2;
}

int lsx_check_read_params(sox_format_t * ft, unsigned channels,
    sox_rate_t rate, sox_encoding_t encoding, unsigned bits_per_sample,
    uint64_t num_samples, sox_bool check_length)
{
  ft->signal.length = ft->signal.length == SOX_IGNORE_LENGTH? SOX_UNSPEC : num_samples;

  if (ft->seekable)
    ft->data_start = lsx_tell(ft);

  if (channels && ft->signal.channels && ft->signal.channels != channels)
    lsx_warn("`%s': overriding number of channels", ft->filename);
  else ft->signal.channels = channels;

  if (rate && ft->signal.rate && ft->signal.rate != rate)
    lsx_warn("`%s': overriding sample rate", ft->filename);
  else ft->signal.rate = rate;

  if (encoding && ft->encoding.encoding && ft->encoding.encoding != encoding)
    lsx_warn("`%s': overriding encoding type", ft->filename);
  else ft->encoding.encoding = encoding;

  if (bits_per_sample && ft->encoding.bits_per_sample && ft->encoding.bits_per_sample != bits_per_sample)
    lsx_warn("`%s': overriding encoding size", ft->filename);
  ft->encoding.bits_per_sample = bits_per_sample;

  if (check_length && ft->encoding.bits_per_sample && lsx_filelength(ft)) {
    uint64_t calculated_length = div_bits(lsx_filelength(ft) - ft->data_start, ft->encoding.bits_per_sample);
    if (!ft->signal.length)
      ft->signal.length = calculated_length;
    else if (num_samples != calculated_length)
      lsx_warn("`%s': file header gives the total number of samples as %" PRIu64 " but file length indicates the number is in fact %" PRIu64, ft->filename, num_samples, calculated_length);
  }

  if (sox_precision(ft->encoding.encoding, ft->encoding.bits_per_sample))
    return SOX_SUCCESS;
  lsx_fail_errno(ft, EINVAL, "invalid format for this file type");
  return SOX_EOF;
}

/* Read in a buffer of data of length len bytes.
 * Returns number of bytes read.
 */
size_t lsx_readbuf(sox_format_t * ft, void *buf, size_t len)
{
  size_t ret = fread(buf, (size_t) 1, len, (FILE*)ft->fp);
  if (ret != len && ferror((FILE*)ft->fp))
    lsx_fail_errno(ft, errno, "lsx_readbuf");
  ft->tell_off += ret;
  return ret;
}

/* Skip input without seeking. */
int lsx_skipbytes(sox_format_t * ft, size_t n)
{
  unsigned char trash;

  while (n--)
    if (lsx_readb(ft, &trash) == SOX_EOF)
      return (SOX_EOF);

  return (SOX_SUCCESS);
}

/* Pad output. */
int lsx_padbytes(sox_format_t * ft, size_t n)
{
  while (n--)
    if (lsx_writeb(ft, '\0') == SOX_EOF)
      return (SOX_EOF);

  return (SOX_SUCCESS);
}

/* Write a buffer of data of length bytes.
 * Returns number of bytes written.
 */
size_t lsx_writebuf(sox_format_t * ft, void const * buf, size_t len)
{
  size_t ret = fwrite(buf, (size_t) 1, len, (FILE*)ft->fp);
  if (ret != len) {
    lsx_fail_errno(ft, errno, "error writing output file");
    clearerr((FILE*)ft->fp); /* Allows us to seek back to write header */
  }
  ft->tell_off += ret;
  return ret;
}

sox_uint64_t lsx_filelength(sox_format_t * ft)
{
  struct stat st;
  int ret = ft->fp ? fstat(fileno((FILE*)ft->fp), &st) : 0;

  return (!ret && (st.st_mode & S_IFREG))? (uint64_t)st.st_size : 0;
}

int lsx_flush(sox_format_t * ft)
{
  return fflush((FILE*)ft->fp);
}

off_t lsx_tell(sox_format_t * ft)
{
  return ft->seekable? (off_t)ftello((FILE*)ft->fp) : (off_t)ft->tell_off;
}

int lsx_eof(sox_format_t * ft)
{
  return feof((FILE*)ft->fp);
}

int lsx_error(sox_format_t * ft)
{
  return ferror((FILE*)ft->fp);
}

void lsx_rewind(sox_format_t * ft)
{
  rewind((FILE*)ft->fp);
  ft->tell_off = 0;
}

void lsx_clearerr(sox_format_t * ft)
{
  clearerr((FILE*)ft->fp);
  ft->sox_errno = 0;
}

int lsx_unreadb(sox_format_t * ft, unsigned b)
{
  return ungetc((int)b, ft->fp);
}

/* Implements traditional fseek() behavior.  Meant to abstract out
 * file operations so that they could one day also work on memory
 * buffers.
 *
 * N.B. Can only seek forwards on non-seekable streams!
 */
int lsx_seeki(sox_format_t * ft, off_t offset, int whence)
{
    if (ft->seekable == 0) {
        /* If a stream peel off chars else EPERM */
        if (whence == SEEK_CUR) {
            while (offset > 0 && !feof((FILE*)ft->fp)) {
                getc((FILE*)ft->fp);
                offset--;
                ++ft->tell_off;
            }
            if (offset)
                lsx_fail_errno(ft,SOX_EOF, "offset past EOF");
            else
                ft->sox_errno = SOX_SUCCESS;
        } else
            lsx_fail_errno(ft,SOX_EPERM, "file not seekable");
    } else {
        if (fseeko((FILE*)ft->fp, offset, whence) == -1)
            lsx_fail_errno(ft,errno, "%s", strerror(errno));
        else
            ft->sox_errno = SOX_SUCCESS;
    }
    return ft->sox_errno;
}

int lsx_offset_seek(sox_format_t * ft, off_t byte_offset, off_t to_sample)
{
  double wide_sample = to_sample - (to_sample % ft->signal.channels);
  double to_d = wide_sample * ft->encoding.bits_per_sample / 8;
  off_t to = to_d;
  return (to != to_d)? SOX_EOF : lsx_seeki(ft, (byte_offset + to), SEEK_SET);
}

/* Read and write known datatypes in "machine format".  Swap if indicated.
 * They all return SOX_EOF on error and SOX_SUCCESS on success.
 */
/* Read n-char string (and possibly null-terminating).
 * Stop reading and null-terminate string if either a 0 or \n is reached.
 */
int lsx_reads(sox_format_t * ft, char *c, size_t len)
{
    char *sc;
    char in;

    sc = c;
    do
    {
        if (lsx_readbuf(ft, &in, (size_t)1) != 1)
        {
            *sc = 0;
            return (SOX_EOF);
        }
        if (in == 0 || in == '\n')
            break;

        *sc = in;
        sc++;
    } while (sc - c < (ptrdiff_t)len);
    *sc = 0;
    return(SOX_SUCCESS);
}

/* Write null-terminated string (without \0). */
int lsx_writes(sox_format_t * ft, char const * c)
{
        if (lsx_writebuf(ft, c, strlen(c)) != strlen(c))
                return(SOX_EOF);
        return(SOX_SUCCESS);
}

/* return swapped 32-bit float */
static void lsx_swapf(float * f)
{
    union {
        uint32_t dw;
        float f;
    } u;

    u.f= *f;
    u.dw= (u.dw>>24) | ((u.dw>>8)&0xff00) | ((u.dw<<8)&0xff0000) | (u.dw<<24);
    *f = u.f;
}

static void swap(void * data, size_t len)
{
  uint8_t * bytes = (uint8_t *)data;
  size_t i;

  for (i = 0; i < len / 2; ++i) {
    char tmp = bytes[i];
    bytes[i] = bytes[len - 1 - i];
    bytes[len - 1 - i] = tmp;
  }
}

static double lsx_swapdf(double data)
{
  swap(&data, sizeof(data));
  return data;
}

static uint64_t lsx_swapqw(uint64_t data)
{
  swap(&data, sizeof(data));
  return data;
}

/* Lookup table to reverse the bit order of a byte. ie MSB become LSB */
static uint8_t const cswap[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
  0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
  0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
  0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
  0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
  0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
  0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
  0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
  0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
  0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
  0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
  0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
  0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
  0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
  0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
  0x3F, 0xBF, 0x7F, 0xFF
};

/* Utilities to byte-swap values, use libc optimized macros if possible  */
#define TWIDDLE_BYTE(ub, type) \
  do { \
    if (ft->encoding.reverse_bits) \
      ub = cswap[ub]; \
    if (ft->encoding.reverse_nibbles) \
      ub = ((ub & 15) << 4) | (ub >> 4); \
  } while (0);

#define TWIDDLE_WORD(uw, type) \
  if (ft->encoding.reverse_bytes) \
    uw = lsx_swap ## type(uw);

#define TWIDDLE_FLOAT(f, type) \
  if (ft->encoding.reverse_bytes) \
    lsx_swapf(&f);

/* N.B. This macro doesn't work for unaligned types (e.g. 3-byte
   types). */
#define READ_FUNC(type, size, ctype, twiddle) \
  size_t lsx_read_ ## type ## _buf( \
      sox_format_t * ft, ctype *buf, size_t len) \
  { \
    size_t n, nread; \
    nread = lsx_readbuf(ft, buf, len * size) / size; \
    for (n = 0; n < nread; n++) \
      twiddle(buf[n], type); \
    return nread; \
  }

/* Unpack a 3-byte value from a uint8_t * */
#define sox_unpack3(p) (ft->encoding.reverse_bytes == MACHINE_IS_BIGENDIAN? \
  ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16)) : \
  ((p)[2] | ((p)[1] << 8) | ((p)[0] << 16)))

/* This (slower) macro works for unaligned types (e.g. 3-byte types)
   that need to be unpacked. */
#define READ_FUNC_UNPACK(type, size, ctype, twiddle) \
  size_t lsx_read_ ## type ## _buf( \
      sox_format_t * ft, ctype *buf, size_t len) \
  { \
    size_t n, nread; \
    uint8_t *data = lsx_malloc(size * len); \
    nread = lsx_readbuf(ft, data, len * size) / size; \
    for (n = 0; n < nread; n++) \
      buf[n] = sox_unpack ## size(data + n * size); \
    free(data); \
    return n; \
  }

READ_FUNC(b, 1, uint8_t, TWIDDLE_BYTE)
READ_FUNC(w, 2, uint16_t, TWIDDLE_WORD)
READ_FUNC_UNPACK(3, 3, sox_uint24_t, TWIDDLE_WORD)
READ_FUNC(dw, 4, uint32_t, TWIDDLE_WORD)
READ_FUNC(qw, 8, uint64_t, TWIDDLE_WORD)
READ_FUNC(f, sizeof(float), float, TWIDDLE_FLOAT)
READ_FUNC(df, sizeof(double), double, TWIDDLE_WORD)

#define READ1_FUNC(type, ctype) \
int lsx_read ## type(sox_format_t * ft, ctype * datum) { \
  if (lsx_read_ ## type ## _buf(ft, datum, (size_t)1) == 1) \
    return SOX_SUCCESS; \
  if (!lsx_error(ft)) \
    lsx_fail_errno(ft, errno, premature_eof); \
  return SOX_EOF; \
}

static char const premature_eof[] = "premature EOF";

READ1_FUNC(b,  uint8_t)
READ1_FUNC(w,  uint16_t)
READ1_FUNC(3,  sox_uint24_t)
READ1_FUNC(dw, uint32_t)
READ1_FUNC(qw, uint64_t)
READ1_FUNC(f,  float)
READ1_FUNC(df, double)

int lsx_readchars(sox_format_t * ft, char * chars, size_t len)
{
  size_t ret = lsx_readbuf(ft, chars, len);
  if (ret == len)
    return SOX_SUCCESS;
  if (!lsx_error(ft))
    lsx_fail_errno(ft, errno, premature_eof);
  return SOX_EOF;
}

int lsx_read_fields(sox_format_t *ft, uint32_t *len,  const char *spec, ...)
{
    int err = SOX_SUCCESS;
    va_list ap;

#define do_read(type, f, n) do {                                \
        size_t nr;                                              \
        if (*len < n * sizeof(type)) {                          \
            err = SOX_EOF;                                      \
            goto end;                                           \
        }                                                       \
        nr = lsx_read_##f##_buf(ft, va_arg(ap, type *), r);     \
        if (nr != n)                                            \
            err = SOX_EOF;                                      \
        *len -= nr * sizeof(type);                              \
    } while (0)

    va_start(ap, spec);

    while (*spec) {
        unsigned long r = 1;
        char c = *spec;

        if (c >= '0' && c <= '9') {
            char *next;
            r = strtoul(spec, &next, 10);
            spec = next;
            c = *spec;
        } else if (c == '*') {
            r = va_arg(ap, int);
            c = *++spec;
        }

        switch (c) {
        case 'b': do_read(uint8_t, b, r);       break;
        case 'h': do_read(uint16_t, w, r);      break;
        case 'i': do_read(uint32_t, dw, r);     break;
        case 'q': do_read(uint64_t, qw, r);     break;
        case 'x': err = lsx_skipbytes(ft, r);   break;

        default:
            lsx_fail("lsx_read_fields: invalid format character '%c'", c);
            err = SOX_EOF;
            break;
        }

        if (err)
            break;

        spec++;
    }

end:
    va_end(ap);

#undef do_read

    return err;
}

/* N.B. This macro doesn't work for unaligned types (e.g. 3-byte
   types). */
#define WRITE_FUNC(type, size, ctype, twiddle) \
  size_t lsx_write_ ## type ## _buf( \
      sox_format_t * ft, ctype *buf, size_t len) \
  { \
    size_t n, nwritten; \
    for (n = 0; n < len; n++) \
      twiddle(buf[n], type); \
    nwritten = lsx_writebuf(ft, buf, len * size); \
    return nwritten / size; \
  }

/* Pack a 3-byte value to a uint8_t * */
#define sox_pack3(p, v) do {if (ft->encoding.reverse_bytes == MACHINE_IS_BIGENDIAN)\
{(p)[0] = v & 0xff; (p)[1] = (v >> 8) & 0xff; (p)[2] = (v >> 16) & 0xff;} else \
{(p)[2] = v & 0xff; (p)[1] = (v >> 8) & 0xff; (p)[0] = (v >> 16) & 0xff;} \
} while (0)

/* This (slower) macro works for unaligned types (e.g. 3-byte types)
   that need to be packed. */
#define WRITE_FUNC_PACK(type, size, ctype, twiddle) \
  size_t lsx_write_ ## type ## _buf( \
      sox_format_t * ft, ctype *buf, size_t len) \
  { \
    size_t n, nwritten; \
    uint8_t *data = lsx_malloc(size * len); \
    for (n = 0; n < len; n++) \
      sox_pack ## size(data + n * size, buf[n]); \
    nwritten = lsx_writebuf(ft, data, len * size); \
    free(data); \
    return nwritten / size; \
  }

WRITE_FUNC(b, 1, uint8_t, TWIDDLE_BYTE)
WRITE_FUNC(w, 2, uint16_t, TWIDDLE_WORD)
WRITE_FUNC_PACK(3, 3, sox_uint24_t, TWIDDLE_WORD)
WRITE_FUNC(dw, 4, uint32_t, TWIDDLE_WORD)
WRITE_FUNC(qw, 8, uint64_t, TWIDDLE_WORD)
WRITE_FUNC(f, sizeof(float), float, TWIDDLE_FLOAT)
WRITE_FUNC(df, sizeof(double), double, TWIDDLE_WORD)

#define WRITE1U_FUNC(type, ctype) \
  int lsx_write ## type(sox_format_t * ft, unsigned d) \
  { ctype datum = (ctype)d; \
    return lsx_write_ ## type ## _buf(ft, &datum, (size_t)1) == 1 ? SOX_SUCCESS : SOX_EOF; \
  }

#define WRITE1S_FUNC(type, ctype) \
  int lsx_writes ## type(sox_format_t * ft, signed d) \
  { ctype datum = (ctype)d; \
    return lsx_write_ ## type ## _buf(ft, &datum, (size_t)1) == 1 ? SOX_SUCCESS : SOX_EOF; \
  }

#define WRITE1_FUNC(type, ctype) \
  int lsx_write ## type(sox_format_t * ft, ctype datum) \
  { \
    return lsx_write_ ## type ## _buf(ft, &datum, (size_t)1) == 1 ? SOX_SUCCESS : SOX_EOF; \
  }

WRITE1U_FUNC(b, uint8_t)
WRITE1U_FUNC(w, uint16_t)
WRITE1U_FUNC(3, sox_uint24_t)
WRITE1U_FUNC(dw, uint32_t)
WRITE1_FUNC(qw, uint64_t)
WRITE1S_FUNC(b, uint8_t)
WRITE1S_FUNC(w, uint16_t)
WRITE1_FUNC(df, double)

int lsx_writef(sox_format_t * ft, double datum)
{
  float f = datum;
  return lsx_write_f_buf(ft, &f, (size_t) 1) == 1 ? SOX_SUCCESS : SOX_EOF;
}
