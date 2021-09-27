/* SoX Effects chain     (c) 2007 robs@users.sourceforge.net
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

#define LSX_EFF_ALIAS
#include "sox_i.h"
#include <assert.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
  #include <strings.h>
#endif

#define DEBUG_EFFECTS_CHAIN 0

/* Default effect handler functions for do-nothing situations: */

static int default_function(sox_effect_t * effp UNUSED)
{
  return SOX_SUCCESS;
}

/* Pass through samples verbatim */
int lsx_flow_copy(sox_effect_t * effp UNUSED, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  *isamp = *osamp = min(*isamp, *osamp);
  memcpy(obuf, ibuf, *isamp * sizeof(*obuf));
  return SOX_SUCCESS;
}

/* Inform no more samples to drain */
static int default_drain(sox_effect_t * effp UNUSED, sox_sample_t *obuf UNUSED, size_t *osamp)
{
  *osamp = 0;
  return SOX_EOF;
}

/* Check that no parameters have been given */
static int default_getopts(sox_effect_t * effp, int argc, char **argv UNUSED)
{
  return --argc? lsx_usage(effp) : SOX_SUCCESS;
}

/* Partially initialise the effect structure; signal info will come later */
sox_effect_t * sox_create_effect(sox_effect_handler_t const * eh)
{
  sox_effect_t * effp = lsx_calloc(1, sizeof(*effp));
  effp->obuf = NULL;

  effp->global_info = sox_get_effects_globals();
  effp->handler = *eh;
  if (!effp->handler.getopts) effp->handler.getopts = default_getopts;
  if (!effp->handler.start  ) effp->handler.start   = default_function;
  if (!effp->handler.flow   ) effp->handler.flow    = lsx_flow_copy;
  if (!effp->handler.drain  ) effp->handler.drain   = default_drain;
  if (!effp->handler.stop   ) effp->handler.stop    = default_function;
  if (!effp->handler.kill   ) effp->handler.kill    = default_function;

  effp->priv = lsx_calloc(1, effp->handler.priv_size);

  return effp;
} /* sox_create_effect */

int sox_effect_options(sox_effect_t *effp, int argc, char * const argv[])
{
  int result;

  char * * argv2 = lsx_malloc((argc + 1) * sizeof(*argv2));
  argv2[0] = (char *)effp->handler.name;
  memcpy(argv2 + 1, argv, argc * sizeof(*argv2));
  result = effp->handler.getopts(effp, argc + 1, argv2);
  free(argv2);
  return result;
} /* sox_effect_options */

/* Effects chain: */

sox_effects_chain_t * sox_create_effects_chain(
    sox_encodinginfo_t const * in_enc, sox_encodinginfo_t const * out_enc)
{
  sox_effects_chain_t * result = lsx_calloc(1, sizeof(sox_effects_chain_t));
  result->global_info = *sox_get_effects_globals();
  result->in_enc = in_enc;
  result->out_enc = out_enc;
  return result;
} /* sox_create_effects_chain */

void sox_delete_effects_chain(sox_effects_chain_t *ecp)
{
    if (ecp && ecp->length)
        sox_delete_effects(ecp);
    free(ecp->effects);
    free(ecp);
} /* sox_delete_effects_chain */

/* Effect can call in start() or flow() to set minimum input size to flow() */
int lsx_effect_set_imin(sox_effect_t * effp, size_t imin)
{
  if (imin > sox_globals.bufsiz / effp->flows) {
    lsx_fail("sox_bufsiz not big enough");
    return SOX_EOF;
  }

  effp->imin = imin;
  return SOX_SUCCESS;
}

/* Effects table to be extended in steps of EFF_TABLE_STEP */
#define EFF_TABLE_STEP 8

/* Add an effect to the chain. *in is the input signal for this effect. *out is
 * a suggestion as to what the output signal should be, but depending on its
 * given options and *in, the effect can choose to do differently.  Whatever
 * output rate and channels the effect does produce are written back to *in,
 * ready for the next effect in the chain.
 */
