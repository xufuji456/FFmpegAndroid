/* Simple example of using SoX libraries
 *
 * Copyright (c) 2007-8 robs@users.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef NDEBUG /* N.B. assert used with active statements so enable always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static sox_format_t * in, * out; /* input and output files */

/* The function that will be called to input samples into the effects chain.
 * In this example, we get samples to process from a SoX-openned audio file.
 * In a different application, they might be generated or come from a different
 * part of the application. */
static int input_drain(
    sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  (void)effp;   /* This parameter is not needed in this example */

  /* ensure that *osamp is a multiple of the number of channels. */
  *osamp -= *osamp % effp->out_signal.channels;

  /* Read up to *osamp samples into obuf; store the actual number read
   * back to *osamp */
  *osamp = sox_read(in, obuf, *osamp);

  /* sox_read may return a number that is less than was requested; only if
   * 0 samples is returned does it indicate that end-of-file has been reached
   * or an error has occurred */
  if (!*osamp && in->sox_errno)
    fprintf(stderr, "%s: %s\n", in->filename, in->sox_errstr);
  return *osamp? SOX_SUCCESS : SOX_EOF;
}

/* The function that will be called to output samples from the effects chain.
 * In this example, we store the samples in a SoX-opened audio file.
 * In a different application, they might perhaps be analysed in some way,
 * or displayed as a wave-form */
static int output_flow(sox_effect_t *effp LSX_UNUSED, sox_sample_t const * ibuf,
    sox_sample_t * obuf LSX_UNUSED, size_t * isamp, size_t * osamp)
{
  /* Write out *isamp samples */
  size_t len = sox_write(out, ibuf, *isamp);

  /* len is the number of samples that were actually written out; if this is
   * different to *isamp, then something has gone wrong--most often, it's
   * out of disc space */
  if (len != *isamp) {
    fprintf(stderr, "%s: %s\n", out->filename, out->sox_errstr);
    return SOX_EOF;
  }

  /* Outputting is the last `effect' in the effect chain so always passes
   * 0 samples on to the next effect (as there isn't one!) */
  *osamp = 0;

  (void)effp;   /* This parameter is not needed in this example */

  return SOX_SUCCESS; /* All samples output successfully */
}

/* A `stub' effect handler to handle inputting samples to the effects
 * chain; the only function needed for this example is `drain' */
static sox_effect_handler_t const * input_handler(void)
{
  static sox_effect_handler_t handler = {
    "input", NULL, SOX_EFF_MCHAN, NULL, NULL, NULL, input_drain, NULL, NULL, 0
  };
  return &handler;
}

/* A `stub' effect handler to handle outputting samples from the effects
 * chain; the only function needed for this example is `flow' */
static sox_effect_handler_t const * output_handler(void)
{
  static sox_effect_handler_t handler = {
    "output", NULL, SOX_EFF_MCHAN, NULL, NULL, output_flow, NULL, NULL, NULL, 0
  };
  return &handler;
}

/*
 * Reads input file, applies vol & flanger effects, stores in output file.
 * E.g. example1 monkey.au monkey.aiff
 */
int main(int argc, char * argv[])
{
  sox_effects_chain_t * chain;
  sox_effect_t * e;
  char * vol[] = {"3dB"};

  assert(argc == 3);

  /* All libSoX applications must start by initialising the SoX library */
  assert(sox_init() == SOX_SUCCESS);

  /* Open the input file (with default parameters) */
  assert((in = sox_open_read(argv[1], NULL, NULL, NULL)));

  /* Open the output file; we must specify the output signal characteristics.
   * Since we are using only simple effects, they are the same as the input
   * file characteristics */
  assert((out = sox_open_write(argv[2], &in->signal, NULL, NULL, NULL, NULL)));

  /* Create an effects chain; some effects need to know about the input
   * or output file encoding so we provide that information here */
  chain = sox_create_effects_chain(&in->encoding, &out->encoding);

  /* The first effect in the effect chain must be something that can source
   * samples; in this case, we have defined an input handler that inputs
   * data from an audio file */
  e = sox_create_effect(input_handler());
  /* This becomes the first `effect' in the chain */
  assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
  free(e);

  /* Create the `vol' effect, and initialise it with the desired parameters: */
  e = sox_create_effect(sox_find_effect("vol"));
  assert(sox_effect_options(e, 1, vol) == SOX_SUCCESS);
  /* Add the effect to the end of the effects processing chain: */
  assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
  free(e);

  /* Create the `flanger' effect, and initialise it with default parameters: */
  e = sox_create_effect(sox_find_effect("flanger"));
  assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
  /* Add the effect to the end of the effects processing chain: */
  assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
  free(e);

  /* The last effect in the effect chain must be something that only consumes
   * samples; in this case, we have defined an output handler that outputs
   * data to an audio file */
  e = sox_create_effect(output_handler());
  assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
  free(e);

  /* Flow samples through the effects processing chain until EOF is reached */
  sox_flow_effects(chain, NULL, NULL);

  /* All done; tidy up: */
  sox_delete_effects_chain(chain);
  sox_close(out);
  sox_close(in);
  sox_quit();
  return 0;
}
