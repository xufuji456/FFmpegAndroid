/* libSoX Yamaha TX-16W sampler file support
 *
 * May 20, 1993
 * Copyright 1993 Rob Talley   (rob@aii.com)
 * This source code is freely redistributable and may be used for
 * any purpose. This copyright notice and the following copyright
 * notice must be maintained intact. No warranty whatsoever is
 * provided. This code is furnished AS-IS as a component of the
 * larger work Copyright 1991 Lance Norskog and Sundry Contributors.
 * Much appreciation to ross-c  for his sampConv utility for SGI/IRIX
 * from where these methods were derived.
 *
 * Jan 24, 1994
 * Pat McElhatton, HP Media Technology Lab <patmc@apollo.hp.com>
 * Handles reading of files which do not have the sample rate field
 * set to one of the expected by looking at some other bytes in the
 * attack/loop length fields, and defaulting to 33kHz if the sample
 * rate is still unknown.
 *
 * January 12, 1995
 * Copyright 1995 Mark Lakata (lakata@physics.berkeley.edu)
 * Additions to tx16w.c SOX handler.  This version writes as well as
 * reads TX16W format.
 *
 * July 31, 1998
 * Cleaned up by Leigh Smith (leigh@psychokiller.dialix.oz.au)
 * for incorporation into the main sox distribution.
 *
 * September 24, 1998
 * Forced output to mono signed words to match input.  It was basically
 * doing this anyways but now the user will see a display that it's been
 * overridden.  cbagwell@sprynet.com
 *
 */

#include "sox_i.h"
#include <stdio.h>
#include <string.h>

#define TXMAXLEN 0x3FF80

/* Private data for TX16 file */
typedef struct {
  size_t   samples_out;
  size_t   bytes_out;
  size_t   rest;                 /* bytes remaining in sample file */
  sox_sample_t odd;
  sox_bool     odd_flag;
} priv_t;

struct WaveHeader_ {
  char filetype[6]; /* = "LM8953", */
  unsigned char
    nulls[10],
    dummy_aeg[6],    /* space for the AEG (never mind this) */
    format,          /* 0x49 = looped, 0xC9 = non-looped */
    sample_rate,     /* 1 = 33 kHz, 2 = 50 kHz, 3 = 16 kHz */
    atc_length[3],   /* I'll get to this... */
    rpt_length[3],
    unused[2];       /* set these to null, to be on the safe side */
};

static const unsigned char magic1[4] = {0, 0x06, 0x10, 0xF6};
static const unsigned char magic2[4] = {0, 0x52, 0x00, 0x52};

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int startread(sox_format_t * ft)
{
    int c;
    char filetype[7];
    int8_t format;
    unsigned char sample_rate;
    size_t num_samp_bytes = 0;
    unsigned char gunk[8];
    int blewIt;
    uint8_t trash;

    priv_t * sk = (priv_t *) ft->priv;
    /* If you need to seek around the input file. */
    if (! ft->seekable)
    {
        lsx_fail_errno(ft,SOX_EOF,"txw input file must be a file, not a pipe");
        return(SOX_EOF);
    }

    /* This is dumb but portable, just count the bytes til EOF */
    while (lsx_read_b_buf(ft, &trash, (size_t) 1) == 1)
        num_samp_bytes++;
    num_samp_bytes -= 32;         /* calculate num samples by sub header size */
    lsx_seeki(ft, (off_t)0, 0);   /* rewind file */
    sk->rest = num_samp_bytes;    /* set how many sample bytes to read */

    /* first 6 bytes are file type ID LM8953 */
    lsx_readchars(ft, filetype, sizeof(filetype) - 1);
    filetype[6] = '\0';
    for( c = 16; c > 0 ; c-- )    /* Discard next 16 bytes */
        lsx_readb(ft, &trash);
    lsx_readsb(ft, &format);
    lsx_readb(ft, &sample_rate);
    /*
     * save next 8 bytes - if sample rate is 0, then we need
     *  to look at gunk[2] and gunk[5] to get real rate
     */
    for( c = 0; c < 8; c++ )
        lsx_readb(ft, &(gunk[c]));
    /*
     * We should now be pointing at start of raw sample data in file
     */

    /* Check to make sure we got a good filetype ID from file */
    lsx_debug("Found header filetype %s",filetype);
    if(strcmp(filetype,"LM8953"))
    {
        lsx_fail_errno(ft,SOX_EHDR,"Invalid filetype ID in input file header, != LM8953");
        return(SOX_EOF);
    }
    /*
     * Set up the sample rate as indicated by the header
     */

    switch( sample_rate ) {
        case 1:
            ft->signal.rate = 1e5 / 3;
            break;
        case 2:
            ft->signal.rate = 1e5 / 2;
            break;
        case 3:
            ft->signal.rate = 1e5 / 6;
            break;
        default:
            blewIt = 1;
            switch( gunk[2] & 0xFE ) {
                case 0x06:
                    if ( (gunk[5] & 0xFE) == 0x52 ) {
                        blewIt = 0;
                        ft->signal.rate = 1e5 / 3;
                    }
                    break;
                case 0x10:
                    if ( (gunk[5] & 0xFE) == 0x00 ) {
                        blewIt = 0;
                        ft->signal.rate = 1e5 / 2;
                    }
                    break;
                case 0xF6:
                    if ( (gunk[5] & 0xFE) == 0x52 ) {
                        blewIt = 0;
                        ft->signal.rate = 1e5 / 6;
                    }
                    break;
            }
            if ( blewIt ) {
                lsx_debug("Invalid sample rate identifier found %d", sample_rate);
                ft->signal.rate = 1e5 / 3;
            }
    }
    lsx_debug("Sample rate = %g", ft->signal.rate);

    ft->signal.channels = 1 ; /* not sure about stereo sample data yet ??? */
    ft->encoding.bits_per_sample = 12;
    ft->encoding.encoding = SOX_ENCODING_SIGN2;

    return(SOX_SUCCESS);
}

