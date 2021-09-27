/* libSoX effect: splice audio   Copyright (c) 2008-9 robs@users.sourceforge.net
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

static double difference(
    const sox_sample_t * a, const sox_sample_t * b, size_t length)
{
  double diff = 0;
  size_t i = 0;

  #define _ diff += sqr((double)a[i] - b[i]), ++i; /* Loop optimisation */
  do {_ _ _ _ _ _ _ _} while (i < length); /* N.B. length ≡ 0 (mod 8) */
  #undef _
  return diff;
}

/* Find where the two segments are most alike over the overlap period. */
static size_t best_overlap_position(sox_sample_t const * f1,
    sox_sample_t const * f2, uint64_t overlap, uint64_t search, size_t channels)
{
  size_t i, best_pos = 0;
  double diff, least_diff = difference(f2, f1, (size_t) (channels * overlap));

  for (i = 1; i < search; ++i) { /* linear search */
    diff = difference(f2 + channels * i, f1, (size_t) (channels * overlap));
    if (diff < least_diff)
      least_diff = diff, best_pos = i;
  }
  return best_pos;
}


typedef struct {
  enum {Cosine_2, Cosine_4, Triangular} fade_type;
  unsigned nsplices;     /* Number of splices requested */
  struct {
    char * str;          /* Command-line argument to parse for this splice */
    uint64_t overlap;    /* Number of samples to overlap */
    uint64_t search;     /* Number of samples to search */
    uint64_t start;      /* Start splicing when in_pos equals this */
  } * splices;

  uint64_t in_pos;       /* Number of samples read from the input stream */
  unsigned splices_pos;  /* Number of splices completed so far */
  size_t buffer_pos;     /* Number of samples through the current splice */
  size_t max_buffer_size;
  sox_sample_t * buffer;
  unsigned state;
} priv_t;

static void splice(sox_effect_t * effp, const sox_sample_t * in1, const
    sox_sample_t * in2, sox_sample_t * output, uint64_t overlap, size_t channels)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i, j, k = 0;

  if (p->fade_type == Cosine_4) {
    double fade_step = M_PI_2 / overlap;
    for (i = 0; i < overlap; ++i) {
      double fade_in  = sin(i * fade_step);
      double fade_out = cos(i * fade_step); /* constant RMS level (`power') */
      for (j = 0; j < channels; ++j, ++k) {
        double d = in1[k] * fade_out + in2[k] * fade_in;
        output[k] = SOX_ROUND_CLIP_COUNT(d, effp->clips); /* Might clip */
      }
    }
  }
  else if (p->fade_type == Cosine_2) {
    double fade_step = M_PI / overlap;
    for (i = 0; i < overlap; ++i) {
      double fade_in  = .5 - .5 * cos(i * fade_step);
      double fade_out = 1 - fade_in;    /* constant peak level (`gain') */
      for (j = 0; j < channels; ++j, ++k) {
        double d = in1[k] * fade_out + in2[k] * fade_in;
        output[k] = SOX_ROUND_CLIP_COUNT(d, effp->clips); /* Should not clip */
      }
    }
  }
  else /* Triangular */ {
    double fade_step = 1. / overlap;
    for (i = 0; i < overlap; ++i) {
      double fade_in  = fade_step * i;
      double fade_out = 1 - fade_in;    /* constant peak level (`gain') */
      for (j = 0; j < channels; ++j, ++k) {
        double d = in1[k] * fade_out + in2[k] * fade_in;
        output[k] = SOX_ROUND_CLIP_COUNT(d, effp->clips); /* Should not clip */
      }
    }
  }
}

static uint64_t do_splice(sox_effect_t * effp,
    sox_sample_t * f, uint64_t overlap, uint64_t search, size_t channels)
{
  uint64_t offset = search? best_overlap_position(
      f, f + overlap * channels, overlap, search, channels) : 0;
  splice(effp, f, f + (overlap + offset) * channels,
      f + (overlap + offset) * channels, overlap, channels);
  return overlap + offset;
}

