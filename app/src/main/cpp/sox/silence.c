/* Silence effect for SoX
 * by Heikki Leinonen (heilei@iki.fi) 25.03.2001
 * Major Modifications by Chris Bagwell 06.08.2001
 * Minor addition by Donnie Smith 13.08.2003
 *
 * This effect can delete samples from the start of a sound file
 * until it sees a specified count of samples exceed a given threshold
 * (any of the channels).
 * This effect can also delete samples from the end of a sound file
 * when it sees a specified count of samples below a given threshold
 * (all channels).
 * It may also be used to delete samples anywhere in a sound file.
 * Thesholds can be given as either a percentage or in decibels.
 */

#include "sox_i.h"

#include <string.h>

/* Private data for silence effect. */

#define SILENCE_TRIM        0
#define SILENCE_TRIM_FLUSH  1
#define SILENCE_COPY        2
#define SILENCE_COPY_FLUSH  3
#define SILENCE_STOP        4

typedef struct {
    char        start;
    int         start_periods;
    char        *start_duration_str;
    size_t   start_duration;
    double      start_threshold;
    char        start_unit; /* "d" for decibels or "%" for percent. */
    int         restart;

    sox_sample_t *start_holdoff;
    size_t   start_holdoff_offset;
    size_t   start_holdoff_end;
    int         start_found_periods;

    char        stop;
    int         stop_periods;
    char        *stop_duration_str;
    size_t   stop_duration;
    double      stop_threshold;
    char        stop_unit;

    sox_sample_t *stop_holdoff;
    size_t   stop_holdoff_offset;
    size_t   stop_holdoff_end;
    int         stop_found_periods;

    double      *window;
    double      *window_current;
    double      *window_end;
    size_t   window_size;
    double      rms_sum;

    char        leave_silence;

    /* State Machine */
    char        mode;
} priv_t;

static void clear_rms(sox_effect_t * effp)

{
    priv_t * silence = (priv_t *) effp->priv;

    memset(silence->window, 0,
           silence->window_size * sizeof(double));

    silence->window_current = silence->window;
    silence->window_end = silence->window + silence->window_size;
    silence->rms_sum = 0;
}

static int sox_silence_getopts(sox_effect_t * effp, int argc, char **argv)
{
    priv_t *   silence = (priv_t *) effp->priv;
    int parse_count;
    uint64_t temp;
    const char *n;
  --argc, ++argv;

    /* check for option switches */
    silence->leave_silence = sox_false;
    if (argc > 0)
    {
        if (!strcmp("-l", *argv)) {
            argc--; argv++;
            silence->leave_silence = sox_true;
        }
    }

    if (argc < 1)
      return lsx_usage(effp);

    /* Parse data related to trimming front side */
    silence->start = sox_false;
    if (sscanf(argv[0], "%d", &silence->start_periods) != 1)
      return lsx_usage(effp);
    if (silence->start_periods < 0)
    {
        lsx_fail("Periods must not be negative");
        return(SOX_EOF);
    }
    argv++;
    argc--;

    if (silence->start_periods > 0)
    {
        silence->start = sox_true;
        if (argc < 2)
          return lsx_usage(effp);

        /* We do not know the sample rate so we can not fully
         * parse the duration info yet.  So save argument off
         * for future processing.
         */
        silence->start_duration_str = lsx_strdup(argv[0]);
        /* Perform a fake parse to do error checking */
        n = lsx_parsesamples(0.,silence->start_duration_str,&temp,'s');
        if (!n || *n)
          return lsx_usage(effp);
        silence->start_duration = temp;

        parse_count = sscanf(argv[1], "%lf%c", &silence->start_threshold,
                &silence->start_unit);
        if (parse_count < 1)
          return lsx_usage(effp);
        else if (parse_count < 2)
            silence->start_unit = '%';

        argv++; argv++;
        argc--; argc--;
    }

    silence->stop = sox_false;
    /* Parse data needed for trimming of backside */
    if (argc > 0)
    {
        if (argc < 3)
          return lsx_usage(effp);
        if (sscanf(argv[0], "%d", &silence->stop_periods) != 1)
          return lsx_usage(effp);
        if (silence->stop_periods < 0)
        {
            silence->stop_periods = -silence->stop_periods;
            silence->restart = 1;
        }
        else
            silence->restart = 0;
        silence->stop = sox_true;
        argv++;
        argc--;

        /* We do not know the sample rate so we can not fully
         * parse the duration info yet.  So save argument off
         * for future processing.
         */
        silence->stop_duration_str = lsx_strdup(argv[0]);
        /* Perform a fake parse to do error checking */
        n = lsx_parsesamples(0.,silence->stop_duration_str,&temp,'s');
        if (!n || *n)
          return lsx_usage(effp);
        silence->stop_duration = temp;

        parse_count = sscanf(argv[1], "%lf%c", &silence->stop_threshold,
                             &silence->stop_unit);
        if (parse_count < 1)
          return lsx_usage(effp);
        else if (parse_count < 2)
            silence->stop_unit = '%';

        argv++; argv++;
        argc--; argc--;
    }

    /* Error checking */
    if (silence->start)
    {
        if ((silence->start_unit != '%') && (silence->start_unit != 'd'))
        {
            lsx_fail("Invalid unit specified");
            return lsx_usage(effp);
        }
        if ((silence->start_unit == '%') && ((silence->start_threshold < 0.0)
            || (silence->start_threshold > 100.0)))
        {
            lsx_fail("silence threshold should be between 0.0 and 100.0 %%");
            return (SOX_EOF);
        }
        if ((silence->start_unit == 'd') && (silence->start_threshold >= 0.0))
        {
            lsx_fail("silence threshold should be less than 0.0 dB");
            return(SOX_EOF);
        }
    }

    if (silence->stop)
    {
        if ((silence->stop_unit != '%') && (silence->stop_unit != 'd'))
        {
            lsx_fail("Invalid unit specified");
            return(SOX_EOF);
        }
        if ((silence->stop_unit == '%') && ((silence->stop_threshold < 0.0) ||
                    (silence->stop_threshold > 100.0)))
        {
            lsx_fail("silence threshold should be between 0.0 and 100.0 %%");
            return (SOX_EOF);
        }
        if ((silence->stop_unit == 'd') && (silence->stop_threshold >= 0.0))
        {
            lsx_fail("silence threshold should be less than 0.0 dB");
            return(SOX_EOF);
        }
    }
    return(SOX_SUCCESS);
}