int sox_add_effect(sox_effects_chain_t * chain, sox_effect_t * effp, sox_signalinfo_t * in, sox_signalinfo_t const * out)
{
  int ret, (*start)(sox_effect_t * effp) = effp->handler.start;
  size_t f;
  sox_effect_t eff0;  /* Copy of effect for flow 0 before calling start */

  effp->global_info = &chain->global_info;
  effp->in_signal = *in;
  effp->out_signal = *out;
  effp->in_encoding = chain->in_enc;
  effp->out_encoding = chain->out_enc;
  if (!(effp->handler.flags & SOX_EFF_CHAN))
    effp->out_signal.channels = in->channels;
  if (!(effp->handler.flags & SOX_EFF_RATE))
    effp->out_signal.rate = in->rate;
  if (!(effp->handler.flags & SOX_EFF_PREC))
    effp->out_signal.precision = (effp->handler.flags & SOX_EFF_MODIFY)?
        in->precision : SOX_SAMPLE_PRECISION;
  if (!(effp->handler.flags & SOX_EFF_GAIN))
    effp->out_signal.mult = in->mult;

  effp->flows =
    (effp->handler.flags & SOX_EFF_MCHAN)? 1 : effp->in_signal.channels;
  effp->clips = 0;
  effp->imin = 0;
  eff0 = *effp, eff0.priv = lsx_memdup(eff0.priv, eff0.handler.priv_size);
  eff0.in_signal.mult = NULL; /* Only used in channel 0 */
  ret = start(effp);
  if (ret == SOX_EFF_NULL) {
    lsx_report("has no effect in this configuration");
    free(eff0.priv);
    effp->handler.kill(effp);
    free(effp->priv);
    effp->priv = NULL;
    return SOX_SUCCESS;
  }
  if (ret != SOX_SUCCESS) {
    free(eff0.priv);
    return SOX_EOF;
  }
  if (in->mult)
    lsx_debug("mult=%g", *in->mult);

  if (!(effp->handler.flags & SOX_EFF_LENGTH)) {
    effp->out_signal.length = in->length;
    if (effp->out_signal.length != SOX_UNKNOWN_LEN) {
      if (effp->handler.flags & SOX_EFF_CHAN)
        effp->out_signal.length =
          effp->out_signal.length / in->channels * effp->out_signal.channels;
      if (effp->handler.flags & SOX_EFF_RATE)
        effp->out_signal.length =
          effp->out_signal.length / in->rate * effp->out_signal.rate + .5;
    }
  }

  *in = effp->out_signal;

  if (chain->length == chain->table_size) {
    chain->table_size += EFF_TABLE_STEP;
    lsx_debug_more("sox_add_effect: extending effects table, "
      "new size = %" PRIuPTR, chain->table_size);
    lsx_revalloc(chain->effects, chain->table_size);
  }

  chain->effects[chain->length] =
    lsx_calloc(effp->flows, sizeof(chain->effects[chain->length][0]));
  chain->effects[chain->length][0] = *effp;

  for (f = 1; f < effp->flows; ++f) {
    chain->effects[chain->length][f] = eff0;
    chain->effects[chain->length][f].flow = f;
    chain->effects[chain->length][f].priv = lsx_memdup(eff0.priv, eff0.handler.priv_size);
    if (start(&chain->effects[chain->length][f]) != SOX_SUCCESS) {
      free(eff0.priv);
      return SOX_EOF;
    }
  }

  ++chain->length;
  free(eff0.priv);
  return SOX_SUCCESS;
}

/* An effect's output buffer (effp->obuf) generally has this layout:
 *   |. . . A1A2A3B1B2B3C1C2C3. . . . . . . . . . . . . . . . . . |
 *    ^0    ^obeg             ^oend                               ^bufsiz
 * (where A1 is the first sample of channel 1, A2 the first sample of
 * channel 2, etc.), i.e. the channels are interleaved.
 * However, while sox_flow_effects() is running, output buffers are
 * adapted to how the following effect expects its input, to avoid
 * back-and-forth conversions.  If the following effect operates on
 * each of several channels separately (flows > 1), the layout is
 * changed to this uninterleaved form:
 *   |. A1B1C1. . . . . . . A2B2C2. . . . . . . A3B3C3. . . . . . |
 *    ^0    ^obeg             ^oend                               ^bufsiz
 *    <--- channel 1 ----><--- channel 2 ----><--- channel 3 ---->
 * The buffer is logically subdivided into channel buffers of size
 * bufsiz/flows each, starting at offsets 0, bufsiz/flows,
 * 2*(bufsiz/flows) etc.  Within the channel buffers, the data starts
 * at position obeg/flows and ends before oend/flows.  In case bufsiz
 * is not evenly divisible by flows, there will be an unused area at
 * the very end of the output buffer.
 * The interleave() and deinterleave() functions convert between these
 * two representations.
 */