/*
 * Read up to len samples from file.
 * Convert to sox_sample_t.
 * Place in buf[].
 * Return number of samples read.
 */

static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
    priv_t * sk = (priv_t *) ft->priv;
    size_t done = 0;
    unsigned char uc1,uc2,uc3;
    unsigned short s1,s2;

    /*
     * This gets called by the top level 'process' routine.
     * We will essentially get called with a buffer pointer
     * and a max length to read. Graciously, it is always
     * an even amount so we don't have to worry about
     * hanging onto the left over odd samples since there
     * won't be any. Something to look out for though :-(
     * We return the number of samples we read.
     * We will get called over and over again until we return
     *  0 bytes read.
     */

    /*
     * This is ugly but it's readable!
     * Read three bytes from stream, then decompose these into
     * two unsigned short samples.
     * TCC 3.0 appeared to do unwanted things, so we really specify
     *  exactly what we want to happen.
     * Convert unsigned short to sox_sample_t then shift up the result
     *  so that the 12-bit sample lives in the most significant
     *  12-bits of the sox_sample_t.
     * This gets our two samples into the internal format which we
     * deposit into the given buffer and adjust our counts respectivly.
     */
    for(done = 0; done < len; ) {
        if(sk->rest < 3) break; /* Finished reading from file? */
        lsx_readb(ft, &uc1);
        lsx_readb(ft, &uc2);
        lsx_readb(ft, &uc3);
        sk->rest -= 3; /* adjust remaining for bytes we just read */
        s1 = (unsigned short) (uc1 << 4) | (((uc2 >> 4) & 017));
        s2 = (unsigned short) (uc3 << 4) | (( uc2 & 017 ));
        *buf = (sox_sample_t) s1;
        *buf = (*buf << 20);
        buf++; /* sample one is done */
        *buf = (sox_sample_t) s2;
        *buf = (*buf << 20);
        buf++; /* sample two is done */
        done += 2; /* adjust converted & stored sample count */
    }
    return done;
}