static int parse(sox_effect_t * effp, char * * argv, sox_rate_t rate)
{
  priv_t * p = (priv_t *)effp->priv;
  char const * next;
  size_t i, buffer_size;
  uint64_t last_seen = 0;
  const uint64_t in_length = argv ? 0 :
    (effp->in_signal.length != SOX_UNKNOWN_LEN ?
     effp->in_signal.length / effp->in_signal.channels : SOX_UNKNOWN_LEN);

  p->max_buffer_size = 0;
  for (i = 0; i < p->nsplices; ++i) {
    if (argv) /* 1st parse only */
      p->splices[i].str = lsx_strdup(argv[i]);

    p->splices[i].overlap = rate * 0.01 + .5;
    p->splices[i].search = p->fade_type == Cosine_4? 0 : p->splices[i].overlap;

    next = lsx_parseposition(rate, p->splices[i].str,
             argv ? NULL : &p->splices[i].start, last_seen, in_length, '=');
    if (next == NULL) break;
    last_seen = p->splices[i].start;

    if (*next == ',') {
      next = lsx_parsesamples(rate, next + 1, &p->splices[i].overlap, 't');
      if (next == NULL) break;
      p->splices[i].overlap *= 2;
      if (*next == ',') {
        next = lsx_parsesamples(rate, next + 1, &p->splices[i].search, 't');
        if (next == NULL) break;
        p->splices[i].search *= 2;
      }
    }
    if (*next != '\0') break;
    p->splices[i].overlap = max(p->splices[i].overlap + 4, 16);
    p->splices[i].overlap &= ~7; /* Make divisible by 8 for loop optimisation */

    if (!argv) {
      if (i > 0 && p->splices[i].start <= p->splices[i-1].start) break;
      if (p->splices[i].start < p->splices[i].overlap) break;
      p->splices[i].start -= p->splices[i].overlap;
      buffer_size = 2 * p->splices[i].overlap + p->splices[i].search;
      p->max_buffer_size = max(p->max_buffer_size, buffer_size);
    }
  }
  if (i < p->nsplices)
    return lsx_usage(effp);
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  --argc, ++argv;
  if (argc) {
    if      (!strcmp(*argv, "-t")) p->fade_type = Triangular, --argc, ++argv;
    else if (!strcmp(*argv, "-q")) p->fade_type = Cosine_4  , --argc, ++argv;
    else if (!strcmp(*argv, "-h")) p->fade_type = Cosine_2  , --argc, ++argv;
  }
  p->nsplices = argc;
  p->splices = lsx_calloc(p->nsplices, sizeof(*p->splices));
  return parse(effp, argv, 1e5); /* No rate yet; parse with dummy */
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;

  parse(effp, 0, effp->in_signal.rate); /* Re-parse now rate is known */
  p->buffer = lsx_calloc(p->max_buffer_size * effp->in_signal.channels, sizeof(*p->buffer));
  p->in_pos = p->buffer_pos = p->splices_pos = 0;
  p->state = p->splices_pos != p->nsplices && p->in_pos == p->splices[p->splices_pos].start;
  effp->out_signal.length = SOX_UNKNOWN_LEN; /* depends on input data */
  for (i = 0; i < p->nsplices; ++i)
    if (p->splices[i].overlap) {
      if (p->fade_type == Cosine_4 && effp->in_signal.mult)
        *effp->in_signal.mult *= pow(.5, .5);
      return SOX_SUCCESS;
    }
  return SOX_EFF_NULL;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t c, idone = 0, odone = 0;
  *isamp /= effp->in_signal.channels;
  *osamp /= effp->in_signal.channels;

  while (sox_true) {
copying:
    if (p->state == 0) {
      for (; idone < *isamp && odone < *osamp; ++idone, ++odone, ++p->in_pos) {
        if (p->splices_pos != p->nsplices && p->in_pos == p->splices[p->splices_pos].start) {
          p->state = 1;
          goto buffering;
        }
        for (c = 0; c < effp->in_signal.channels; ++c)
          *obuf++ = *ibuf++;
      }
      break;
    }

buffering:
    if (p->state == 1) {
      size_t buffer_size = (2 * p->splices[p->splices_pos].overlap + p->splices[p->splices_pos].search) * effp->in_signal.channels;
      for (; idone < *isamp; ++idone, ++p->in_pos) {
        if (p->buffer_pos == buffer_size) {
          p->buffer_pos = do_splice(effp, p->buffer,
              p->splices[p->splices_pos].overlap,
              p->splices[p->splices_pos].search,
              (size_t)effp->in_signal.channels) * effp->in_signal.channels;
          p->state = 2;
          goto flushing;
          break;
        }
        for (c = 0; c < effp->in_signal.channels; ++c)
          p->buffer[p->buffer_pos++] = *ibuf++;
      }
      break;
    }

flushing:
    if (p->state == 2) {
      size_t buffer_size = (2 * p->splices[p->splices_pos].overlap + p->splices[p->splices_pos].search) * effp->in_signal.channels;
      for (; odone < *osamp; ++odone) {
        if (p->buffer_pos == buffer_size) {
          p->buffer_pos = 0;
          ++p->splices_pos;
          p->state = p->splices_pos != p->nsplices && p->in_pos == p->splices[p->splices_pos].start;
          goto copying;
        }
        for (c = 0; c < effp->in_signal.channels; ++c)
          *obuf++ = p->buffer[p->buffer_pos++];
      }
      break;
    }
  }

  *isamp = idone * effp->in_signal.channels;
  *osamp = odone * effp->in_signal.channels;
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  size_t isamp = 0;
  return flow(effp, 0, obuf, &isamp, osamp);
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  if (p->splices_pos != p->nsplices)
    lsx_warn("Input audio too short; splices not made: %u", p->nsplices - p->splices_pos);
  free(p->buffer);
  return SOX_SUCCESS;
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;
  for (i = 0; i < p->nsplices; ++i)
    free(p->splices[i].str);
  free(p->splices);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_splice_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "splice", "[-h|-t|-q] {position[,excess[,leeway]]}"
    "\n  -h        Half sine fade (default); constant gain (for correlated audio)"
    "\n  -t        Triangular (linear) fade; constant gain (for correlated audio)"
    "\n  -q        Quarter sine fade; constant power (for correlated audio e.g. x-fade)"
    "\n  position  The length of part 1 (including the excess)"
    "\n  excess    At the end of part 1 & the start of part2 (default 0.005)"
    "\n  leeway    Before part2 (default 0.005; set to 0 for cross-fade)",
    SOX_EFF_MCHAN | SOX_EFF_LENGTH,
    create, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };
  return &handler;
}
