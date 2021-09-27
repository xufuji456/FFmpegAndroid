/* Psion Record format (format of sound files used for EPOC machines).
 * The file normally has no extension, so SoX uses .prc (Psion ReCord).
 * Based (heavily) on the wve.c format file.
 * Hacked by Bert van Leeuwen (bert@e.co.za)
 *
 * Header check improved, ADPCM encoding added, and other improvements
 * by Reuben Thomas <rrt@sc3d.org>, using file format info at
 * http://software.frodo.looijaard.name/psiconv/formats/
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
 * Includes code for ADPCM framing based on code carrying the
 * following copyright:
 *
 *******************************************************************
 Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
 Netherlands.

                        All Rights Reserved

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and that
 both that copyright notice and this permission notice appear in
 supporting documentation, and that the names of Stichting Mathematisch
 Centrum or CWI not be used in advertising or publicity pertaining to
 distribution of the software without specific, written prior permission.

 STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
 THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
 FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************/


#include "sox_i.h"

#include "adpcms.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef struct {
  uint32_t nsamp, nbytes;
  short padding;
  short repeats;
  off_t data_start;         /* for seeking */
  adpcm_io_t adpcm;
  unsigned frame_samp;     /* samples left to read in current frame */
} priv_t;

static void prcwriteheader(sox_format_t * ft);

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * p = (priv_t *)ft->priv;
  if (ft->encoding.encoding == SOX_ENCODING_ALAW)
    return lsx_offset_seek(ft, (off_t)p->data_start, (off_t)offset);
  return SOX_EOF;
}

/* File header. The first 4 words are fixed; the rest of the header
   could theoretically be different, and this is the first place to
   check with apparently invalid files.

   N.B. All offsets are from start of file. */
static const char prc_header[41] = {
  /* Header section */
  '\x37','\x00','\x00','\x10', /* 0x00: File type (UID 1) */
  '\x6d','\x00','\x00','\x10', /* 0x04: File kind (UID 2) */
  '\x7e','\x00','\x00','\x10', /* 0x08: Application ID (UID 3) */
  '\xcf','\xac','\x08','\x55', /* 0x0c: Checksum of UIDs 1-3 */
  '\x14','\x00','\x00','\x00', /* 0x10: File offset of Section Table Section */
  /* Section Table Section: a BListL, i.e. a list of longs preceded by
     length byte.
     The longs are in (ID, offset) pairs, each pair identifying a
     section. */
  '\x04',                      /* 0x14: List has 4 bytes, i.e. 2 pairs */
  '\x52','\x00','\x00','\x10', /* 0x15: ID: Record Section */
  '\x34','\x00','\x00','\x00', /* 0x19: Offset to Record Section */
  '\x89','\x00','\x00','\x10', /* 0x1d: ID: Application ID Section */
  '\x25','\x00','\x00','\x00', /* 0x21: Offset to Application ID Section */
  '\x7e','\x00','\x00','\x10', /* 0x25: Application ID Section:
                                  Record.app identifier */
  /* Next comes the string, which can be either case. */
};

/* Format of the Record Section (offset 0x34):

00 L Uncompressed data length
04 ID a1 01 00 10 for ADPCM, 00 00 00 00 for A-law
08 W number of times sound will be repeated (0 = played once)
0a B Volume setting (01-05)
0b B Always 00 (?)
0c L Time between repeats in usec
10 LListB (i.e. long giving number of bytes followed by bytes) Sound Data
*/

static int prc_checkheader(sox_format_t * ft, char *head)
{
  lsx_readbuf(ft, head, sizeof(prc_header));
  return memcmp(head, prc_header, sizeof(prc_header)) == 0;
}

