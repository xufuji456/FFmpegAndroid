/* August 24, 1998
 * Copyright (C) 1998 Juergen Mueller And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Juergen Mueller And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

/*
 *      Chorus effect.
 *
 * Flow diagram scheme for n delays ( 1 <= n <= MAX_CHORUS ):
 *
 *        * gain-in                                           ___
 * ibuff -----+--------------------------------------------->|   |
 *            |      _________                               |   |
 *            |     |         |                   * decay 1  |   |
 *            +---->| delay 1 |----------------------------->|   |
 *            |     |_________|                              |   |
 *            |        /|\                                   |   |
 *            :         |                                    |   |
 *            : +-----------------+   +--------------+       | + |
 *            : | Delay control 1 |<--| mod. speed 1 |       |   |
 *            : +-----------------+   +--------------+       |   |
 *            |      _________                               |   |
 *            |     |         |                   * decay n  |   |
 *            +---->| delay n |----------------------------->|   |
 *                  |_________|                              |   |
 *                     /|\                                   |___|
 *                      |                                      |
 *              +-----------------+   +--------------+         | * gain-out
 *              | Delay control n |<--| mod. speed n |         |
 *              +-----------------+   +--------------+         +----->obuff
 *
 *
 * The delay i is controled by a sine or triangle modulation i ( 1 <= i <= n).
 *
 * Usage:
 *   chorus gain-in gain-out delay-1 decay-1 speed-1 depth-1 -s1|t1 [
 *       delay-2 decay-2 speed-2 depth-2 -s2|-t2 ... ]
 *
 * Where:
 *   gain-in, decay-1 ... decay-n :  0.0 ... 1.0      volume
 *   gain-out :  0.0 ...      volume
 *   delay-1 ... delay-n :  20.0 ... 100.0 msec
 *   speed-1 ... speed-n :  0.1 ... 5.0 Hz       modulation 1 ... n
 *   depth-1 ... depth-n :  0.0 ... 10.0 msec    modulated delay 1 ... n
 *   -s1 ... -sn : modulation by sine 1 ... n
 *   -t1 ... -tn : modulation by triangle 1 ... n
 *
 * Note:
 *   when decay is close to 1.0, the samples can begin clipping and the output
 *   can saturate!
 *
 * Hint:
 *   1 / out-gain < gain-in ( 1 + decay-1 + ... + decay-n )
 *
*/

/*
 * libSoX chorus effect file.
 */

#include "sox_i.h"

#include <stdlib.h> /* Harmless, and prototypes atof() etc. --dgc */
#include <string.h>

#define MOD_SINE        0
#define MOD_TRIANGLE    1
#define MAX_CHORUS      7

typedef struct {
        int     num_chorus;
        int     modulation[MAX_CHORUS];
        int     counter;
        long    phase[MAX_CHORUS];
        float   *chorusbuf;
        float   in_gain, out_gain;
        float   delay[MAX_CHORUS], decay[MAX_CHORUS];
        float   speed[MAX_CHORUS], depth[MAX_CHORUS];
        long    length[MAX_CHORUS];
        int     *lookup_tab[MAX_CHORUS];
        int     depth_samples[MAX_CHORUS], samples[MAX_CHORUS];
        int maxsamples;
        unsigned int fade_out;
} priv_t;

/*
 * Process options
 */
static int sox_chorus_getopts(sox_effect_t * effp, int argc, char **argv)
{
        priv_t * chorus = (priv_t *) effp->priv;
        int i;
  --argc, ++argv;

        chorus->num_chorus = 0;
        i = 0;

        if ( ( argc < 7 ) || (( argc - 2 ) % 5 ) )
          return lsx_usage(effp);

        sscanf(argv[i++], "%f", &chorus->in_gain);
        sscanf(argv[i++], "%f", &chorus->out_gain);
        while ( i < argc ) {
                if ( chorus->num_chorus > MAX_CHORUS )
                {
                        lsx_fail("chorus: to many delays, use less than %i delays", MAX_CHORUS);
                        return (SOX_EOF);
                }
                sscanf(argv[i++], "%f", &chorus->delay[chorus->num_chorus]);
                sscanf(argv[i++], "%f", &chorus->decay[chorus->num_chorus]);
                sscanf(argv[i++], "%f", &chorus->speed[chorus->num_chorus]);
                sscanf(argv[i++], "%f", &chorus->depth[chorus->num_chorus]);
                if ( !strcmp(argv[i], "-s"))
                        chorus->modulation[chorus->num_chorus] = MOD_SINE;
                else if ( ! strcmp(argv[i], "-t"))
                        chorus->modulation[chorus->num_chorus] = MOD_TRIANGLE;
                else
                  return lsx_usage(effp);
                i++;
                chorus->num_chorus++;
        }
        return (SOX_SUCCESS);
}

