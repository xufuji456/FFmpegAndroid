/* libSoX libsndfile formats.
 *
 * Copyright 2007 Reuben Thomas <rrt@sc3d.org>
 * Copyright 1999-2005 Erik de Castro Lopo <eridk@mega-nerd.com>
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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sndfile.h>

#define LOG_MAX 2048 /* As per the SFC_GET_LOG_INFO example */

static const char* const sndfile_library_names[] =
{
#ifdef DL_LIBSNDFILE
  "libsndfile",
  "libsndfile-1",
  "cygsndfile-1",
#endif
  NULL
};

#ifdef DL_LIBSNDFILE
  #define SNDFILE_FUNC      LSX_DLENTRY_DYNAMIC
  #define SNDFILE_FUNC_STOP LSX_DLENTRY_STUB
#else
  #define SNDFILE_FUNC      LSX_DLENTRY_STATIC
#ifdef HACKED_LSF
  #define SNDFILE_FUNC_STOP LSX_DLENTRY_STATIC
#else
  #define SNDFILE_FUNC_STOP LSX_DLENTRY_STUB
#endif
#endif /* DL_LIBSNDFILE */

#define SNDFILE_FUNC_OPEN(f,x) \
  SNDFILE_FUNC(f,x, SNDFILE*, sf_open_virtual, (SF_VIRTUAL_IO *sfvirtual, int mode, SF_INFO *sfinfo, void *user_data))

#define SNDFILE_FUNC_ENTRIES(f,x) \
  SNDFILE_FUNC_OPEN(f,x) \
  SNDFILE_FUNC_STOP(f,x, int, sf_stop, (SNDFILE *sndfile)) \
  SNDFILE_FUNC(f,x, int, sf_close, (SNDFILE *sndfile)) \
  SNDFILE_FUNC(f,x, int, sf_format_check, (const SF_INFO *info)) \
  SNDFILE_FUNC(f,x, int, sf_command, (SNDFILE *sndfile, int command, void *data, int datasize)) \
  SNDFILE_FUNC(f,x, sf_count_t, sf_read_int, (SNDFILE *sndfile, int *ptr, sf_count_t items)) \
  SNDFILE_FUNC(f,x, sf_count_t, sf_write_int, (SNDFILE *sndfile, const int *ptr, sf_count_t items)) \
  SNDFILE_FUNC(f,x, sf_count_t, sf_seek, (SNDFILE *sndfile, sf_count_t frames, int whence)) \
  SNDFILE_FUNC(f,x, const char*, sf_strerror, (SNDFILE *sndfile))

/* Private data for sndfile files */
typedef struct {
  SNDFILE *sf_file;
  SF_INFO *sf_info;
  char * log_buffer;
  char const * log_buffer_ptr;
  LSX_DLENTRIES_TO_PTRS(SNDFILE_FUNC_ENTRIES, sndfile_dl);
} priv_t;

/*
 * Drain LSF's wonderful log buffer
 */
static void drain_log_buffer(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;
  sf->sf_command(sf->sf_file, SFC_GET_LOG_INFO, sf->log_buffer, LOG_MAX);
  while (*sf->log_buffer_ptr) {
    static char const warning_prefix[] = "*** Warning : ";
    char const * end = strchr(sf->log_buffer_ptr, '\n');
    if (!end)
      end = strchr(sf->log_buffer_ptr, '\0');
    if (!strncmp(sf->log_buffer_ptr, warning_prefix, strlen(warning_prefix))) {
      sf->log_buffer_ptr += strlen(warning_prefix);
      lsx_warn("`%s': %.*s",
          ft->filename, (int)(end - sf->log_buffer_ptr), sf->log_buffer_ptr);
    } else
      lsx_debug("`%s': %.*s",
          ft->filename, (int)(end - sf->log_buffer_ptr), sf->log_buffer_ptr);
    sf->log_buffer_ptr = end;
    if (*sf->log_buffer_ptr == '\n')
      ++sf->log_buffer_ptr;
  }
}