static int sox_silence_start(sox_effect_t * effp)
{
    priv_t *silence = (priv_t *)effp->priv;
    uint64_t temp;

    /* When you want to remove silence, small window sizes are
     * better or else RMS will look like non-silence at
     * aburpt changes from load to silence.
     */
    silence->window_size = (effp->in_signal.rate / 50) * 
        effp->in_signal.channels;
    silence->window = lsx_malloc(silence->window_size * sizeof(double));

    clear_rms(effp);

    /* Now that we know sample rate, reparse duration. */
    if (silence->start)
    {
        if (lsx_parsesamples(effp->in_signal.rate, silence->start_duration_str,
                             &temp, 's') == NULL)
            return lsx_usage(effp);
        silence->start_duration = temp * effp->in_signal.channels;
    }
    if (silence->stop)
    {
        if (lsx_parsesamples(effp->in_signal.rate,silence->stop_duration_str,
                             &temp,'s') == NULL)
            return lsx_usage(effp);
        silence->stop_duration = temp * effp->in_signal.channels;
    }

    if (silence->start)
        silence->mode = SILENCE_TRIM;
    else
        silence->mode = SILENCE_COPY;

    silence->start_holdoff = lsx_malloc(sizeof(sox_sample_t)*silence->start_duration);
    silence->start_holdoff_offset = 0;
    silence->start_holdoff_end = 0;
    silence->start_found_periods = 0;

    silence->stop_holdoff = lsx_malloc(sizeof(sox_sample_t)*silence->stop_duration);
    silence->stop_holdoff_offset = 0;
    silence->stop_holdoff_end = 0;
    silence->stop_found_periods = 0;

    effp->out_signal.length = SOX_UNKNOWN_LEN; /* depends on input data */

    return(SOX_SUCCESS);
}

static sox_bool aboveThreshold(sox_effect_t const * effp,
    sox_sample_t value /* >= 0 */, double threshold, int unit)
{
  /* When scaling low bit data, noise values got scaled way up */
  /* Only consider the original bits when looking for silence */
  sox_sample_t masked_value = value & (-1 << (32 - effp->in_signal.precision));

  double scaled_value = (double)masked_value / SOX_SAMPLE_MAX;

  if (unit == '%')
    scaled_value *= 100;
  else if (unit == 'd')
    scaled_value = linear_to_dB(scaled_value);

  return scaled_value > threshold;
}

