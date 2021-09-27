/* Simple example of using SoX libraries
 *
 * Copyright (c) 2008 robs@users.sourceforge.net
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
#include <math.h>
#include <assert.h>

/*
 * Reads input file and displays a few seconds of wave-form, starting from
 * a given time through the audio.   E.g. example2 song2.au 30.75 1
 */
int main(int argc, char * argv[])
{
  sox_format_t * in;
  sox_sample_t * buf;
  size_t blocks, block_size;
  /* Period of audio over which we will measure its volume in order to
   * display the wave-form: */
  static const double block_period = 0.025; /* seconds */
  double start_secs = 0, period = 2;
  char dummy;
  uint64_t seek;

  /* All libSoX applications must start by initialising the SoX library */
  assert(sox_init() == SOX_SUCCESS);

  assert(argc > 1);
  ++argv, --argc; /* Move to 1st parameter */

  /* Open the input file (with default parameters) */
  assert((in = sox_open_read(*argv, NULL, NULL, NULL)));
  ++argv, --argc; /* Move past this parameter */

  if (argc) { /* If given, read the start time: */
    assert(sscanf(*argv, "%lf%c", &start_secs, &dummy) == 1);
    ++argv, --argc; /* Move past this parameter */
  }

  if (argc) { /* If given, read the period of time to display: */
    assert(sscanf(*argv, "%lf%c", &period, &dummy) == 1);
    ++argv, --argc; /* Move past this parameter */
  }

  /* Calculate the start position in number of samples: */
  seek = start_secs * in->signal.rate * in->signal.channels + .5;
  /* Make sure that this is at a `wide sample' boundary: */
  seek -= seek % in->signal.channels;
  /* Move the file pointer to the desired starting position */
  assert(sox_seek(in, seek, SOX_SEEK_SET) == SOX_SUCCESS);

  /* Convert block size (in seconds) to a number of samples: */
  block_size = block_period * in->signal.rate * in->signal.channels + .5;
  /* Make sure that this is at a `wide sample' boundary: */
  block_size -= block_size % in->signal.channels;
  /* Allocate a block of memory to store the block of audio samples: */
  assert((buf = malloc(sizeof(sox_sample_t) * block_size)));

  /* This example program requires that the audio has precisely 2 channels: */
  assert(in->signal.channels == 2);

  /* Read and process blocks of audio for the selected period or until EOF: */
  for (blocks = 0; sox_read(in, buf, block_size) == block_size && blocks * block_period < period; ++blocks) {
    double left = 0, right = 0;
    size_t i;
    static const char line[] = "===================================";
    int l, r;

    for (i = 0; i < block_size; ++i) {
      SOX_SAMPLE_LOCALS;
      /* convert the sample from SoX's internal format to a `double' for
       * processing in this application: */
      double sample = SOX_SAMPLE_TO_FLOAT_64BIT(buf[i],);

      /* The samples for each channel are interleaved; in this example
       * we allow only stereo audio, so the left channel audio can be found in
       * even-numbered samples, and the right channel audio in odd-numbered
       * samples: */
      if (i & 1)
        right = max(right, fabs(sample)); /* Find the peak volume in the block */
      else
        left = max(left, fabs(sample)); /* Find the peak volume in the block */
    }

    /* Build up the wave form by displaying the left & right channel
     * volume as a line length: */
    l = (1 - left) * 35 + .5;
    r = (1 - right) * 35 + .5;
    printf("%8.3f%36s|%s\n", start_secs + blocks * block_period, line + l, line + r);
  }

  /* All done; tidy up: */
  free(buf);
  sox_close(in);
  sox_quit();
  return 0;
}
