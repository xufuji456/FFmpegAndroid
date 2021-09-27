/* noisered - Noise Reduction Effect.
 *
 * Written by Ian Turner (vectro@vectro.org)
 *
 * Copyright 1999 Ian Turner
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Authors are not responsible for the consequences of using this software.
 */

#include "noisered.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

typedef struct {
    float *window;
    float *lastwindow;
    float *noisegate;
    float *smoothing;
} chandata_t;

/* Holds profile information */
typedef struct {
    char* profile_filename;
    float threshold;

    chandata_t *chandata;
    size_t bufdata;
} priv_t;

static void FFT(unsigned NumSamples,
         int InverseTransform,
         const float *RealIn, float *ImagIn, float *RealOut, float *ImagOut)
{
  unsigned i;
  double * work = malloc(2 * NumSamples * sizeof(*work));
  for (i = 0; i < 2 * NumSamples; i += 2) {
    work[i] = RealIn[i >> 1];
    work[i + 1] = ImagIn? ImagIn[i >> 1] : 0;
  }
  lsx_safe_cdft(2 * (int)NumSamples, InverseTransform? -1 : 1, work);
  if (InverseTransform) for (i = 0; i < 2 * NumSamples; i += 2) {
    RealOut[i >> 1] = work[i] / NumSamples;
    ImagOut[i >> 1] = work[i + 1] / NumSamples;
  }
  else for (i = 0; i < 2 * NumSamples; i += 2) {
    RealOut[i >> 1] = work[i];
    ImagOut[i >> 1] = work[i + 1];
  }
  free(work);
}

/*
 * Get the options. Default file is stdin (if the audio
 * input file isn't coming from there, of course!)
 */
static int sox_noisered_getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *) effp->priv;
  --argc, ++argv;

  if (argc > 0) {
    p->profile_filename = argv[0];
    ++argv;
    --argc;
  }

  p->threshold = 0.5;
  do {     /* break-able block */
    NUMERIC_PARAMETER(threshold, 0, 1);
  } while (0);

  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

/*
 * Prepare processing.
 * Do all initializations.
 */
static int sox_noisered_start(sox_effect_t * effp)
{
    priv_t * data = (priv_t *) effp->priv;
    size_t fchannels = 0;
    size_t channels = effp->in_signal.channels;
    size_t i;
    FILE * ifp = lsx_open_input_file(effp, data->profile_filename, sox_false);

    if (!ifp)
      return SOX_EOF;

    data->chandata = lsx_calloc(channels, sizeof(*(data->chandata)));
    data->bufdata = 0;
    for (i = 0; i < channels; i ++) {
        data->chandata[i].noisegate = lsx_calloc(FREQCOUNT, sizeof(float));
        data->chandata[i].smoothing = lsx_calloc(FREQCOUNT, sizeof(float));
        data->chandata[i].lastwindow = NULL;
    }
    while (1) {
        unsigned long i1_ul;
        size_t i1;
        float f1;
        if (2 != fscanf(ifp, " Channel %lu: %f", &i1_ul, &f1))
            break;
        i1 = i1_ul;
        if (i1 != fchannels) {
            lsx_fail("noisered: Got channel %lu, expected channel %lu.",
                    (unsigned long)i1, (unsigned long)fchannels);
            return SOX_EOF;
        }

        data->chandata[fchannels].noisegate[0] = f1;
        for (i = 1; i < FREQCOUNT; i ++) {
            if (1 != fscanf(ifp, ", %f", &f1)) {
                lsx_fail("noisered: Not enough data for channel %lu "
                        "(expected %d, got %lu)", (unsigned long)fchannels, FREQCOUNT, (unsigned long)i);
                return SOX_EOF;
            }
            data->chandata[fchannels].noisegate[i] = f1;
        }
        fchannels ++;
    }
    if (fchannels != channels) {
        lsx_fail("noisered: channel mismatch: %lu in input, %lu in profile.",
                (unsigned long)channels, (unsigned long)fchannels);
        return SOX_EOF;
    }
    if (ifp != stdin)
      fclose(ifp);

  effp->out_signal.length = SOX_UNKNOWN_LEN; /* TODO: calculate actual length */

    return (SOX_SUCCESS);
}

