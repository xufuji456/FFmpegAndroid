/* libSoX effect: Voice Activity Detector  (c) 2009 robs@users.sourceforge.net
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

typedef struct {
  double    * dftBuf, * noiseSpectrum, * spectrum, * measures, meanMeas;
} chan_t;

typedef struct {                /* Configuration parameters: */
  double    bootTime, noiseTcUp, noiseTcDown, noiseReductionAmount;
  double    measureFreq, measureDuration, measureTc, preTriggerTime;
  double    hpFilterFreq, lpFilterFreq, hpLifterFreq, lpLifterFreq;
  double    triggerTc, triggerLevel, searchTime, gapTime;
                                /* Working variables: */
  sox_sample_t  * samples;
  unsigned  dftLen_ws, samplesLen_ns, samplesIndex_ns, flushedLen_ns, gapLen;
  unsigned  measurePeriod_ns, measuresLen, measuresIndex;
  unsigned  measureTimer_ns, measureLen_ws, measureLen_ns;
  unsigned  spectrumStart, spectrumEnd, cepstrumStart, cepstrumEnd; /* bins */
  int       bootCountMax, bootCount;
  double    noiseTcUpMult, noiseTcDownMult;
  double    measureTcMult, triggerMeasTcMult;
  double    * spectrumWindow, * cepstrumWindow;
  chan_t    * channels;
} priv_t;