static sox_sample_t compute_rms(sox_effect_t * effp, sox_sample_t sample)
{
    priv_t * silence = (priv_t *) effp->priv;
    double new_sum;
    sox_sample_t rms;

    new_sum = silence->rms_sum;
    new_sum -= *silence->window_current;
    new_sum += ((double)sample * (double)sample);

    rms = sqrt(new_sum / silence->window_size);

    return (rms);
}

static void update_rms(sox_effect_t * effp, sox_sample_t sample)
{
    priv_t * silence = (priv_t *) effp->priv;

    silence->rms_sum -= *silence->window_current;
    *silence->window_current = ((double)sample * (double)sample);
    silence->rms_sum += *silence->window_current;

    silence->window_current++;
    if (silence->window_current >= silence->window_end)
        silence->window_current = silence->window;
}

/* Process signed long samples from ibuf to obuf. */
/* Return number of samples processed in isamp and osamp. */
static int sox_silence_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                    size_t *isamp, size_t *osamp)
{
    priv_t * silence = (priv_t *) effp->priv;
    int threshold;
    size_t i, j;
    size_t nrOfTicks, /* sometimes wide, sometimes non-wide samples */
      nrOfInSamplesRead, nrOfOutSamplesWritten; /* non-wide samples */

    nrOfInSamplesRead = 0;
    nrOfOutSamplesWritten = 0;

    switch (silence->mode)
    {
        case SILENCE_TRIM:
            /* Reads and discards all input data until it detects a
             * sample that is above the specified threshold.  Turns on
             * copy mode when detected.
             * Need to make sure and copy input in groups of "channels" to
             * prevent getting buffers out of sync.
             * nrOfTicks counts wide samples here.
             */
silence_trim:
            nrOfTicks = min((*isamp-nrOfInSamplesRead),
                            (*osamp-nrOfOutSamplesWritten)) /
                           effp->in_signal.channels;
            for(i = 0; i < nrOfTicks; i++)
            {
                threshold = 0;
                for (j = 0; j < effp->in_signal.channels; j++)
                {
                    threshold |= aboveThreshold(effp,
                                                compute_rms(effp, ibuf[j]),
                                                silence->start_threshold,
                                                silence->start_unit);
                }

                if (threshold)
                {
                    /* Add to holdoff buffer */
                    for (j = 0; j < effp->in_signal.channels; j++)
                    {
                        update_rms(effp, *ibuf);
                        silence->start_holdoff[
                            silence->start_holdoff_end++] = *ibuf++;
                        nrOfInSamplesRead++;
                    }

                    if (silence->start_holdoff_end >=
                            silence->start_duration)
                    {
                        if (++silence->start_found_periods >=
                                silence->start_periods)
                        {
                            silence->mode = SILENCE_TRIM_FLUSH;
                            goto silence_trim_flush;
                        }
                        /* Trash holdoff buffer since its not
                         * needed.  Start looking again.
                         */
                        silence->start_holdoff_offset = 0;
                        silence->start_holdoff_end = 0;
                    }
                }
                else /* !above Threshold */
                {
                    silence->start_holdoff_end = 0;
                    for (j = 0; j < effp->in_signal.channels; j++)
                    {
                        update_rms(effp, ibuf[j]);
                    }
                    ibuf += effp->in_signal.channels;
                    nrOfInSamplesRead += effp->in_signal.channels;
                }
            } /* for nrOfTicks */
            break;

        case SILENCE_TRIM_FLUSH:
             /* nrOfTicks counts non-wide samples here. */
silence_trim_flush:
            nrOfTicks = min((silence->start_holdoff_end -
                             silence->start_holdoff_offset),
                             (*osamp-nrOfOutSamplesWritten));
            nrOfTicks -= nrOfTicks % effp->in_signal.channels;
            for(i = 0; i < nrOfTicks; i++)
            {
                *obuf++ = silence->start_holdoff[silence->start_holdoff_offset++];
                nrOfOutSamplesWritten++;
            }

            /* If fully drained holdoff then switch to copy mode */
            if (silence->start_holdoff_offset == silence->start_holdoff_end)
            {
                silence->start_holdoff_offset = 0;
                silence->start_holdoff_end = 0;
                silence->mode = SILENCE_COPY;
                goto silence_copy;
            }
            break;

        case SILENCE_COPY:
            /* Attempts to copy samples into output buffer.
             *
             * Case B:
             * If not looking for silence to terminate copy then
             * blindly copy data into output buffer.
             *
             * Case A:
             *
             * Case 1a:
             * If previous silence was detect then see if input sample is
             * above threshold.  If found then flush out hold off buffer
             * and copy over to output buffer.
             *
             * Case 1b:
             * If no previous silence detect then see if input sample
             * is above threshold.  If found then copy directly
             * to output buffer.
             *
             * Case 2:
             * If not above threshold then silence is detect so
             * store in hold off buffer and do not write to output
             * buffer.  Even though it wasn't put in output
             * buffer, inform user that input was consumed.
             *
             * If hold off buffer is full after this then stop
             * copying data and discard data in hold off buffer.
             *
             * Special leave_silence logic:
             *
             * During this mode, go ahead and copy input
             * samples to output buffer instead of holdoff buffer
             * Then also short ciruit any flushes that would occur
             * when non-silence is detect since samples were already
             * copied.  This has the effect of always leaving
             * holdoff[] amount of silence but deleting any
             * beyond that amount.
             *
             * nrOfTicks counts wide samples here.
             */
silence_copy:
            nrOfTicks = min((*isamp-nrOfInSamplesRead),
                            (*osamp-nrOfOutSamplesWritten)) /
                           effp->in_signal.channels;
            if (silence->stop)
            {
                /* Case A */
                for(i = 0; i < nrOfTicks; i++)
                {
                    threshold = 1;
                    for (j = 0; j < effp->in_signal.channels; j++)
                    {
                        threshold &= aboveThreshold(effp,
                                                    compute_rms(effp, ibuf[j]),
                                                    silence->stop_threshold,
                                                    silence->stop_unit);
                    }

                    /* Case 1a
                     * If above threshold, check to see if we where holding
                     * off previously.  If so then flush this buffer.
                     * We haven't incremented any pointers yet so nothing
                     * is lost.
                     *
                     * If user wants to leave_silence, then we
                     * were already copying the data and so no
                     * need to flush the old data.  Just resume
                     * copying as if we were not holding off.
                     */
                    if (threshold && silence->stop_holdoff_end
                        && !silence->leave_silence)
                    {
                        silence->mode = SILENCE_COPY_FLUSH;
                        goto silence_copy_flush;
                    }
                    /* Case 1b */
                    else if (threshold)
                    {
                        /* Not holding off so copy into output buffer */
                        for (j = 0; j < effp->in_signal.channels; j++)
                        {
                            update_rms(effp, *ibuf);
                            *obuf++ = *ibuf++;
                            nrOfInSamplesRead++;
                            nrOfOutSamplesWritten++;
                        }
                    }
                    /* Case 2 */
                    else if (!threshold)
                    {
                        /* Add to holdoff buffer */
                        for (j = 0; j < effp->in_signal.channels; j++)
                        {
                            update_rms(effp, *ibuf);
                            if (silence->leave_silence) {
                                *obuf++ = *ibuf;
                                nrOfOutSamplesWritten++;
                            }
                            silence->stop_holdoff[
                                silence->stop_holdoff_end++] = *ibuf++;
                            nrOfInSamplesRead++;
                        }

                        /* Check if holdoff buffer is greater than duration
                         */
                        if (silence->stop_holdoff_end >=
                                silence->stop_duration)
                        {
                            /* Increment found counter and see if this
                             * is the last period.  If so then exit.
                             */
                            if (++silence->stop_found_periods >=
                                    silence->stop_periods)
                            {
                                silence->stop_holdoff_offset = 0;
                                silence->stop_holdoff_end = 0;
                                if (!silence->restart)
                                {
                                    *isamp = nrOfInSamplesRead;
                                    *osamp = nrOfOutSamplesWritten;
                                    silence->mode = SILENCE_STOP;
                                    /* Return SOX_EOF since no more processing */
                                    return (SOX_EOF);
                                }
                                else
                                {
                                    silence->stop_found_periods = 0;
                                    silence->start_found_periods = 0;
                                    silence->start_holdoff_offset = 0;
                                    silence->start_holdoff_end = 0;
                                    clear_rms(effp);
                                    silence->mode = SILENCE_TRIM;

                                    goto silence_trim;
                                }
                            }
                            else
                            {
                                /* Flush this buffer and start
                                 * looking again.
                                 */
                                silence->mode = SILENCE_COPY_FLUSH;
                                goto silence_copy_flush;
                            }
                            break;
                        } /* Filled holdoff buffer */
                    } /* Detected silence */
                } /* For # of samples */
            } /* Trimming off backend */
            else /* !(silence->stop) */
            {
                /* Case B */
                memcpy(obuf, ibuf, sizeof(sox_sample_t)*nrOfTicks*
                                   effp->in_signal.channels);
                nrOfInSamplesRead += (nrOfTicks*effp->in_signal.channels);
                nrOfOutSamplesWritten += (nrOfTicks*effp->in_signal.channels);
            }
            break;

        case SILENCE_COPY_FLUSH:
             /* nrOfTicks counts non-wide samples here. */
silence_copy_flush:
            nrOfTicks = min((silence->stop_holdoff_end -
                                silence->stop_holdoff_offset),
                            (*osamp-nrOfOutSamplesWritten));
            nrOfTicks -= nrOfTicks % effp->in_signal.channels;

            for(i = 0; i < nrOfTicks; i++)
            {
                *obuf++ = silence->stop_holdoff[silence->stop_holdoff_offset++];
                nrOfOutSamplesWritten++;
            }

            /* If fully drained holdoff then return to copy mode */
            if (silence->stop_holdoff_offset == silence->stop_holdoff_end)
            {
                silence->stop_holdoff_offset = 0;
                silence->stop_holdoff_end = 0;
                silence->mode = SILENCE_COPY;
                goto silence_copy;
            }
            break;

        case SILENCE_STOP:
            /* This code can't be reached. */
            nrOfInSamplesRead = *isamp;
            break;
        }

        *isamp = nrOfInSamplesRead;
        *osamp = nrOfOutSamplesWritten;

        return (SOX_SUCCESS);
}

