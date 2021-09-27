/* multiband compander effect for SoX
 * by Daniel Pouzzner <douzzer@mega.nu> 2002-Oct-8
 *
 * Compander code adapted from the SoX compand effect, by Nick Bailey
 *
 * SoX is Copyright 1999 Chris Bagwell And Nick Bailey This source code is
 * freely redistributable and may be used for any purpose.  This copyright
 * notice must be maintained.  Chris Bagwell And Nick Bailey are not
 * responsible for the consequences of using this software.
 *
 *
 * Usage:
 *   mcompand quoted_compand_args [crossover_frequency
 *      quoted_compand_args [...]]
 *
 *   quoted_compand_args are as for the compand effect:
 *
 *   attack1,decay1[,attack2,decay2...]
 *                  in-dB1,out-dB1[,in-dB2,out-dB2...]
 *                 [ gain [ initial-volume [ delay ] ] ]
 *
 *   Beware a variety of headroom (clipping) bugaboos.
 *
 * Implementation details:
 *   The input is divided into bands using 4th order Linkwitz-Riley IIRs.
 *   This is akin to the crossover of a loudspeaker, and results in flat
 *   frequency response absent compander action.
 *
 *   The outputs of the array of companders is summed, and sample truncation
 *   is done on the final sum.
 *
 *   Modifications to the predictive compression code properly maintain
 *   alignment of the outputs of the array of companders when the companders
 *   have different prediction intervals (volume application delays).  Note
 *   that the predictive mode of the limiter needs some TLC - in fact, a
 *   rewrite - since what's really useful is to assure that a waveform won't
 *   be clipped, by slewing the volume in advance so that the peak is at
 *   limit (or below, if there's a higher subsequent peak visible in the
 *   lookahead window) once it's reached.  */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox_i.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "compandt.h"
#include "mcompand_xover.h"

typedef struct {
  sox_compandt_t transfer_fn;

  size_t expectedChannels; /* Also flags that channels aren't to be treated
                           individually when = 1 and input not mono */
  double *attackRate;   /* An array of attack rates */
  double *decayRate;    /*    ... and of decay rates */
  double *volume;       /* Current "volume" of each channel */
  double delay;         /* Delay to apply before companding */
  double topfreq;       /* upper bound crossover frequency */
  crossover_t filter;
  sox_sample_t *delay_buf;   /* Old samples, used for delay processing */
  size_t delay_size;    /* lookahead for this band (in samples) - function of delay, above */
  ptrdiff_t delay_buf_ptr; /* Index into delay_buf */
  size_t delay_buf_cnt; /* No. of active entries in delay_buf */
} comp_band_t;

typedef struct {
  size_t nBands;
  sox_sample_t *band_buf1, *band_buf2, *band_buf3;
  size_t band_buf_len;
  size_t delay_buf_size;/* Size of delay_buf in samples */
  comp_band_t *bands;

  char *arg; /* copy of current argument */
} priv_t;

/*
 * Process options
 *
 * Don't do initialization now.
 * The 'info' fields are not yet filled in.
 */
static int sox_mcompand_getopts_1(comp_band_t * l, size_t n, char **argv)
{
      char *s;
      size_t rates, i, commas;

      /* Start by checking the attack and decay rates */

      for (s = argv[0], commas = 0; *s; ++s)
        if (*s == ',') ++commas;

      if (commas % 2 == 0) /* There must be an even number of
                              attack/decay parameters */
      {
        lsx_fail("compander: Odd number of attack & decay rate parameters");
        return (SOX_EOF);
      }

      rates = 1 + commas/2;
      l->attackRate = lsx_malloc(sizeof(double) * rates);
      l->decayRate  = lsx_malloc(sizeof(double) * rates);
      l->volume = lsx_malloc(sizeof(double) * rates);
      l->expectedChannels = rates;
      l->delay_buf = NULL;

      /* Now tokenise the rates string and set up these arrays.  Keep
         them in seconds at the moment: we don't know the sample rate yet. */

      s = strtok(argv[0], ","); i = 0;
      do {
        l->attackRate[i] = atof(s); s = strtok(NULL, ",");
        l->decayRate[i]  = atof(s); s = strtok(NULL, ",");
        ++i;
      } while (s != NULL);

      if (!lsx_compandt_parse(&l->transfer_fn, argv[1], n>2 ? argv[2] : 0))
        return SOX_EOF;

      /* Set the initial "volume" to be attibuted to the input channels.
         Unless specified, choose 1.0 (maximum) otherwise clipping will
         result if the user has seleced a long attack time */
      for (i = 0; i < l->expectedChannels; ++i) {
        double v = n>=4 ? pow(10.0, atof(argv[3])/20) : 1.0;
        l->volume[i] = v;

        /* If there is a delay, store it. */
        if (n >= 5) l->delay = atof(argv[4]);
        else l->delay = 0.0;
      }
    return (SOX_SUCCESS);
}

