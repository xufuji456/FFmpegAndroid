/* libSoX earwax - makes listening to headphones easier     November 9, 2000
 *
 * Copyright (c) 2000 Edward Beingessner And Sundry Contributors.
 * This source code is freely redistributable and may be used for any purpose.
 * This copyright notice must be maintained.  Edward Beingessner And Sundry
 * Contributors are not responsible for the consequences of using this
 * software.
 *
 * This effect takes a 44.1kHz stereo (CD format) signal that is meant to be
 * listened to on headphones, and adds audio cues to move the soundstage from
 * inside your head (standard for headphones) to outside and in front of the
 * listener (standard for speakers). This makes the sound much easier to listen
 * to on headphones.
 */

#include "sox_i.h"
#include <string.h>

static const sox_sample_t filt[32 * 2] = {
/* 30°  330° */
    4,   -6,     /* 32 tap stereo FIR filter. */
    4,  -11,     /* One side filters as if the */
   -1,   -5,     /* signal was from 30 degrees */
    3,    3,     /* from the ear, the other as */
   -2,    5,     /* if 330 degrees. */
   -5,    0,
    9,    1,
    6,    3,     /*                         Input                         */
   -4,   -1,     /*                   Left         Right                  */
   -5,   -3,     /*                __________   __________                */
   -2,   -5,     /*               |          | |          |               */
   -7,    1,     /*           .---|  Hh,0(f) | |  Hh,0(f) |---.           */
    6,   -7,     /*          /    |__________| |__________|    \          */
   30,  -29,     /*         /                \ /                \         */
   12,   -3,     /*        /                  X                  \        */
  -11,    4,     /*       /                  / \                  \       */
   -3,    7,     /*  ____V_____   __________V   V__________   _____V____  */
  -20,   23,     /* |          | |          |   |          | |          | */
    2,    0,     /* | Hh,30(f) | | Hh,330(f)|   | Hh,330(f)| | Hh,30(f) | */
    1,   -6,     /* |__________| |__________|   |__________| |__________| */
  -14,   -5,     /*      \     ___      /           \      ___     /      */
   15,  -18,     /*       \   /   \    /    _____    \    /   \   /       */
    6,    7,     /*        `->| + |<--'    /     \    `-->| + |<-'        */
   15,  -10,     /*           \___/      _/       \_      \___/           */
  -14,   22,     /*               \     / \       / \     /               */
   -7,   -2,     /*                `--->| |       | |<---'                */
   -4,    9,     /*                     \_/       \_/                     */
    6,  -12,     /*                                                       */
    6,   -6,     /*                       Headphones                      */
    0,  -11,
    0,   -5,
    4,    0};

#define NUMTAPS array_length(filt)
typedef struct {sox_sample_t tap[NUMTAPS];} priv_t; /* FIR filter z^-1 delays */

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  if (effp->in_signal.rate != 44100 || effp->in_signal.channels != 2) {
    lsx_fail("works only with stereo audio sampled at 44100Hz (i.e. CDDA)");
    return SOX_EOF;
  }
  memset(p->tap, 0, NUMTAPS * sizeof(*p->tap)); /* zero tap memory */
  if (effp->in_signal.mult)
    *effp->in_signal.mult *= dB_to_linear(-4.4);
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
                sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i, len = *isamp = *osamp = min(*isamp, *osamp);

  while (len--) {       /* update taps and calculate output */
    double output = 0;

    for (i = NUMTAPS - 1; i; --i) {
      p->tap[i] = p->tap[i - 1];
      output += p->tap[i] * filt[i];
    }
    p->tap[0] = *ibuf++ / 64; /* scale output */
    output += p->tap[0] * filt[0];
    *obuf++ = SOX_ROUND_CLIP_COUNT(output, effp->clips);
  }
  return SOX_SUCCESS;
}

/* No drain: preserve audio file length; it's only 32 samples anyway. */

sox_effect_handler_t const *lsx_earwax_effect_fn(void)
{
  static sox_effect_handler_t handler = {"earwax", NULL, SOX_EFF_MCHAN,
    NULL, start, flow, NULL, NULL, NULL, sizeof(priv_t)};
  return &handler;
}
