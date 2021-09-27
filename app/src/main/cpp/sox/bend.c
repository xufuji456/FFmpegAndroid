/* libSoX effect: Pitch Bend   (c) 2008 robs@users.sourceforge.net
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

/* Portions based on http://www.dspdimension.com/download smbPitchShift.cpp:
 *
 * COPYRIGHT 1999-2006 Stephan M. Bernsee <smb [AT] dspdimension [DOT] com>
 *
 *             The Wide Open License (WOL)
 *
 * Permission to use, copy, modify, distribute and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this license appear in all source copies. 
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
 * ANY KIND. See http://www.dspguru.com/wol.htm for more information.
 */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox_i.h"
#include <assert.h>

#define MAX_FRAME_LENGTH 8192

typedef struct {
  unsigned nbends;       /* Number of bends requested */
  struct {
    char *str;           /* Command-line argument to parse for this bend */
    uint64_t start;      /* Start bending when in_pos equals this */
    double cents;
    uint64_t duration;   /* Number of samples to bend */
  } *bends;

  unsigned frame_rate;
  size_t in_pos;         /* Number of samples read from the input stream */
  unsigned bends_pos;    /* Number of bends completed so far */

  double shift;

  float gInFIFO[MAX_FRAME_LENGTH];
  float gOutFIFO[MAX_FRAME_LENGTH];
  double gFFTworksp[2 * MAX_FRAME_LENGTH];
  float gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
  float gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
  float gOutputAccum[2 * MAX_FRAME_LENGTH];
  float gAnaFreq[MAX_FRAME_LENGTH];
  float gAnaMagn[MAX_FRAME_LENGTH];
  float gSynFreq[MAX_FRAME_LENGTH];
  float gSynMagn[MAX_FRAME_LENGTH];
  long gRover;
  int fftFrameSize, ovsamp;
} priv_t;

static int parse(sox_effect_t * effp, char **argv, sox_rate_t rate)
{
  priv_t *p = (priv_t *) effp->priv;
  size_t i;
  char const *next;
  uint64_t last_seen = 0;
  const uint64_t in_length = argv ? 0 :
    (effp->in_signal.length != SOX_UNKNOWN_LEN ?
     effp->in_signal.length / effp->in_signal.channels : SOX_UNKNOWN_LEN);

  for (i = 0; i < p->nbends; ++i) {
    if (argv)   /* 1st parse only */
      p->bends[i].str = lsx_strdup(argv[i]);

    next = lsx_parseposition(rate, p->bends[i].str,
             argv ? NULL : &p->bends[i].start, last_seen, in_length, '+');
    last_seen = p->bends[i].start;
    if (next == NULL || *next != ',')
      break;

    p->bends[i].cents = strtod(next + 1, (char **)&next);
    if (p->bends[i].cents == 0 || *next != ',')
      break;

    next = lsx_parseposition(rate, next + 1,
             argv ? NULL : &p->bends[i].duration, last_seen, in_length, '+');
    last_seen = p->bends[i].duration;
    if (next == NULL || *next != '\0')
      break;

    /* sanity checks */
    if (!argv && p->bends[i].duration < p->bends[i].start) {
      lsx_fail("Bend %" PRIuPTR " has negative width", i+1);
      break;
    }
    if (!argv && i && p->bends[i].start < p->bends[i-1].start) {
      lsx_fail("Bend %" PRIuPTR " overlaps with previous one", i+1);
      break;
    }

    p->bends[i].duration -= p->bends[i].start;
  }
  if (i < p->nbends)
    return lsx_usage(effp);
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char **argv)
{
  priv_t *p = (priv_t *) effp->priv;
  char const * opts = "f:o:";
  int c;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, opts, NULL, lsx_getopt_flag_none, 1, &optstate);

  p->frame_rate = 25;
  p->ovsamp = 16;
  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    GETOPT_NUMERIC(optstate, 'f', frame_rate, 10 , 80)
    GETOPT_NUMERIC(optstate, 'o', ovsamp,  4 , 32)
    default: lsx_fail("unknown option `-%c'", optstate.opt); return lsx_usage(effp);
  }
  argc -= optstate.ind, argv += optstate.ind;

  p->nbends = argc;
  p->bends = lsx_calloc(p->nbends, sizeof(*p->bends));
  return parse(effp, argv, 0.);     /* No rate yet; parse with dummy */
}