static int startread(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  char head[sizeof(prc_header)];
  uint8_t byte;
  uint16_t reps;
  uint32_t len, listlen, encoding, repgap;
  unsigned char volume;
  char appname[0x40]; /* Maximum possible length of name */

  /* Check the header */
  if (prc_checkheader(ft, head))
    lsx_debug("Found Psion Record header");
  else {
      lsx_fail_errno(ft,SOX_EHDR,"Not a Psion Record file");
      return (SOX_EOF);
  }

  lsx_readb(ft, &byte);
  if ((byte & 0x3) != 0x2) {
    lsx_fail_errno(ft, SOX_EHDR, "Invalid length byte for application name string %d", (int)(byte));
    return SOX_EOF;
  }

  byte >>= 2;
  assert(byte < 64);
  lsx_reads(ft, appname, (size_t)byte);
  if (strncasecmp(appname, "record.app", (size_t) byte) != 0) {
    lsx_fail_errno(ft, SOX_EHDR, "Invalid application name string %.63s", appname);
    return SOX_EOF;
  }

  lsx_readdw(ft, &len);
  p->nsamp = len;
  lsx_debug("Number of samples: %d", len);

  lsx_readdw(ft, &encoding);
  lsx_debug("Encoding of samples: %x", encoding);
  if (encoding == 0)
    ft->encoding.encoding = SOX_ENCODING_ALAW;
  else if (encoding == 0x100001a1)
    ft->encoding.encoding = SOX_ENCODING_IMA_ADPCM;
  else {
    lsx_fail_errno(ft, SOX_EHDR, "Unrecognised encoding");
    return SOX_EOF;
  }

  lsx_readw(ft, &reps);    /* Number of repeats */
  lsx_debug("Repeats: %d", reps);

  lsx_readb(ft, &volume);
  lsx_debug("Volume: %d", (unsigned)volume);
  if (volume < 1 || volume > 5)
    lsx_warn("Volume %d outside range 1..5", volume);

  lsx_readb(ft, &byte);   /* Unused and seems always zero */

  lsx_readdw(ft, &repgap); /* Time between repeats in usec */
  lsx_debug("Time between repeats (usec): %u", repgap);

  lsx_readdw(ft, &listlen); /* Length of samples list */
  lsx_debug("Number of bytes in samples list: %u", listlen);

  if (ft->signal.rate != 0 && ft->signal.rate != 8000)
    lsx_report("PRC only supports 8 kHz; overriding.");
  ft->signal.rate = 8000;

  if (ft->signal.channels != 1 && ft->signal.channels != 0)
    lsx_report("PRC only supports 1 channel; overriding.");
  ft->signal.channels = 1;

  p->data_start = lsx_tell(ft);
  ft->signal.length = p->nsamp / ft->signal.channels;

  if (ft->encoding.encoding == SOX_ENCODING_ALAW) {
    ft->encoding.bits_per_sample = 8;
    if (lsx_rawstartread(ft))
      return SOX_EOF;
  } else if (ft->encoding.encoding == SOX_ENCODING_IMA_ADPCM) {
    p->frame_samp = 0;
    if (lsx_adpcm_ima_start(ft, &p->adpcm))
      return SOX_EOF;
  }

  return (SOX_SUCCESS);
}

/* Read a variable-length encoded count */
/* Ignore return code of lsx_readb, as it doesn't really matter if EOF
   is delayed until the caller. */
static unsigned read_cardinal(sox_format_t * ft)
{
  unsigned a;
  uint8_t byte;

  if (lsx_readb(ft, &byte) == SOX_EOF)
    return (unsigned)SOX_EOF;
  lsx_debug_more("Cardinal byte 1: %x", byte);
  a = byte;
  if (!(a & 1))
    a >>= 1;
  else {
    if (lsx_readb(ft, &byte) == SOX_EOF)
      return (unsigned)SOX_EOF;
    lsx_debug_more("Cardinal byte 2: %x", byte);
    a |= byte << 8;
    if (!(a & 2))
      a >>= 2;
    else if (!(a & 4)) {
      if (lsx_readb(ft, &byte) == SOX_EOF)
        return (unsigned)SOX_EOF;
      lsx_debug_more("Cardinal byte 3: %x", byte);
      a |= byte << 16;
      if (lsx_readb(ft, &byte) == SOX_EOF)
        return (unsigned)SOX_EOF;
      lsx_debug_more("Cardinal byte 4: %x", byte);
      a |= byte << 24;
      a >>= 3;
    }
  }

  return a;
}

static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t samp)
{
  priv_t * p = (priv_t *)ft->priv;

  lsx_debug_more("length now = %d", p->nsamp);

  if (ft->encoding.encoding == SOX_ENCODING_IMA_ADPCM) {
    size_t nsamp, read;

    if (p->frame_samp == 0) {
      unsigned framelen = read_cardinal(ft);
      uint32_t trash;

      if (framelen == (unsigned)SOX_EOF)
        return 0;

      lsx_debug_more("frame length %d", framelen);
      p->frame_samp = framelen;

      /* Discard length of compressed data */
      lsx_debug_more("compressed length %d", read_cardinal(ft));
      /* Discard length of BListL */
      lsx_readdw(ft, &trash);
      lsx_debug_more("list length %d", trash);

      /* Reset CODEC for start of frame */
      lsx_adpcm_reset(&p->adpcm, ft->encoding.encoding);
    }
    nsamp = min(p->frame_samp, samp);
    p->nsamp += nsamp;
    read = lsx_adpcm_read(ft, &p->adpcm, buf, nsamp);
    p->frame_samp -= read;
    lsx_debug_more("samples left in this frame: %d", p->frame_samp);
    return read;
  } else {
    p->nsamp += samp;
    return lsx_rawread(ft, buf, samp);
  }
}

static int stopread(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;

  if (ft->encoding.encoding == SOX_ENCODING_IMA_ADPCM)
    return lsx_adpcm_stopread(ft, &p->adpcm);
  else
    return SOX_SUCCESS;
}

/* When writing, the header is supposed to contain the number of
   data bytes written, unless it is written to a pipe.
   Since we don't know how many bytes will follow until we're done,
   we first write the header with an unspecified number of bytes,
   and at the end we rewind the file and write the header again
   with the right size.  This only works if the file is seekable;
   if it is not, the unspecified size remains in the header
   (this is illegal). */

