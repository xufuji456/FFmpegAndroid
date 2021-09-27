/* libSoX Compander Crossover Filter  (c) 2008 robs@users.sourceforge.net
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

#define N 4          /* 4th order Linkwitz-Riley IIRs */
#define CONVOLVE _ _ _ _

typedef struct {double in, out_low, out_high;} previous_t[N * 2];

typedef struct {
  previous_t * previous;
  size_t       pos;
  double       coefs[3 *(N+1)];
} crossover_t;

static void square_quadratic(char const * name, double const * x, double * y)
{
  assert(N == 4);
  y[0] = x[0] * x[0];
  y[1] = 2 * x[0] * x[1];
  y[2] = 2 * x[0] * x[2] + x[1] * x[1];
  y[3] = 2 * x[1] * x[2];
  y[4] = x[2] * x[2];
  lsx_debug("%s=[%.16g %.16g %.16g %.16g %.16g];", name,
      y[0], y[1], y[2], y[3], y[4]);
}

static int crossover_setup(sox_effect_t * effp, crossover_t * p, double frequency)
{
  double w0 = 2 * M_PI * frequency / effp->in_signal.rate;
  double Q = sqrt(.5), alpha = sin(w0)/(2*Q);
  double x[9], norm;
  int i;

  if (w0 > M_PI) {
    lsx_fail("frequency must not exceed half the sample-rate (Nyquist rate)");
    return SOX_EOF;
  }
  x[0] =  (1 - cos(w0))/2;           /* Cf. filter_LPF in biquads.c */
  x[1] =   1 - cos(w0);
  x[2] =  (1 - cos(w0))/2;
  x[3] =  (1 + cos(w0))/2;           /* Cf. filter_HPF in biquads.c */
  x[4] = -(1 + cos(w0));
  x[5] =  (1 + cos(w0))/2;
  x[6] =   1 + alpha;
  x[7] =  -2*cos(w0);
  x[8] =   1 - alpha;
  for (norm = x[6], i = 0; i < 9; ++i) x[i] /= norm;
  square_quadratic("lb", x    , p->coefs);
  square_quadratic("hb", x + 3, p->coefs + 5);
  square_quadratic("a" , x + 6, p->coefs + 10);

  p->previous = lsx_calloc(effp->in_signal.channels, sizeof(*p->previous));
  return SOX_SUCCESS;
}

static int crossover_flow(sox_effect_t * effp, crossover_t * p, sox_sample_t
    *ibuf, sox_sample_t *obuf_low, sox_sample_t *obuf_high, size_t len0)
{
  double out_low, out_high;
  size_t c, len = len0 / effp->in_signal.channels;
  assert(len * effp->in_signal.channels == len0);

  while (len--) {
    p->pos = p->pos? p->pos - 1 : N - 1;
    for (c = 0; c < effp->in_signal.channels; ++c) {
#define _ out_low += p->coefs[j] * p->previous[c][p->pos + j].in \
        - p->coefs[2*N+2 + j] * p->previous[c][p->pos + j].out_low, ++j;
      {
        int j = 1;
        out_low = p->coefs[0] * *ibuf;
        CONVOLVE
        assert(j == N+1);
        *obuf_low++ = SOX_ROUND_CLIP_COUNT(out_low, effp->clips);
      }
#undef _
#define _ out_high += p->coefs[j+N+1] * p->previous[c][p->pos + j].in \
        - p->coefs[2*N+2 + j] * p->previous[c][p->pos + j].out_high, ++j;
      {
        int j = 1;
        out_high = p->coefs[N+1] * *ibuf;
        CONVOLVE
        assert(j == N+1);
        *obuf_high++ = SOX_ROUND_CLIP_COUNT(out_high, effp->clips);
      }
      p->previous[c][p->pos + N].in = p->previous[c][p->pos].in = *ibuf++;
      p->previous[c][p->pos + N].out_low = p->previous[c][p->pos].out_low = out_low;
      p->previous[c][p->pos + N].out_high = p->previous[c][p->pos].out_high = out_high;
    }
  }
  return SOX_SUCCESS;
}