static int parse_subarg(char *s, char **subargv, size_t *subargc) {
  char **ap;
  char *s_p;

  s_p = s;
  *subargc = 0;
  for (ap = subargv; (*ap = strtok(s_p, " \t")) != NULL;) {
    s_p = NULL;
    if (*subargc == 5) {
      ++*subargc;
      break;
    }
    if (**ap != '\0') {
      ++ap;
      ++*subargc;
    }
  }

  if (*subargc < 2 || *subargc > 5)
    {
      lsx_fail("Wrong number of parameters for the compander effect within mcompand; usage:\n"
  "\tattack1,decay1{,attack2,decay2} [soft-knee-dB:]in-dB1[,out-dB1]{,in-dB2,out-dB2} [gain [initial-volume-dB [delay]]]\n"
  "\twhere {} means optional and repeatable and [] means optional.\n"
  "\tdB values are floating point or -inf'; times are in seconds.");
      return (SOX_EOF);
    } else
      return SOX_SUCCESS;
}

static int getopts(sox_effect_t * effp, int argc, char **argv)
{
  char *subargv[6], *cp;
  size_t subargc, i;

  priv_t * c = (priv_t *) effp->priv;
  --argc, ++argv;

  c->band_buf1 = c->band_buf2 = c->band_buf3 = 0;
  c->band_buf_len = 0;

  /* how many bands? */
  if (! (argc&1)) {
    lsx_fail("mcompand accepts only an odd number of arguments:\argc"
            "  mcompand quoted_compand_args [crossover_freq quoted_compand_args [...]]");
    return SOX_EOF;
  }
  c->nBands = (argc+1)>>1;

  c->bands = lsx_calloc(c->nBands, sizeof(comp_band_t));

  for (i=0;i<c->nBands;++i) {
    c->arg = lsx_strdup(argv[i<<1]);
    if (parse_subarg(c->arg,subargv,&subargc) != SOX_SUCCESS)
      return SOX_EOF;
    if (sox_mcompand_getopts_1(&c->bands[i], subargc, &subargv[0]) != SOX_SUCCESS)
      return SOX_EOF;
    free(c->arg);
    c->arg = NULL;
    if (i == (c->nBands-1))
      c->bands[i].topfreq = 0;
    else {
      c->bands[i].topfreq = lsx_parse_frequency(argv[(i<<1)+1],&cp);
      if (*cp) {
        lsx_fail("bad frequency in args to mcompand");
        return SOX_EOF;
      }
      if ((i>0) && (c->bands[i].topfreq < c->bands[i-1].topfreq)) {
        lsx_fail("mcompand crossover frequencies must be in ascending order.");
        return SOX_EOF;
      }
    }
  }

  return SOX_SUCCESS;
}

/*
 * Prepare processing.
 * Do all initializations.
 */
static int start(sox_effect_t * effp)
{
  priv_t * c = (priv_t *) effp->priv;
  comp_band_t * l;
  size_t i;
  size_t band;

  for (band=0;band<c->nBands;++band) {
    l = &c->bands[band];
    l->delay_size = c->bands[band].delay * effp->out_signal.rate * effp->out_signal.channels;
    if (l->delay_size > c->delay_buf_size)
      c->delay_buf_size = l->delay_size;
  }

  for (band=0;band<c->nBands;++band) {
    l = &c->bands[band];
    /* Convert attack and decay rates using number of samples */

    for (i = 0; i < l->expectedChannels; ++i) {
      if (l->attackRate[i] > 1.0/effp->out_signal.rate)
        l->attackRate[i] = 1.0 -
          exp(-1.0/(effp->out_signal.rate * l->attackRate[i]));
      else
        l->attackRate[i] = 1.0;
      if (l->decayRate[i] > 1.0/effp->out_signal.rate)
        l->decayRate[i] = 1.0 -
          exp(-1.0/(effp->out_signal.rate * l->decayRate[i]));
      else
        l->decayRate[i] = 1.0;
    }

    /* Allocate the delay buffer */
    if (c->delay_buf_size > 0)
      l->delay_buf = lsx_calloc(sizeof(long), c->delay_buf_size);
    l->delay_buf_ptr = 0;
    l->delay_buf_cnt = 0;

    if (l->topfreq != 0)
      crossover_setup(effp, &l->filter, l->topfreq);
  }
  return (SOX_SUCCESS);
}

