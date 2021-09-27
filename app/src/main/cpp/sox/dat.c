/* libSoX text format file.  Tom Littlejohn, March 93.
 *
 * Reads/writes sound files as text.
 *
 * Copyright 1998-2006 Chris Bagwell and SoX Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"
#include <string.h>

#define LINEWIDTH (size_t)256

/* Private data for dat file */
typedef struct {
    double timevalue, deltat;
    int buffered;
    char prevline[LINEWIDTH];
} priv_t;

static int sox_datstartread(sox_format_t * ft)
{
    char inpstr[LINEWIDTH];
    long rate;
    int chan;
    int status;
    char sc;

    /* Read lines until EOF or first non-comment line */
    while ((status = lsx_reads(ft, inpstr, LINEWIDTH-1)) != SOX_EOF) {
      inpstr[LINEWIDTH-1] = 0;
      if ((sscanf(inpstr," %c", &sc) != 0) && (sc != ';')) break;
      if (sscanf(inpstr," ; Sample Rate %ld", &rate)) {
        ft->signal.rate=rate;
      } else if (sscanf(inpstr," ; Channels %d", &chan)) {
        ft->signal.channels=chan;
      }
    }
    /* Hold a copy of the last line we read (first non-comment) */
    if (status != SOX_EOF) {
      strncpy(((priv_t *)ft->priv)->prevline, inpstr, (size_t)LINEWIDTH);
      ((priv_t *)ft->priv)->buffered = 1;
    } else {
      ((priv_t *)ft->priv)->buffered = 0;
    }

    /* Default channels to 1 if not found */
    if (ft->signal.channels == 0)
       ft->signal.channels = 1;

    ft->encoding.encoding = SOX_ENCODING_FLOAT_TEXT;

    return (SOX_SUCCESS);
}

static int sox_datstartwrite(sox_format_t * ft)
{
    priv_t * dat = (priv_t *) ft->priv;
    char s[LINEWIDTH];

    dat->timevalue = 0.0;
    dat->deltat = 1.0 / (double)ft->signal.rate;
    /* Write format comments to start of file */
    sprintf(s,"; Sample Rate %ld\015\n", (long)ft->signal.rate);
    lsx_writes(ft, s);
    sprintf(s,"; Channels %d\015\n", (int)ft->signal.channels);
    lsx_writes(ft, s);

    return (SOX_SUCCESS);
}

static size_t sox_datread(sox_format_t * ft, sox_sample_t *buf, size_t nsamp)
{
    char inpstr[LINEWIDTH];
    int  inpPtr = 0;
    int  inpPtrInc = 0;
    double sampval = 0.0;
    int retc = 0;
    char sc = 0;
    size_t done = 0;
    size_t i=0;

    /* Always read a complete set of channels */
    nsamp -= (nsamp % ft->signal.channels);

    while (done < nsamp) {

      /* Read a line or grab the buffered first line */
      if (((priv_t *)ft->priv)->buffered) {
        strncpy(inpstr, ((priv_t *)ft->priv)->prevline, (size_t)LINEWIDTH);
        inpstr[LINEWIDTH-1] = 0;
        ((priv_t *)ft->priv)->buffered=0;
      } else {
        lsx_reads(ft, inpstr, LINEWIDTH-1);
        inpstr[LINEWIDTH-1] = 0;
        if (lsx_eof(ft)) return (done);
      }

      /* Skip over comments - ie. 0 or more whitespace, then ';' */
      if ((sscanf(inpstr," %c", &sc) != 0) && (sc==';')) continue;

      /* Read a complete set of channels */
      sscanf(inpstr," %*s%n", &inpPtr);
      for (i=0; i<ft->signal.channels; i++) {
        SOX_SAMPLE_LOCALS;
        retc = sscanf(&inpstr[inpPtr]," %lg%n", &sampval, &inpPtrInc);
        inpPtr += inpPtrInc;
        if (retc != 1) {
          lsx_fail_errno(ft,SOX_EOF,"Unable to read sample.");
          return 0;
        }
        *buf++ = SOX_FLOAT_64BIT_TO_SAMPLE(sampval, ft->clips);
        done++;
      }
    }

    return (done);
}

static size_t sox_datwrite(sox_format_t * ft, const sox_sample_t *buf, size_t nsamp)
{
    priv_t * dat = (priv_t *) ft->priv;
    size_t done = 0;
    double sampval=0.0;
    char s[LINEWIDTH];
    size_t i=0;

    /* Always write a complete set of channels */
    nsamp -= (nsamp % ft->signal.channels);

    /* Write time, then sample values, then CRLF newline */
    while(done < nsamp) {
      sprintf(s," %15.8g ",dat->timevalue);
      lsx_writes(ft, s);
      for (i=0; i<ft->signal.channels; i++) {
        sampval = SOX_SAMPLE_TO_FLOAT_64BIT(*buf++, ft->clips);
        sprintf(s," %15.11g", sampval);
        lsx_writes(ft, s);
        done++;
      }
      sprintf(s," \r\n");
      lsx_writes(ft, s);
      dat->timevalue += dat->deltat;
    }
    return done;
}

LSX_FORMAT_HANDLER(dat)
{
  static char const * const names[] = {"dat", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_FLOAT_TEXT, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Textual representation of the sampled audio", names, 0,
    sox_datstartread, sox_datread, NULL,
    sox_datstartwrite, sox_datwrite, NULL,
    NULL, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