/* Make libsndfile subtype from sample encoding and size */
static int ft_enc(unsigned size, sox_encoding_t e)
{
  if (e == SOX_ENCODING_ULAW      && size ==  8) return SF_FORMAT_ULAW;
  if (e == SOX_ENCODING_ALAW      && size ==  8) return SF_FORMAT_ALAW;
  if (e == SOX_ENCODING_SIGN2     && size ==  8) return SF_FORMAT_PCM_S8;
  if (e == SOX_ENCODING_SIGN2     && size == 16) return SF_FORMAT_PCM_16;
  if (e == SOX_ENCODING_SIGN2     && size == 24) return SF_FORMAT_PCM_24;
  if (e == SOX_ENCODING_SIGN2     && size == 32) return SF_FORMAT_PCM_32;
  if (e == SOX_ENCODING_UNSIGNED  && size ==  8) return SF_FORMAT_PCM_U8;
  if (e == SOX_ENCODING_FLOAT     && size == 32) return SF_FORMAT_FLOAT;
  if (e == SOX_ENCODING_FLOAT     && size == 64) return SF_FORMAT_DOUBLE;
  if (e == SOX_ENCODING_G721      && size ==  4) return SF_FORMAT_G721_32;
  if (e == SOX_ENCODING_G723      && size ==  3) return SF_FORMAT_G723_24;
  if (e == SOX_ENCODING_G723      && size ==  5) return SF_FORMAT_G723_40;
  if (e == SOX_ENCODING_MS_ADPCM  && size ==  4) return SF_FORMAT_MS_ADPCM;
  if (e == SOX_ENCODING_IMA_ADPCM && size ==  4) return SF_FORMAT_IMA_ADPCM;
  if (e == SOX_ENCODING_OKI_ADPCM && size ==  4) return SF_FORMAT_VOX_ADPCM;
  if (e == SOX_ENCODING_DPCM      && size ==  8) return SF_FORMAT_DPCM_8;
  if (e == SOX_ENCODING_DPCM      && size == 16) return SF_FORMAT_DPCM_16;
  if (e == SOX_ENCODING_DWVW      && size == 12) return SF_FORMAT_DWVW_12;
  if (e == SOX_ENCODING_DWVW      && size == 16) return SF_FORMAT_DWVW_16;
  if (e == SOX_ENCODING_DWVW      && size == 24) return SF_FORMAT_DWVW_24;
  if (e == SOX_ENCODING_DWVWN     && size ==  0) return SF_FORMAT_DWVW_N;
  if (e == SOX_ENCODING_GSM       && size ==  0) return SF_FORMAT_GSM610;
  if (e == SOX_ENCODING_FLAC      && size ==  8) return SF_FORMAT_PCM_S8;
  if (e == SOX_ENCODING_FLAC      && size == 16) return SF_FORMAT_PCM_16;
  if (e == SOX_ENCODING_FLAC      && size == 24) return SF_FORMAT_PCM_24;
  if (e == SOX_ENCODING_FLAC      && size == 32) return SF_FORMAT_PCM_32;
  return 0; /* Bad encoding */
}

/* Convert format's encoding type to libSoX encoding type & size. */
static sox_encoding_t sox_enc(int ft_encoding, unsigned * size)
{
  int sub = ft_encoding & SF_FORMAT_SUBMASK;
  int type = ft_encoding & SF_FORMAT_TYPEMASK;

  if (type == SF_FORMAT_FLAC) switch (sub) {
    case SF_FORMAT_PCM_S8   : *size =  8; return SOX_ENCODING_FLAC;
    case SF_FORMAT_PCM_16   : *size = 16; return SOX_ENCODING_FLAC;
    case SF_FORMAT_PCM_24   : *size = 24; return SOX_ENCODING_FLAC;
  }
  switch (sub) {
    case SF_FORMAT_ULAW     : *size =  8; return SOX_ENCODING_ULAW;
    case SF_FORMAT_ALAW     : *size =  8; return SOX_ENCODING_ALAW;
    case SF_FORMAT_PCM_S8   : *size =  8; return SOX_ENCODING_SIGN2;
    case SF_FORMAT_PCM_16   : *size = 16; return SOX_ENCODING_SIGN2;
    case SF_FORMAT_PCM_24   : *size = 24; return SOX_ENCODING_SIGN2;
    case SF_FORMAT_PCM_32   : *size = 32; return SOX_ENCODING_SIGN2;
    case SF_FORMAT_PCM_U8   : *size =  8; return SOX_ENCODING_UNSIGNED;
    case SF_FORMAT_FLOAT    : *size = 32; return SOX_ENCODING_FLOAT;
    case SF_FORMAT_DOUBLE   : *size = 64; return SOX_ENCODING_FLOAT;
    case SF_FORMAT_G721_32  : *size =  4; return SOX_ENCODING_G721;
    case SF_FORMAT_G723_24  : *size =  3; return SOX_ENCODING_G723;
    case SF_FORMAT_G723_40  : *size =  5; return SOX_ENCODING_G723;
    case SF_FORMAT_MS_ADPCM : *size =  4; return SOX_ENCODING_MS_ADPCM;
    case SF_FORMAT_IMA_ADPCM: *size =  4; return SOX_ENCODING_IMA_ADPCM;
    case SF_FORMAT_VOX_ADPCM: *size =  4; return SOX_ENCODING_OKI_ADPCM;
    case SF_FORMAT_DPCM_8   : *size =  8; return SOX_ENCODING_DPCM;
    case SF_FORMAT_DPCM_16  : *size = 16; return SOX_ENCODING_DPCM;
    case SF_FORMAT_DWVW_12  : *size = 12; return SOX_ENCODING_DWVW;
    case SF_FORMAT_DWVW_16  : *size = 16; return SOX_ENCODING_DWVW;
    case SF_FORMAT_DWVW_24  : *size = 24; return SOX_ENCODING_DWVW;
    case SF_FORMAT_DWVW_N   : *size =  0; return SOX_ENCODING_DWVWN;
    case SF_FORMAT_GSM610   : *size =  0; return SOX_ENCODING_GSM;
    default                 : *size =  0; return SOX_ENCODING_UNKNOWN;
  }
}

