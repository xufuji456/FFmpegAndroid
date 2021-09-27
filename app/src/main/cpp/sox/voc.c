/* libSoX Sound Blaster VOC handler sources.
 * Copyright 1991 Lance Norskog And Sundry Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * September 8, 1993
 * Copyright 1993 T. Allen Grider - for changes to support block type 9
 * and word sized samples.  Same caveats and disclaimer as above.
 *
 * February 22, 1996
 * by Chris Bagwell (cbagwell@sprynet.com)
 * Added support for block type 8 (extended) which allows for 8-bit stereo
 * files.  Added support for saving stereo files and 16-bit files.
 * Added VOC format info from audio format FAQ so I don't have to keep
 * looking around for it.
 *
 * February 5, 2001
 * For sox-12-17 by Annonymous (see notes ANN)
 * Added comments and notes for each procedure.
 * Fixed so this now works with pipes, input does not have to
 * be seekable anymore (in startread() )
 * Added support for uLAW and aLaw (aLaw not tested).
 * Fixed support of multi-part VOC files, and files with
 * block 9 but no audio in the block....
 * The following need to be tested:  16-bit, 2 channel, and aLaw.
 *
 * December 10, 2001
 * For sox-12-17-3 by Annonymous (see notes ANN)
 * Patch for sox-12-17 merged with sox-12-17-3-pre3 code.
 *
 */

/*------------------------------------------------------------------------
The following is taken from the Audio File Formats FAQ dated 2-Jan-1995
and submitted by Guido van Rossum <guido@cwi.nl>.
--------------------------------------------------------------------------
Creative Voice (VOC) file format
--------------------------------

From: galt@dsd.es.com

(byte numbers are hex!)

    HEADER (bytes 00-19)
    Series of DATA BLOCKS (bytes 1A+) [Must end w/ Terminator Block]

- ---------------------------------------------------------------

HEADER:
-------
     byte #     Description
     ------     ------------------------------------------
     00-12      "Creative Voice File"
     13         1A (eof to abort printing of file)
     14-15      Offset of first datablock in .voc file (std 1A 00
                in Intel Notation)
     16-17      Version number (minor,major) (VOC-HDR puts 0A 01)
     18-19      2's Comp of Ver. # + 1234h (VOC-HDR puts 29 11)

- ---------------------------------------------------------------

DATA BLOCK:
-----------

   Data Block:  TYPE(1-byte), SIZE(3-bytes), INFO(0+ bytes)
   NOTE: Terminator Block is an exception -- it has only the TYPE byte.

      TYPE   Description     Size (3-byte int)   Info
      ----   -----------     -----------------   -----------------------
      00     Terminator      (NONE)              (NONE)
      01     Sound data      2+length of data    *
      02     Sound continue  length of data      Voice Data
      03     Silence         3                   **
      04     Marker          2                   Marker# (2 bytes)
      05     ASCII           length of string    null terminated string
      06     Repeat          2                   Count# (2 bytes)
      07     End repeat      0                   (NONE)
      08     Extended        4                   ***
      09     New Header      16                  see below


      *Sound Info Format:       **Silence Info Format:
       ---------------------      ----------------------------
       00   Sample Rate           00-01  Length of silence - 1
       01   Compression Type      02     Sample Rate
       02+  Voice Data

    ***Extended Info Format:
       ---------------------
       00-01  Time Constant: Mono: 65536 - (256000000/sample_rate)
                             Stereo: 65536 - (25600000/(2*sample_rate))
       02     Pack
       03     Mode: 0 = mono
                    1 = stereo


  Marker#           -- Driver keeps the most recent marker in a status byte
  Count#            -- Number of repetitions + 1
                         Count# may be 1 to FFFE for 0 - FFFD repetitions
                         or FFFF for endless repetitions
  Sample Rate       -- SR byte = 256-(1000000/sample_rate)
  Length of silence -- in units of sampling cycle
  Compression Type  -- of voice data
                         8-bits    = 0
                         4-bits    = 1
                         2.6-bits  = 2
                         2-bits    = 3
                         Multi DAC = 3+(# of channels) [interesting--
                                       this isn't in the developer's manual]

Detailed description of new data blocks (VOC files version 1.20 and above):

        (Source is fax from Barry Boone at Creative Labs, 405/742-6622)

BLOCK 8 - digitized sound attribute extension, must preceed block 1.
          Used to define stereo, 8 bit audio
        BYTE bBlockID;       // = 8
        BYTE nBlockLen[3];   // 3 byte length
        WORD wTimeConstant;  // time constant = same as block 1
        BYTE bPackMethod;    // same as in block 1
        BYTE bVoiceMode;     // 0-mono, 1-stereo

        Data is stored left, right

BLOCK 9 - data block that supersedes blocks 1 and 8.
          Used for stereo, 16 bit (and uLaw, aLaw).

        BYTE bBlockID;          // = 9
        BYTE nBlockLen[3];      // length 12 plus length of sound
        DWORD dwSamplesPerSec;  // samples per second, not time const.
        BYTE bBitsPerSample;    // e.g., 8 or 16
        BYTE bChannels;         // 1 for mono, 2 for stereo
        WORD wFormat;           // see below
        BYTE reserved[4];       // pad to make block w/o data
                                // have a size of 16 bytes

        Valid values of wFormat are:

                0x0000  8-bit unsigned PCM
                0x0001  Creative 8-bit to 4-bit ADPCM
                0x0002  Creative 8-bit to 3-bit ADPCM
                0x0003  Creative 8-bit to 2-bit ADPCM
                0x0004  16-bit signed PCM
                0x0006  CCITT a-Law
                0x0007  CCITT u-Law
                0x02000 Creative 16-bit to 4-bit ADPCM

        Data is stored left, right

        ANN:  Multi-byte quantities are in Intel byte order (Little Endian).

------------------------------------------------------------------------*/

