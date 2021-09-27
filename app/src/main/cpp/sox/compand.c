/* libSoX compander effect
 *
 * Written by Nick Bailey (nick@bailey-family.org.uk or
 *                         n.bailey@elec.gla.ac.uk)
 *
 * Copyright 1999 Chris Bagwell And Nick Bailey
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And Nick Bailey are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"

#include <string.h>
#include <stdlib.h>
#include "compandt.h"

/*
 * Compressor/expander effect for libSoX.
 *
 * Flow diagram for one channel:
 *
 *               ------------      ---------------
 *              |            |    |               |     ---
 * ibuff ---+---| integrator |--->| transfer func |--->|   |
 *          |   |            |    |               |    |   |
 *          |    ------------      ---------------     |   |  * gain
 *          |                                          | * |----------->obuff
 *          |       -------                            |   |
 *          |      |       |                           |   |
 *          +----->| delay |-------------------------->|   |
 *                 |       |                            ---
 *                  -------
 */
#define compand_usage \
  "attack1,decay1{,attack2,decay2} [soft-knee-dB:]in-dB1[,out-dB1]{,in-dB2,out-dB2} [gain [initial-volume-dB [delay]]]\n" \
  "\twhere {} means optional and repeatable and [] means optional.\n" \
  "\tdB values are floating point or -inf'; times are in seconds."
/*
 * Note: clipping can occur if the transfer function pushes things too
 * close to 0 dB.  In that case, use a negative gain, or reduce the
 * output level of the transfer function.
 */

typedef struct {
  sox_compandt_t transfer_fn;

  struct {
    double attack_times[2]; /* 0:attack_time, 1:decay_time */
    double volume;          /* Current "volume" of each channel */
  } * channels;
  unsigned expectedChannels;/* Also flags that channels aren't to be treated
                               individually when = 1 and input not mono */
  double delay;             /* Delay to apply before companding */
  sox_sample_t *delay_buf;   /* Old samples, used for delay processing */
  ptrdiff_t delay_buf_size;/* Size of delay_buf in samples */
  ptrdiff_t delay_buf_index; /* Index into delay_buf */
  ptrdiff_t delay_buf_cnt; /* No. of active entries in delay_buf */
  int delay_buf_full;       /* Shows buffer situation (important for drain) */

  char *arg0;  /* copies of arguments, so that they may be modified */
  char *arg1;
  char *arg2;
} priv_t;

static int getopts(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * l = (priv_t *) effp->priv;
  char * s;
  char dummy;     /* To check for extraneous chars. */
  unsigned pairs, i, j, commas;

  --argc, ++argv;
  if (argc < 2 || argc > 5)
    return lsx_usage(effp);

  l->arg0 = lsx_strdup(argv[0]);
  l->arg1 = lsx_strdup(argv[1]);
  l->arg2 = argc > 2 ? lsx_strdup(argv[2]) : NULL;

  /* Start by checking the attack and decay rates */
  for (s = l->arg0, commas = 0; *s; ++s) if (*s == ',') ++commas;
  if ((commas % 2) == 0) {
    lsx_fail("there must be an even number of attack/decay parameters");
    return SOX_EOF;
  }
  pairs = 1 + commas/2;
  l->channels = lsx_calloc(pairs, sizeof(*l->channels));
  l->expectedChannels = pairs;

  /* Now tokenise the rates string and set up these arrays.  Keep
     them in seconds at the moment: we don't know the sample rate yet. */
  for (i = 0, s = strtok(l->arg0, ","); s != NULL; ++i) {
    for (j = 0; j < 2; ++j) {
      if (sscanf(s, "%lf %c", &l->channels[i].attack_times[j], &dummy) != 1) {
        lsx_fail("syntax error trying to read attack/decay time");
        return SOX_EOF;
      } else if (l->channels[i].attack_times[j] < 0) {
        lsx_fail("attack & decay times can't be less than 0 seconds");
        return SOX_EOF;
      }
      s = strtok(NULL, ",");
    }
  }

  if (!lsx_compandt_parse(&l->transfer_fn, l->arg1, l->arg2))
    return SOX_EOF;

  /* Set the initial "volume" to be attibuted to the input channels.
     Unless specified, choose 0dB otherwise clipping will
     result if the user has seleced a long attack time */
  for (i = 0; i < l->expectedChannels; ++i) {
    double init_vol_dB = 0;
    if (argc > 3 && sscanf(argv[3], "%lf %c", &init_vol_dB, &dummy) != 1) {
      lsx_fail("syntax error trying to read initial volume");
      return SOX_EOF;
    } else if (init_vol_dB > 0) {
      lsx_fail("initial volume is relative to maximum volume so can't exceed 0dB");
      return SOX_EOF;
    }
    l->channels[i].volume = pow(10., init_vol_dB / 20);
  }

  /* If there is a delay, store it. */
  if (argc > 4 && sscanf(argv[4], "%lf %c", &l->delay, &dummy) != 1) {
    lsx_fail("syntax error trying to read delay value");
    return SOX_EOF;
  } else if (l->delay < 0) {
    lsx_fail("delay can't be less than 0 seconds");
    return SOX_EOF;
  }

  return SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * l = (priv_t *) effp->priv;
  unsigned i, j;

  lsx_debug("%i input channel(s) expected: actually %i",
      l->expectedChannels, effp->out_signal.channels);
  for (i = 0; i < l->expectedChannels; ++i)
    lsx_debug("Channel %i: attack = %g decay = %g", i,
        l->channels[i].attack_times[0], l->channels[i].attack_times[1]);
  if (!lsx_compandt_show(&l->transfer_fn, effp->global_info->plot))
    return SOX_EOF;

  /* Convert attack and decay rates using number of samples */
  for (i = 0; i < l->expectedChannels; ++i)
    for (j = 0; j < 2; ++j)
      if (l->channels[i].attack_times[j] > 1.0/effp->out_signal.rate)
        l->channels[i].attack_times[j] = 1.0 -
          exp(-1.0/(effp->out_signal.rate * l->channels[i].attack_times[j]));
      else
        l->channels[i].attack_times[j] = 1.0;

  /* Allocate the delay buffer */
  l->delay_buf_size = l->delay * effp->out_signal.rate * effp->out_signal.channels;
  if (l->delay_buf_size > 0)
    l->delay_buf = lsx_calloc((size_t)l->delay_buf_size, sizeof(*l->delay_buf));
  l->delay_buf_index = 0;
  l->delay_buf_cnt = 0;
  l->delay_buf_full= 0;

  return SOX_SUCCESS;
}

