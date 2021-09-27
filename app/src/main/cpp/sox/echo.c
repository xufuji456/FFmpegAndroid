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
 *        * gain-in                                              ___
 * ibuff -----------+------------------------------------------>|   |
 *                  |       _________                           |   |
 *                  |      |         |                * decay 1 |   |
 *                  +----->| delay 1 |------------------------->|   |
 *                  |      |_________|                          |   |
 *                  |            _________                      | + |
 *                  |           |         |           * decay 2 |   |
 *                  +---------->| delay 2 |-------------------->|   |
 *                  |           |_________|                     |   |
 *                  :                 _________                 |   |
 *                  |                |         |      * decay n |   |
 *                  +--------------->| delay n |--------------->|___|
 *                                   |_________|                  |
 *                                                                | * gain-out
 *                                                                |
 *                                                                +----->obuff
 * Usage:
 *   echo gain-in gain-out delay-1 decay-1 [delay-2 decay-2 ... delay-n decay-n]
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
 */

#include "sox_i.h"

#include <stdlib.h> /* Harmless, and prototypes atof() etc. --dgc */

#define DELAY_BUFSIZ ( 50 * 50U * 1024 )
#define MAX_ECHOS 7     /* 24 bit x ( 1 + MAX_ECHOS ) = */
                        /* 24 bit x 8 = 32 bit !!!      */

/* Private data for SKEL file */
typedef struct {
        int     counter;
        int     num_delays;
        double  *delay_buf;
        float   in_gain, out_gain;
        float   delay[MAX_ECHOS], decay[MAX_ECHOS];
        ptrdiff_t samples[MAX_ECHOS], maxsamples;
        size_t fade_out;
} priv_t;

/* Private data for SKEL file */


/*
 * Process options
 */
static int sox_echo_getopts(sox_effect_t * effp, int argc, char **argv)
{
        priv_t * echo = (priv_t *) effp->priv;
        int i;

  --argc, ++argv;
        echo->num_delays = 0;

        if ((argc < 4) || (argc % 2))
          return lsx_usage(effp);

        i = 0;
        sscanf(argv[i++], "%f", &echo->in_gain);
        sscanf(argv[i++], "%f", &echo->out_gain);
        while (i < argc) {
                if ( echo->num_delays >= MAX_ECHOS )
                        lsx_fail("echo: to many delays, use less than %i delays",
                                MAX_ECHOS);
                /* Linux bug and it's cleaner. */
                sscanf(argv[i++], "%f", &echo->delay[echo->num_delays]);
                sscanf(argv[i++], "%f", &echo->decay[echo->num_delays]);
                echo->num_delays++;
        }
        return (SOX_SUCCESS);
}

/*
 * Prepare for processing.
 */