#include "sox_i.h"
#include "g711.h"
#include "adpcms.h"
#include <assert.h>
#include <string.h>

/* Private data for VOC file */
typedef struct {
  long block_remaining;         /* bytes remaining in current block */
  long rate;                    /* rate code (byte) of this chunk */
  int silent;                   /* sound or silence? */
  long srate;                   /* rate code (byte) of silence */
  size_t blockseek;         /* start of current output block */
  long samples;                 /* number of samples output */
  int format;                   /* VOC audio format */
  int size;                     /* word length of data */
  int channels;                 /* number of sound channels */
  long total_size;              /* total size of all audio in file */
  int extended;                 /* Has an extended block been read? */
  adpcm_t adpcm;
} priv_t;

#define VOC_TERM        0
#define VOC_DATA        1
#define VOC_CONT        2
#define VOC_SILENCE     3
#define VOC_MARKER      4
#define VOC_TEXT        5
#define VOC_LOOP        6
#define VOC_LOOPEND     7
#define VOC_EXTENDED    8
#define VOC_DATA_16     9

/* ANN:  Format encoding types */
#define VOC_FMT_LIN8U          0        /* 8 bit unsigned linear PCM */
#define VOC_FMT_CRLADPCM4      1        /* Creative 8-bit to 4-bit ADPCM */
#define VOC_FMT_CRLADPCM3      2        /* Creative 8-bit to 3-bit ADPCM */
#define VOC_FMT_CRLADPCM2      3        /* Creative 8-bit to 2-bit ADPCM */
#define VOC_FMT_LIN16          4        /* 16-bit signed PCM */
#define VOC_FMT_ALAW           6        /* CCITT a-Law 8-bit PCM */
#define VOC_FMT_MU255          7        /* CCITT u-Law 8-bit PCM */
#define VOC_FMT_CRLADPCM4A 0x200        /* Creative 16-bit to 4-bit ADPCM */

/* Prototypes for internal functions */
static int getblock(sox_format_t *);
static void blockstart(sox_format_t *);

/* Conversion macros (from raw.c) */
#define SOX_ALAW_BYTE_TO_SAMPLE(d) ((sox_sample_t)(sox_alaw2linear16(d)) << 16)
#define SOX_ULAW_BYTE_TO_SAMPLE(d) ((sox_sample_t)(sox_ulaw2linear16(d)) << 16)

/* public VOC functions for SOX */

/*-----------------------------------------------------------------
 * startread() -- start reading a VOC file
 *-----------------------------------------------------------------*/