static int start(sox_effect_t * effp)
{
  priv_t *p = (priv_t *) effp->priv;
  unsigned i;

  int n = effp->in_signal.rate / p->frame_rate + .5;
  for (p->fftFrameSize = 2; n > 2; p->fftFrameSize <<= 1, n >>= 1);
  assert(p->fftFrameSize <= MAX_FRAME_LENGTH);
  p->shift = 1;
  parse(effp, 0, effp->in_signal.rate); /* Re-parse now rate is known */
  p->in_pos = p->bends_pos = 0;
  for (i = 0; i < p->nbends; ++i)
    if (p->bends[i].duration)
      return SOX_SUCCESS;
  return SOX_EFF_NULL;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
                sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t *p = (priv_t *) effp->priv;
  size_t i, len = *isamp = *osamp = min(*isamp, *osamp);
  double magn, phase, tmp, window, real, imag;
  double freqPerBin, expct;
  long k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;
  float pitchShift = p->shift;

  /* set up some handy variables */
  fftFrameSize2 = p->fftFrameSize / 2;
  stepSize = p->fftFrameSize / p->ovsamp;
  freqPerBin = effp->in_signal.rate / p->fftFrameSize;
  expct = 2. * M_PI * (double) stepSize / (double) p->fftFrameSize;
  inFifoLatency = p->fftFrameSize - stepSize;
  if (!p->gRover)
    p->gRover = inFifoLatency;

  /* main processing loop */
  for (i = 0; i < len; i++) {
    SOX_SAMPLE_LOCALS;
    ++p->in_pos;

    /* As long as we have not yet collected enough data just read in */
    p->gInFIFO[p->gRover] = SOX_SAMPLE_TO_FLOAT_32BIT(ibuf[i], effp->clips);
    obuf[i] = SOX_FLOAT_32BIT_TO_SAMPLE(
        p->gOutFIFO[p->gRover - inFifoLatency], effp->clips);
    p->gRover++;

    /* now we have enough data for processing */
    if (p->gRover >= p->fftFrameSize) {
      if (p->bends_pos != p->nbends && p->in_pos >=
          p->bends[p->bends_pos].start + p->bends[p->bends_pos].duration) {
        pitchShift = p->shift *= pow(2., p->bends[p->bends_pos].cents / 1200);
        ++p->bends_pos;
      }
      if (p->bends_pos != p->nbends && p->in_pos >= p->bends[p->bends_pos].start) {
        double progress = (double)(p->in_pos - p->bends[p->bends_pos].start) /
             p->bends[p->bends_pos].duration;
        progress = 1 - cos(M_PI * progress);
        progress *= p->bends[p->bends_pos].cents * (.5 / 1200);
        pitchShift = p->shift * pow(2., progress);
      }

      p->gRover = inFifoLatency;

      /* do windowing and re,im interleave */
      for (k = 0; k < p->fftFrameSize; k++) {
        window = -.5 * cos(2 * M_PI * k / (double) p->fftFrameSize) + .5;
        p->gFFTworksp[2 * k] = p->gInFIFO[k] * window;
        p->gFFTworksp[2 * k + 1] = 0.;
      }

      /* ***************** ANALYSIS ******************* */
      lsx_safe_cdft(2 * p->fftFrameSize, 1, p->gFFTworksp);

      /* this is the analysis step */
      for (k = 0; k <= fftFrameSize2; k++) {
        /* de-interlace FFT buffer */
        real = p->gFFTworksp[2 * k];
        imag = - p->gFFTworksp[2 * k + 1];

        /* compute magnitude and phase */
        magn = 2. * sqrt(real * real + imag * imag);
        phase = atan2(imag, real);

        /* compute phase difference */
        tmp = phase - p->gLastPhase[k];
        p->gLastPhase[k] = phase;

        tmp -= (double) k *expct; /* subtract expected phase difference */

        /* map delta phase into +/- Pi interval */
        qpd = tmp / M_PI;
        if (qpd >= 0)
          qpd += qpd & 1;
        else qpd -= qpd & 1;
        tmp -= M_PI * (double) qpd;

        /* get deviation from bin frequency from the +/- Pi interval */
        tmp = p->ovsamp * tmp / (2. * M_PI);

        /* compute the k-th partials' true frequency */
        tmp = (double) k *freqPerBin + tmp * freqPerBin;

        /* store magnitude and true frequency in analysis arrays */
        p->gAnaMagn[k] = magn;
        p->gAnaFreq[k] = tmp;

      }

      /* this does the actual pitch shifting */
      memset(p->gSynMagn, 0, p->fftFrameSize * sizeof(float));
      memset(p->gSynFreq, 0, p->fftFrameSize * sizeof(float));
      for (k = 0; k <= fftFrameSize2; k++) {
        index = k * pitchShift;
        if (index <= fftFrameSize2) {
          p->gSynMagn[index] += p->gAnaMagn[k];
          p->gSynFreq[index] = p->gAnaFreq[k] * pitchShift;
        }
      }

      for (k = 0; k <= fftFrameSize2; k++) { /* SYNTHESIS */
        /* get magnitude and true frequency from synthesis arrays */
        magn = p->gSynMagn[k], tmp = p->gSynFreq[k];
        tmp -= (double) k *freqPerBin; /* subtract bin mid frequency */
        tmp /= freqPerBin; /* get bin deviation from freq deviation */
        tmp = 2. * M_PI * tmp / p->ovsamp; /* take p->ovsamp into account */
        tmp += (double) k *expct; /* add the overlap phase advance back in */
        p->gSumPhase[k] += tmp; /* accumulate delta phase to get bin phase */
        phase = p->gSumPhase[k];
        /* get real and imag part and re-interleave */
        p->gFFTworksp[2 * k] = magn * cos(phase);
        p->gFFTworksp[2 * k + 1] = - magn * sin(phase);
      }

      for (k = p->fftFrameSize + 2; k < 2 * p->fftFrameSize; k++)
        p->gFFTworksp[k] = 0.; /* zero negative frequencies */

      lsx_safe_cdft(2 * p->fftFrameSize, -1, p->gFFTworksp);

      /* do windowing and add to output accumulator */
      for (k = 0; k < p->fftFrameSize; k++) {
        window =
            -.5 * cos(2. * M_PI * (double) k / (double) p->fftFrameSize) + .5;
        p->gOutputAccum[k] +=
            2. * window * p->gFFTworksp[2 * k] / (fftFrameSize2 * p->ovsamp);
      }
      for (k = 0; k < stepSize; k++)
        p->gOutFIFO[k] = p->gOutputAccum[k];

      memmove(p->gOutputAccum, /* shift accumulator */
          p->gOutputAccum + stepSize, p->fftFrameSize * sizeof(float));

      for (k = 0; k < inFifoLatency; k++) /* move input FIFO */
        p->gInFIFO[k] = p->gInFIFO[k + stepSize];
    }
  }
  return SOX_SUCCESS;
}

static int stop(sox_effect_t * effp)
{
  priv_t *p = (priv_t *) effp->priv;

  if (p->bends_pos != p->nbends)
    lsx_warn("Input audio too short; bends not applied: %u",
        p->nbends - p->bends_pos);
  return SOX_SUCCESS;
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t *p = (priv_t *) effp->priv;
  unsigned i;

  for (i = 0; i < p->nbends; ++i)
    free(p->bends[i].str);
  free(p->bends);
  return SOX_SUCCESS;
}

sox_effect_handler_t const *lsx_bend_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "bend", "[-f frame-rate(25)] [-o over-sample(16)] {start,cents,end}",
    0, create, start, flow, 0, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