/*
 * Update a volume value using the given sample
 * value, the attack rate and decay rate
 */

static void doVolume(double *v, double samp, comp_band_t * l, size_t chan)
{
  double s = samp/(~((sox_sample_t)1<<31));
  double delta = s - *v;

  if (delta > 0.0) /* increase volume according to attack rate */
    *v += delta * l->attackRate[chan];
  else             /* reduce volume according to decay rate */
    *v += delta * l->decayRate[chan];
}

static int sox_mcompand_flow_1(sox_effect_t * effp, priv_t * c, comp_band_t * l, const sox_sample_t *ibuf, sox_sample_t *obuf, size_t len, size_t filechans)
{
  size_t idone, odone;

  for (idone = 0, odone = 0; idone < len; ibuf += filechans) {
    size_t chan;

    /* Maintain the volume fields by simulating a leaky pump circuit */

    if (l->expectedChannels == 1 && filechans > 1) {
      /* User is expecting same compander for all channels */
      double maxsamp = 0.0;
      for (chan = 0; chan < filechans; ++chan) {
        double rect = fabs((double)ibuf[chan]);
        if (rect > maxsamp)
          maxsamp = rect;
      }
      doVolume(&l->volume[0], maxsamp, l, (size_t) 0);
    } else {
      for (chan = 0; chan < filechans; ++chan)
        doVolume(&l->volume[chan], fabs((double)ibuf[chan]), l, chan);
    }

    /* Volume memory is updated: perform compand */
    for (chan = 0; chan < filechans; ++chan) {
      int ch = l->expectedChannels > 1 ? chan : 0;
      double level_in_lin = l->volume[ch];
      double level_out_lin = lsx_compandt(&l->transfer_fn, level_in_lin);
      double checkbuf;

      if (c->delay_buf_size <= 0) {
        checkbuf = ibuf[chan] * level_out_lin;
        SOX_SAMPLE_CLIP_COUNT(checkbuf, effp->clips);
        obuf[odone++] = checkbuf;
        idone++;
      } else {
        /* FIXME: note that this lookahead algorithm is really lame:
           the response to a peak is released before the peak
           arrives. */

        /* because volume application delays differ band to band, but
           total delay doesn't, the volume is applied in an iteration
           preceding that in which the sample goes to obuf, except in
           the band(s) with the longest vol app delay.

           the offset between delay_buf_ptr and the sample to apply
           vol to, is a constant equal to the difference between this
           band's delay and the longest delay of all the bands. */

        if (l->delay_buf_cnt >= l->delay_size) {
          checkbuf = l->delay_buf[(l->delay_buf_ptr + c->delay_buf_size - l->delay_size)%c->delay_buf_size] * level_out_lin;
          SOX_SAMPLE_CLIP_COUNT(checkbuf, effp->clips);
          l->delay_buf[(l->delay_buf_ptr + c->delay_buf_size - l->delay_size)%c->delay_buf_size] = checkbuf;
        }
        if (l->delay_buf_cnt >= c->delay_buf_size) {
          obuf[odone] = l->delay_buf[l->delay_buf_ptr];
          odone++;
          idone++;
        } else {
          l->delay_buf_cnt++;
          idone++; /* no "odone++" because we did not fill obuf[...] */
        }
        l->delay_buf[l->delay_buf_ptr++] = ibuf[chan];
        l->delay_buf_ptr %= c->delay_buf_size;
      }
    }
  }

  if (idone != odone || idone != len) {
    /* Emergency brake - will lead to memory corruption otherwise since we
       cannot report back to flow() how many samples were consumed/emitted.
       Additionally, flow() doesn't know how to handle diverging
       sub-compander delays. */
    lsx_fail("Using a compander delay within mcompand is currently not supported");
    exit(1);
    /* FIXME */
  }

  return (SOX_SUCCESS);
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
static int flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                     size_t *isamp, size_t *osamp) {
  priv_t * c = (priv_t *) effp->priv;
  comp_band_t * l;
  size_t len = min(*isamp, *osamp);
  size_t band, i;
  sox_sample_t *abuf, *bbuf, *cbuf, *oldabuf, *ibuf_copy;
  double out;

  if (c->band_buf_len < len) {
    c->band_buf1 = lsx_realloc(c->band_buf1,len*sizeof(sox_sample_t));
    c->band_buf2 = lsx_realloc(c->band_buf2,len*sizeof(sox_sample_t));
    c->band_buf3 = lsx_realloc(c->band_buf3,len*sizeof(sox_sample_t));
    c->band_buf_len = len;
  }

  len -= len % effp->out_signal.channels;

  ibuf_copy = lsx_malloc(*isamp * sizeof(sox_sample_t));
  memcpy(ibuf_copy, ibuf, *isamp * sizeof(sox_sample_t));

  /* split ibuf into bands using filters, pipe each band through sox_mcompand_flow_1, then add back together and write to obuf */

  memset(obuf,0,len * sizeof *obuf);
  for (band=0,abuf=ibuf_copy,bbuf=c->band_buf2,cbuf=c->band_buf1;band<c->nBands;++band) {
    l = &c->bands[band];

    if (l->topfreq)
      crossover_flow(effp, &l->filter, abuf, bbuf, cbuf, len);
    else {
      bbuf = abuf;
      abuf = cbuf;
    }
    if (abuf == ibuf_copy)
      abuf = c->band_buf3;
    (void)sox_mcompand_flow_1(effp, c,l,bbuf,abuf,len, (size_t)effp->out_signal.channels);
    for (i=0;i<len;++i)
    {
      out = (double)obuf[i] + (double)abuf[i];
      SOX_SAMPLE_CLIP_COUNT(out, effp->clips);
      obuf[i] = out;
    }
    oldabuf = abuf;
    abuf = cbuf;
    cbuf = oldabuf;
  }

  *isamp = *osamp = len;

  free(ibuf_copy);

  return SOX_SUCCESS;
}