static int startread(sox_format_t * ft)
{
  char header[20];
  priv_t * v = (priv_t *) ft->priv;
  unsigned short sbseek;
  int rc;
  int ii;                       /* for getting rid of lseek */
  unsigned char uc;

  if (lsx_readbuf(ft, header, (size_t)20) != 20) {
    lsx_fail_errno(ft, SOX_EHDR, "unexpected EOF in VOC header");
    return (SOX_EOF);
  }
  if (strncmp(header, "Creative Voice File\032", (size_t)19)) {
    lsx_fail_errno(ft, SOX_EHDR, "VOC file header incorrect");
    return (SOX_EOF);
  }

  /* read the offset to data, from start of file */
  /* after this read we have read 20 bytes of header + 2 */
  lsx_readw(ft, &sbseek);

  /* ANN:  read to skip the header, instead of lseek */
  /* this should allow use with pipes.... */
  for (ii = 22; ii < sbseek; ii++)
    lsx_readb(ft, &uc);

  v->rate = -1;
  v->format = -1;
  v->channels = -1;
  v->block_remaining = 0;
  v->total_size = 0;    /* ANN added */
  v->extended = 0;

  /* read until we get the format information.... */
  rc = getblock(ft);
  if (rc)
    return rc;

  /* get rate of data */
  if (v->rate == -1) {
    lsx_fail_errno(ft, SOX_EOF, "Input .voc file had no sound!");
    return (SOX_EOF);
  }

  /* setup word length of data */

  /* ANN:  Check VOC format and map to the proper libSoX format value */
  switch (v->format) {
    case VOC_FMT_LIN8U:        /*     0    8 bit unsigned linear PCM */
      ft->encoding.encoding = SOX_ENCODING_UNSIGNED;
      v->size = 8;
      break;
    case VOC_FMT_CRLADPCM4:    /*     1    Creative 8-bit to 4-bit ADPCM */
      ft->encoding.encoding = SOX_ENCODING_CL_ADPCM;
      v->size = 4;
      break;
    case VOC_FMT_CRLADPCM3:    /*     2    Creative 8-bit to 3-bit ADPCM */
      ft->encoding.encoding = SOX_ENCODING_CL_ADPCM;
      v->size = 3;
      break;
    case VOC_FMT_CRLADPCM2:    /*     3    Creative 8-bit to 2-bit ADPCM */
      ft->encoding.encoding = SOX_ENCODING_CL_ADPCM;
      v->size = 2;
      break;
    case VOC_FMT_LIN16:        /*     4    16-bit signed PCM */
      ft->encoding.encoding = SOX_ENCODING_SIGN2;
      v->size = 16;
      break;
    case VOC_FMT_ALAW: /*     6    CCITT a-Law 8-bit PCM */
      ft->encoding.encoding = SOX_ENCODING_ALAW;
      v->size = 8;
      break;
    case VOC_FMT_MU255:        /*     7    CCITT u-Law 8-bit PCM */
      ft->encoding.encoding = SOX_ENCODING_ULAW;
      v->size = 8;
      break;
    case VOC_FMT_CRLADPCM4A:   /*0x200    Creative 16-bit to 4-bit ADPCM */
      ft->encoding.encoding = SOX_ENCODING_CL_ADPCM16;
      v->size = 4;
      break;
    default:
      lsx_fail("Unknown VOC format %d", v->format);
      break;
  }
  ft->encoding.bits_per_sample = v->size;

  /* setup number of channels */
  if (ft->signal.channels == 0)
    ft->signal.channels = v->channels;

  return (SOX_SUCCESS);
}

/*-----------------------------------------------------------------
 * read() -- read data from a VOC file
 * ANN:  Major changes here to support multi-part files and files
 *       that do not have audio in block 9's.
 *-----------------------------------------------------------------*/