static struct {
  const char *ext;
  int format;
} format_map[] =
{
  { "aif",      SF_FORMAT_AIFF },
  { "aiff",     SF_FORMAT_AIFF },
  { "wav",      SF_FORMAT_WAV },
  { "au",       SF_FORMAT_AU },
  { "snd",      SF_FORMAT_AU },
  { "caf",      SF_FORMAT_CAF },
  { "flac",     SF_FORMAT_FLAC },
  { "wve",      SF_FORMAT_WVE },
  { "ogg",      SF_FORMAT_OGG },
  { "svx",      SF_FORMAT_SVX },
  { "8svx",     SF_FORMAT_SVX },
  { "paf",      SF_ENDIAN_BIG | SF_FORMAT_PAF },
  { "fap",      SF_ENDIAN_LITTLE | SF_FORMAT_PAF },
  { "gsm",      SF_FORMAT_RAW | SF_FORMAT_GSM610 },
  { "nist",     SF_FORMAT_NIST },
  { "sph",      SF_FORMAT_NIST },
  { "ircam",    SF_FORMAT_IRCAM },
  { "sf",       SF_FORMAT_IRCAM },
  { "voc",      SF_FORMAT_VOC },
  { "w64",      SF_FORMAT_W64 },
  { "raw",      SF_FORMAT_RAW },
  { "mat4",     SF_FORMAT_MAT4 },
  { "mat5",     SF_FORMAT_MAT5 },
  { "mat",      SF_FORMAT_MAT4 },
  { "pvf",      SF_FORMAT_PVF },
  { "sds",      SF_FORMAT_SDS },
  { "sd2",      SF_FORMAT_SD2 },
  { "vox",      SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM },
  { "xi",       SF_FORMAT_XI }
};

static int sf_stop_stub(SNDFILE *sndfile UNUSED)
{
    return 1;
}

static sf_count_t vio_get_filelen(void *user_data)
{
  sox_format_t *ft = (sox_format_t *)user_data;

  /* lsf excepts unbuffered I/O behavior for get_filelen() so force that */
  lsx_flush(ft);

  return (sf_count_t)lsx_filelength((sox_format_t *)user_data);
}

static sf_count_t vio_seek(sf_count_t offset, int whence, void *user_data)
{
    return lsx_seeki((sox_format_t *)user_data, (off_t)offset, whence);
}

static sf_count_t vio_read(void *ptr, sf_count_t count, void *user_data)
{
    return lsx_readbuf((sox_format_t *)user_data, ptr, (size_t)count);
}

static sf_count_t vio_write(const void *ptr, sf_count_t count, void *user_data)
{
    return lsx_writebuf((sox_format_t *)user_data, ptr, (size_t)count);
}

static sf_count_t vio_tell(void *user_data)
{
    return lsx_tell((sox_format_t *)user_data);
}

static SF_VIRTUAL_IO vio =
{
    vio_get_filelen,
    vio_seek,
    vio_read,
    vio_write,
    vio_tell
};