static void interleave(size_t flows, size_t length, sox_sample_t *from,
    size_t bufsiz, size_t offset, sox_sample_t *to);
static void deinterleave(size_t flows, size_t length, sox_sample_t *from,
    sox_sample_t *to, size_t bufsiz, size_t offset);

static int flow_effect(sox_effects_chain_t * chain, size_t n)
{
  sox_effect_t *effp1 = chain->effects[n - 1];
  sox_effect_t *effp = chain->effects[n];
  int effstatus = SOX_SUCCESS;
  size_t f = 0;
  size_t idone = effp1->oend - effp1->obeg;
  size_t obeg = sox_globals.bufsiz - effp->oend;
  sox_bool il_change = (effp->flows == 1) !=
      (chain->length == n + 1 || chain->effects[n+1]->flows == 1);
#if DEBUG_EFFECTS_CHAIN
  size_t pre_idone = idone;
  size_t pre_odone = obeg;
#endif

  if (effp->flows == 1) {     /* Run effect on all channels at once */
    idone -= idone % effp->in_signal.channels;
    effstatus = effp->handler.flow(effp, effp1->obuf + effp1->obeg,
                    il_change ? chain->il_buf : effp->obuf + effp->oend,
                    &idone, &obeg);
    if (obeg % effp->out_signal.channels != 0) {
      lsx_fail("multi-channel effect flowed asymmetrically!");
      effstatus = SOX_EOF;
    }
    if (il_change)
      deinterleave(chain->effects[n+1]->flows, obeg, chain->il_buf,
          effp->obuf, sox_globals.bufsiz, effp->oend);
  } else {               /* Run effect on each channel individually */
    sox_sample_t *obuf = il_change ? chain->il_buf : effp->obuf;
    size_t flow_offs = sox_globals.bufsiz/effp->flows;
    size_t idone_min = SOX_SIZE_MAX, idone_max = 0;
    size_t odone_min = SOX_SIZE_MAX, odone_max = 0;

#ifdef HAVE_OPENMP_3_1
    #pragma omp parallel for \
        if(sox_globals.use_threads) \
        schedule(static) default(none) \
        shared(effp,effp1,idone,obeg,obuf,flow_offs,chain,n,effstatus) \
        reduction(min:idone_min,odone_min) reduction(max:idone_max,odone_max)
#elif defined HAVE_OPENMP
    #pragma omp parallel for \
        if(sox_globals.use_threads) \
        schedule(static) default(none) \
        shared(effp,effp1,idone,obeg,obuf,flow_offs,chain,n,effstatus) \
        firstprivate(idone_min,odone_min,idone_max,odone_max) \
        lastprivate(idone_min,odone_min,idone_max,odone_max)
#endif
    for (f = 0; f < effp->flows; ++f) {
      size_t idonec = idone / effp->flows;
      size_t odonec = obeg / effp->flows;
      int eff_status_c = effp->handler.flow(&chain->effects[n][f],
          effp1->obuf + f*flow_offs + effp1->obeg/effp->flows,
          obuf + f*flow_offs + effp->oend/effp->flows,
          &idonec, &odonec);
      idone_min = min(idonec, idone_min); idone_max = max(idonec, idone_max);
      odone_min = min(odonec, odone_min); odone_max = max(odonec, odone_max);

      if (eff_status_c != SOX_SUCCESS)
        effstatus = SOX_EOF;
    }

    if (idone_min != idone_max || odone_min != odone_max) {
      lsx_fail("flowed asymmetrically!");
      effstatus = SOX_EOF;
    }
    idone = effp->flows * idone_max;
    obeg = effp->flows * odone_max;

    if (il_change)
      interleave(effp->flows, obeg, chain->il_buf, sox_globals.bufsiz,
          effp->oend, effp->obuf + effp->oend);
  }
  effp1->obeg += idone;
  if (effp1->obeg == effp1->oend)
    effp1->obeg = effp1->oend = 0;
  else if (effp1->oend - effp1->obeg < effp->imin) { /* Need to refill? */
    size_t flow_offs = sox_globals.bufsiz/effp->flows;
    for (f = 0; f < effp->flows; ++f)
      memcpy(effp1->obuf + f * flow_offs,
          effp1->obuf + f * flow_offs + effp1->obeg/effp->flows,
          (effp1->oend - effp1->obeg)/effp->flows * sizeof(*effp1->obuf));
    effp1->oend -= effp1->obeg;
    effp1->obeg = 0;
  }

  effp->oend += obeg;

#if DEBUG_EFFECTS_CHAIN
  lsx_report("\t" "flow:  %2" PRIuPTR " (%1" PRIuPTR ")  "
      "%5" PRIuPTR " %5" PRIuPTR " %5" PRIuPTR " %5" PRIuPTR "  "
      "%5" PRIuPTR " [%" PRIuPTR "-%" PRIuPTR "]",
      n, effp->flows, pre_idone, pre_odone, idone, obeg,
      effp1->oend - effp1->obeg, effp1->obeg, effp1->oend);
#endif

  return effstatus == SOX_SUCCESS? SOX_SUCCESS : SOX_EOF;
}