static size_t read_samples(sox_format_t * ft, sox_sample_t * buf,
                               size_t len)
{
  priv_t * v = (priv_t *) ft->priv;
  size_t done = 0;
  int rc = 0;
  int16_t sw;
  unsigned char uc;

  if (v->block_remaining == 0) {        /* handle getting another cont. buffer */
    rc = getblock(ft);
    if (rc)
      return 0;
  }

  if (v->block_remaining == 0)  /* if no more data, return 0, i.e., done */
    return 0;

  if (v->silent) {
    for (; v->block_remaining && (done < len); v->block_remaining--, done++)
      *buf++ = 0;       /* Fill in silence */
  } else {      /* not silence; read len samples of audio from the file */
    size_t per = max(1, 9 / v->size);

    for (; (done + per <= len); done += per) {
      if (v->block_remaining == 0) {    /* IF no more in this block, get another */
        while (v->block_remaining == 0) {       /* until have either EOF or a block with data */
          rc = getblock(ft);
          if (rc)
            break;
        }
        if (rc) /* IF EOF, break out, no more data, next will return 0 */
          break;
      }

      /* Read the data in the file */
      if (v->size <= 4) {
        if (!v->adpcm.setup.sign) {
          SOX_SAMPLE_LOCALS;
          if (lsx_readb(ft, &uc) == SOX_EOF) {
            lsx_warn("VOC input: short file");
            v->block_remaining = 0;
            return done;
          }
          *buf = SOX_UNSIGNED_8BIT_TO_SAMPLE(uc,);
          lsx_adpcm_init(&v->adpcm, 6 - v->size, SOX_SAMPLE_TO_SIGNED_16BIT(*buf, ft->clips));
          ++buf;
          --v->block_remaining;
          ++done;
        }
        if (lsx_readb(ft, &uc) == SOX_EOF) {
          lsx_warn("VOC input: short file");
          v->block_remaining = 0;
          return done;
        }
        switch (v->size) {
          case 2:
            if (v->format == VOC_FMT_CRLADPCM2) {
              int u = uc;

              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 6, &v->adpcm),);
              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 4, &v->adpcm),);
              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 2, &v->adpcm),);
              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u     , &v->adpcm),);
            }
            break;
          case 3:
            if (v->format == VOC_FMT_CRLADPCM3) {
              int u = uc;

              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 5, &v->adpcm),);
              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 2, &v->adpcm),);
              *buf++ =                              /* A bit from nowhere! */
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u << 1, &v->adpcm),);
            }
            break;
          case 4:
            if (v->format == VOC_FMT_CRLADPCM4) {
              int u = uc;

              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u >> 4, &v->adpcm),);
              *buf++ =
                  SOX_SIGNED_16BIT_TO_SAMPLE(lsx_adpcm_decode (u     , &v->adpcm),);
            }
            break;
        }
      } else
        switch (v->size) {
          case 8:
            if (lsx_readb(ft, &uc) == SOX_EOF) {
              lsx_warn("VOC input: short file");
              v->block_remaining = 0;
              return done;
            }
            if (v->format == VOC_FMT_MU255) {
              *buf++ = SOX_ULAW_BYTE_TO_SAMPLE(uc);
            } else if (v->format == VOC_FMT_ALAW) {
              *buf++ = SOX_ALAW_BYTE_TO_SAMPLE(uc);
            } else {
              *buf++ = SOX_UNSIGNED_8BIT_TO_SAMPLE(uc,);
            }
            break;
          case 16:
            lsx_readsw(ft, &sw);
            if (lsx_eof(ft)) {
              lsx_warn("VOC input: short file");
              v->block_remaining = 0;
              return done;
            }
            *buf++ = SOX_SIGNED_16BIT_TO_SAMPLE(sw,);
            v->block_remaining--;       /* Processed 2 bytes so update */
            break;
        }
      /* decrement count of processed bytes */
      v->block_remaining--;
    }
  }
  v->total_size += done;
  return done;
}


/* When saving samples in VOC format the following outline is followed:
 * If an 8-bit mono sample then use a VOC_DATA header.
 * If an 8-bit stereo sample then use a VOC_EXTENDED header followed
 * by a VOC_DATA header.
 * If a 16-bit sample (either stereo or mono) then save with a
 * VOC_DATA_16 header.
 *
 * ANN:  Not supported:  uLaw and aLaw output VOC files....
 *
 * This approach will cause the output to be an its most basic format
 * which will work with the oldest software (eg. an 8-bit mono sample
 * will be able to be played with a really old SB VOC player.)
 */
static int startwrite(sox_format_t * ft)
{
  priv_t * v = (priv_t *) ft->priv;

  if (!ft->seekable) {
    lsx_fail_errno(ft, SOX_EOF,
                   "Output .voc file must be a file, not a pipe");
    return (SOX_EOF);
  }

  v->samples = 0;

  /* File format name and a ^Z (aborts printing under DOS) */
  lsx_writes(ft, "Creative Voice File\032");
  lsx_writew(ft, 26);   /* size of header */
  lsx_writew(ft, 0x10a);        /* major/minor version number */
  lsx_writew(ft, 0x1129);       /* checksum of version number */

  return (SOX_SUCCESS);
}

