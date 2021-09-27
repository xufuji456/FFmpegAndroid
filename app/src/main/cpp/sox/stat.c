/* libSoX statistics "effect" file.
 *
 * Compute various statistics on file and print them.
 *
 * Output is unmodified from input.
 *
 * July 5, 1991
 * Copyright 1991 Lance Norskog And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"

#include <string.h>

/* Private data for stat effect */
typedef struct {
  double min, max, mid;
  double asum;
  double sum1, sum2;            /* amplitudes */
  double dmin, dmax;
  double dsum1, dsum2;          /* deltas */
  double scale;                 /* scale-factor */
  double last;                  /* previous sample */
  uint64_t read;               /* samples processed */
  int volume;
  int srms;
  int fft;
  unsigned long bin[4];
  float *re_in;
  float *re_out;
  unsigned long fft_size;
  unsigned long fft_offset;
} priv_t;


/*
 * Process options
 */
static int sox_stat_getopts(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * stat = (priv_t *) effp->priv;

  stat->scale = SOX_SAMPLE_MAX;
  stat->volume = 0;
  stat->srms = 0;
  stat->fft = 0;

  --argc, ++argv;
  for (; argc > 0; argc--, argv++) {
    if (!(strcmp(*argv, "-v")))
      stat->volume = 1;
    else if (!(strcmp(*argv, "-s"))) {
      if (argc <= 1) {
        lsx_fail("-s option: invalid argument");
        return SOX_EOF;
      }
      argc--, argv++;              /* Move to next argument. */
      if (!sscanf(*argv, "%lf", &stat->scale)) {
        lsx_fail("-s option: invalid argument");
        return SOX_EOF;
      }
    } else if (!(strcmp(*argv, "-rms")))
      stat->srms = 1;
    else if (!(strcmp(*argv, "-freq")))
      stat->fft = 1;
    else if (!(strcmp(*argv, "-d")))
      stat->volume = 2;
    else {
      lsx_fail("Summary effect: unknown option");
      return SOX_EOF;
    }
  }

  return SOX_SUCCESS;
}

/*
 * Prepare processing.
 */
static int sox_stat_start(sox_effect_t * effp)
{
  priv_t * stat = (priv_t *) effp->priv;
  int i;

  stat->min = stat->max = stat->mid = 0;
  stat->asum = 0;
  stat->sum1 = stat->sum2 = 0;

  stat->dmin = stat->dmax = 0;
  stat->dsum1 = stat->dsum2 = 0;

  stat->last = 0;
  stat->read = 0;

  for (i = 0; i < 4; i++)
    stat->bin[i] = 0;

  stat->fft_size = 4096;
  stat->re_in = stat->re_out = NULL;

  if (stat->fft) {
    stat->fft_offset = 0;
    stat->re_in = lsx_malloc(sizeof(float) * stat->fft_size);
    stat->re_out = lsx_malloc(sizeof(float) * (stat->fft_size / 2 + 1));
  }

  return SOX_SUCCESS;
}

/*
 * Print power spectrum to given stream
 */
static void print_power_spectrum(unsigned samples, double rate, float *re_in, float *re_out)
{
  float ffa = rate / samples;
  unsigned i;

  lsx_power_spectrum_f((int)samples, re_in, re_out);
  for (i = 0; i < samples / 2; i++) /* FIXME: should be <= samples / 2 */
    fprintf(stderr, "%f  %f\n", ffa * i, re_out[i]);
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
static int sox_stat_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                        size_t *isamp, size_t *osamp)
{
  priv_t * stat = (priv_t *) effp->priv;
  int done, x, len = min(*isamp, *osamp);
  short count = 0;

  if (len) {
    if (stat->read == 0)          /* 1st sample */
      stat->min = stat->max = stat->mid = stat->last = (*ibuf)/stat->scale;

    if (stat->fft) {
      for (x = 0; x < len; x++) {
        SOX_SAMPLE_LOCALS;
        stat->re_in[stat->fft_offset++] = SOX_SAMPLE_TO_FLOAT_32BIT(ibuf[x], effp->clips);

        if (stat->fft_offset >= stat->fft_size) {
          stat->fft_offset = 0;
          print_power_spectrum((unsigned) stat->fft_size, effp->in_signal.rate, stat->re_in, stat->re_out);
        }

      }
    }

    for (done = 0; done < len; done++) {
      long lsamp = *ibuf++;
      double delta, samp = (double)lsamp / stat->scale;
      /* work in scaled levels for both sample and delta */
      stat->bin[(lsamp >> 30) + 2]++;
      *obuf++ = lsamp;

      if (stat->volume == 2) {
          fprintf(stderr,"%08lx ",lsamp);
          if (count++ == 5) {
              fprintf(stderr,"\n");
              count = 0;
          }
      }

      /* update min/max */
      if (stat->min > samp)
        stat->min = samp;
      else if (stat->max < samp)
        stat->max = samp;
      stat->mid = stat->min / 2 + stat->max / 2;

      stat->sum1 += samp;
      stat->sum2 += samp*samp;
      stat->asum += fabs(samp);

      delta = fabs(samp - stat->last);
      if (delta < stat->dmin)
        stat->dmin = delta;
      else if (delta > stat->dmax)
        stat->dmax = delta;

      stat->dsum1 += delta;
      stat->dsum2 += delta*delta;

      stat->last = samp;
    }
    stat->read += len;
  }

  *isamp = *osamp = len;
  /* Process all samples */

  return SOX_SUCCESS;
}

