/* libSoX dcshift.c
 * (c) 2000.04.15 Chris Ausbrooks <weed@bucket.pp.ualr.edu>
 *
 * based on vol.c which is
 * (c) 20/03/2000 Fabien COELHO <fabien@coelho.net> for sox.
 *
 * DC shift a sound file, with basic linear amplitude formula.
 * Beware of saturations! clipping is checked and reported.
 * Cannot handle different number of channels.
 * Cannot handle rate change.
 */

#include "sox_i.h"

typedef struct {
    double dcshift; /* DC shift. */
    int uselimiter; /* boolean: are we using the limiter? */
    double limiterthreshhold;
    double limitergain; /* limiter gain. */
    uint64_t limited; /* number of limited values to report. */
    uint64_t totalprocessed;
} priv_t;

/*
 * Process options: dcshift (double) type (amplitude, power, dB)
 */
static int sox_dcshift_getopts(sox_effect_t * effp, int argc, char **argv)
{
    priv_t * dcs = (priv_t *) effp->priv;
    dcs->dcshift = 1.0; /* default is no change */
    dcs->uselimiter = 0; /* default is no limiter */

  --argc, ++argv;
    if (argc < 1)
      return lsx_usage(effp);

    if (argc && (!sscanf(argv[0], "%lf", &dcs->dcshift)))
      return lsx_usage(effp);

    if (argc>1)
    {
        if (!sscanf(argv[1], "%lf", &dcs->limitergain))
          return lsx_usage(effp);

        dcs->uselimiter = 1; /* ok, we'll use it */
        /* The following equation is derived so that there is no
         * discontinuity in output amplitudes */
        /* and a SOX_SAMPLE_MAX input always maps to a SOX_SAMPLE_MAX output
         * when the limiter is activated. */
        /* (NOTE: There **WILL** be a discontinuity in the slope of the
         * output amplitudes when using the limiter.) */
        dcs->limiterthreshhold = SOX_SAMPLE_MAX * (1.0 - (fabs(dcs->dcshift) - dcs->limitergain));
    }

    return SOX_SUCCESS;
}

/*
 * Start processing
 */
static int sox_dcshift_start(sox_effect_t * effp)
{
    priv_t * dcs = (priv_t *) effp->priv;

    if (dcs->dcshift == 0)
      return SOX_EFF_NULL;

    dcs->limited = 0;
    dcs->totalprocessed = 0;

    return SOX_SUCCESS;
}

/*
 * Process data.
 */
static int sox_dcshift_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                    size_t *isamp, size_t *osamp)
{
    priv_t * dcs = (priv_t *) effp->priv;
    double dcshift = dcs->dcshift;
    double limitergain = dcs->limitergain;
    double limiterthreshhold = dcs->limiterthreshhold;
    double sample;
    size_t len;

    len = min(*osamp, *isamp);

    /* report back dealt with amount. */
    *isamp = len; *osamp = len;

    if (dcs->uselimiter)
    {
        dcs->totalprocessed += len;

        for (;len>0; len--)
            {
                sample = *ibuf++;

                if (sample > limiterthreshhold && dcshift > 0)
                {
                        sample =  (sample - limiterthreshhold) * limitergain / (SOX_SAMPLE_MAX - limiterthreshhold) + limiterthreshhold + dcshift;
                        dcs->limited++;
                }
                else if (sample < -limiterthreshhold && dcshift < 0)
                {
                        /* Note this should really be SOX_SAMPLE_MIN but
                         * the clip() below will take care of the overflow.
                         */
                        sample =  (sample + limiterthreshhold) * limitergain / (SOX_SAMPLE_MAX - limiterthreshhold) - limiterthreshhold + dcshift;
                        dcs->limited++;
                }
                else
                {
                        /* Note this should consider SOX_SAMPLE_MIN but
                         * the clip() below will take care of the overflow.
                         */
                        sample = dcshift * SOX_SAMPLE_MAX + sample;
                }

                SOX_SAMPLE_CLIP_COUNT(sample, effp->clips);
                *obuf++ = sample;
            }
    }
    else for (; len > 0; --len) {            /* quite basic, with clipping */
      double d = dcshift * (SOX_SAMPLE_MAX + 1.) + *ibuf++;
      *obuf++ = SOX_ROUND_CLIP_COUNT(d, effp->clips);
    }
    return SOX_SUCCESS;
}

/*
 * Do anything required when you stop reading samples.
 * Don't close input file!
 */
static int sox_dcshift_stop(sox_effect_t * effp)
{
    priv_t * dcs = (priv_t *) effp->priv;

    if (dcs->limited)
    {
        lsx_warn("DCSHIFT limited %" PRIu64 " values (%d percent).",
             dcs->limited, (int) (dcs->limited * 100.0 / dcs->totalprocessed));
    }
    return SOX_SUCCESS;
}

static sox_effect_handler_t sox_dcshift_effect = {
   "dcshift",
   "shift [ limitergain ]\n"
   "\tThe peak limiter has a gain much less than 1.0 (ie 0.05 or 0.02) which\n"
   "\tis only used on peaks to prevent clipping. (default is no limiter)",
   SOX_EFF_MCHAN | SOX_EFF_GAIN,
   sox_dcshift_getopts,
   sox_dcshift_start,
   sox_dcshift_flow,
   NULL,
   sox_dcshift_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_dcshift_effect_fn(void)
{
    return &sox_dcshift_effect;
}