/*-----------------------------------------------------------------
 * write() -- write a VOC file
 *-----------------------------------------------------------------*/
static size_t write_samples(sox_format_t * ft, const sox_sample_t * buf,
                                size_t len)
{
  priv_t * v = (priv_t *) ft->priv;
  unsigned char uc;
  int16_t sw;
  size_t done = 0;

  if (len && v->samples == 0) {
    /* No silence packing yet. */
    v->silent = 0;
    blockstart(ft);
  }
  v->samples += len;
  while (done < len) {
    SOX_SAMPLE_LOCALS;
    if (ft->encoding.bits_per_sample == 8) {
      uc = SOX_SAMPLE_TO_UNSIGNED_8BIT(*buf++, ft->clips);
      lsx_writeb(ft, uc);
    } else {
      sw = (int) SOX_SAMPLE_TO_SIGNED_16BIT(*buf++, ft->clips);
      lsx_writesw(ft, sw);
    }
    done++;
  }
  return done;
}

/*-----------------------------------------------------------------
 * blockstop() -- stop an output block
 * End the current data or silence block.
 *-----------------------------------------------------------------*/
static void blockstop(sox_format_t * ft)
{
  priv_t * v = (priv_t *) ft->priv;
  sox_sample_t datum;

  lsx_writeb(ft, 0);    /* End of file block code */
  lsx_seeki(ft, (off_t) v->blockseek, 0); /* seek back to block length */
  lsx_seeki(ft, (off_t)1, 1);  /* seek forward one */
  if (v->silent) {
    lsx_writesw(ft, (signed)v->samples);
  } else {
    if (ft->encoding.bits_per_sample == 8) {
      if (ft->signal.channels > 1) {
        lsx_seeki(ft, (off_t)8, 1);    /* forward 7 + 1 for new block header */
      }
    }
    v->samples += 2;    /* adjustment: SBDK pp. 3-5 */
    datum = (v->samples * (ft->encoding.bits_per_sample >> 3)) & 0xff;
    lsx_writesb(ft, datum);     /* low byte of length */
    datum = ((v->samples * (ft->encoding.bits_per_sample >> 3)) >> 8) & 0xff;
    lsx_writesb(ft, datum);     /* middle byte of length */
    datum = ((v->samples * (ft->encoding.bits_per_sample >> 3)) >> 16) & 0xff;
    lsx_writesb(ft, datum);     /* high byte of length */
  }
}

/*-----------------------------------------------------------------
 * stopwrite() -- stop writing a VOC file
 *-----------------------------------------------------------------*/
static int stopwrite(sox_format_t * ft)
{
  blockstop(ft);
  return (SOX_SUCCESS);
}

/*-----------------------------------------------------------------
 * Voc-file handlers (static, private to this module)
 *-----------------------------------------------------------------*/

/*-----------------------------------------------------------------
 * getblock() -- Read next block header, save info,
 *               leave position at start of dat
 *-----------------------------------------------------------------*/
