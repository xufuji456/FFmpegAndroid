/* libSoX effect: Skeleton effect used as sample for creating new effects.
 *
 * Copyright 1999-2008 Chris Bagwell And SoX Contributors
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

/* Private data for effect */
typedef struct {
  int  localdata;
} priv_t;

/*
 * Process command-line options but don't do other
 * initialization now: effp->in_signal & effp->out_signal are not
 * yet filled in.
 */
static int getopts(sox_effect_t * effp, int argc, char UNUSED **argv)
{
  priv_t * UNUSED p = (priv_t *)effp->priv;

  if (argc != 2)
    return lsx_usage(effp);

  p->localdata = atoi(argv[1]);

  return p->localdata > 0 ? SOX_SUCCESS : SOX_EOF;
}

/*
 * Prepare processing.
 * Do all initializations.
 */
static int start(sox_effect_t * effp)
{
  if (effp->out_signal.channels == 1) {
    lsx_fail("Can't run on mono data.");
    return SOX_EOF;
  }

  return SOX_SUCCESS;
}

/*
 * Process up to *isamp samples from ibuf and produce up to *osamp samples
 * in obuf.  Write back the actual numbers of samples to *isamp and *osamp.
 * Return SOX_SUCCESS or, if error occurs, SOX_EOF.
 */
static int flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                           size_t *isamp, size_t *osamp)
{
  priv_t * UNUSED p = (priv_t *)effp->priv;
  size_t len, done;

  switch (effp->out_signal.channels) {
  case 2:
    /* Length to process will be buffer length / 2 since we
     * work with two samples at a time.
     */
    len = min(*isamp, *osamp) / 2;
    for (done = 0; done < len; done++)
      {
        obuf[0] = ibuf[0];
        obuf[1] = ibuf[1];
        /* Advance buffer by 2 samples */
        ibuf += 2;
        obuf += 2;
      }

    *isamp = len * 2;
    *osamp = len * 2;

    break;
  }

  return SOX_SUCCESS;
}

/*
 * Drain out remaining samples if the effect generates any.
 */
static int drain(sox_effect_t UNUSED * effp, sox_sample_t UNUSED *obuf, size_t *osamp)
{
  *osamp = 0;
  /* Return SOX_EOF when drain
   * will not output any more samples.
   * *osamp == 0 also indicates that.
   */
  return SOX_EOF;
}

/*
 * Do anything required when you stop reading samples.
 */
static int stop(sox_effect_t UNUSED * effp)
{
  return SOX_SUCCESS;
}

/*
 * Do anything required when you kill an effect.
 *      (free allocated memory, etc.)
 */
static int lsx_kill(sox_effect_t UNUSED * effp)
{
  return SOX_SUCCESS;
}

/*
 * Function returning effect descriptor. This should be the only
 * externally visible object.
 */
const sox_effect_handler_t *lsx_skel_effect_fn(void);
const sox_effect_handler_t *lsx_skel_effect_fn(void)
{
  /*
   * Effect descriptor.
   * If no specific processing is needed for any of
   * the 6 functions, then the function above can be deleted
   * and NULL used in place of the its name below.
   */
  static sox_effect_handler_t sox_skel_effect = {
    "skel", "[OPTION]", SOX_EFF_MCHAN,
    getopts, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
  };
  return &sox_skel_effect;
}