/*
 * Process tail of input samples.
 */
static int sox_stat_drain(sox_effect_t * effp, sox_sample_t *obuf UNUSED, size_t *osamp)
{
  priv_t * stat = (priv_t *) effp->priv;

  /* When we run out of samples, then we need to pad buffer with
   * zeros and then run FFT one last time to process any unprocessed
   * samples.
   */
  if (stat->fft && stat->fft_offset) {
    unsigned int x;

    for (x = stat->fft_offset; x < stat->fft_size; x++)
      stat->re_in[x] = 0;

    print_power_spectrum((unsigned) stat->fft_size, effp->in_signal.rate, stat->re_in, stat->re_out);
  }

  *osamp = 0;
  return SOX_EOF;
}

/*
 * Do anything required when you stop reading samples.
 * Don't close input file!
 */
static int sox_stat_stop(sox_effect_t * effp)
{
  priv_t * stat = (priv_t *) effp->priv;
  double amp, scale, rms = 0, freq;
  double x, ct;

  ct = stat->read;

  if (stat->srms) {  /* adjust results to units of rms */
    double f;
    rms = sqrt(stat->sum2/ct);
    f = 1.0/rms;
    stat->max *= f;
    stat->min *= f;
    stat->mid *= f;
    stat->asum *= f;
    stat->sum1 *= f;
    stat->sum2 *= f*f;
    stat->dmax *= f;
    stat->dmin *= f;
    stat->dsum1 *= f;
    stat->dsum2 *= f*f;
    stat->scale *= rms;
  }

  scale = stat->scale;

  amp = -stat->min;
  if (amp < stat->max)
    amp = stat->max;

  /* Just print the volume adjustment */
  if (stat->volume == 1 && amp > 0) {
    fprintf(stderr, "%.3f\n", SOX_SAMPLE_MAX/(amp*scale));
    return SOX_SUCCESS;
  }
  if (stat->volume == 2)
    fprintf(stderr, "\n\n");
  /* print out the info */
  fprintf(stderr, "Samples read:      %12" PRIu64 "\n", stat->read);
  fprintf(stderr, "Length (seconds):  %12.6f\n", (double)stat->read/effp->in_signal.rate/effp->in_signal.channels);
  if (stat->srms)
    fprintf(stderr, "Scaled by rms:     %12.6f\n", rms);
  else
    fprintf(stderr, "Scaled by:         %12.1f\n", scale);
  fprintf(stderr, "Maximum amplitude: %12.6f\n", stat->max);
  fprintf(stderr, "Minimum amplitude: %12.6f\n", stat->min);
  fprintf(stderr, "Midline amplitude: %12.6f\n", stat->mid);
  fprintf(stderr, "Mean    norm:      %12.6f\n", stat->asum/ct);
  fprintf(stderr, "Mean    amplitude: %12.6f\n", stat->sum1/ct);
  fprintf(stderr, "RMS     amplitude: %12.6f\n", sqrt(stat->sum2/ct));

  fprintf(stderr, "Maximum delta:     %12.6f\n", stat->dmax);
  fprintf(stderr, "Minimum delta:     %12.6f\n", stat->dmin);
  fprintf(stderr, "Mean    delta:     %12.6f\n", stat->dsum1/(ct-1));
  fprintf(stderr, "RMS     delta:     %12.6f\n", sqrt(stat->dsum2/(ct-1)));
  freq = sqrt(stat->dsum2/stat->sum2)*effp->in_signal.rate/(M_PI*2);
  fprintf(stderr, "Rough   frequency: %12d\n", (int)freq);

  if (amp>0)
    fprintf(stderr, "Volume adjustment: %12.3f\n", SOX_SAMPLE_MAX/(amp*scale));

  if (stat->bin[2] == 0 && stat->bin[3] == 0)
    fprintf(stderr, "\nProbably text, not sound\n");
  else {

    x = (float)(stat->bin[0] + stat->bin[3]) / (float)(stat->bin[1] + stat->bin[2]);

    if (x >= 3.0) {             /* use opposite encoding */
      if (effp->in_encoding->encoding == SOX_ENCODING_UNSIGNED)
        fprintf(stderr,"\nTry: -t raw -e signed-integer -b 8 \n");
      else
        fprintf(stderr,"\nTry: -t raw -e unsigned-integer -b 8 \n");
    } else if (x <= 1.0 / 3.0)
      ;                         /* correctly decoded */
    else if (x >= 0.5 && x <= 2.0) { /* use ULAW */
      if (effp->in_encoding->encoding == SOX_ENCODING_ULAW)
        fprintf(stderr,"\nTry: -t raw -e unsigned-integer -b 8 \n");
      else
        fprintf(stderr,"\nTry: -t raw -e mu-law -b 8 \n");
    } else
      fprintf(stderr, "\nCan't guess the type\n");
  }

  /* Release FFT memory */
  free(stat->re_in);
  free(stat->re_out);

  return SOX_SUCCESS;

}

static sox_effect_handler_t sox_stat_effect = {
  "stat",
  "[ -s N ] [ -rms ] [-freq] [ -v ] [ -d ]",
  SOX_EFF_MCHAN | SOX_EFF_MODIFY,
  sox_stat_getopts,
  sox_stat_start,
  sox_stat_flow,
  sox_stat_drain,
  sox_stat_stop,
  NULL, sizeof(priv_t)
};

const sox_effect_handler_t *lsx_stat_effect_fn(void)
{
  return &sox_stat_effect;
}