#define GETOPT_FREQ(optstate, c, name, min) \
    case c: p->name = lsx_parse_frequency(optstate.arg, &parseIndex); \
      if (p->name < min || *parseIndex) return lsx_usage(effp); \
      break;

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  #define opt_str "+b:N:n:r:f:m:M:h:l:H:L:T:t:s:g:p:"
  int c;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, opt_str, NULL, lsx_getopt_flag_none, 1, &optstate);

  p->bootTime        = .35;
  p->noiseTcUp       = .1;
  p->noiseTcDown     = .01;
  p->noiseReductionAmount = 1.35;

  p->measureFreq     = 20;
  p->measureDuration = 2 / p->measureFreq; /* 50% overlap */
  p->measureTc       = .4;

  p->hpFilterFreq    = 50;
  p->lpFilterFreq    = 6000;
  p->hpLifterFreq    = 150;
  p->lpLifterFreq    = 2000;

  p->triggerTc       = .25;
  p->triggerLevel    = 7;

  p->searchTime      = 1;
  p->gapTime         = .25;

  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    char * parseIndex;
    GETOPT_NUMERIC(optstate, 'b', bootTime      ,  .1 , 10)
    GETOPT_NUMERIC(optstate, 'N', noiseTcUp     ,  .1 , 10)
    GETOPT_NUMERIC(optstate, 'n', noiseTcDown   ,.001 , .1)
    GETOPT_NUMERIC(optstate, 'r', noiseReductionAmount,0 , 2)
    GETOPT_NUMERIC(optstate, 'f', measureFreq   ,   5 , 50)
    GETOPT_NUMERIC(optstate, 'm', measureDuration, .01 , 1)
    GETOPT_NUMERIC(optstate, 'M', measureTc     ,  .1 , 1)
    GETOPT_FREQ(   optstate, 'h', hpFilterFreq  ,  10)
    GETOPT_FREQ(   optstate, 'l', lpFilterFreq  ,  1000)
    GETOPT_FREQ(   optstate, 'H', hpLifterFreq  ,  10)
    GETOPT_FREQ(   optstate, 'L', lpLifterFreq  ,  1000)
    GETOPT_NUMERIC(optstate, 'T', triggerTc     , .01 , 1)
    GETOPT_NUMERIC(optstate, 't', triggerLevel  ,   0 , 20)
    GETOPT_NUMERIC(optstate, 's', searchTime    ,  .1 , 4)
    GETOPT_NUMERIC(optstate, 'g', gapTime       ,  .1 , 1)
    GETOPT_NUMERIC(optstate, 'p', preTriggerTime,   0 , 4)
    default: lsx_fail("invalid option `-%c'", optstate.opt); return lsx_usage(effp);
  }
  return optstate.ind !=argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i, fixedPreTriggerLen_ns, searchPreTriggerLen_ns;

  fixedPreTriggerLen_ns = p->preTriggerTime * effp->in_signal.rate + .5;
  fixedPreTriggerLen_ns *= effp->in_signal.channels;

  p->measureLen_ws = effp->in_signal.rate * p->measureDuration + .5;
  p->measureLen_ns = p->measureLen_ws * effp->in_signal.channels;
  for (p->dftLen_ws = 16; p->dftLen_ws < p->measureLen_ws; p->dftLen_ws <<= 1);
  lsx_debug("dftLen_ws=%u measureLen_ws=%u", p->dftLen_ws, p->measureLen_ws);

  p->measurePeriod_ns = effp->in_signal.rate / p->measureFreq + .5;
  p->measurePeriod_ns *= effp->in_signal.channels;
  p->measuresLen = ceil(p->searchTime * p->measureFreq);
  searchPreTriggerLen_ns = p->measuresLen * p->measurePeriod_ns;
  p->gapLen = p->gapTime * p->measureFreq + .5;

  p->samplesLen_ns =
    fixedPreTriggerLen_ns + searchPreTriggerLen_ns + p->measureLen_ns;
  lsx_Calloc(p->samples, p->samplesLen_ns);

  lsx_Calloc(p->channels, effp->in_signal.channels);
  for (i = 0; i < effp->in_signal.channels; ++i) {
    chan_t * c = &p->channels[i];
    lsx_Calloc(c->dftBuf, p->dftLen_ws);
    lsx_Calloc(c->spectrum, p->dftLen_ws);
    lsx_Calloc(c->noiseSpectrum, p->dftLen_ws);
    lsx_Calloc(c->measures, p->measuresLen);
  }

  lsx_Calloc(p->spectrumWindow, p->measureLen_ws);
  for (i = 0; i < p->measureLen_ws; ++i)
    p->spectrumWindow[i] = -2./ SOX_SAMPLE_MIN / sqrt((double)p->measureLen_ws);
  lsx_apply_hann(p->spectrumWindow, (int)p->measureLen_ws);

  p->spectrumStart = p->hpFilterFreq / effp->in_signal.rate * p->dftLen_ws + .5;
  p->spectrumStart = max(p->spectrumStart, 1);
  p->spectrumEnd = p->lpFilterFreq / effp->in_signal.rate * p->dftLen_ws + .5;
  p->spectrumEnd = min(p->spectrumEnd, p->dftLen_ws / 2);

  lsx_Calloc(p->cepstrumWindow, p->spectrumEnd - p->spectrumStart);
  for (i = 0; i < p->spectrumEnd - p->spectrumStart; ++i)
    p->cepstrumWindow[i] = 2 / sqrt((double)p->spectrumEnd - p->spectrumStart);
  lsx_apply_hann(p->cepstrumWindow,(int)(p->spectrumEnd - p->spectrumStart));

  p->cepstrumStart = ceil(effp->in_signal.rate * .5 / p->lpLifterFreq);
  p->cepstrumEnd  = floor(effp->in_signal.rate * .5 / p->hpLifterFreq);
  p->cepstrumEnd = min(p->cepstrumEnd, p->dftLen_ws / 4);
  if (p->cepstrumEnd <= p->cepstrumStart)
    return SOX_EOF;

  p->noiseTcUpMult     = exp(-1 / (p->noiseTcUp   * p->measureFreq));
  p->noiseTcDownMult   = exp(-1 / (p->noiseTcDown * p->measureFreq));
  p->measureTcMult     = exp(-1 / (p->measureTc   * p->measureFreq));
  p->triggerMeasTcMult = exp(-1 / (p->triggerTc   * p->measureFreq));

  p->bootCountMax = p->bootTime * p->measureFreq - .5;
  p->measureTimer_ns = p->measureLen_ns;
  p->bootCount = p->measuresIndex = p->flushedLen_ns = p->samplesIndex_ns = 0;

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* depends on input data */
  return SOX_SUCCESS;
}

