/* Simple example of using SoX libraries
 *
 * Copyright (c) 2009 robs@users.sourceforge.net
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

#include "sox.h"
#include <stdio.h>

/* Concatenate audio files.  Note that the files must have the same number
 * of channels and the same sample rate.
 *
 * Usage: example4 input-1 input-2 [... input-n] output
 */

#define check(x) do if (!(x)) { \
  fprintf(stderr, "check failed: %s\n", #x); goto error; } while (0)

int main(int argc, char * argv[])
{
  sox_format_t * output = NULL;
  int i;

  check(argc >= 1 + 2 + 1); /* Need at least 2 input files + 1 output file. */
  check(sox_init() == SOX_SUCCESS);

  /* Defer openning the output file as we want to set its characteristics
   * based on those of the input files. */

  for (i = 1; i < argc - 1; ++i) { /* For each input file... */
    sox_format_t * input;
    static sox_signalinfo_t signal; /* static quashes `uninitialised' warning.*/

    /* The (maximum) number of samples that we shall read/write at a time;
     * chosen as a rough match to typical operating system I/O buffer size: */
    #define MAX_SAMPLES (size_t)2048
    sox_sample_t samples[MAX_SAMPLES]; /* Temporary store whilst copying. */
    size_t number_read;

    /* Open this input file: */
    check(input = sox_open_read(argv[i], NULL, NULL, NULL));

    if (i == 1) { /* If this is the first input file... */

      /* Open the output file using the same signal and encoding character-
       * istics as the first input file.  Note that here, input->signal.length
       * will not be equal to the output file length so we are relying on
       * libSoX to set the output length correctly (i.e. non-seekable output
       * is not catered for); an alternative would be to first calculate the
       * output length by summing the lengths of the input files and modifying
       * the second parameter to sox_open_write accordingly. */
      check(output = sox_open_write(argv[argc - 1],
            &input->signal, &input->encoding, NULL, NULL, NULL));
      
      /* Also, we'll store the signal characteristics of the first file
       * so that we can check that these match those of the other inputs: */
      signal = input->signal;
    }
    else { /* Second or subsequent input file... */

      /* Check that this input file's signal matches that of the first file: */
      check(input->signal.channels == signal.channels);
      check(input->signal.rate == signal.rate);
    }

    /* Copy all of the audio from this input file to the output file: */
    while ((number_read = sox_read(input, samples, MAX_SAMPLES)))
      check(sox_write(output, samples, number_read) == number_read);

    check(sox_close(input) == SOX_SUCCESS); /* Finished with this input file.*/
  }

  check(sox_close(output) == SOX_SUCCESS); /* Finished with the output file. */
  output = NULL;
  check(sox_quit() == SOX_SUCCESS);
  return 0;

error: /* Truncate output file on error: */
  if (output) {
    FILE * f;
    sox_close(output);
    if ((f = fopen(argv[argc - 1], "w"))) fclose(f);
  }
  return 1;
}
