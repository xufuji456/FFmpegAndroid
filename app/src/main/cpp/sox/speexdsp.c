/* libSoX effect: SpeexDsp effect to apply processing from libspeexdsp.
 *
 * Copyright 1999-2009 Chris Bagwell And SoX Contributors
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

#ifdef HAVE_SPEEXDSP

#include <speex/speex_types.h>
#include <speex/speex_preprocess.h>

/* Private data for effect */
typedef struct speexdsp_priv_t {
    size_t buffer_end;        /* Index of the end of the buffer. */
    size_t buffer_ipos;       /* Index for the next input sample. */
    size_t buffer_opos;       /* Index of the next sample that has not been drained. */
    int16_t* buffer;          /* Work buffer. */
    SpeexPreprocessState* sps;/* DSP state. */
    size_t agc;               /* Param: Automatic Gain Control target volume level: 0 to disable, or 1-100 (target volume). */
    size_t denoise;           /* Param: Denoise: 0 to disable, or maximum noise attenuation in dB. */
    size_t dereverb;          /* Param: Dereverb: 0 to disable, 1 to enable. */
    size_t frames_per_second; /* Param: Used to compute buffer size from sample rate. */
    size_t samples_per_frame; /* Param: Used to compute buffer size directly. Default is to use frames_per_second instead. */
} priv_t;

static int get_param(
    int* pArgc,
    char*** pArgv,
    size_t* pParam,
    size_t default_val,
    size_t min_valid,
    size_t max_valid)
{
    *pParam = default_val;
    if (*pArgc > 1 && (*pArgv)[1][0] != '-')
    {
        char* arg_end;
        *pParam = strtoul((*pArgv)[1], &arg_end, 0);
        if (!arg_end || arg_end[0] || *pParam < min_valid || max_valid <= *pParam)
            return 0;

        --*pArgc;
        ++*pArgv;
    }

    return 1;
}

/*
 * Process command-line options but don't do other
 * initialization now: effp->in_signal & effp->out_signal are not
 * yet filled in.
 */
static int getopts(sox_effect_t* effp, int argc, char** argv)
{
    priv_t* p = (priv_t*)effp->priv;
    const size_t agcDefault = 100;
    const size_t denoiseDefault = 15;
    const size_t fpsDefault = 50;

    for (argc--, argv++; argc; argc--, argv++)
    {
        if (!strcasecmp("-agc", argv[0]))
        {
            /* AGC level argument is optional. If not specified, it defaults to agcDefault.
               If specified, it must be from 0 to 100. */
            if (!get_param(&argc, &argv, &p->agc, agcDefault, 0, 100))
            {
                lsx_fail("Invalid argument \"%s\" to -agc parameter - expected number from 0 to 100.", argv[1]);
                return lsx_usage(effp);
            }
        }
        else if (!strcasecmp("-denoise", argv[0]))
        {
            /* Denoise level argument is optional. If not specified, it defaults to denoiseDefault.
               If specified, it must be from 0 to 100. */
            if (!get_param(&argc, &argv, &p->denoise, denoiseDefault, 0, 100))
            {
                lsx_fail("Invalid argument \"%s\" to -denoise parameter - expected number from 0 to 100.", argv[1]);
                return lsx_usage(effp);
            }
        }
        else if (!strcasecmp("-dereverb", argv[0]))
        {
            p->dereverb = 1;
        }
        else if (!strcasecmp("-spf", argv[0]))
        {
            /* If samples_per_frame option is given, argument is required and must be
               greater than 0. */
            if (!get_param(&argc, &argv, &p->samples_per_frame, 0, 1, 1000000000) || !p->samples_per_frame)
            {
                lsx_fail("Invalid argument \"%s\" to -spf parameter - expected positive number.", argv[1]);
                return lsx_usage(effp);
            }
        }
        else if (!strcasecmp("-fps", argv[0]))
        {
            /* If frames_per_second option is given, argument is required and must be
               from 1 to 100. This will be used later to compute samples_per_frame once
               we know the sample rate). */
            if (!get_param(&argc, &argv, &p->frames_per_second, 0, 1, 100) || !p->frames_per_second)
            {
                lsx_fail("Invalid argument \"%s\" to -fps parameter - expected number from 1 to 100.", argv[1]);
                return lsx_usage(effp);
            }
        }
        else
        {
            lsx_fail("Invalid parameter \"%s\".", argv[0]);
            return lsx_usage(effp);
        }
    }

    if (!p->frames_per_second)
        p->frames_per_second = fpsDefault;

    if (!p->agc && !p->denoise && !p->dereverb)
    {
        lsx_report("No features specified. Enabling default settings \"-agc %u -denoise %u\".", agcDefault, denoiseDefault);
        p->agc = agcDefault;
        p->denoise = denoiseDefault;
    }

    return SOX_SUCCESS;
}