static int startwrite(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;

  if (ft->encoding.encoding == SOX_ENCODING_ALAW) {
    if (lsx_rawstartwrite(ft))
      return SOX_EOF;
  } else if (ft->encoding.encoding == SOX_ENCODING_IMA_ADPCM) {
    if (lsx_adpcm_ima_start(ft, &p->adpcm))
      return SOX_EOF;
  }

  p->nsamp = 0;
  p->nbytes = 0;
  if (p->repeats == 0)
    p->repeats = 1;

  prcwriteheader(ft);

  p->data_start = lsx_tell(ft);

  return SOX_SUCCESS;
}

static void write_cardinal(sox_format_t * ft, unsigned a)
{
  uint8_t byte;

  if (a < 0x80) {
    byte = a << 1;
    lsx_debug_more("Cardinal byte 1: %x", byte);
    lsx_writeb(ft, byte);
  } else if (a < 0x8000) {
    byte = (a << 2) | 1;
    lsx_debug_more("Cardinal byte 1: %x", byte);
    lsx_writeb(ft, byte);
    byte = a >> 6;
    lsx_debug_more("Cardinal byte 2: %x", byte);
    lsx_writeb(ft, byte);
  } else {
    byte = (a << 3) | 3;
    lsx_debug_more("Cardinal byte 1: %x", byte);
    lsx_writeb(ft, byte);
    byte = a >> 5;
    lsx_debug_more("Cardinal byte 2: %x", byte);
    lsx_writeb(ft, byte);
    byte = a >> 13;
    lsx_debug_more("Cardinal byte 3: %x", byte);
    lsx_writeb(ft, byte);
    byte = a >> 21;
    lsx_debug_more("Cardinal byte 4: %x", byte);
    lsx_writeb(ft, byte);
  }
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t nsamp)
{
  priv_t * p = (priv_t *)ft->priv;
  /* Psion Record seems not to be able to handle frames > 800 samples */
  size_t written = 0;
  lsx_debug_more("length now = %d", p->nsamp);
  if (ft->encoding.encoding == SOX_ENCODING_IMA_ADPCM) {
    while (written < nsamp) {
      size_t written1, samp = min(nsamp - written, 800);

      write_cardinal(ft, (unsigned) samp);
      /* Write compressed length */
      write_cardinal(ft, (unsigned) ((samp / 2) + (samp % 2) + 4));
      /* Write length again (seems to be a BListL) */
      lsx_debug_more("list length %lu", (unsigned long)samp);
      lsx_writedw(ft, (unsigned) samp);
      lsx_adpcm_reset(&p->adpcm, ft->encoding.encoding);
      written1 = lsx_adpcm_write(ft, &p->adpcm, buf + written, samp);
      if (written1 != samp)
        break;
      lsx_adpcm_flush(ft, &p->adpcm);
      written += written1;
    }
  } else
    written = lsx_rawwrite(ft, buf, nsamp);
  p->nsamp += written;
  return written;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;

  p->nbytes = lsx_tell(ft) - p->data_start;

  if (!ft->seekable) {
      lsx_warn("Header will have invalid file length since file is not seekable");
      return SOX_SUCCESS;
  }

  if (lsx_seeki(ft, (off_t)0, 0) != 0) {
      lsx_fail_errno(ft,errno,"Can't rewind output file to rewrite Psion header.");
      return(SOX_EOF);
  }
  prcwriteheader(ft);
  return SOX_SUCCESS;
}

static void prcwriteheader(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;

  lsx_writebuf(ft, prc_header, sizeof(prc_header));
  lsx_writes(ft, "\x2arecord.app");

  lsx_debug("Number of samples: %d",p->nsamp);
  lsx_writedw(ft, p->nsamp);

  if (ft->encoding.encoding == SOX_ENCODING_ALAW)
    lsx_writedw(ft, 0);
  else
    lsx_writedw(ft, 0x100001a1); /* ADPCM */

  lsx_writew(ft, 0);             /* Number of repeats */
  lsx_writeb(ft, 3);             /* Volume: use default value of Record.app */
  lsx_writeb(ft, 0);             /* Unused and seems always zero */
  lsx_writedw(ft, 0);            /* Time between repeats in usec */

  lsx_debug("Number of bytes: %d", p->nbytes);
  lsx_writedw(ft, p->nbytes);    /* Number of bytes of data */
}

LSX_FORMAT_HANDLER(prc)
{
  static char const * const names[]           = {"prc", NULL};
  static sox_rate_t   const write_rates[]     = {8000, 0};
  static unsigned     const write_encodings[] = {
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_IMA_ADPCM, 4, 0,
    0};
  static sox_format_handler_t const handler = {
    SOX_LIB_VERSION_CODE,
    "Psion Record; used in EPOC devices (Series 5, Revo and similar)",
    names, SOX_FILE_LIT_END | SOX_FILE_MONO,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    seek, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