/* Mangle a single window. Each output sample (except the first and last
 * half-window) is the result of two distinct calls to this function,
 * due to overlapping windows. */
static void reduce_noise(chandata_t* chan, float* window, double level)
{
    float *inr, *ini, *outr, *outi, *power;
    float *smoothing = chan->smoothing;
    int i;

    inr = lsx_calloc(WINDOWSIZE * 5, sizeof(float));
    ini = inr + WINDOWSIZE;
    outr = ini + WINDOWSIZE;
    outi = outr + WINDOWSIZE;
    power = outi + WINDOWSIZE;

    for (i = 0; i < FREQCOUNT; i ++)
        assert(smoothing[i] >= 0 && smoothing[i] <= 1);

    memcpy(inr, window, WINDOWSIZE*sizeof(float));

    FFT(WINDOWSIZE, 0, inr, NULL, outr, outi);

    memcpy(inr, window, WINDOWSIZE*sizeof(float));
    lsx_apply_hann_f(inr, WINDOWSIZE);
    lsx_power_spectrum_f(WINDOWSIZE, inr, power);

    for (i = 0; i < FREQCOUNT; i ++) {
        float smooth;
        float plog;
        plog = log(power[i]);
        if (power[i] != 0 && plog < chan->noisegate[i] + level*8.0)
            smooth = 0.0;
        else
            smooth = 1.0;

        smoothing[i] = smooth * 0.5 + smoothing[i] * 0.5;
    }

    /* Audacity says this code will eliminate tinkle bells.
     * I have no idea what that means. */
    for (i = 2; i < FREQCOUNT - 2; i ++) {
        if (smoothing[i]>=0.5 &&
            smoothing[i]<=0.55 &&
            smoothing[i-1]<0.1 &&
            smoothing[i-2]<0.1 &&
            smoothing[i+1]<0.1 &&
            smoothing[i+2]<0.1)
            smoothing[i] = 0.0;
    }

    outr[0] *= smoothing[0];
    outi[0] *= smoothing[0];
    outr[FREQCOUNT-1] *= smoothing[FREQCOUNT-1];
    outi[FREQCOUNT-1] *= smoothing[FREQCOUNT-1];

    for (i = 1; i < FREQCOUNT-1; i ++) {
        int j = WINDOWSIZE - i;
        float smooth = smoothing[i];

        outr[i] *= smooth;
        outi[i] *= smooth;
        outr[j] *= smooth;
        outi[j] *= smooth;
    }

    FFT(WINDOWSIZE, 1, outr, outi, inr, ini);
    lsx_apply_hann_f(inr, WINDOWSIZE);

    memcpy(window, inr, WINDOWSIZE*sizeof(float));

    for (i = 0; i < FREQCOUNT; i ++)
        assert(smoothing[i] >= 0 && smoothing[i] <= 1);

    free(inr);
}

/* Do window management once we have a complete window, including mangling
 * the current window. */
static int process_window(sox_effect_t * effp, priv_t * data, unsigned chan_num, unsigned num_chans,
                          sox_sample_t *obuf, unsigned len) {
    int j;
    float* nextwindow;
    int use = min(len, WINDOWSIZE)-min(len,(WINDOWSIZE/2));
    chandata_t *chan = &(data->chandata[chan_num]);
    int first = (chan->lastwindow == NULL);
    SOX_SAMPLE_LOCALS;

    if ((nextwindow = lsx_calloc(WINDOWSIZE, sizeof(float))) == NULL)
        return SOX_EOF;

    memcpy(nextwindow, chan->window+WINDOWSIZE/2,
           sizeof(float)*(WINDOWSIZE/2));

    reduce_noise(chan, chan->window, data->threshold);
    if (!first) {
        for (j = 0; j < use; j ++) {
            float s = chan->window[j] + chan->lastwindow[WINDOWSIZE/2 + j];
            obuf[chan_num + num_chans * j] =
                SOX_FLOAT_32BIT_TO_SAMPLE(s, effp->clips);
        }
        free(chan->lastwindow);
    } else {
        for (j = 0; j < use; j ++) {
            assert(chan->window[j] >= -1 && chan->window[j] <= 1);
            obuf[chan_num + num_chans * j] =
                SOX_FLOAT_32BIT_TO_SAMPLE(chan->window[j], effp->clips);
        }
    }
    chan->lastwindow = chan->window;
    chan->window = nextwindow;

    return use;
}

