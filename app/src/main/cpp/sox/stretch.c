/* libSoX Basic time stretcher.
 * (c) march/april 2000 Fabien COELHO <fabien@coelho.net> for sox.
 *
 * cross fade samples so as to go slower or faster.
 *
 * The filter is based on 6 parameters:
 * - stretch factor f
 * - window size w
 * - input step i
 *   output step o=f*i
 * - steady state of window s, ss = s*w
 *
 * I decided of the default values of these parameters based
 * on some small non extensive tests. maybe better defaults
 * can be suggested.
 */
#include "sox_i.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEFAULT_SLOW_SHIFT_RATIO        0.8
#define DEFAULT_FAST_SHIFT_RATIO        1.0

#define DEFAULT_STRETCH_WINDOW          20.0  /* ms */

typedef enum { input_state, output_state } stretch_status_t;

typedef struct {
  /* options
   * FIXME: maybe shift could be allowed > 1.0 with factor < 1.0 ???
   */
  double factor;   /* strech factor. 1.0 means copy. */
  double window;   /* window in ms */
  double shift;    /* shift ratio wrt window. <1.0 */
  double fading;   /* fading ratio wrt window. <0.5 */

  /* internal stuff */
  stretch_status_t state; /* automaton status */

  size_t segment;         /* buffer size */
  size_t index;        /* next available element */
  sox_sample_t *ibuf;      /* input buffer */
  size_t ishift;       /* input shift */

  size_t oindex;       /* next evailable element */
  double * obuf;   /* output buffer */
  size_t oshift;       /* output shift */

  size_t overlap;        /* fading size */
  double * fade_coefs;   /* fading, 1.0 -> 0.0 */

} priv_t;

/*
 * Process options
 */
static int getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *) effp->priv;
  --argc, ++argv;

  /* default options */
  p->factor = 1.0; /* default is no change */
  p->window = DEFAULT_STRETCH_WINDOW;

  if (argc > 0 && !sscanf(argv[0], "%lf", &p->factor)) {
    lsx_fail("error while parsing factor");
    return lsx_usage(effp);
  }

  if (argc > 1 && !sscanf(argv[1], "%lf", &p->window)) {
    lsx_fail("error while parsing window size");
    return lsx_usage(effp);
  }

  if (argc > 2) {
    switch (argv[2][0]) {
    case 'l':
    case 'L':
      break;
    default:
      lsx_fail("error while parsing fade type");
      return lsx_usage(effp);
    }
  }

  /* default shift depends whether we go slower or faster */
  p->shift = (p->factor <= 1.0) ?
    DEFAULT_FAST_SHIFT_RATIO: DEFAULT_SLOW_SHIFT_RATIO;

  if (argc > 3 && !sscanf(argv[3], "%lf", &p->shift)) {
    lsx_fail("error while parsing shift ratio");
    return lsx_usage(effp);
  }

  if (p->shift > 1.0 || p->shift <= 0.0) {
    lsx_fail("error with shift ratio value");
    return lsx_usage(effp);
  }

  /* default fading stuff...
     it makes sense for factor >= 0.5 */
  if (p->factor < 1.0)
    p->fading = 1.0 - (p->factor * p->shift);
  else
    p->fading = 1.0 - p->shift;
  if (p->fading > 0.5)
    p->fading = 0.5;

  if (argc > 4 && !sscanf(argv[4], "%lf", &p->fading)) {
    lsx_fail("error while parsing fading ratio");
    return lsx_usage(effp);
  }

  if (p->fading > 0.5 || p->fading < 0.0) {
    lsx_fail("error with fading ratio value");
    return lsx_usage(effp);
  }

  return SOX_SUCCESS;
}

/*
 * Start processing
 */