static int sox_echo_start(sox_effect_t * effp)
{
        priv_t * echo = (priv_t *) effp->priv;
        int i;
        float sum_in_volume;
        long j;

        echo->maxsamples = 0;
        if ( echo->in_gain < 0.0 )
        {
                lsx_fail("echo: gain-in must be positive!");
                return (SOX_EOF);
        }
        if ( echo->in_gain > 1.0 )
        {
                lsx_fail("echo: gain-in must be less than 1.0!");
                return (SOX_EOF);
        }
        if ( echo->out_gain < 0.0 )
        {
                lsx_fail("echo: gain-in must be positive!");
                return (SOX_EOF);
        }
        for ( i = 0; i < echo->num_delays; i++ ) {
                echo->samples[i] = echo->delay[i] * effp->in_signal.rate / 1000.0;
                if ( echo->samples[i] < 1 )
                {
                    lsx_fail("echo: delay must be positive!");
                    return (SOX_EOF);
                }
                if ( echo->samples[i] > (ptrdiff_t)DELAY_BUFSIZ )
                {
                        lsx_fail("echo: delay must be less than %g seconds!",
                                DELAY_BUFSIZ / effp->in_signal.rate );
                        return (SOX_EOF);
                }
                if ( echo->decay[i] < 0.0 )
                {
                    lsx_fail("echo: decay must be positive!" );
                    return (SOX_EOF);
                }
                if ( echo->decay[i] > 1.0 )
                {
                    lsx_fail("echo: decay must be less than 1.0!" );
                    return (SOX_EOF);
                }
                if ( echo->samples[i] > echo->maxsamples )
                        echo->maxsamples = echo->samples[i];
        }
        echo->delay_buf = lsx_malloc(sizeof (double) * echo->maxsamples);
        for ( j = 0; j < echo->maxsamples; ++j )
                echo->delay_buf[j] = 0.0;
        /* Be nice and check the hint with warning, if... */
        sum_in_volume = 1.0;
        for ( i = 0; i < echo->num_delays; i++ )
                sum_in_volume += echo->decay[i];
        if ( sum_in_volume * echo->in_gain > 1.0 / echo->out_gain )
                lsx_warn("echo: warning >>> gain-out can cause saturation of output <<<");
        echo->counter = 0;
        echo->fade_out = echo->maxsamples;

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* TODO: calculate actual length */

        return (SOX_SUCCESS);
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
static int sox_echo_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                 size_t *isamp, size_t *osamp)
{
        priv_t * echo = (priv_t *) effp->priv;
        int j;
        double d_in, d_out;
        sox_sample_t out;
        size_t len = min(*isamp, *osamp);
        *isamp = *osamp = len;

        while (len--) {
                /* Store delays as 24-bit signed longs */
                d_in = (double) *ibuf++ / 256;
                /* Compute output first */
                d_out = d_in * echo->in_gain;
                for ( j = 0; j < echo->num_delays; j++ ) {
                        d_out += echo->delay_buf[
(echo->counter + echo->maxsamples - echo->samples[j]) % echo->maxsamples]
                        * echo->decay[j];
                }
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * echo->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Store input in delay buffer */
                echo->delay_buf[echo->counter] = d_in;
                /* Adjust the counter */
                echo->counter = ( echo->counter + 1 ) % echo->maxsamples;
        }
        /* processed all samples */
        return (SOX_SUCCESS);
}

/*
 * Drain out reverb lines.
 */
static int sox_echo_drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
        priv_t * echo = (priv_t *) effp->priv;
        double d_in, d_out;
        sox_sample_t out;
        int j;
        size_t done;

        done = 0;
        /* drain out delay samples */
        while ( ( done < *osamp ) && ( done < echo->fade_out ) ) {
                d_in = 0;
                d_out = 0;
                for ( j = 0; j < echo->num_delays; j++ ) {
                        d_out += echo->delay_buf[
(echo->counter + echo->maxsamples - echo->samples[j]) % echo->maxsamples]
                        * echo->decay[j];
                }
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * echo->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Store input in delay buffer */
                echo->delay_buf[echo->counter] = d_in;
                /* Adjust the counters */
                echo->counter = ( echo->counter + 1 ) % echo->maxsamples;
                done++;
                echo->fade_out--;
        };
        /* samples played, it remains */
        *osamp = done;
        if (echo->fade_out == 0)
            return SOX_EOF;
        else
            return SOX_SUCCESS;
}

static int sox_echo_stop(sox_effect_t * effp)
{
        priv_t * echo = (priv_t *) effp->priv;

        free(echo->delay_buf);
        echo->delay_buf = NULL;
        return (SOX_SUCCESS);
}

static sox_effect_handler_t sox_echo_effect = {
  "echo",
  "gain-in gain-out delay decay [ delay decay ... ]",
  SOX_EFF_LENGTH | SOX_EFF_GAIN,
  sox_echo_getopts,
  sox_echo_start,
  sox_echo_flow,
  sox_echo_drain,
  sox_echo_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_echo_effect_fn(void)
{
    return &sox_echo_effect;
}
