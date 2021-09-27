/* libSoX effect: Pad With Silence   (c) 2006 robs@users.sourceforge.net
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

typedef struct {
  unsigned npads;     /* Number of pads requested */
  struct {
    char * str;       /* Command-line argument to parse for this pad */
    uint64_t start; /* Start padding when in_pos equals this */
    uint64_t pad;   /* Number of samples to pad */
  } * pads;

  uint64_t in_pos;  /* Number of samples read from the input stream */
  unsigned pads_pos;  /* Number of pads completed so far */
  uint64_t pad_pos; /* Number of samples through the current pad */
} priv_t;

static int parse(sox_effect_t * effp, char * * argv, sox_rate_t rate)
{
  priv_t * p = (priv_t *)effp->priv;
  char const * next;
  unsigned i;
  uint64_t last_seen = 0;
  const uint64_t in_length = argv ? 0 :
    (effp->in_signal.length != SOX_UNKNOWN_LEN ?
     effp->in_signal.length / effp->in_signal.channels : SOX_UNKNOWN_LEN);

  for (i = 0; i < p->npads; ++i) {
    if (argv) /* 1st parse only */
      p->pads[i].str = lsx_strdup(argv[i]);
    next = lsx_parsesamples(rate, p->pads[i].str, &p->pads[i].pad, 't');
    if (next == NULL) break;
    if (*next == '\0')
      p->pads[i].start = i? UINT64_MAX : 0;
    else {
      if (*next != '@') break;
      next = lsx_parseposition(rate, next+1, argv ? NULL : &p->pads[i].start,
               last_seen, in_length, '=');
      if (next == NULL || *next != '\0') break;
      last_seen = p->pads[i].start;
      if (p->pads[i].start == SOX_UNKNOWN_LEN)
        p->pads[i].start = UINT64_MAX; /* currently the same value, but ... */
    }
    if (!argv) {
      /* Do this check only during the second pass when the actual
         sample rate is known, otherwise it might fail on legal
         commands like
           pad 1@0.5 1@30000s
         if the rate is, e.g., 48k. */
      if (i > 0 && p->pads[i].start <= p->pads[i-1].start) break;
    }
  }
  if (i < p->npads)
    return lsx_usage(effp);
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  --argc, ++argv;
  p->npads = argc;
  p->pads = lsx_calloc(p->npads, sizeof(*p->pads));
  return parse(effp, argv, 1e5); /* No rate yet; parse with dummy */
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;

  /* Re-parse now rate is known */
  if (parse(effp, 0, effp->in_signal.rate) != SOX_SUCCESS)
    return SOX_EOF;

  if ((effp->out_signal.length = effp->in_signal.length) != SOX_UNKNOWN_LEN) {
    for (i = 0; i < p->npads; ++i)
      effp->out_signal.length +=
        p->pads[i].pad * effp->in_signal.channels;

    /* Check that the last pad position (except for "at the end")
       is within bounds. */
    i = p->npads;
    if (i > 0 && p->pads[i-1].start == UINT64_MAX)
      i--;
    if (i > 0 &&
        p->pads[i-1].start * effp->in_signal.channels
          > effp->in_signal.length)
    {
      lsx_fail("pad position after end of audio");
      return SOX_EOF;
    }
  }

  p->in_pos = p->pad_pos = p->pads_pos = 0;
  for (i = 0; i < p->npads; ++i)
    if (p->pads[i].pad)
      return SOX_SUCCESS;
  return SOX_EFF_NULL;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t c, idone = 0, odone = 0;
  *isamp /= effp->in_signal.channels;
  *osamp /= effp->in_signal.channels;

  do {
    /* Copying: */
    for (; idone < *isamp && odone < *osamp && !(p->pads_pos != p->npads && p->in_pos == p->pads[p->pads_pos].start); ++idone, ++odone, ++p->in_pos)
      for (c = 0; c < effp->in_signal.channels; ++c) *obuf++ = *ibuf++;

    /* Padding: */
    if (p->pads_pos != p->npads && p->in_pos == p->pads[p->pads_pos].start) {
      for (; odone < *osamp && p->pad_pos < p->pads[p->pads_pos].pad; ++odone, ++p->pad_pos)
        for (c = 0; c < effp->in_signal.channels; ++c) *obuf++ = 0;
      if (p->pad_pos == p->pads[p->pads_pos].pad) { /* Move to next pad? */
        ++p->pads_pos;
        p->pad_pos = 0;
      }
    }
  } while (idone < *isamp && odone < *osamp);

  *isamp = idone * effp->in_signal.channels;
  *osamp = odone * effp->in_signal.channels;
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  static size_t isamp = 0;
  if (p->pads_pos != p->npads && p->in_pos != p->pads[p->pads_pos].start)
    p->in_pos = UINT64_MAX;  /* Invoke the final pad (with no given start) */
  return flow(effp, 0, obuf, &isamp, osamp);
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  if (p->pads_pos != p->npads)
    lsx_warn("Input audio too short; pads not applied: %u", p->npads-p->pads_pos);
  return SOX_SUCCESS;
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;
  for (i = 0; i < p->npads; ++i)
    free(p->pads[i].str);
  free(p->pads);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_pad_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "pad", "{length[@position]}", SOX_EFF_MCHAN|SOX_EFF_LENGTH|SOX_EFF_MODIFY,
    create, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