static int getblock(sox_format_t * ft)
{
  priv_t * v = (priv_t *) ft->priv;
  unsigned char uc, block;
  uint16_t u16;
  sox_uint24_t sblen;
  uint16_t new_rate_16;
  uint32_t new_rate_32;

  v->silent = 0;
  /* DO while we have no audio to read */
  while (v->block_remaining == 0) {
    if (lsx_eof(ft))
      return SOX_EOF;

    if (lsx_readb(ft, &block) == SOX_EOF)
      return SOX_EOF;

    if (block == VOC_TERM)
      return SOX_EOF;

    if (lsx_eof(ft))
      return SOX_EOF;

    lsx_read3(ft, &sblen);

    /* Based on VOC block type, process the block */
    /* audio may be in one or multiple blocks */
    switch (block) {
      case VOC_DATA:
        lsx_readb(ft, &uc);
        /* When DATA block preceeded by an EXTENDED     */
        /* block, the DATA blocks rate value is invalid */
        if (!v->extended) {
          if (uc == 0) {
            lsx_fail_errno(ft, SOX_EFMT, "Sample rate is zero?");
            return (SOX_EOF);
          }
          if ((v->rate != -1) && (uc != v->rate)) {
            lsx_fail_errno(ft, SOX_EFMT,
                           "sample rate codes differ: %ld != %d", v->rate,
                           uc);
            return (SOX_EOF);
          }
          v->rate = uc;
          ft->signal.rate = 1000000.0 / (256 - v->rate);
          if (v->channels != -1 && v->channels != 1) {
            lsx_fail_errno(ft, SOX_EFMT, "channel count changed");
            return SOX_EOF;
          }
          v->channels = 1;
        }
        lsx_readb(ft, &uc);
        if (v->format != -1 && uc != v->format) {
          lsx_fail_errno(ft, SOX_EFMT, "format changed");
          return SOX_EOF;
        }
        v->format = uc;
        v->extended = 0;
        v->block_remaining = sblen - 2;
        return (SOX_SUCCESS);
      case VOC_DATA_16:
        lsx_readdw(ft, &new_rate_32);
        if (new_rate_32 == 0) {
          lsx_fail_errno(ft, SOX_EFMT, "Sample rate is zero?");
          return (SOX_EOF);
        }
        if ((v->rate != -1) && ((long) new_rate_32 != v->rate)) {
          lsx_fail_errno(ft, SOX_EFMT, "sample rate codes differ: %ld != %d",
                         v->rate, new_rate_32);
          return (SOX_EOF);
        }
        v->rate = new_rate_32;
        ft->signal.rate = new_rate_32;
        lsx_readb(ft, &uc);
        v->size = uc;
        lsx_readb(ft, &uc);
        if (v->channels != -1 && uc != v->channels) {
          lsx_fail_errno(ft, SOX_EFMT, "channel count changed");
          return SOX_EOF;
        }
        v->channels = uc;
        lsx_readw(ft, &u16);    /* ANN: added format */
        if (v->format != -1 && u16 != v->format) {
          lsx_fail_errno(ft, SOX_EFMT, "format changed");
          return SOX_EOF;
        }
        v->format = u16;
        lsx_skipbytes(ft, (size_t) 4);
        v->block_remaining = sblen - 12;
        return (SOX_SUCCESS);
      case VOC_CONT:
        v->block_remaining = sblen;
        return (SOX_SUCCESS);
      case VOC_SILENCE:
        {
          unsigned short period;

          lsx_readw(ft, &period);
          lsx_readb(ft, &uc);
          if (uc == 0) {
            lsx_fail_errno(ft, SOX_EFMT, "Silence sample rate is zero");
            return (SOX_EOF);
          }
          /*
           * Some silence-packed files have gratuitously
           * different sample rate codes in silence.
           * Adjust period.
           */
          if ((v->rate != -1) && (uc != v->rate))
            period = (period * (256. - uc)) / (256 - v->rate) + .5;
          else
            v->rate = uc;
          v->block_remaining = period;
          v->silent = 1;
          return (SOX_SUCCESS);
        }
      case VOC_MARKER:
        lsx_readb(ft, &uc);
        lsx_readb(ft, &uc);
        /* Falling! Falling! */
      case VOC_TEXT:
        {
          uint32_t i = sblen;
          int8_t c                /*, line_buf[80];
                                 * int len = 0 */ ;

          lsx_warn("VOC TEXT");
          while (i--) {
            lsx_readsb(ft, &c);
            /* FIXME: this needs to be tested but I couldn't
             * find a voc file with a VOC_TEXT chunk :(
             if (c != '\0' && c != '\r')
             line_buf[len++] = c;
             if (len && (c == '\0' || c == '\r' ||
             i == 0 || len == sizeof(line_buf) - 1))
             {
             lsx_report("%s", line_buf);
             line_buf[len] = '\0';
             len = 0;
             }
             */
          }
        }
        continue;       /* get next block */
      case VOC_LOOP:
      case VOC_LOOPEND:
        lsx_debug("skipping repeat loop");
        lsx_skipbytes(ft, (size_t) sblen);
        break;
      case VOC_EXTENDED:
        /* An Extended block is followed by a data block */
        /* Set this byte so we know to use the rate      */
        /* value from the extended block and not the     */
        /* data block.                                   */
        v->extended = 1;
        lsx_readw(ft, &new_rate_16);
        if (new_rate_16 == 0) {
          lsx_fail_errno(ft, SOX_EFMT, "Sample rate is zero?");
          return (SOX_EOF);
        }
        if ((v->rate != -1) && (new_rate_16 != v->rate)) {
          lsx_fail_errno(ft, SOX_EFMT, "sample rate codes differ: %ld != %d",
                         v->rate, new_rate_16);
          return (SOX_EOF);
        }
        v->rate = new_rate_16;
        lsx_readb(ft, &uc); /* bits_per_sample */
        lsx_readb(ft, &uc);
        if (v->channels != -1 && uc != v->channels) {
          lsx_fail_errno(ft, SOX_EFMT, "channel count changed");
          return SOX_EOF;
        }
        v->channels = uc;
        ft->signal.channels = uc? 2 : 1;      /* Stereo */
        /* Needed number of channels before finishing
         * compute for rate */
        ft->signal.rate = (256e6 / (65536 - v->rate)) / ft->signal.channels;
        /* An extended block must be followed by a data */
        /* block to be valid so loop back to top so it  */
        /* can be grabed.                               */
        continue;
      default:
        lsx_debug("skipping unknown block code %d", block);
        lsx_skipbytes(ft, (size_t) sblen);
    }
  }
  return SOX_SUCCESS;
}