/*
 * Prepare for processing.
 */
static int sox_chorus_start(sox_effect_t * effp)
{
        priv_t * chorus = (priv_t *) effp->priv;
        int i;
        float sum_in_volume;

        chorus->maxsamples = 0;

        if ( chorus->in_gain < 0.0 )
        {
                lsx_fail("chorus: gain-in must be positive!");
                return (SOX_EOF);
        }
        if ( chorus->in_gain > 1.0 )
        {
                lsx_fail("chorus: gain-in must be less than 1.0!");
                return (SOX_EOF);
        }
        if ( chorus->out_gain < 0.0 )
        {
                lsx_fail("chorus: gain-out must be positive!");
                return (SOX_EOF);
        }
        for ( i = 0; i < chorus->num_chorus; i++ ) {
                chorus->samples[i] = (int) ( ( chorus->delay[i] +
                        chorus->depth[i] ) * effp->in_signal.rate / 1000.0);
                chorus->depth_samples[i] = (int) (chorus->depth[i] *
                        effp->in_signal.rate / 1000.0);

                if ( chorus->delay[i] < 20.0 )
                {
                        lsx_fail("chorus: delay must be more than 20.0 msec!");
                        return (SOX_EOF);
                }
                if ( chorus->delay[i] > 100.0 )
                {
                        lsx_fail("chorus: delay must be less than 100.0 msec!");
                        return (SOX_EOF);
                }
                if ( chorus->speed[i] < 0.1 )
                {
                        lsx_fail("chorus: speed must be more than 0.1 Hz!");
                        return (SOX_EOF);
                }
                if ( chorus->speed[i] > 5.0 )
                {
                        lsx_fail("chorus: speed must be less than 5.0 Hz!");
                        return (SOX_EOF);
                }
                if ( chorus->depth[i] < 0.0 )
                {
                        lsx_fail("chorus: delay must be more positive!");
                        return (SOX_EOF);
                }
                if ( chorus->depth[i] > 10.0 )
                {
                    lsx_fail("chorus: delay must be less than 10.0 msec!");
                    return (SOX_EOF);
                }
                if ( chorus->decay[i] < 0.0 )
                {
                        lsx_fail("chorus: decay must be positive!" );
                        return (SOX_EOF);
                }
                if ( chorus->decay[i] > 1.0 )
                {
                        lsx_fail("chorus: decay must be less that 1.0!" );
                        return (SOX_EOF);
                }
                chorus->length[i] = effp->in_signal.rate / chorus->speed[i];
                chorus->lookup_tab[i] = lsx_malloc(sizeof (int) * chorus->length[i]);

                if (chorus->modulation[i] == MOD_SINE)
                  lsx_generate_wave_table(SOX_WAVE_SINE, SOX_INT, chorus->lookup_tab[i],
                                         (size_t)chorus->length[i], 0., (double)chorus->depth_samples[i], 0.);
                else
                  lsx_generate_wave_table(SOX_WAVE_TRIANGLE, SOX_INT, chorus->lookup_tab[i],
                                         (size_t)chorus->length[i],
                                         (double)(chorus->samples[i] - 1 - 2 * chorus->depth_samples[i]),
                                         (double)(chorus->samples[i] - 1), 3 * M_PI_2);
                chorus->phase[i] = 0;

                if ( chorus->samples[i] > chorus->maxsamples )
                  chorus->maxsamples = chorus->samples[i];
        }

        /* Be nice and check the hint with warning, if... */
        sum_in_volume = 1.0;
        for ( i = 0; i < chorus->num_chorus; i++ )
                sum_in_volume += chorus->decay[i];
        if ( chorus->in_gain * ( sum_in_volume ) > 1.0 / chorus->out_gain )
        lsx_warn("chorus: warning >>> gain-out can cause saturation or clipping of output <<<");


        chorus->chorusbuf = lsx_malloc(sizeof (float) * chorus->maxsamples);
        for ( i = 0; i < chorus->maxsamples; i++ )
                chorus->chorusbuf[i] = 0.0;

        chorus->counter = 0;
        chorus->fade_out = chorus->maxsamples;

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* TODO: calculate actual length */

        return (SOX_SUCCESS);
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
static int sox_chorus_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                   size_t *isamp, size_t *osamp)
{
        priv_t * chorus = (priv_t *) effp->priv;
        int i;
        float d_in, d_out;
        sox_sample_t out;
        size_t len = min(*isamp, *osamp);
        *isamp = *osamp = len;

        while (len--) {
                /* Store delays as 24-bit signed longs */
                d_in = (float) *ibuf++ / 256;
                /* Compute output first */
                d_out = d_in * chorus->in_gain;
                for ( i = 0; i < chorus->num_chorus; i++ )
                        d_out += chorus->chorusbuf[(chorus->maxsamples +
                        chorus->counter - chorus->lookup_tab[i][chorus->phase[i]]) %
                        chorus->maxsamples] * chorus->decay[i];
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * chorus->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Mix decay of delay and input */
                chorus->chorusbuf[chorus->counter] = d_in;
                chorus->counter =
                        ( chorus->counter + 1 ) % chorus->maxsamples;
                for ( i = 0; i < chorus->num_chorus; i++ )
                        chorus->phase[i]  =
                                ( chorus->phase[i] + 1 ) % chorus->length[i];
        }
        /* processed all samples */
        return (SOX_SUCCESS);
}