/*
 * Do anything required when you stop reading samples.
 */
static int stop(sox_effect_t* effp)
{
    priv_t* p = (priv_t*)effp->priv;

    if (p->sps)
    {
        speex_preprocess_state_destroy(p->sps);
        p->sps = NULL;
    }

    if (p->buffer)
    {
        free(p->buffer);
        p->buffer = NULL;
    }

    return SOX_SUCCESS;
}

/*
 * Prepare processing.
 * Do all initializations.
 */
static int start(sox_effect_t* effp)
{
    priv_t* p = (priv_t*)effp->priv;
    int result = SOX_SUCCESS;
    spx_int32_t int_val;
    float float_val;

    if (p->samples_per_frame)
    {
        p->buffer_end = p->samples_per_frame;
    }
    else
    {
        p->buffer_end = effp->in_signal.rate / p->frames_per_second;
        if (!p->buffer_end)
        {
            lsx_fail("frames_per_second too large for the current sample rate.");
            return SOX_EOF;
        }
    }

    p->buffer_opos = p->buffer_end;
    effp->out_signal.precision = 16;

    p->buffer = lsx_malloc(p->buffer_end * sizeof(p->buffer[0]));
    if (!p->buffer)
    {
        result = SOX_ENOMEM;
        goto Done;
    }

    p->sps = speex_preprocess_state_init((int)p->buffer_end, (int)(effp->in_signal.rate + .5));
    if (!p->sps)
    {
        lsx_fail("Failed to initialize preprocessor DSP.");
        result = SOX_EOF;
        goto Done;
    }

    int_val = p->agc ? 1 : 2;
    speex_preprocess_ctl(p->sps, SPEEX_PREPROCESS_SET_AGC, &int_val);
    if (p->agc)
    {
        float_val = p->agc * 327.68f;
        speex_preprocess_ctl(p->sps, SPEEX_PREPROCESS_SET_AGC_LEVEL, &float_val);
    }

    int_val = p->denoise ? 1 : 2;
    speex_preprocess_ctl(p->sps, SPEEX_PREPROCESS_SET_DENOISE, &int_val);
    if (p->denoise)
    {
        int_val = -(spx_int32_t)p->denoise;
        speex_preprocess_ctl(p->sps, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &int_val);
    }

    int_val = p->dereverb ? 1 : 2;
    speex_preprocess_ctl(p->sps, SPEEX_PREPROCESS_SET_DEREVERB, &int_val);

Done:
    if (result != SOX_SUCCESS)
        stop(effp);

    return result;
}

/*
 * Process up to *isamp samples from ibuf and produce up to *osamp samples
 * in obuf.  Write back the actual numbers of samples to *isamp and *osamp.
 * Return SOX_SUCCESS or, if error occurs, SOX_EOF.
 */