/*
 * Read in windows, and call process_window once we get a whole one.
 */
static int sox_noisered_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                    size_t *isamp, size_t *osamp)
{
    priv_t * data = (priv_t *) effp->priv;
    size_t samp = min(*isamp, *osamp);
    size_t tracks = effp->in_signal.channels;
    size_t track_samples = samp / tracks;
    size_t ncopy = min(track_samples, WINDOWSIZE-data->bufdata);
    size_t whole_window = (ncopy + data->bufdata == WINDOWSIZE);
    int oldbuf = data->bufdata;
    size_t i;

    /* FIXME: Make this automatic for all effects */
    assert(effp->in_signal.channels == effp->out_signal.channels);

    if (whole_window)
        data->bufdata = WINDOWSIZE/2;
    else
        data->bufdata += ncopy;

    /* Reduce noise on every channel. */
    for (i = 0; i < tracks; i ++) {
        SOX_SAMPLE_LOCALS;
        chandata_t* chan = &(data->chandata[i]);
        size_t j;

        if (chan->window == NULL)
            chan->window = lsx_calloc(WINDOWSIZE, sizeof(float));

        for (j = 0; j < ncopy; j ++)
            chan->window[oldbuf + j] =
                SOX_SAMPLE_TO_FLOAT_32BIT(ibuf[i + tracks * j], effp->clips);

        if (!whole_window)
            continue;
        else
            process_window(effp, data, (unsigned) i, (unsigned) tracks, obuf, (unsigned) (oldbuf + ncopy));
    }

    *isamp = tracks*ncopy;
    if (whole_window)
        *osamp = tracks*(WINDOWSIZE/2);
    else
        *osamp = 0;

    return SOX_SUCCESS;
}

/*
 * We have up to half a window left to dump.
 */

static int sox_noisered_drain(sox_effect_t * effp, sox_sample_t *obuf, size_t *osamp)
{
    priv_t * data = (priv_t *)effp->priv;
    unsigned i;
    unsigned tracks = effp->in_signal.channels;
    for (i = 0; i < tracks; i ++)
        *osamp = process_window(effp, data, i, tracks, obuf, (unsigned) data->bufdata);

    /* FIXME: This is very picky.  osamp needs to be big enough to get all
     * remaining data or it will be discarded.
     */
    return (SOX_EOF);
}

/*
 * Clean up.
 */
static int sox_noisered_stop(sox_effect_t * effp)
{
    priv_t * data = (priv_t *) effp->priv;
    size_t i;

    for (i = 0; i < effp->in_signal.channels; i ++) {
        chandata_t* chan = &(data->chandata[i]);
        free(chan->lastwindow);
        free(chan->window);
        free(chan->smoothing);
        free(chan->noisegate);
    }

    free(data->chandata);

    return (SOX_SUCCESS);
}

static sox_effect_handler_t sox_noisered_effect = {
  "noisered",
  "[profile-file [amount]]",
  SOX_EFF_MCHAN|SOX_EFF_LENGTH,
  sox_noisered_getopts,
  sox_noisered_start,
  sox_noisered_flow,
  sox_noisered_drain,
  sox_noisered_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_noisered_effect_fn(void)
{
    return &sox_noisered_effect;
}