/*
 * Update a volume value using the given sample
 * value, the attack rate and decay rate
 */
static void doVolume(double *v, double samp, priv_t * l, int chan)
{
  double s = -samp / SOX_SAMPLE_MIN;
  double delta = s - *v;

  if (delta > 0.0) /* increase volume according to attack rate */
    *v += delta * l->channels[chan].attack_times[0];
  else             /* reduce volume according to decay rate */
    *v += delta * l->channels[chan].attack_times[1];
}

static int flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                    size_t *isamp, size_t *osamp)
{
  priv_t * l = (priv_t *) effp->priv;
  int len =  (*isamp > *osamp) ? *osamp : *isamp;
  int filechans = effp->out_signal.channels;
  int idone,odone;

  for (idone = 0,odone = 0; idone < len; ibuf += filechans) {
    int chan;

    /* Maintain the volume fields by simulating a leaky pump circuit */
    for (chan = 0; chan < filechans; ++chan) {
      if (l->expectedChannels == 1 && filechans > 1) {
        /* User is expecting same compander for all channels */
        int i;
        double maxsamp = 0.0;
        for (i = 0; i < filechans; ++i) {
          double rect = fabs((double)ibuf[i]);
          if (rect > maxsamp) maxsamp = rect;
        }
        doVolume(&l->channels[0].volume, maxsamp, l, 0);
        break;
      } else
        doVolume(&l->channels[chan].volume, fabs((double)ibuf[chan]), l, chan);
    }

    /* Volume memory is updated: perform compand */
    for (chan = 0; chan < filechans; ++chan) {
      int ch = l->expectedChannels > 1 ? chan : 0;
      double level_in_lin = l->channels[ch].volume;
      double level_out_lin = lsx_compandt(&l->transfer_fn, level_in_lin);
      double checkbuf;

      if (l->delay_buf_size <= 0) {
        checkbuf = ibuf[chan] * level_out_lin;
        SOX_SAMPLE_CLIP_COUNT(checkbuf, effp->clips);
        obuf[odone++] = checkbuf;
        idone++;
      } else {
        if (l->delay_buf_cnt >= l->delay_buf_size) {
          l->delay_buf_full=1; /* delay buffer is now definitely full */
          checkbuf = l->delay_buf[l->delay_buf_index] * level_out_lin;
          SOX_SAMPLE_CLIP_COUNT(checkbuf, effp->clips);
          obuf[odone] = checkbuf;
          odone++;
          idone++;
        } else {
          l->delay_buf_cnt++;
          idone++; /* no "odone++" because we did not fill obuf[...] */
        }
        l->delay_buf[l->delay_buf_index++] = ibuf[chan];
        l->delay_buf_index %= l->delay_buf_size;
      }
    }
  }

  *isamp = idone; *osamp = odone;
  return (SOX_SUCCESS);
}

static int drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
  priv_t * l = (priv_t *) effp->priv;
  size_t chan, done = 0;

  if (l->delay_buf_full == 0)
    l->delay_buf_index = 0;
  while (done+effp->out_signal.channels <= *osamp && l->delay_buf_cnt > 0)
    for (chan = 0; chan < effp->out_signal.channels; ++chan) {
      int c = l->expectedChannels > 1 ? chan : 0;
      double level_in_lin = l->channels[c].volume;
      double level_out_lin = lsx_compandt(&l->transfer_fn, level_in_lin);
      obuf[done++] = l->delay_buf[l->delay_buf_index++] * level_out_lin;
      l->delay_buf_index %= l->delay_buf_size;
      l->delay_buf_cnt--;
    }
  *osamp = done;
  return l->delay_buf_cnt > 0 ? SOX_SUCCESS : SOX_EOF;
}

static int stop(sox_effect_t * effp)
{
  priv_t * l = (priv_t *) effp->priv;

  free(l->delay_buf);
  return SOX_SUCCESS;
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * l = (priv_t *) effp->priv;

  lsx_compandt_kill(&l->transfer_fn);
  free(l->channels);
  free(l->arg0);
  free(l->arg1);
  free(l->arg2);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_compand_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "compand", compand_usage, SOX_EFF_MCHAN | SOX_EFF_GAIN,
    getopts, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