/* Convert file name or type to libsndfile format */
static int name_to_format(const char *name)
{
  int k;
#define FILE_TYPE_BUFLEN (size_t)15
  char buffer[FILE_TYPE_BUFLEN + 1], *cptr;

  if ((cptr = strrchr(name, '.')) != NULL) {
    strncpy(buffer, cptr + 1, FILE_TYPE_BUFLEN);
    buffer[FILE_TYPE_BUFLEN] = '\0';

    for (k = 0; buffer[k]; k++)
      buffer[k] = tolower((buffer[k]));
  } else {
    strncpy(buffer, name, FILE_TYPE_BUFLEN);
    buffer[FILE_TYPE_BUFLEN] = '\0';
  }

  for (k = 0; k < (int)(sizeof(format_map) / sizeof(format_map [0])); k++) {
    if (strcmp(buffer, format_map[k].ext) == 0)
      return format_map[k].format;
  }

  return 0;
}

static int start(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;
  int subtype = ft_enc(ft->encoding.bits_per_sample? ft->encoding.bits_per_sample : ft->signal.precision, ft->encoding.encoding);
  int open_library_result;

  LSX_DLLIBRARY_OPEN(
      sf,
      sndfile_dl,
      SNDFILE_FUNC_ENTRIES,
      "libsndfile library",
      sndfile_library_names,
      open_library_result);
  if (open_library_result)
    return SOX_EOF;

  sf->log_buffer_ptr = sf->log_buffer = lsx_malloc((size_t)LOG_MAX);
  sf->sf_info = lsx_calloc(1, sizeof(SF_INFO));

  /* Copy format info */
  if (subtype) {
    if (strcmp(ft->filetype, "sndfile") == 0)
      sf->sf_info->format = name_to_format(ft->filename) | subtype;
    else
      sf->sf_info->format = name_to_format(ft->filetype) | subtype;
  }
  sf->sf_info->samplerate = (int)ft->signal.rate;
  sf->sf_info->channels = ft->signal.channels;
  if (ft->signal.channels)
    sf->sf_info->frames = ft->signal.length / ft->signal.channels;

  return SOX_SUCCESS;
}

static int check_read_params(sox_format_t * ft, unsigned channels,
    sox_rate_t rate, sox_encoding_t encoding, unsigned bits_per_sample, uint64_t length)
{
  ft->signal.length = length;

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

  if (sox_precision(ft->encoding.encoding, ft->encoding.bits_per_sample))
    return SOX_SUCCESS;
  lsx_fail_errno(ft, EINVAL, "invalid format for this file type");
  return SOX_EOF;
}

/*
 * Open file in sndfile.
 */
static int startread(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;
  unsigned bits_per_sample;
  sox_encoding_t encoding;
  sox_rate_t rate;

  if (start(ft) == SOX_EOF)
      return SOX_EOF;

  sf->sf_file = sf->sf_open_virtual(&vio, SFM_READ, sf->sf_info, ft);
  drain_log_buffer(ft);

  if (sf->sf_file == NULL) {
    memset(ft->sox_errstr, 0, sizeof(ft->sox_errstr));
    strncpy(ft->sox_errstr, sf->sf_strerror(sf->sf_file), sizeof(ft->sox_errstr)-1);
    free(sf->sf_file);
    return SOX_EOF;
  }

  if (!(encoding = sox_enc(sf->sf_info->format, &bits_per_sample))) {
    lsx_fail_errno(ft, SOX_EFMT, "unsupported sndfile encoding %#x", sf->sf_info->format);
    return SOX_EOF;
  }

  /* Don't believe LSF's rate for raw files */
  if ((sf->sf_info->format & SF_FORMAT_TYPEMASK) == SF_FORMAT_RAW && !ft->signal.rate) {
    lsx_warn("`%s': sample rate not specified; trying 8kHz", ft->filename);
    rate = 8000;
  }
  else rate = sf->sf_info->samplerate;

  if ((sf->sf_info->format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT) {
    sf->sf_command(sf->sf_file, SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);
    sf->sf_command(sf->sf_file, SFC_SET_CLIPPING, NULL, SF_TRUE);
  }

#if 0 /* FIXME */
    sox_append_comments(&ft->oob.comments, buf);
#endif

  return check_read_params(ft, (unsigned)sf->sf_info->channels, rate,
      encoding, bits_per_sample, (uint64_t)(sf->sf_info->frames * sf->sf_info->channels));
}

/*
 * Read up to len samples of type sox_sample_t from file into buf[].
 * Return number of samples read.
 */
static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
  priv_t * sf = (priv_t *)ft->priv;
  /* FIXME: We assume int == sox_sample_t here */
  return (size_t)sf->sf_read_int(sf->sf_file, (int *)buf, (sf_count_t)len);
}