static int sox_silence_drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
    priv_t * silence = (priv_t *) effp->priv;
    size_t i;
    size_t nrOfTicks, nrOfOutSamplesWritten = 0; /* non-wide samples */

    /* Only if in flush mode will there be possible samples to write
     * out during drain() call.
     */
    if (silence->mode == SILENCE_COPY_FLUSH ||
        silence->mode == SILENCE_COPY)
    {
        nrOfTicks = min((silence->stop_holdoff_end -
                            silence->stop_holdoff_offset), *osamp);
        nrOfTicks -= nrOfTicks % effp->in_signal.channels;
        for(i = 0; i < nrOfTicks; i++)
        {
            *obuf++ = silence->stop_holdoff[silence->stop_holdoff_offset++];
            nrOfOutSamplesWritten++;
        }

        /* If fully drained holdoff then stop */
        if (silence->stop_holdoff_offset == silence->stop_holdoff_end)
        {
            silence->stop_holdoff_offset = 0;
            silence->stop_holdoff_end = 0;
            silence->mode = SILENCE_STOP;
        }
    }

    *osamp = nrOfOutSamplesWritten;
    if (silence->mode == SILENCE_STOP || *osamp == 0)
        return SOX_EOF;
    else
        return SOX_SUCCESS;
}

static int sox_silence_stop(sox_effect_t * effp)
{
  priv_t * silence = (priv_t *) effp->priv;

  free(silence->window);
  free(silence->start_holdoff);
  free(silence->stop_holdoff);

  return(SOX_SUCCESS);
}

static int lsx_kill(sox_effect_t * effp)
{
  priv_t * silence = (priv_t *) effp->priv;

  free(silence->start_duration_str);
  free(silence->stop_duration_str);

  return SOX_SUCCESS;
}

static sox_effect_handler_t sox_silence_effect = {
  "silence",
  "[ -l ] above_periods [ duration threshold[d|%] ] [ below_periods duration threshold[d|%] ]",
  SOX_EFF_MCHAN | SOX_EFF_MODIFY | SOX_EFF_LENGTH,
  sox_silence_getopts,
  sox_silence_start,
  sox_silence_flow,
  sox_silence_drain,
  sox_silence_stop,
  lsx_kill, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_silence_effect_fn(void)
{
    return &sox_silence_effect;
}