/*-----------------------------------------------------------------
 * vlockstart() -- start an output block
 *-----------------------------------------------------------------*/
static void blockstart(sox_format_t * ft)
{
  priv_t * v = (priv_t *) ft->priv;

  v->blockseek = lsx_tell(ft);
  if (v->silent) {
    lsx_writeb(ft, VOC_SILENCE);        /* Silence block code */
    lsx_writeb(ft, 0);  /* Period length */
    lsx_writeb(ft, 0);  /* Period length */
    lsx_writesb(ft, (signed)v->rate);   /* Rate code */
  } else {
    if (ft->encoding.bits_per_sample == 8) {
      /* 8-bit sample section.  By always setting the correct     */
      /* rate value in the DATA block (even when its preceeded    */
      /* by an EXTENDED block) old software can still play stereo */
      /* files in mono by just skipping over the EXTENDED block.  */
      /* Prehaps the rate should be doubled though to make up for */
      /* double amount of samples for a given time????            */
      if (ft->signal.channels > 1) {
        lsx_writeb(ft, VOC_EXTENDED);   /* Voice Extended block code */
        lsx_writeb(ft, 4);      /* block length = 4 */
        lsx_writeb(ft, 0);      /* block length = 4 */
        lsx_writeb(ft, 0);      /* block length = 4 */
        v->rate = 65536 - (256000000.0 / (2 * ft->signal.rate)) + .5;
        lsx_writesw(ft, (signed) v->rate);       /* Rate code */
        lsx_writeb(ft, 0);      /* File is not packed */
        lsx_writeb(ft, 1);      /* samples are in stereo */
      }
      lsx_writeb(ft, VOC_DATA); /* Voice Data block code */
      lsx_writeb(ft, 0);        /* block length (for now) */
      lsx_writeb(ft, 0);        /* block length (for now) */
      lsx_writeb(ft, 0);        /* block length (for now) */
      v->rate = 256 - (1000000.0 / ft->signal.rate) + .5;
      lsx_writesb(ft, (signed) v->rate); /* Rate code */
      lsx_writeb(ft, 0);        /* 8-bit raw data */
    } else {
      lsx_writeb(ft, VOC_DATA_16);      /* Voice Data block code */
      lsx_writeb(ft, 0);        /* block length (for now) */
      lsx_writeb(ft, 0);        /* block length (for now) */
      lsx_writeb(ft, 0);        /* block length (for now) */
      v->rate = ft->signal.rate + .5;
      lsx_writedw(ft, (unsigned) v->rate);      /* Rate code */
      lsx_writeb(ft, 16);       /* Sample Size */
      lsx_writeb(ft, ft->signal.channels);      /* Sample Size */
      lsx_writew(ft, 0x0004);   /* Encoding */
      lsx_writeb(ft, 0);        /* Unused */
      lsx_writeb(ft, 0);        /* Unused */
      lsx_writeb(ft, 0);        /* Unused */
      lsx_writeb(ft, 0);        /* Unused */
    }
  }
}

LSX_FORMAT_HANDLER(voc)
{
  static char const *const names[] = { "voc", NULL };
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 16, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    0
  };
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Creative Technology Sound Blaster format",
    names, SOX_FILE_LIT_END | SOX_FILE_MONO | SOX_FILE_STEREO,
    startread, read_samples, NULL,
    startwrite, write_samples, stopwrite,
    NULL, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