static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t i;

  if (p->factor == 1)
    return SOX_EFF_NULL;

  p->state = input_state;

  p->segment = (int)(effp->out_signal.rate * 0.001 * p->window);
  /* start in the middle of an input to avoid initial fading... */
  p->index = p->segment / 2;
  p->ibuf = lsx_malloc(p->segment * sizeof(sox_sample_t));

  /* the shift ratio deal with the longest of ishift/oshift
     hence ishift<=segment and oshift<=segment. */
  if (p->factor < 1.0) {
    p->ishift = p->shift * p->segment;
    p->oshift = p->factor * p->ishift;
  } else {
    p->oshift = p->shift * p->segment;
    p->ishift = p->oshift / p->factor;
  }
  assert(p->ishift <= p->segment);
  assert(p->oshift <= p->segment);

  p->oindex = p->index; /* start as synchronized */
  p->obuf = lsx_malloc(p->segment * sizeof(double));
  p->overlap = (int)(p->fading * p->segment);
  p->fade_coefs = lsx_malloc(p->overlap * sizeof(double));

  /* initialize buffers */
  for (i = 0; i<p->segment; i++)
    p->ibuf[i] = 0;

  for (i = 0; i<p->segment; i++)
    p->obuf[i] = 0.0;

  if (p->overlap>1) {
    double slope = 1.0 / (p->overlap - 1);
    p->fade_coefs[0] = 1.0;
    for (i = 1; i < p->overlap - 1; i++)
      p->fade_coefs[i] = slope * (p->overlap - i - 1);
    p->fade_coefs[p->overlap - 1] = 0.0;
  } else if (p->overlap == 1)
    p->fade_coefs[0] = 1.0;

  lsx_debug("start: (factor=%g segment=%g shift=%g overlap=%g)\nstate=%d\n"
      "segment=%" PRIuPTR "\nindex=%" PRIuPTR "\n"
      "ishift=%" PRIuPTR "\noindex=%" PRIuPTR "\n"
      "oshift=%" PRIuPTR "\noverlap=%" PRIuPTR,
      p->factor, p->window, p->shift, p->fading, p->state,
      p->segment, p->index, p->ishift, p->oindex, p->oshift, p->overlap);

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* TODO: calculate actual length */
  return SOX_SUCCESS;
}

/* accumulates input ibuf to output obuf with fading fade_coefs */
static void combine(priv_t * p)
{
  size_t i;

  /* fade in */
  for (i = 0; i < p->overlap; i++)
    p->obuf[i] += p->fade_coefs[p->overlap - 1 - i] * p->ibuf[i];

  /* steady state */
  for (; i < p->segment - p->overlap; i++)
    p->obuf[i] += p->ibuf[i];

  /* fade out */
  for (; i<p->segment; i++)
    p->obuf[i] += p->fade_coefs[i - p->segment + p->overlap] * p->ibuf[i];
}

/*
 * Processes flow.
 */
static int flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                    size_t *isamp, size_t *osamp)
{
  priv_t * p = (priv_t *) effp->priv;
  size_t iindex = 0, oindex = 0;
  size_t i;

  while (iindex<*isamp && oindex<*osamp) {
    if (p->state == input_state) {
      size_t tocopy = min(*isamp-iindex,
                             p->segment-p->index);

      memcpy(p->ibuf + p->index, ibuf + iindex, tocopy * sizeof(sox_sample_t));

      iindex += tocopy;
      p->index += tocopy;

      if (p->index == p->segment) {
        /* compute */
        combine(p);

        /* shift input */
        for (i = 0; i + p->ishift < p->segment; i++)
          p->ibuf[i] = p->ibuf[i+p->ishift];

        p->index -= p->ishift;

        /* switch to output state */
        p->state = output_state;
      }
    }

    if (p->state == output_state) {
      while (p->oindex < p->oshift && oindex < *osamp) {
        float f;
        f = p->obuf[p->oindex++];
        SOX_SAMPLE_CLIP_COUNT(f, effp->clips);
        obuf[oindex++] = f;
      }

      if (p->oindex >= p->oshift && oindex<*osamp) {
        p->oindex -= p->oshift;

        /* shift internal output buffer */
        for (i = 0; i + p->oshift < p->segment; i++)
          p->obuf[i] = p->obuf[i + p->oshift];

        /* pad with 0 */
        for (; i < p->segment; i++)
          p->obuf[i] = 0.0;

        p->state = input_state;
      }
    }
  }

  *isamp = iindex;
  *osamp = oindex;

  return SOX_SUCCESS;
}


/*
 * Drain buffer at the end
 * maybe not correct ? end might be artificially faded?
 */
static int drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
  priv_t * p = (priv_t *) effp->priv;
  size_t i;
  size_t oindex = 0;

  if (p->state == input_state) {
    for (i=p->index; i<p->segment; i++)
      p->ibuf[i] = 0;

    combine(p);

    p->state = output_state;
  }

  while (oindex<*osamp && p->oindex<p->index) {
    float f = p->obuf[p->oindex++];
    SOX_SAMPLE_CLIP_COUNT(f, effp->clips);
    obuf[oindex++] = f;
  }

  *osamp = oindex;

  if (p->oindex == p->index)
    return SOX_EOF;
  else
    return SOX_SUCCESS;
}


static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;

  free(p->ibuf);
  free(p->obuf);
  free(p->fade_coefs);
  return SOX_SUCCESS;
}

const sox_effect_handler_t *lsx_stretch_effect_fn(void)
{
  static const sox_effect_handler_t handler = {
    "stretch",
    "factor [window fade shift fading]\n"
    "       (expansion, frame in ms, lin/..., unit<1.0, unit<0.5)\n"
    "       (defaults: 1.0 20 lin ...)",
    SOX_EFF_LENGTH,
    getopts, start, flow, drain, stop, NULL, sizeof(priv_t)
  };
  return &handler;
}