static int flowFlush(sox_effect_t * effp, sox_sample_t const * ibuf,
    sox_sample_t * obuf, size_t * ilen, size_t * olen)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t odone = min(p->samplesLen_ns - p->flushedLen_ns, *olen);
  size_t odone1 = min(odone, p->samplesLen_ns - p->samplesIndex_ns);

  memcpy(obuf, p->samples + p->samplesIndex_ns, odone1 * sizeof(*obuf));
  if ((p->samplesIndex_ns += odone1) == p->samplesLen_ns) {
    memcpy(obuf + odone1, p->samples, (odone - odone1) * sizeof(*obuf));
    p->samplesIndex_ns = odone - odone1;
  }
  if ((p->flushedLen_ns += odone) == p->samplesLen_ns) {
    size_t olen1 = *olen - odone;
    (effp->handler.flow = lsx_flow_copy)(effp, ibuf, obuf +odone, ilen, &olen1);
    odone += olen1;
  }
  else *ilen = 0;
  *olen = odone;
  return SOX_SUCCESS;
}

static double measure(
    priv_t * p, chan_t * c, size_t index_ns, unsigned step_ns, int bootCount)
{
  double mult, result = 0;
  size_t i;

  for (i = 0; i < p->measureLen_ws; ++i, index_ns = (index_ns + step_ns) % p->samplesLen_ns)
    c->dftBuf[i] = p->samples[index_ns] * p->spectrumWindow[i];
  memset(c->dftBuf + i, 0, (p->dftLen_ws - i) * sizeof(*c->dftBuf));
  lsx_safe_rdft((int)p->dftLen_ws, 1, c->dftBuf);

  memset(c->dftBuf, 0, p->spectrumStart * sizeof(*c->dftBuf));
  for (i = p->spectrumStart; i < p->spectrumEnd; ++i) {
    double d = sqrt(sqr(c->dftBuf[2 * i]) + sqr(c->dftBuf[2 * i + 1]));
    mult = bootCount >= 0? bootCount / (1. + bootCount) : p->measureTcMult;
    c->spectrum[i] = c->spectrum[i] * mult + d * (1 - mult);
    d = sqr(c->spectrum[i]);
    mult = bootCount >= 0? 0 :
        d > c->noiseSpectrum[i]? p->noiseTcUpMult : p->noiseTcDownMult;
    c->noiseSpectrum[i] = c->noiseSpectrum[i] * mult + d * (1 - mult);
    d = sqrt(max(0, d - p->noiseReductionAmount * c->noiseSpectrum[i]));
    c->dftBuf[i] = d * p->cepstrumWindow[i - p->spectrumStart];
  }
  memset(c->dftBuf + i, 0, ((p->dftLen_ws >> 1) - i) * sizeof(*c->dftBuf));
  lsx_safe_rdft((int)p->dftLen_ws >> 1, 1, c->dftBuf);

  for (i = p->cepstrumStart; i < p->cepstrumEnd; ++i)
    result += sqr(c->dftBuf[2 * i]) + sqr(c->dftBuf[2 * i + 1]);
  result = log(result / (p->cepstrumEnd - p->cepstrumStart));
  return max(0, 21 + result);
}

