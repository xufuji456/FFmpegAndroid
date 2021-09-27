/* libSoX effect: remix   Copyright (c) 2008-9 robs@users.sourceforge.net
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
#include <string.h>

typedef struct {
  enum {semi, automatic, manual} mode;
  sox_bool mix_power;
  unsigned num_out_channels, min_in_channels;
  struct {
    char * str;          /* Command-line argument to parse for this out_spec */
    unsigned num_in_channels;
    struct in_spec {
      unsigned channel_num;
      double   multiplier;
    } * in_specs;
  } * out_specs;
} priv_t;

#define PARSE(SEP, SCAN, VAR, MIN, SEPARATORS) do {\
  end = strpbrk(text, SEPARATORS); \
  if (end == text) \
    SEP = *text++; \
  else { \
    SEP = (SEPARATORS)[strlen(SEPARATORS) - 1]; \
    n = sscanf(text, SCAN"%c", &VAR, &SEP); \
    if (n == 0 || VAR < MIN || (n == 2 && !strchr(SEPARATORS, SEP))) \
      return lsx_usage(effp); \
    text = end? end + 1 : text + strlen(text); \
  } \
} while (0)

static int parse(sox_effect_t * effp, char * * argv, unsigned channels)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i, j;
  double mult;

  p->min_in_channels = 0;
  for (i = 0; i < p->num_out_channels; ++i) {
    sox_bool mul_spec = sox_false;
    char * text, * end;
    if (argv) /* 1st parse only */
      p->out_specs[i].str = lsx_strdup(argv[i]);
    for (j = 0, text = p->out_specs[i].str; *text;) {
      static char const separators[] = "-vpi,";
      char sep1, sep2;
      int chan1 = 1, chan2 = channels, n;
      double multiplier = HUGE_VAL;

      PARSE(sep1, "%i", chan1, 0, separators);
      if (!chan1) {
       if (j || *text)
         return lsx_usage(effp);
       continue;
      }
      if (sep1 == '-')
        PARSE(sep1, "%i", chan2, 0, separators + 1);
      else chan2 = chan1;
      if (sep1 != ',') {
        multiplier = sep1 == 'v' ? 1 : 0;
        PARSE(sep2, "%lf", multiplier, -HUGE_VAL, separators + 4);
        if (sep1 != 'v')
          multiplier = (sep1 == 'p'? 1 : -1) * dB_to_linear(multiplier);
        mul_spec = sox_true;
      }
      if (chan2 < chan1) {int t = chan1; chan1 = chan2; chan2 = t;}
      p->out_specs[i].in_specs = lsx_realloc(p->out_specs[i].in_specs,
          (j + chan2 - chan1 + 1) * sizeof(*p->out_specs[i].in_specs));
      while (chan1 <= chan2) {
        p->out_specs[i].in_specs[j].channel_num = chan1++ - 1;
        p->out_specs[i].in_specs[j++].multiplier = multiplier;
      }
      p->min_in_channels = max(p->min_in_channels, (unsigned)chan2);
    }
    p->out_specs[i].num_in_channels = j;
    mult = 1. / (p->mix_power? sqrt((double)j) : j);
    for (j = 0; j < p->out_specs[i].num_in_channels; ++j)
      if (p->out_specs[i].in_specs[j].multiplier == HUGE_VAL)
        p->out_specs[i].in_specs[j].multiplier = (p->mode == automatic || (p->mode == semi && !mul_spec)) ? mult : 1;
  }
  effp->out_signal.channels = p->num_out_channels;
  return SOX_SUCCESS;
}