/* The same as flow_effect but with no input */
static int drain_effect(sox_effects_chain_t * chain, size_t n)
{
  sox_effect_t *effp = chain->effects[n];
  int effstatus = SOX_SUCCESS;
  size_t f = 0;
  size_t obeg = sox_globals.bufsiz - effp->oend;
  sox_bool il_change = (effp->flows == 1) !=
      (chain->length == n + 1 || chain->effects[n+1]->flows == 1);
#if DEBUG_EFFECTS_CHAIN
  size_t pre_odone = obeg;
#endif

  if (effp->flows == 1) { /* Run effect on all channels at once */
    effstatus = effp->handler.drain(effp,
                    il_change ? chain->il_buf : effp->obuf + effp->oend,
                    &obeg);
    if (obeg % effp->out_signal.channels != 0) {
      lsx_fail("multi-channel effect drained asymmetrically!");
      effstatus = SOX_EOF;
    }
    if (il_change)
      deinterleave(chain->effects[n+1]->flows, obeg, chain->il_buf,
          effp->obuf, sox_globals.bufsiz, effp->oend);
  } else {                       /* Run effect on each channel individually */
    sox_sample_t *obuf = il_change ? chain->il_buf : effp->obuf;
    size_t flow_offs = sox_globals.bufsiz/effp->flows;
    size_t odone_last = 0; /* Initialised to prevent warning */

    for (f = 0; f < effp->flows; ++f) {
      size_t odonec = obeg / effp->flows;
      int eff_status_c = effp->handler.drain(&chain->effects[n][f],
          obuf + f*flow_offs + effp->oend/effp->flows,
          &odonec);
      if (f && (odonec != odone_last)) {
        lsx_fail("drained asymmetrically!");
        effstatus = SOX_EOF;
      }
      odone_last = odonec;

      if (eff_status_c != SOX_SUCCESS)
        effstatus = SOX_EOF;
    }

    obeg = effp->flows * odone_last;

    if (il_change)
      interleave(effp->flows, obeg, chain->il_buf, sox_globals.bufsiz,
          effp->oend, effp->obuf + effp->oend);
  }
  if (!obeg)   /* This is the only thing that drain has and flow hasn't */
    effstatus = SOX_EOF;

  effp->oend += obeg;

#if DEBUG_EFFECTS_CHAIN
  lsx_report("\t" "drain: %2" PRIuPTR " (%1" PRIuPTR ")  "
      "%5" PRIuPTR " %5" PRIuPTR " %5" PRIuPTR " %5" PRIuPTR,
      n, effp->flows, (size_t)0, pre_odone, (size_t)0, obeg);
#endif

  return effstatus == SOX_SUCCESS? SOX_SUCCESS : SOX_EOF;
}

