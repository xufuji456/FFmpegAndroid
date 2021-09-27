/* libSoX Echo effect             August 24, 1998
 *
 * Copyright (C) 1998 Juergen Mueller And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Juergen Mueller And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 *
 * Flow diagram scheme for n delays ( 1 <= n <= MAX_ECHOS ):
 *
 *                                                    * gain-in  ___
 * ibuff --+--------------------------------------------------->|   |
 *         |                                          * decay 1 |   |
 *         |               +----------------------------------->|   |
 *         |               |                          * decay 2 | + |
 *         |               |             +--------------------->|   |
 *         |               |             |            * decay n |   |
 *         |    _________  |  _________  |     _________   +--->|___|
 *         |   |         | | |         | |    |         |  |      |
 *         +-->| delay 1 |-+-| delay 2 |-+...-| delay n |--+      | * gain-out
 *             |_________|   |_________|      |_________|         |
 *                                                                +----->obuff
 * Usage:
 *   echos gain-in gain-out delay-1 decay-1 [delay-2 decay-2 ... delay-n decay-n]
 *
 * Where:
 *   gain-in, decay-1 ... decay-n :  0.0 ... 1.0      volume
 *   gain-out :  0.0 ...      volume
 *   delay-1 ... delay-n :  > 0.0 msec
 *
 * Note:
 *   when decay is close to 1.0, the samples can begin clipping and the output
 *   can saturate!
 *
 * Hint:
 *   1 / out-gain > gain-in ( 1 + decay-1 + ... + decay-n )
 *
 */

#include "sox_i.h"

#include <stdlib.h> /* Harmless, and prototypes atof() etc. --dgc */

#define DELAY_BUFSIZ ( 50 * 50U * 1024 )
#define MAX_ECHOS 7     /* 24 bit x ( 1 + MAX_ECHOS ) = */
                        /* 24 bit x 8 = 32 bit !!!      */

/* Private data for SKEL file */
typedef struct {
        int     counter[MAX_ECHOS];
        int     num_delays;
        double  *delay_buf;
        float   in_gain, out_gain;
        float   delay[MAX_ECHOS], decay[MAX_ECHOS];
        ptrdiff_t samples[MAX_ECHOS], pointer[MAX_ECHOS];
        size_t sumsamples;
} priv_t;

/* Private data for SKEL file */

/*
 * Process options
 */
static int sox_echos_getopts(sox_effect_t * effp, int argc, char **argv)
{
        priv_t * echos = (priv_t *) effp->priv;
        int i;

        echos->num_delays = 0;

  --argc, ++argv;
        if ((argc < 4) || (argc % 2))
          return lsx_usage(effp);

        i = 0;
        sscanf(argv[i++], "%f", &echos->in_gain);
        sscanf(argv[i++], "%f", &echos->out_gain);
        while (i < argc) {
                /* Linux bug and it's cleaner. */
                sscanf(argv[i++], "%f", &echos->delay[echos->num_delays]);
                sscanf(argv[i++], "%f", &echos->decay[echos->num_delays]);
                echos->num_delays++;
                if ( echos->num_delays > MAX_ECHOS )
                {
                        lsx_fail("echos: to many delays, use less than %i delays",
                                MAX_ECHOS);
                        return (SOX_EOF);
                }
        }
        echos->sumsamples = 0;
        return (SOX_SUCCESS);
}

/*
 * Prepare for processing.
 */
