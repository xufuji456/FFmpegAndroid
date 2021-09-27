/* libSoX skeleton file format handler.
 *
 * Copyright 1999 Chris Bagwell And Sundry Contributors
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

/* Private data for SKEL file */
typedef struct {
  size_t remaining_samples;
} priv_t;

/* Note that if any of your methods doesn't need to do anything, you
   can instead use the relevant sox_*_nothing* method */

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int startread(sox_format_t * ft)
{
  priv_t * sk = (priv_t *)ft->priv;
  size_t samples_in_file;

  /* If you need to seek around the input file. */
  if (!ft->seekable) {
    lsx_fail_errno(ft, SOX_EOF, "skel inputfile must be a file");
    return SOX_EOF;
  }

  /*
   * If your format is headerless and has fixed values for
   * the following items, you can hard code them here (see cdr.c).
   * If your format contains a header with format information
   * then you should set it here.
   */
  ft->signal.rate = 44100; /* or 8000, 16000, 32000, 48000, ... */
  ft->signal.channels = 1; /* or 2 or 3 ... */
  ft->encoding.bits_per_sample = 8; /* or 16 ... */
  ft->encoding.encoding = SOX_ENCODING_UNSIGNED; /* or SIGN2 ... */
  sox_append_comment(&ft->oob.comments, "any comment in file header.");

  /* If your format doesn't have a header then samples_in_file
   * can be determined by the file size.
   */
  samples_in_file = lsx_filelength(ft) / (ft->encoding.bits_per_sample >> 3);

  /* If you can detect the length of your file, record it here. */
  ft->signal.length = samples_in_file;
  sk->remaining_samples = samples_in_file;

  return SOX_SUCCESS;
}

/*
 * Read up to len samples of type sox_sample_t from file into buf[].
 * Return number of samples read, or 0 if at end of file.
 */
static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
  priv_t * UNUSED sk = (priv_t *)ft->priv;
  size_t done;
  unsigned char sample;

  for (done = 0; done < len; done++) {
    if (lsx_eof(ft) || lsx_readb(ft, &sample)) /* no more samples */
      break;
    switch (ft->encoding.bits_per_sample) {
    case 8:
      switch (ft->encoding.encoding) {
      case SOX_ENCODING_UNSIGNED:
        *buf++ = SOX_UNSIGNED_8BIT_TO_SAMPLE(sample,);
        break;
      default:
        lsx_fail("Undetected sample encoding in read!");
        return 0;
      }
      break;
    default:
      lsx_fail("Undetected bad sample size in read!");
      return 0;
    }
  }

  return done;
}

/*
 * Do anything required when you stop reading samples.
 * Don't close input file!
 */
static int stopread(sox_format_t UNUSED * ft)
{
  return SOX_SUCCESS;
}

static int startwrite(sox_format_t * ft)
{
  priv_t * UNUSED sk = (priv_t *)ft->priv;

  /* If you have to seek around the output file. */
  /* If header contains a length value then seeking will be
   * required.  Instead of failing, it's sometimes nice to
   * just set the length to max value and not fail.
   */
  if (!ft->seekable) {
    lsx_fail("Output .skel file must be a file, not a pipe");
    return SOX_EOF;
  }

  if (ft->signal.rate != 44100)
    lsx_fail("Output .skel file must have a sample rate of 44100Hz");

  if (ft->encoding.bits_per_sample == 0) {
    lsx_fail("Did not specify a size for .skel output file");
    return SOX_EOF;
  }

  /* error check ft->encoding.encoding */
  /* error check ft->signal.channels */

  /* Write file header, if any */
  /* Write comment field, if any */

  return SOX_SUCCESS;

}

/*
 * Write len samples of type sox_sample_t from buf[] to file.
 * Return number of samples written.
 */
static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
  priv_t * sk = (priv_t *)ft->priv;
  size_t done = 0;

  (void)sk;
  switch (ft->encoding.bits_per_sample) {
  case 8:
    switch (ft->encoding.encoding) {
    SOX_SAMPLE_LOCALS;
    case SOX_ENCODING_UNSIGNED:
      while (done < len && lsx_writeb(ft, SOX_SAMPLE_TO_UNSIGNED_8BIT(*buf++, ft->clips)) == SOX_SUCCESS)
        ++done;
      break;
    default:
      lsx_fail("Undetected bad sample encoding in write!");
      return 0;
    }
    break;
  default:
    lsx_fail("Undetected bad sample size in write!");
    return 0;
  }
  return done;
}

static int stopwrite(sox_format_t UNUSED * ft)
{
  /* All samples are already written out. */
  /* If file header needs fixing up, for example it needs the number
     of samples in a field, seek back and write them here. */
  return SOX_SUCCESS;
}

static int seek(sox_format_t UNUSED * ft, uint64_t UNUSED offset)
{
  /* Seek relative to current position. */
  return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(skel)
{
  /* Format file suffixes */
  static const char *names[] = {"skel",NULL };

  /* Encoding types and sizes that this handler can write */
  static const unsigned encodings[] = {
    SOX_ENCODING_SIGN2, 16, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    0};

  /* Format descriptor
   * If no specific processing is needed for any of
   * the 7 functions, then the function above can be deleted
   * and NULL used in place of the its name below.
   */
  static sox_format_handler_t handler = {
    SOX_LIB_VERSION_CODE,
    "My first SoX format!",
    names, 0,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    seek, encodings, NULL, sizeof(priv_t)
  };

  return &handler;
}
