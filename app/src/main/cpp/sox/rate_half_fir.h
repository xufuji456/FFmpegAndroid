/* Effect: change sample rate  Copyright (c) 2008,12 robs@users.sourceforge.net
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

/* Down-sample by a factor of 2 using a FIR with odd length (LEN).*/
/* Input must be preceded and followed by LEN >> 1 samples. */

#define _ sum += (input[-(2*j +1)] + input[(2*j +1)]) * COEFS[j], ++j;
static void FUNCTION(stage_t * p, fifo_t * output_fifo)
{
  sample_t const * input = stage_read_p(p);
  int i, num_out = (stage_occupancy(p) + 1) / 2;
  sample_t * output = fifo_reserve(output_fifo, num_out);

  for (i = 0; i < num_out; ++i, input += 2) {
    int j = 0;
    sample_t sum = input[0] * .5;
    CONVOLVE
    output[i] = sum;
  }
  fifo_read(&p->fifo, 2 * num_out, NULL);
}
#undef _
#undef COEFS
#undef CONVOLVE
#undef FUNCTION