/* Flow data through the effects chain until an effect or callback gives EOF */
int sox_flow_effects(sox_effects_chain_t * chain, int (* callback)(sox_bool all_done, void * client_data), void * client_data)
{
  int flow_status = SOX_SUCCESS;
  size_t e, source_e = 0;               /* effect indices */
  size_t max_flows = 0;
  sox_bool draining = sox_true;

  for (e = 0; e < chain->length; ++e) {
    sox_effect_t *effp = chain->effects[e];
    effp->obuf =
        lsx_realloc(effp->obuf, sox_globals.bufsiz * sizeof(*effp->obuf));
      /* Memory will be freed by sox_delete_effect() later. */
      /* Possibly there was already a buffer, if this is a used effect;
         it may still contain samples in that case. */
      if (effp->oend > sox_globals.bufsiz) {
        lsx_warn("buffer size insufficient; buffered samples were dropped");
        /* can only happen if bufsize has been reduced since the last run */
        effp->obeg = effp->oend = 0;
      }
    max_flows = max(max_flows, effp->flows);
  }
  if (max_flows > 1) /* might need interleave buffer */
    chain->il_buf = lsx_malloc(sox_globals.bufsiz * sizeof(sox_sample_t));
  else
    chain->il_buf = NULL;

  /* Go through the effects, and if there are samples in one of the
     buffers, deinterleave it (if necessary).  */
  for (e = 0; e + 1 < chain->length; e++) {
    sox_effect_t *effp = chain->effects[e];
    if (effp->oend > effp->obeg && chain->effects[e+1]->flows > 1) {
      sox_sample_t *sw = chain->il_buf; chain->il_buf = effp->obuf; effp->obuf = sw;
      deinterleave(chain->effects[e+1]->flows, effp->oend - effp->obeg,
          chain->il_buf, effp->obuf, sox_globals.bufsiz, effp->obeg);
    }
  }

  e = chain->length - 1;
  while (source_e < chain->length) {
#define have_imin (e > 0 && e < chain->length && chain->effects[e - 1]->oend - chain->effects[e - 1]->obeg >= chain->effects[e]->imin)
    size_t osize = chain->effects[e]->oend - chain->effects[e]->obeg;
    if (e == source_e && (draining || !have_imin)) {
      if (drain_effect(chain, e) == SOX_EOF) {
        ++source_e;
        draining = sox_false;
      }
    } else if (have_imin && flow_effect(chain, e) == SOX_EOF) {
      flow_status = SOX_EOF;
      if (e == chain->length - 1)
        break;
      source_e = e;
      draining = sox_true;
    }
    if (e < chain->length && chain->effects[e]->oend - chain->effects[e]->obeg > osize) /* False for output */
      ++e;
    else if (e == source_e)
      draining = sox_true;
    else if (e < source_e)
      e = source_e;
    else
      --e;

    if (callback && callback(source_e == chain->length, client_data) != SOX_SUCCESS) {
      flow_status = SOX_EOF; /* Client has requested to stop the flow. */
      break;
    }
  }

  /* If an effect's output buffer still has samples, and if it is
     uninterleaved, then re-interleave it. Necessary since it might
     be reused, and at that time possibly followed by an MCHAN effect. */
  for (e = 0; e + 1 < chain->length; e++) {
    sox_effect_t *effp = chain->effects[e];
    if (effp->oend > effp->obeg && chain->effects[e+1]->flows > 1) {
      sox_sample_t *sw = chain->il_buf; chain->il_buf = effp->obuf; effp->obuf = sw;
      interleave(chain->effects[e+1]->flows, effp->oend - effp->obeg,
          chain->il_buf, sox_globals.bufsiz, effp->obeg, effp->obuf);
    }
  }

  free(chain->il_buf);
  return flow_status;
}

sox_uint64_t sox_effects_clips(sox_effects_chain_t * chain)
{
  size_t i, f;
  uint64_t clips = 0;
  for (i = 1; i < chain->length - 1; ++i)
    for (f = 0; f < chain->effects[i][0].flows; ++f)
      clips += chain->effects[i][f].clips;
  return clips;
}

sox_uint64_t sox_stop_effect(sox_effect_t *effp)
{
  size_t f;
  uint64_t clips = 0;

  for (f = 0; f < effp->flows; ++f) {
    effp[f].handler.stop(&effp[f]);
    clips += effp[f].clips;
  }
  return clips;
}

void sox_push_effect_last(sox_effects_chain_t *chain, sox_effect_t *effp)
{
  if (chain->length == chain->table_size) {
    chain->table_size += EFF_TABLE_STEP;
    lsx_debug_more("sox_push_effect_last: extending effects table, "
        "new size = %" PRIuPTR, chain->table_size);
    lsx_revalloc(chain->effects, chain->table_size);
  }

  chain->effects[chain->length++] = effp;
} /* sox_push_effect_last */

sox_effect_t *sox_pop_effect_last(sox_effects_chain_t *chain)
{
  if (chain->length > 0)
  {
    sox_effect_t *effp;
    chain->length--;
    effp = chain->effects[chain->length];
    chain->effects[chain->length] = NULL;
    return effp;
  }
  else
    return NULL;
} /* sox_pop_effect_last */