/*
 * Drain out reverb lines.
 */
static int sox_chorus_drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
        priv_t * chorus = (priv_t *) effp->priv;
        size_t done;
        int i;

        float d_in, d_out;
        sox_sample_t out;

        done = 0;
        while ( ( done < *osamp ) && ( done < chorus->fade_out ) ) {
                d_in = 0;
                d_out = 0;
                /* Compute output first */
                for ( i = 0; i < chorus->num_chorus; i++ )
                        d_out += chorus->chorusbuf[(chorus->maxsamples +
                chorus->counter - chorus->lookup_tab[i][chorus->phase[i]]) %
                chorus->maxsamples] * chorus->decay[i];
                /* Adjust the output volume and size to 24 bit */
                d_out = d_out * chorus->out_gain;
                out = SOX_24BIT_CLIP_COUNT((sox_sample_t) d_out, effp->clips);
                *obuf++ = out * 256;
                /* Mix decay of delay and input */
                chorus->chorusbuf[chorus->counter] = d_in;
                chorus->counter =
                        ( chorus->counter + 1 ) % chorus->maxsamples;
                for ( i = 0; i < chorus->num_chorus; i++ )
                        chorus->phase[i]  =
                                ( chorus->phase[i] + 1 ) % chorus->length[i];
                done++;
                chorus->fade_out--;
        }
        /* samples played, it remains */
        *osamp = done;
        if (chorus->fade_out == 0)
            return SOX_EOF;
        else
            return SOX_SUCCESS;
}

/*
 * Clean up chorus effect.
 */
static int sox_chorus_stop(sox_effect_t * effp)
{
        priv_t * chorus = (priv_t *) effp->priv;
        int i;

        free(chorus->chorusbuf);
        chorus->chorusbuf = NULL;
        for ( i = 0; i < chorus->num_chorus; i++ ) {
                free(chorus->lookup_tab[i]);
                chorus->lookup_tab[i] = NULL;
        }
        return (SOX_SUCCESS);
}

static sox_effect_handler_t sox_chorus_effect = {
  "chorus",
  "gain-in gain-out delay decay speed depth [ -s | -t ]",
  SOX_EFF_LENGTH | SOX_EFF_GAIN,
  sox_chorus_getopts,
  sox_chorus_start,
  sox_chorus_flow,
  sox_chorus_drain,
  sox_chorus_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_chorus_effect_fn(void)
{
    return &sox_chorus_effect;
}