static int show(priv_t *p)
{
  unsigned i, j;

  for (j = 0; j < p->num_out_channels; j++) {
    lsx_debug("%i: ", j);
    for (i = 0; i < p->out_specs[j].num_in_channels; i++)
      lsx_debug("\t%i %g", p->out_specs[j].in_specs[i].channel_num, p->out_specs[j].in_specs[i].multiplier);
  }
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  --argc, ++argv;
  if (argc && !strcmp(*argv, "-m")) p->mode = manual   , ++argv, --argc;
  if (argc && !strcmp(*argv, "-a")) p->mode = automatic, ++argv, --argc;
  if (argc && !strcmp(*argv, "-p")) p->mix_power = sox_true, ++argv, --argc;
  if (!argc) {
    lsx_fail("must specify at least one output channel");
    return SOX_EOF;
  }
  p->num_out_channels = argc;
  p->out_specs = lsx_calloc(p->num_out_channels, sizeof(*p->out_specs));
  return parse(effp, argv, 1); /* No channels yet; parse with dummy */
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  double max_sum = 0;
  unsigned i, j;
  int non_integer = 0;

  parse(effp, NULL, effp->in_signal.channels);
  if (effp->in_signal.channels < p->min_in_channels) {
    lsx_fail("too few input channels");
    return SOX_EOF;
  }

  for (j = 0; j < effp->out_signal.channels; j++) {
    double sum = 0;
    for (i = 0; i < p->out_specs[j].num_in_channels; i++) {
      double mult = p->out_specs[j].in_specs[i].multiplier;
      sum += fabs(mult);
      non_integer += floor(mult) != mult;
    }
    max_sum = max(max_sum, sum);
  }
  if (effp->in_signal.mult && max_sum > 1)
    *effp->in_signal.mult /= max_sum;
  if (!non_integer)
    effp->out_signal.precision = effp->in_signal.precision;
  else
    effp->out_signal.precision = SOX_SAMPLE_PRECISION;
  show(p);
  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i, j, len;
  len =  min(*isamp / effp->in_signal.channels, *osamp / effp->out_signal.channels);
  *isamp = len * effp->in_signal.channels;
  *osamp = len * effp->out_signal.channels;

  for (; len--; ibuf += effp->in_signal.channels) for (j = 0; j < effp->out_signal.channels; j++) {
    double out = 0;
    for (i = 0; i < p->out_specs[j].num_in_channels; i++)
      out += ibuf[p->out_specs[j].in_specs[i].channel_num] * p->out_specs[j].in_specs[i].multiplier;
    *obuf++ = SOX_ROUND_CLIP_COUNT(out, effp->clips);
  }
  return SOX_SUCCESS;
}

static int closedown(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned i;
  for (i = 0; i < p->num_out_channels; ++i) {
    free(p->out_specs[i].str);
    free(p->out_specs[i].in_specs);
  }
  free(p->out_specs);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_remix_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "remix", "[-m|-a] [-p] <0|in-chan[v|p|i volume]{,in-chan[v|p|i volume]}>",
    SOX_EFF_MCHAN | SOX_EFF_CHAN | SOX_EFF_GAIN | SOX_EFF_PREC,
    create, start, flow, NULL, NULL, closedown, sizeof(priv_t)
  };
  return &handler;
}

/*----------------------- The `channels' effect alias ------------------------*/

static int channels_create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t * p = (priv_t *)effp->priv;
  char dummy;     /* To check for extraneous chars. */

  if (argc == 2) {
    if (sscanf(argv[1], "%d %c", (int *)&p->num_out_channels,
          &dummy) != 1 || (int)p->num_out_channels <= 0)
      return lsx_usage(effp);
    effp->out_signal.channels = p->num_out_channels;
  }
  else if (argc != 1)
    return lsx_usage(effp);
  return SOX_SUCCESS;
}

static int channels_start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  unsigned num_out_channels = p->num_out_channels != 0 ?
      p->num_out_channels : effp->out_signal.channels;
  unsigned i, j;

  p->out_specs = lsx_calloc(num_out_channels, sizeof(*p->out_specs));
  if (effp->in_signal.channels == num_out_channels)
    return SOX_EFF_NULL;

  if (effp->in_signal.channels > num_out_channels) {
    for (j = 0; j < num_out_channels; j++) {
      unsigned in_per_out = (effp->in_signal.channels +
          num_out_channels - 1 - j) / num_out_channels;
      lsx_valloc(p->out_specs[j].in_specs, in_per_out);
      p->out_specs[j].num_in_channels = in_per_out;
      for (i = 0; i < in_per_out; ++i) {
        p->out_specs[j].in_specs[i].channel_num = i * num_out_channels + j;
        p->out_specs[j].in_specs[i].multiplier = 1. / in_per_out;
      }
    }
  }
  else for (j = 0; j < num_out_channels; j++) {
    lsx_valloc(p->out_specs[j].in_specs, 1);
    p->out_specs[j].num_in_channels = 1;
    p->out_specs[j].in_specs[0].channel_num = j % effp->in_signal.channels;
    p->out_specs[j].in_specs[0].multiplier = 1;
  }
  effp->out_signal.channels = p->num_out_channels = num_out_channels;
  effp->out_signal.precision = (effp->in_signal.channels > num_out_channels) ?
    SOX_SAMPLE_PRECISION : effp->in_signal.precision;
  show(p);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_channels_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_remix_effect_fn();
  handler.name = "channels";
  handler.usage = "number";
  handler.flags &= ~SOX_EFF_GAIN;
  handler.getopts = channels_create;
  handler.start = channels_start;
  return &handler;
}

/*------------------------- The `oops' effect alias --------------------------*/

static int oops_getopts(sox_effect_t *effp, int argc, char **argv)
{
  char *args[] = {0, "1,2i", "1,2i"};
  args[0] = argv[0];
  return --argc? lsx_usage(effp) : create(effp, 3, args);
}

sox_effect_handler_t const * lsx_oops_effect_fn(void)
{
  static sox_effect_handler_t handler;
  handler = *lsx_remix_effect_fn();
  handler.name = "oops";
  handler.usage = NULL;
  handler.getopts = oops_getopts;
  return &handler;
}