static int sox_mcompand_drain_1(sox_effect_t * effp, priv_t * c, comp_band_t * l, sox_sample_t *obuf, size_t maxdrain)
{
  size_t done;
  double out;

  /*
   * Drain out delay samples.  Note that this loop does all channels.
   */
  for (done = 0;  done < maxdrain  &&  l->delay_buf_cnt > 0;  done++) {
    out = obuf[done] + l->delay_buf[l->delay_buf_ptr++];
    SOX_SAMPLE_CLIP_COUNT(out, effp->clips);
    obuf[done] = out;
    l->delay_buf_ptr %= c->delay_buf_size;
    l->delay_buf_cnt--;
  }

  /* tell caller number of samples played */
  return done;

}

/*
 * Drain out compander delay lines.
 */
static int drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
  size_t band, drained, mostdrained = 0;
  priv_t * c = (priv_t *)effp->priv;
  comp_band_t * l;

  *osamp -= *osamp % effp->out_signal.channels;

  memset(obuf,0,*osamp * sizeof *obuf);
  for (band=0;band<c->nBands;++band) {
    l = &c->bands[band];
    drained = sox_mcompand_drain_1(effp, c,l,obuf,*osamp);
    if (drained > mostdrained)
      mostdrained = drained;
  }

  *osamp = mostdrained;

  if (mostdrained)
      return SOX_SUCCESS;
  else
      return SOX_EOF;
}

/*
 * Clean up compander effect.
 */
static int stop(sox_effect_t * effp)
{
  priv_t * c = (priv_t *) effp->priv;
  comp_band_t * l;
  size_t band;

  free(c->band_buf1);
  c->band_buf1 = NULL;
  free(c->band_buf2);
  c->band_buf2 = NULL;
  free(c->band_buf3);
  c->band_buf3 = NULL;

  for (band = 0; band < c->nBands; band++) {
    l = &c->bands[band];
    free(l->delay_buf);
    if (l->topfreq != 0)
      free(l->filter.previous);
  }

  return SOX_SUCCESS;
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * c = (priv_t *) effp->priv;
  comp_band_t * l;
  size_t band;

  for (band = 0; band < c->nBands; band++) {
    l = &c->bands[band];
    lsx_compandt_kill(&l->transfer_fn);
    free(l->decayRate);
    free(l->attackRate);
    free(l->volume);
  }
  free(c->arg);
  free(c->bands);
  c->bands = NULL;

  return SOX_SUCCESS;
}

const sox_effect_handler_t *lsx_mcompand_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "mcompand",
    "quoted_compand_args [crossover_frequency[k] quoted_compand_args [...]]\n"
    "\n"
    "quoted_compand_args are as for the compand effect:\n"
    "\n"
    "  attack1,decay1[,attack2,decay2...]\n"
    "                 in-dB1,out-dB1[,in-dB2,out-dB2...]\n"
    "                [ gain [ initial-volume [ delay ] ] ]",
    SOX_EFF_MCHAN | SOX_EFF_GAIN,
    getopts, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };

  return &handler;
}