static int flowTrigger(sox_effect_t * effp, sox_sample_t const * ibuf,
    sox_sample_t * obuf, size_t * ilen, size_t * olen)
{
  priv_t * p = (priv_t *)effp->priv;
  sox_bool hasTriggered = sox_false;
  size_t i, idone = 0, numMeasuresToFlush = 0;

  while (idone < *ilen && !hasTriggered) {
    p->measureTimer_ns -= effp->in_signal.channels;
    for (i = 0; i < effp->in_signal.channels; ++i, ++idone) {
      chan_t * c = &p->channels[i];
      p->samples[p->samplesIndex_ns++] = *ibuf++;
      if (!p->measureTimer_ns) {
        size_t x = (p->samplesIndex_ns + p->samplesLen_ns - p->measureLen_ns) % p->samplesLen_ns;
        double meas = measure(p, c, x, effp->in_signal.channels, p->bootCount);
        c->measures[p->measuresIndex] = meas;
        c->meanMeas = c->meanMeas * p->triggerMeasTcMult +
            meas *(1 - p->triggerMeasTcMult);

        if (hasTriggered |= c->meanMeas >= p->triggerLevel) {
          unsigned n = p->measuresLen, k = p->measuresIndex;
          unsigned j, jTrigger = n, jZero = n;
          for (j = 0; j < n; ++j, k = (k + n - 1) % n)
            if (c->measures[k] >= p->triggerLevel && j <= jTrigger + p->gapLen)
              jZero = jTrigger = j;
            else if (!c->measures[k] && jTrigger >= jZero)
              jZero = j;
          j = min(j, jZero);
          numMeasuresToFlush = range_limit(j, numMeasuresToFlush, n);
        }
        lsx_debug_more("%12g %12g %u",
            meas, c->meanMeas, (unsigned)numMeasuresToFlush);
      }
    }
    if (p->samplesIndex_ns == p->samplesLen_ns)
      p->samplesIndex_ns = 0;
    if (!p->measureTimer_ns) {
      p->measureTimer_ns = p->measurePeriod_ns;
      ++p->measuresIndex;
      p->measuresIndex %= p->measuresLen;
      if (p->bootCount >= 0)
        p->bootCount = p->bootCount == p->bootCountMax? -1 : p->bootCount + 1;
    }
  }
  if (hasTriggered) {
    size_t ilen1 = *ilen - idone;
    p->flushedLen_ns = (p->measuresLen - numMeasuresToFlush) * p->measurePeriod_ns;
    p->samplesIndex_ns = (p->samplesIndex_ns + p->flushedLen_ns) % p->samplesLen_ns;
    (effp->handler.flow = flowFlush)(effp, ibuf, obuf, &ilen1, olen);
    idone += ilen1;
  }
  else *olen = 0;
  *ilen = idone;
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * olen)
{
  size_t ilen = 0;
  return effp->handler.flow(effp, NULL, obuf, &ilen, olen);
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;

  for (i = 0; i < effp->in_signal.channels; ++i) {
    chan_t * c = &p->channels[i];
    free(c->measures);
    free(c->noiseSpectrum);
    free(c->spectrum);
    free(c->dftBuf);
  }
  free(p->channels);
  free(p->cepstrumWindow);
  free(p->spectrumWindow);
  free(p->samples);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_vad_effect_fn(void)
{
  static sox_effect_handler_t handler = {"vad", NULL,
    SOX_EFF_MCHAN | SOX_EFF_LENGTH | SOX_EFF_MODIFY,
    create, start, flowTrigger, drain, stop, NULL, sizeof(priv_t)
  };
  static char const * lines[] = {
    "[options]",
    "\t-t trigger-level                (7)",
    "\t-T trigger-time-constant        (0.25 s)",
    "\t-s search-time                  (1 s)",
    "\t-g allowed-gap                  (0.25 s)",
    "\t-p pre-trigger-time             (0 s)",
    "Advanced options:",
    "\t-b noise-est-boot-time          (0.35 s)",
    "\t-N noise-est-time-constant-up   (0.1 s)",
    "\t-n noise-est-time-constant-down (0.01 s)",
    "\t-r noise-reduction-amount       (1.35)",
    "\t-f measurement-frequency        (20 Hz)",
    "\t-m measurement-duration         (0.1 s)",
    "\t-M measurement-time-constant    (0.4 s)",
    "\t-h high-pass-filter             (50 Hz)",
    "\t-l low-pass-filter              (6000 Hz)",
    "\t-H high-pass-lifter             (150 Hz)",
    "\t-L low-pass-lifter              (2000 Hz)",
  };
  static char * usage;
  handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));
  return &handler;
}