/*
 * Close file for libsndfile (this doesn't close the file handle)
 */
static int stopread(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;
  sf->sf_stop(sf->sf_file);
  drain_log_buffer(ft);
  sf->sf_close(sf->sf_file);
  LSX_DLLIBRARY_CLOSE(sf, sndfile_dl);
  return SOX_SUCCESS;
}

static int startwrite(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;

  if (start(ft) == SOX_EOF)
      return SOX_EOF;

  /* If output format is invalid, try to find a sensible default */
  if (!sf->sf_format_check(sf->sf_info)) {
    SF_FORMAT_INFO format_info;
    int i, count;

    sf->sf_command(sf->sf_file, SFC_GET_SIMPLE_FORMAT_COUNT, &count, (int) sizeof(int));
    for (i = 0; i < count; i++) {
      format_info.format = i;
      sf->sf_command(sf->sf_file, SFC_GET_SIMPLE_FORMAT, &format_info, (int) sizeof(format_info));
      if ((format_info.format & SF_FORMAT_TYPEMASK) == (sf->sf_info->format & SF_FORMAT_TYPEMASK)) {
        sf->sf_info->format = format_info.format;
        /* FIXME: Print out exactly what we chose, needs sndfile ->
           sox encoding conversion functions */
        break;
      }
    }

    if (!sf->sf_format_check(sf->sf_info)) {
      lsx_fail("cannot find a usable output encoding");
      return SOX_EOF;
    }
    if ((sf->sf_info->format & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
      lsx_warn("cannot use desired output encoding, choosing default");
  }

  sf->sf_file = sf->sf_open_virtual(&vio, SFM_WRITE, sf->sf_info, ft);
  drain_log_buffer(ft);

  if (sf->sf_file == NULL) {
    memset(ft->sox_errstr, 0, sizeof(ft->sox_errstr));
    strncpy(ft->sox_errstr, sf->sf_strerror(sf->sf_file), sizeof(ft->sox_errstr)-1);
    free(sf->sf_file);
    return SOX_EOF;
  }

  if ((sf->sf_info->format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT)
    sf->sf_command(sf->sf_file, SFC_SET_SCALE_INT_FLOAT_WRITE, NULL, SF_TRUE);

  return SOX_SUCCESS;
}

/*
 * Write len samples of type sox_sample_t from buf[] to file.
 * Return number of samples written.
 */
static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
  priv_t * sf = (priv_t *)ft->priv;
  /* FIXME: We assume int == sox_sample_t here */
  return (size_t)sf->sf_write_int(sf->sf_file, (int *)buf, (sf_count_t)len);
}

/*
 * Close file for libsndfile (this doesn't close the file handle)
 */
static int stopwrite(sox_format_t * ft)
{
  priv_t * sf = (priv_t *)ft->priv;
  sf->sf_stop(sf->sf_file);
  drain_log_buffer(ft);
  sf->sf_close(sf->sf_file);
  LSX_DLLIBRARY_CLOSE(sf, sndfile_dl);
  return SOX_SUCCESS;
}

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * sf = (priv_t *)ft->priv;
  sf->sf_seek(sf->sf_file, (sf_count_t)(offset / ft->signal.channels), SEEK_CUR);
  return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(sndfile)
{
  static char const * const names[] = {
    "sndfile", /* Special type to force use of sndfile for the following: */
  /* LSF implementation of formats built in to SoX: */
    /* "aif", */
    /* "au", */
    /* "gsm", */
    /* "nist", */
    /* "raw", */
    /* "sf", "ircam", */
    /* "snd", */
    /* "svx", */
    /* "voc", */
    /* "vox", */
    /* "wav", */
  /* LSF wrappers of formats already wrapped in SoX: */
    /* "flac", */

    "sds",  /* ?? */
    NULL
  };

  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 16, 24, 32, 8, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_IMA_ADPCM, 4, 0,
    SOX_ENCODING_MS_ADPCM, 4, 0,
    SOX_ENCODING_OKI_ADPCM, 4, 0,
    SOX_ENCODING_GSM, 0,
    0};

  static sox_format_handler_t const format = {SOX_LIB_VERSION_CODE,
    "Pseudo format to use libsndfile", names, 0,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    seek, write_encodings, NULL, sizeof(priv_t)
  };

  return &format;
}
