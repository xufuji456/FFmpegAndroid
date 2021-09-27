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

#ifdef NDEBUG /* N.B. assert used with active statements so enable always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox.h"
#include "util.h"
#include <stdio.h>
#include <assert.h>

/* Example of reading and writing audio files stored in memory buffers
 * rather than actual files.
 *
 * Usage: example5 input output
 */

/* Uncomment following line for fixed instead of malloc'd buffer: */
/*#define FIXED_BUFFER */

#if defined FIXED_BUFFER
#define buffer_size 123456
static char buffer[buffer_size];
#endif

int main(int argc, char * argv[])
{
  static sox_format_t * in, * out; /* input and output files */
  #define MAX_SAMPLES (size_t)2048
  sox_sample_t samples[MAX_SAMPLES]; /* Temporary store whilst copying. */
#if !defined FIXED_BUFFER
  char * buffer;
  size_t buffer_size;
#endif
  size_t number_read;

  assert(argc == 3);

  /* All libSoX applications must start by initialising the SoX library */
  assert(sox_init() == SOX_SUCCESS);

  /* Open the input file (with default parameters) */
  assert((in = sox_open_read(argv[1], NULL, NULL, NULL)));
#if defined FIXED_BUFFER
  assert((out = sox_open_mem_write(buffer, buffer_size, &in->signal, NULL, "sox", NULL)));
#else
  assert((out = sox_open_memstream_write(&buffer, &buffer_size, &in->signal, NULL, "sox", NULL)));
#endif
  while ((number_read = sox_read(in, samples, MAX_SAMPLES)))
    assert(sox_write(out, samples, number_read) == number_read);
  sox_close(out);
  sox_close(in);

  assert((in = sox_open_mem_read(buffer, buffer_size, NULL, NULL, NULL)));
  assert((out = sox_open_write(argv[2], &in->signal, NULL, NULL, NULL, NULL)));
  while ((number_read = sox_read(in, samples, MAX_SAMPLES)))
    assert(sox_write(out, samples, number_read) == number_read);
  sox_close(out);
  sox_close(in);
#if !defined FIXED_BUFFER
  free(buffer);
#endif

  sox_quit();
  return 0;
}