static int startwrite(sox_format_t * ft)
{
  priv_t * sk = (priv_t *) ft->priv;
    struct WaveHeader_ WH;

    lsx_debug("tx16w selected output");

    memset(&WH, 0, sizeof(struct WaveHeader_));

    /* If you have to seek around the output file */
    if (! ft->seekable)
    {
        lsx_fail_errno(ft,SOX_EOF,"Output .txw file must be a file, not a pipe");
        return(SOX_EOF);
    }

    /* dummy numbers, just for place holder, real header is written
       at end of processing, since byte count is needed */

    lsx_writebuf(ft, &WH, (size_t) 32);
    sk->bytes_out = 32;
    return(SOX_SUCCESS);
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len0)
{
  priv_t * sk = (priv_t *) ft->priv;
  size_t last_i, i = 0, len = min(len0, TXMAXLEN - sk->samples_out);
  sox_sample_t w1, w2;

  while (i < len) {
    last_i = i;
    if (sk->odd_flag) {
      w1 = sk->odd;
      sk->odd_flag = sox_false;
    }
    else w1 = *buf++ >> 20, ++i;

    if (i < len) {
      w2 = *buf++ >> 20, ++i;
      if (lsx_writesb(ft, (w1 >> 4) & 0xFF) ||
          lsx_writesb(ft, (((w1 & 0x0F) << 4) | (w2 & 0x0F)) & 0xFF) ||
          lsx_writesb(ft, (w2 >> 4) & 0xFF)) {
        i = last_i;
        break;
      }
      sk->samples_out += 2;
      sk->bytes_out += 3;
    }
    else {
      sk->odd = w1;
      sk->odd_flag = sox_true;
    }
  }
  return i;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * sk = (priv_t *) ft->priv;
    struct WaveHeader_ WH;
    int AttackLength, LoopLength, i;

    if (sk->odd_flag) {
      sox_sample_t pad = 0;
      write_samples(ft, &pad, (size_t) 1);
    }

    /* All samples are already written out. */
    /* If file header needs fixing up, for example it needs the */
    /* the number of samples in a field, seek back and write them here. */

    lsx_debug("tx16w:output finished");

    memset(&WH, 0, sizeof(struct WaveHeader_));
    memcpy(WH.filetype, "LM8953", 6);
    for (i=0;i<10;i++) WH.nulls[i]=0;
    for (i=0;i<6;i++)  WH.dummy_aeg[i]=0;
    for (i=0;i<2;i++)  WH.unused[i]=0;
    for (i=0;i<2;i++)  WH.dummy_aeg[i] = 0;
    for (i=2;i<6;i++)  WH.dummy_aeg[i] = 0x7F;

    WH.format = 0xC9;   /* loop off */

    /* the actual sample rate is not that important ! */
    if (ft->signal.rate < 24000)      WH.sample_rate = 3;
    else if (ft->signal.rate < 41000) WH.sample_rate = 1;
    else                            WH.sample_rate = 2;

    if (sk->samples_out >= TXMAXLEN) {
        lsx_warn("Sound too large for TX16W. Truncating, Loop Off");
        AttackLength       = TXMAXLEN/2;
        LoopLength         = TXMAXLEN/2;
    }
    else if (sk->samples_out >=TXMAXLEN/2) {
        AttackLength       = TXMAXLEN/2;
        LoopLength         = sk->samples_out - TXMAXLEN/2;
        if (LoopLength < 0x40) {
            LoopLength   +=0x40;
            AttackLength -= 0x40;
        }
    }
    else if (sk->samples_out >= 0x80) {
        AttackLength                       = sk->samples_out -0x40;
        LoopLength                         = 0x40;
    }
    else {
        AttackLength                       = 0x40;
        LoopLength                         = 0x40;
        for(i=sk->samples_out;i<0x80;i++) {
            lsx_writeb(ft, 0);
            lsx_writeb(ft, 0);
            lsx_writeb(ft, 0);
            sk->bytes_out += 3;
        }
    }

    /* Fill up to 256 byte blocks; the TX16W seems to like that */

    while ((sk->bytes_out % 0x100) != 0) {
        lsx_writeb(ft, 0);
        sk->bytes_out++;
    }

    WH.atc_length[0] = 0xFF & AttackLength;
    WH.atc_length[1] = 0xFF & (AttackLength >> 8);
    WH.atc_length[2] = (0x01 & (AttackLength >> 16)) +
        magic1[WH.sample_rate];

    WH.rpt_length[0] = 0xFF & LoopLength;
    WH.rpt_length[1] = 0xFF & (LoopLength >> 8);
    WH.rpt_length[2] = (0x01 & (LoopLength >> 16)) +
        magic2[WH.sample_rate];

    lsx_rewind(ft);
    lsx_writebuf(ft, &WH, (size_t) 32);

    return(SOX_SUCCESS);
}

LSX_FORMAT_HANDLER(txw)
{
  static char const * const names[] = {"txw", NULL};
  static sox_rate_t   const write_rates[] = {1e5/6, 1e5/3, 1e5/2, 0};
  static unsigned const write_encodings[] = {SOX_ENCODING_SIGN2, 12, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Yamaha TX-16W sampler", names, SOX_FILE_MONO,
    startread, read_samples, NULL,
    startwrite, write_samples, stopwrite,
    NULL, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
