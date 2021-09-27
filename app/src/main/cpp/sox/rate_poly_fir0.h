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

/* Resample using a non-interpolated poly-phase FIR with length LEN.*/
/* Input must be followed by LEN-1 samples. */

#define _ sum += (coef(p->shared->poly_fir_coefs, 0, FIR_LENGTH, divided.rem, 0, j)) *at[j], ++j;

static void FUNCTION(stage_t * p, fifo_t * output_fifo)
{
  sample_t const * input = stage_read_p(p);
  int i, num_in = stage_occupancy(p), max_num_out = 1 + num_in*p->out_in_ratio;
  sample_t * output = fifo_reserve(output_fifo, max_num_out);
  div_t divided2;

  for (i = 0; p->at.parts.integer < num_in * p->L; ++i, p->at.parts.integer += p->step.parts.integer) {
    div_t divided = div(p->at.parts.integer, p->L);
    sample_t const * at = input + divided.quot;
    sample_t sum = 0;
    int j = 0;
    CONVOLVE
    output[i] = sum;
  }
  assert(max_num_out - i >= 0);
  fifo_trim_by(output_fifo, max_num_out - i);
  divided2 = div(p->at.parts.integer, p->L);
  fifo_read(&p->fifo, divided2.quot, NULL);
  p->at.parts.integer = divided2.rem;
}

#undef _
#undef CONVOLVE
#undef FIR_LENGTH
#undef FUNCTION