static int sox_echos_start(sox_effect_t * effp)
{
        priv_t * echos = (priv_t *) effp->priv;
        int i;
        float sum_in_volume;
        unsigned long j;

        if ( echos->in_gain < 0.0 )
        {
                lsx_fail("echos: gain-in must be positive!");
                return (SOX_EOF);
        }
        if ( echos->in_gain > 1.0 )
        {
                lsx_fail("echos: gain-in must be less than 1.0!");
                return (SOX_EOF);
        }
        if ( echos->out_gain < 0.0 )
        {
                lsx_fail("echos: gain-in must be positive!");
                return (SOX_EOF);
        }
        for ( i = 0; i < echos->num_delays; i++ ) {
                echos->samples[i] = echos->delay[i] * effp->in_signal.rate / 1000.0;
                if ( echos->samples[i] < 1 )
                {
                    lsx_fail("echos: delay must be positive!");
                    return (SOX_EOF);
                }
                if ( echos->samples[i] > (ptrdiff_t)DELAY_BUFSIZ )
                {
                        lsx_fail("echos: delay must be less than %g seconds!",
                                DELAY_BUFSIZ / effp->in_signal.rate );
                        return (SOX_EOF);
                }
                if ( echos->decay[i] < 0.0 )
                {
                    lsx_fail("echos: decay must be positive!" );
                    return (SOX_EOF);
                }
                if ( echos->decay[i] > 1.0 )
                {
                    lsx_fail("echos: decay must be less than 1.0!" );
                    return (SOX_EOF);
                }
                echos->counter[i] = 0;
                echos->pointer[i] = echos->sumsamples;
                echos->sumsamples += echos->samples[i];
        }
        echos->delay_buf = lsx_malloc(sizeof (double) * echos->sumsamples);
        for ( j = 0; j < echos->sumsamples; ++j )
                echos->delay_buf[j] = 0.0;
        /* Be nice and check the hint with warning, if... */
        sum_in_volume = 1.0;
        for ( i = 0; i < echos->num_delays; i++ )
                sum_in_volume += echos->decay[i];
        if ( sum_in_volume * echos->in_gain > 1.0 / echos->out_gain )
                lsx_warn("echos: warning >>> gain-out can cause saturation of output <<<");

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* TODO: calculate actual length */

        return (SOX_SUCCESS);
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
static int sox_echos_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                size_t *isamp, size_t *osamp)
{
        priv_t * echos = (priv_t *) effp->priv;
        int j;
        double d_in, d_out;
        sox_sample_t out;
        size_t len = min(*isamp, *osamp);
        *isamp = *osamp = len;

        while (len--) {
                /* Store delays as 24-bit signed longs */
                d_in = (double) *ibuf++ / 256;
                /* Compute output first */
                d_out = d_in * echos->in_gain;
                for ( j = 0; j < echos->num_delays; j++ ) {
                        d_out += echos->delay_buf[echos->counter[j] + echos->pointer[j]] * echos->decay[j];
                }
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * echos->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Mix decay of delays and input */
                for ( j = 0; j < echos->num_delays; j++ ) {
                        if ( j == 0 )
                                echos->delay_buf[echos->counter[j] + echos->pointer[j]] = d_in;
                        else
                                echos->delay_buf[echos->counter[j] + echos->pointer[j]] =
                                   echos->delay_buf[echos->counter[j-1] + echos->pointer[j-1]] + d_in;
                }
                /* Adjust the counters */
                for ( j = 0; j < echos->num_delays; j++ )
                        echos->counter[j] =
                           ( echos->counter[j] + 1 ) % echos->samples[j];
        }
        /* processed all samples */
        return (SOX_SUCCESS);
}

/*
 * Drain out reverb lines.
 */
static int sox_echos_drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
        priv_t * echos = (priv_t *) effp->priv;
        double d_in, d_out;
        sox_sample_t out;
        int j;
        size_t done;

        done = 0;
        /* drain out delay samples */
        while ( ( done < *osamp ) && ( done < echos->sumsamples ) ) {
                d_in = 0;
                d_out = 0;
                for ( j = 0; j < echos->num_delays; j++ ) {
                        d_out += echos->delay_buf[echos->counter[j] + echos->pointer[j]] * echos->decay[j];
                }
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * echos->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Mix decay of delays and input */
                for ( j = 0; j < echos->num_delays; j++ ) {
                        if ( j == 0 )
                                echos->delay_buf[echos->counter[j] + echos->pointer[j]] = d_in;
                        else
                                echos->delay_buf[echos->counter[j] + echos->pointer[j]] =
                                   echos->delay_buf[echos->counter[j-1] + echos->pointer[j-1]];
                }
                /* Adjust the counters */
                for ( j = 0; j < echos->num_delays; j++ )
                        echos->counter[j] =
                           ( echos->counter[j] + 1 ) % echos->samples[j];
                done++;
                echos->sumsamples--;
        };
        /* samples played, it remains */
        *osamp = done;
        if (echos->sumsamples == 0)
            return SOX_EOF;
        else
            return SOX_SUCCESS;
}

/*
 * Clean up echos effect.
 */
static int sox_echos_stop(sox_effect_t * effp)
{
        priv_t * echos = (priv_t *) effp->priv;

        free(echos->delay_buf);
        echos->delay_buf = NULL;
        return (SOX_SUCCESS);
}

static sox_effect_handler_t sox_echos_effect = {
  "echos",
  "gain-in gain-out delay decay [ delay decay ... ]",
  SOX_EFF_LENGTH | SOX_EFF_GAIN,
  sox_echos_getopts,
  sox_echos_start,
  sox_echos_flow,
  sox_echos_drain,
  sox_echos_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_echos_effect_fn(void)
{
    return &sox_echos_effect;
}