static int flow(
    sox_effect_t* effp,
    const sox_sample_t* ibuf,
    sox_sample_t* obuf,
    size_t* isamp,
    size_t* osamp)
{
    priv_t* p = (priv_t*)effp->priv;
    size_t ibuf_pos = 0;
    size_t ibuf_end = *isamp;
    size_t obuf_pos = 0;
    size_t obuf_end = *osamp;
    size_t end_pos;
    SOX_SAMPLE_LOCALS;

    for (;;)
    {
        /* Write any processed data in working buffer to the output buffer. */
        end_pos = obuf_pos + min(p->buffer_end - p->buffer_opos, obuf_end - obuf_pos);
        for (; obuf_pos < end_pos; obuf_pos++, p->buffer_opos++)
            obuf[obuf_pos] = SOX_SIGNED_16BIT_TO_SAMPLE(p->buffer[p->buffer_opos], dummy);
        if (p->buffer_opos != p->buffer_end)
            break; /* Output buffer is full and we still have more processed data. */

        /* Fill working buffer from input buffer. */
        end_pos = ibuf_pos + min(p->buffer_end - p->buffer_ipos, ibuf_end - ibuf_pos);
        for (; ibuf_pos < end_pos; ibuf_pos++, p->buffer_ipos++)
            p->buffer[p->buffer_ipos] = SOX_SAMPLE_TO_SIGNED_16BIT(ibuf[ibuf_pos], effp->clips);
        if (p->buffer_ipos != p->buffer_end)
            break; /* Working buffer is not full and there is no more input data. */

        speex_preprocess_run(p->sps, p->buffer);
        p->buffer_ipos = 0;
        p->buffer_opos = 0;
    }

    *isamp = ibuf_pos;
    *osamp = obuf_pos;
    return SOX_SUCCESS;
}

/*
 * Drain out remaining samples if the effect generates any.
 */
static int drain(sox_effect_t* effp, sox_sample_t* obuf, size_t* osamp)
{
    priv_t* p = (priv_t*)effp->priv;
    size_t obuf_pos = 0;
    size_t obuf_end = *osamp;
    size_t i;
    size_t end_pos;

    /* Input that hasn't been processed yet? */
    if (p->buffer_ipos != 0)
    {
        /* DSP only works on full frames, so fill the remaining space with 0s. */
        for (i = p->buffer_ipos; i < p->buffer_end; i++)
            p->buffer[i] = 0;
        speex_preprocess_run(p->sps, p->buffer);
        p->buffer_end = p->buffer_ipos;
        p->buffer_ipos = 0;
        p->buffer_opos = 0;
    }

    end_pos = obuf_pos + min(p->buffer_end - p->buffer_opos, obuf_end - obuf_pos);
    for (; obuf_pos < end_pos; obuf_pos++, p->buffer_opos++)
        obuf[obuf_pos] = SOX_SIGNED_16BIT_TO_SAMPLE(p->buffer[p->buffer_opos], dummy);

    *osamp = obuf_pos;
    return
        p->buffer_opos != p->buffer_end
        ? SOX_SUCCESS
        : SOX_EOF;
}

/*
 * Function returning effect descriptor. This should be the only
 * externally visible object.
 */
const sox_effect_handler_t* lsx_speexdsp_effect_fn(void)
{
  /*
   * Effect descriptor.
   * If no specific processing is needed for any of
   * the 6 functions, then the function above can be deleted
   * and NULL used in place of the its name below.
   */
  static sox_effect_handler_t descriptor = {
    "speexdsp", 0, SOX_EFF_PREC | SOX_EFF_GAIN | SOX_EFF_ALPHA,
    getopts, start, flow, drain, stop, NULL, sizeof(priv_t)
  };
  static char const * lines[] = {
    "Uses the Speex DSP library to improve perceived sound quality.",
    "If no options are specified, the -agc and -denoise features are enabled.",
    "Options:",
    "-agc [target_level]    Enable automatic gain control, and optionally specify a",
    "                       target volume level from 1-100 (default is 100).",
    "-denoise [max_dB]      Enable noise reduction, and optionally specify the max",
    "                       attenuation (default is 15).",
    "-dereverb              Enable reverb reduction.",
    "-fps frames_per_second Specify the number of frames per second from 1-100",
    "                       (default is 20).",
    "-spf samples_per_frame Specify the number of samples per frame. Default is to",
    "                       use the -fps setting.",
  };
  static char * usage;
  descriptor.usage = lsx_usage_lines(&usage, lines, array_length(lines));
  return &descriptor;
}

#endif /* HAVE_SPEEXDSP */
