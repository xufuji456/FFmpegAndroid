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

/* Resample using an interpolated poly-phase FIR with length LEN.*/
/* Input must be followed by LEN-1 samples. */

#define a (coef(p->shared->poly_fir_coefs, COEF_INTERP, FIR_LENGTH, phase, 0,j))
#define b (coef(p->shared->poly_fir_coefs, COEF_INTERP, FIR_LENGTH, phase, 1,j))
#define c (coef(p->shared->poly_fir_coefs, COEF_INTERP, FIR_LENGTH, phase, 2,j))
#define d (coef(p->shared->poly_fir_coefs, COEF_INTERP, FIR_LENGTH, phase, 3,j))
#if COEF_INTERP == 0
  #define _ sum += a *in[j], ++j;
#elif COEF_INTERP == 1
  #define _ sum += (b *x + a)*in[j], ++j;
#elif COEF_INTERP == 2
  #define _ sum += ((c *x + b)*x + a)*in[j], ++j;
#elif COEF_INTERP == 3
  #define _ sum += (((d*x + c)*x + b)*x + a)*in[j], ++j;
#else
  #error COEF_INTERP
#endif

static void FUNCTION(stage_t * p, fifo_t * output_fifo)
{
  sample_t const * input = stage_read_p(p);
  int i, num_in = stage_occupancy(p), max_num_out = 1 + num_in*p->out_in_ratio;
  sample_t * output = fifo_reserve(output_fifo, max_num_out);

#if defined HI_PREC_CLOCK
  if (p->use_hi_prec_clock) {
    hi_prec_clock_t at = p->at.hi_prec_clock;
    for (i = 0; (int)at < num_in; ++i, at += p->step.hi_prec_clock) {
      sample_t const * in = input + (int)at;
      hi_prec_clock_t fraction = at - (int)at;
      int phase = fraction * (1 << PHASE_BITS);
#if COEF_INTERP > 0
      sample_t x = fraction * (1 << PHASE_BITS) - phase;
#endif
      sample_t sum = 0;
      int j = 0;
      CONVOLVE
      output[i] = sum;
    }
    fifo_read(&p->fifo, (int)at, NULL);
    p->at.hi_prec_clock = at - (int)at;
  } else
#endif
  {
    for (i = 0; p->at.parts.integer < num_in; ++i, p->at.all += p->step.all) {
      sample_t const * in = input + p->at.parts.integer;
      uint32_t fraction = p->at.parts.fraction;
      int phase = fraction >> (32 - PHASE_BITS); /* high-order bits */
#if COEF_INTERP > 0              /* low-order bits, scaled to [0,1) */
      sample_t x = (sample_t) (fraction << PHASE_BITS) * (1 / MULT32);
#endif
      sample_t sum = 0;
      int j = 0;
      CONVOLVE
      output[i] = sum;
    }
    fifo_read(&p->fifo, p->at.parts.integer, NULL);
    p->at.parts.integer = 0;
  }
  assert(max_num_out - i >= 0);
  fifo_trim_by(output_fifo, max_num_out - i);
}

#undef _
#undef a
#undef b
#undef c
#undef d
#undef COEF_INTERP
#undef CONVOLVE
#undef FIR_LENGTH
#undef FUNCTION
#undef PHASE_BITS
