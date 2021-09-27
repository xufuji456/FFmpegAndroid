/* Simple example of using SoX libraries
 *
 * Copyright (c) 2007-14 robs@users.sourceforge.net
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
#include "util.h"
#include <stdio.h>
#include <assert.h>

/*
 * Shows how to explicitly specify output file signal and encoding attributes.
 * 
 * The example converts a given input file of any type to mono mu-law at 8kHz
 * sampling-rate (providing that this is supported by the given output file
 * type).
 *
 * For example:
 *
 *   sox -r 16k -n input.wav synth 8 sin 0:8k sin 8k:0 gain -1
 *   ./example6 input.wav output.wav
 *   soxi input.wav output.wav
 *
 * gives:
 *
 * Input File     : 'input.wav'
 * Channels       : 2
 * Sample Rate    : 16000
 * Precision      : 32-bit
 * Duration       : 00:00:08.00 = 128000 samples ~ 600 CDDA sectors
 * File Size      : 1.02M
 * Bit Rate       : 1.02M
 * Sample Encoding: 32-bit Signed Integer PCM
 *
 * Input File     : 'output.wav'
 * Channels       : 1
 * Sample Rate    : 8000
 * Precision      : 14-bit
 * Duration       : 00:00:08.00 = 64000 samples ~ 600 CDDA sectors
 * File Size      : 64.1k
 * Bit Rate       : 64.1k
 * Sample Encoding: 8-bit u-law
 */

int main(int argc, char * argv[])
{
  static sox_format_t * in, * out; /* input and output files */
  sox_effects_chain_t * chain;
  sox_effect_t * e;
  char * args[10];
  sox_signalinfo_t interm_signal; /* @ intermediate points in the chain. */
  sox_encodinginfo_t out_encoding = {
    SOX_ENCODING_ULAW,
    8,
    0,
    sox_option_default,
    sox_option_default,
    sox_option_default,
    sox_false
  };
  sox_signalinfo_t out_signal = {
    8000,
    1,
    0,
    0,
    NULL
  };

  assert(argc == 3);
  assert(sox_init() == SOX_SUCCESS);
  assert((in = sox_open_read(argv[1], NULL, NULL, NULL)));
  assert((out = sox_open_write(argv[2], &out_signal, &out_encoding, NULL, NULL, NULL)));

  chain = sox_create_effects_chain(&in->encoding, &out->encoding);

  interm_signal = in->signal; /* NB: deep copy */

  e = sox_create_effect(sox_find_effect("input"));
  args[0] = (char *)in, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &in->signal) == SOX_SUCCESS);
  free(e);

  if (in->signal.rate != out->signal.rate) {
    e = sox_create_effect(sox_find_effect("rate"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }

  if (in->signal.channels != out->signal.channels) {
    e = sox_create_effect(sox_find_effect("channels"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }

  e = sox_create_effect(sox_find_effect("output"));
  args[0] = (char *)out, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
  free(e);

  sox_flow_effects(chain, NULL, NULL);

  sox_delete_effects_chain(chain);
  sox_close(out);
  sox_close(in);
  sox_quit();

  return 0;
}