/* Free resources related to effect.
 * Note: This currently closes down the effect which might
 * not be obvious from name.
 */
void sox_delete_effect(sox_effect_t *effp)
{
  uint64_t clips;
  size_t f;

  if ((clips = sox_stop_effect(effp)) != 0)
    lsx_warn("%s clipped %" PRIu64 " samples; decrease volume?",
        effp->handler.name, clips);
  if (effp->obeg != effp->oend)
    lsx_debug("output buffer still held %" PRIuPTR " samples; dropped.",
        (effp->oend - effp->obeg)/effp->out_signal.channels);
      /* May or may not indicate a problem; it is normal if the user aborted
         processing, or if an effect like "trim" stopped early. */
  effp->handler.kill(effp); /* N.B. only one kill; not one per flow */
  for (f = 0; f < effp->flows; ++f)
    free(effp[f].priv);
  free(effp->obuf);
  free(effp);
}

void sox_delete_effect_last(sox_effects_chain_t *chain)
{
  if (chain->length > 0)
  {
    chain->length--;
    sox_delete_effect(chain->effects[chain->length]);
    chain->effects[chain->length] = NULL;
  }
} /* sox_delete_effect_last */

/* Remove all effects from the chain.
 * Note: This currently closes down the effect which might
 * not be obvious from name.
 */
void sox_delete_effects(sox_effects_chain_t * chain)
{
  size_t e;

  for (e = 0; e < chain->length; ++e) {
    sox_delete_effect(chain->effects[e]);
    chain->effects[e] = NULL;
  }
  chain->length = 0;
}

/*----------------------------- Effects library ------------------------------*/

static sox_effect_fn_t s_sox_effect_fns[] = {
#define EFFECT(f) lsx_##f##_effect_fn,
#include "effects.h"
#undef EFFECT
  NULL
};

const sox_effect_fn_t*
sox_get_effect_fns(void)
{
    return s_sox_effect_fns;
}

/* Find a named effect in the effects library */
sox_effect_handler_t const * sox_find_effect(char const * name)
{
  int e;
  sox_effect_fn_t const * fns = sox_get_effect_fns();
  for (e = 0; fns[e]; ++e) {
    const sox_effect_handler_t *eh = fns[e] ();
    if (eh && eh->name && strcasecmp(eh->name, name) == 0)
      return eh;                 /* Found it. */
  }
  return NULL;
}


/*----------------------------- Helper functions -----------------------------*/

/* interleave() parameters:
 *   flows: number of samples per wide sample
 *   length: number of samples to copy
 *     [pertaining to the (non-interleaved) source buffer:]
 *   from: start address
 *   bufsiz: total size
 *   offset: position at which to start reading
 *     [pertaining to the (interleaved) destination buffer:]
 *   to: start address
 */
static void interleave(size_t flows, size_t length, sox_sample_t *from,
    size_t bufsiz, size_t offset, sox_sample_t *to)
{
  size_t i;
  const size_t wide_samples = length/flows;
  const size_t flow_offs = bufsiz/flows;
  from += offset/flows;
  for (i = 0; i < wide_samples; i++) {
    sox_sample_t *inner_from = from + i;
    sox_sample_t *inner_to = to + i * flows;
    size_t f;
    for (f = 0; f < flows; f++) {
      *inner_to++ = *inner_from;
      inner_from += flow_offs;
    }
  }
}

/* deinterleave() parameters:
 *   flows: number of samples per wide sample
 *   length: number of samples to copy
 *     [pertaining to the (interleaved) source buffer:]
 *   from: start address
 *     [pertaining to the (non-interleaved) destination buffer:]
 *   to: start address
 *   bufsiz: total size
 *   offset: position at which to start writing
 */
static void deinterleave(size_t flows, size_t length, sox_sample_t *from,
    sox_sample_t *to, size_t bufsiz, size_t offset)
{
  const size_t wide_samples = length/flows;
  const size_t flow_offs = bufsiz/flows;
  size_t f;
  to += offset/flows;
  for (f = 0; f < flows; f++) {
    sox_sample_t *inner_to = to + f*flow_offs;
    sox_sample_t *inner_from = from + f;
    size_t i = wide_samples;
    while (i--) {
      *inner_to++ = *inner_from;
      inner_from += flows;
    }
  }
}
