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

/* Inspired by, and builds upon some of the ideas presented in:
 * `The Quest For The Perfect Resampler' by Laurent De Soras;
 * http://ldesoras.free.fr/doc/articles/resampler-en.pdf */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#define _GNU_SOURCE
#include "sox_i.h"
#include "fft4g.h"
#include "dft_filter.h"
#include <assert.h>
#include <string.h>

#define calloc     lsx_calloc
#define malloc     lsx_malloc
#define raw_coef_t double

#if 0 /* For float32 version, as used in foobar */
  #define sample_t   float
  #define num_coefs4 ((num_coefs + 3) & ~3) /* align coefs for SSE */
  #define coefs4_check(i) ((i) < num_coefs)
#else
  #define sample_t   double
  #define num_coefs4 num_coefs
  #define coefs4_check(i) 1
#endif

#if defined M_PIl
  #define hi_prec_clock_t long double /* __float128 is also a (slow) option */
#else
  #define hi_prec_clock_t double
#endif

#define coef(coef_p, interp_order, fir_len, phase_num, coef_interp_num, fir_coef_num) coef_p[(fir_len) * ((interp_order) + 1) * (phase_num) + ((interp_order) + 1) * (fir_coef_num) + (interp_order - coef_interp_num)]

static sample_t * prepare_coefs(raw_coef_t const * coefs, int num_coefs,
    int num_phases, int interp_order, int multiplier)
{
  int i, j, length = num_coefs4 * num_phases;
  sample_t * result = malloc(length * (interp_order + 1) * sizeof(*result));
  double fm1 = coefs[0], f1 = 0, f2 = 0;

  for (i = num_coefs4 - 1; i >= 0; --i)
    for (j = num_phases - 1; j >= 0; --j) {
      double f0 = fm1, b = 0, c = 0, d = 0; /* = 0 to kill compiler warning */
      int pos = i * num_phases + j - 1;
      fm1 = coefs4_check(i) && pos > 0 ? coefs[pos - 1] * multiplier : 0;
      switch (interp_order) {
        case 1: b = f1 - f0; break;
        case 2: b = f1 - (.5 * (f2+f0) - f1) - f0; c = .5 * (f2+f0) - f1; break;
        case 3: c=.5*(f1+fm1)-f0;d=(1/6.)*(f2-f1+fm1-f0-4*c);b=f1-f0-d-c; break;
        default: if (interp_order) assert(0);
      }
      #define coef_coef(x) \
        coef(result, interp_order, num_coefs4, j, x, num_coefs4 - 1 - i)
      coef_coef(0) = f0;
      if (interp_order > 0) coef_coef(1) = b;
      if (interp_order > 1) coef_coef(2) = c;
      if (interp_order > 2) coef_coef(3) = d;
      #undef coef_coef
      f2 = f1, f1 = f0;
    }
  return result;
}

typedef struct { /* So generated filter coefs may be shared between channels */
  sample_t   * poly_fir_coefs;
  dft_filter_t dft_filter[2];
} rate_shared_t;

struct stage;
typedef void (* stage_fn_t)(struct stage * input, fifo_t * output);
typedef struct stage {
  /* Common to all stage types: */
  stage_fn_t fn;
  fifo_t     fifo;
  int        pre;       /* Number of past samples to store */
  int        pre_post;  /* pre + number of future samples to store */
  int        preload;   /* Number of zero samples to pre-load the fifo */
  double     out_in_ratio; /* For buffer management. */

  /* For a stage with variable (run-time generated) filter coefs: */
  rate_shared_t * shared;
  int        dft_filter_num; /* Which, if any, of the 2 DFT filters to use */

  /* For a stage with variable L/M: */
  union {               /* 32bit.32bit fixed point arithmetic */
    #if defined(WORDS_BIGENDIAN)
    struct {int32_t integer; uint32_t fraction;} parts;
    #else
    struct {uint32_t fraction; int32_t integer;} parts;
    #endif
    int64_t all;
    #define MULT32 (65536. * 65536.)

    hi_prec_clock_t hi_prec_clock;
  } at, step;
  sox_bool   use_hi_prec_clock;
  int        L, remL, remM;
  int        n, phase_bits;
} stage_t;

#define stage_occupancy(s) max(0, fifo_occupancy(&(s)->fifo) - (s)->pre_post)
#define stage_read_p(s) ((sample_t *)fifo_read_ptr(&(s)->fifo) + (s)->pre)

static void cubic_stage_fn(stage_t * p, fifo_t * output_fifo)
{
  int i, num_in = stage_occupancy(p), max_num_out = 1 + num_in*p->out_in_ratio;
  sample_t const * input = stage_read_p(p);
  sample_t * output = fifo_reserve(output_fifo, max_num_out);

  for (i = 0; p->at.parts.integer < num_in; ++i, p->at.all += p->step.all) {
    sample_t const * s = input + p->at.parts.integer;
    sample_t x = p->at.parts.fraction * (1 / MULT32);
    sample_t b = .5*(s[1]+s[-1])-*s, a = (1/6.)*(s[2]-s[1]+s[-1]-*s-4*b);
    sample_t c = s[1]-*s-a-b;
    output[i] = ((a*x + b)*x + c)*x + *s;
  }
  assert(max_num_out - i >= 0);
  fifo_trim_by(output_fifo, max_num_out - i);
  fifo_read(&p->fifo, p->at.parts.integer, NULL);
  p->at.parts.integer = 0;
}

static void dft_stage_fn(stage_t * p, fifo_t * output_fifo)
{
  sample_t * output, tmp;
  int i, j, num_in = max(0, fifo_occupancy(&p->fifo));
  rate_shared_t const * s = p->shared;
  dft_filter_t const * f = &s->dft_filter[p->dft_filter_num];
  int const overlap = f->num_taps - 1;

  while (p->remL + p->L * num_in >= f->dft_length) {
    div_t divd = div(f->dft_length - overlap - p->remL + p->L - 1, p->L);
    sample_t const * input = fifo_read_ptr(&p->fifo);
    fifo_read(&p->fifo, divd.quot, NULL);
    num_in -= divd.quot;

    output = fifo_reserve(output_fifo, f->dft_length);
    if (lsx_is_power_of_2(p->L)) { /* F-domain */
      int portion = f->dft_length / p->L;
      memcpy(output, input, (unsigned)portion * sizeof(*output));
      lsx_safe_rdft(portion, 1, output);
      for (i = portion + 2; i < (portion << 1); i += 2)
        output[i] = output[(portion << 1) - i],
        output[i+1] = -output[(portion << 1) - i + 1];
      output[portion] = output[1];
      output[portion + 1] = 0;
      output[1] = output[0];
      for (portion <<= 1; i < f->dft_length; i += portion, portion <<= 1) {
        memcpy(output + i, output, portion * sizeof(*output));
        output[i + 1] = 0;
      }
    } else {
      if (p->L == 1)
        memcpy(output, input, f->dft_length * sizeof(*output));
      else {
        memset(output, 0, f->dft_length * sizeof(*output));
        for (j = 0, i = p->remL; i < f->dft_length; ++j, i += p->L)
          output[i] = input[j];
        p->remL = p->L - 1 - divd.rem;
      }
      lsx_safe_rdft(f->dft_length, 1, output);
    }
    output[0] *= f->coefs[0];
    if (p->step.parts.integer > 0) {
      output[1] *= f->coefs[1];
      for (i = 2; i < f->dft_length; i += 2) {
        tmp = output[i];
        output[i  ] = f->coefs[i  ] * tmp - f->coefs[i+1] * output[i+1];
        output[i+1] = f->coefs[i+1] * tmp + f->coefs[i  ] * output[i+1];
      }
      lsx_safe_rdft(f->dft_length, -1, output);
      if (p->step.parts.integer != 1) {
        for (j = 0, i = p->remM; i < f->dft_length - overlap; ++j,
            i += p->step.parts.integer)
          output[j] = output[i];
        p->remM = i - (f->dft_length - overlap);
        fifo_trim_by(output_fifo, f->dft_length - j);
      }
      else fifo_trim_by(output_fifo, overlap);
    }
    else { /* F-domain */
      int m = -p->step.parts.integer;
      for (i = 2; i < (f->dft_length >> m); i += 2) {
        tmp = output[i];
        output[i  ] = f->coefs[i  ] * tmp - f->coefs[i+1] * output[i+1];
        output[i+1] = f->coefs[i+1] * tmp + f->coefs[i  ] * output[i+1];
      }
      output[1] = f->coefs[i] * output[i] - f->coefs[i+1] * output[i+1];
      lsx_safe_rdft(f->dft_length >> m, -1, output);
      fifo_trim_by(output_fifo, (((1 << m) - 1) * f->dft_length + overlap) >>m);
    }
  }
}

static void dft_stage_init(
    unsigned instance, double Fp, double Fs, double Fn, double att,
    double phase, stage_t * stage, int L, int M)
{
  dft_filter_t * f = &stage->shared->dft_filter[instance];
  
  if (!f->num_taps) {
    int num_taps = 0, dft_length, i;
    int k = phase == 50 && lsx_is_power_of_2(L) && Fn == L? L << 1 : 4;
    double * h = lsx_design_lpf(Fp, Fs, Fn, att, &num_taps, -k, -1.);

    if (phase != 50)
      lsx_fir_to_phase(&h, &num_taps, &f->post_peak, phase);
    else f->post_peak = num_taps / 2;

    dft_length = lsx_set_dft_length(num_taps);
    f->coefs = calloc(dft_length, sizeof(*f->coefs));
    for (i = 0; i < num_taps; ++i)
      f->coefs[(i + dft_length - num_taps + 1) & (dft_length - 1)]
        = h[i] / dft_length * 2 * L;
    free(h);
    f->num_taps = num_taps;
    f->dft_length = dft_length;
    lsx_safe_rdft(dft_length, 1, f->coefs);
    lsx_debug("fir_len=%i dft_length=%i Fp=%g Fs=%g Fn=%g att=%g %i/%i",
        num_taps, dft_length, Fp, Fs, Fn, att, L, M);
  }
  stage->fn = dft_stage_fn;
  stage->preload = f->post_peak / L;
  stage->remL    = f->post_peak % L;
  stage->L = L;
  stage->step.parts.integer = abs(3-M) == 1 && Fs == 1? -M/2 : M;
  stage->dft_filter_num = instance;
}

#include "rate_filters.h"

typedef struct {
  double     factor;
  uint64_t   samples_in, samples_out;
  int        num_stages;
  stage_t    * stages;
} rate_t;

#define pre_stage       p->stages[shift]
#define arb_stage       p->stages[shift + have_pre_stage]
#define post_stage      p->stages[shift + have_pre_stage + have_arb_stage]
#define have_pre_stage  (preM  * preL  != 1)
#define have_arb_stage  (arbM  * arbL  != 1)
#define have_post_stage (postM * postL != 1)

#define TO_3dB(a)       ((1.6e-6*a-7.5e-4)*a+.646)
#define LOW_Q_BW0_PC    (67 + 5 / 8.)

typedef enum {
  rolloff_none, rolloff_small /* <= 0.01 dB */, rolloff_medium /* <= 0.35 dB */
} rolloff_t;

static void rate_init(
  /* Private work areas (to be supplied by the client):                       */
  rate_t * p,                /* Per audio channel.                            */
  rate_shared_t * shared,    /* Between channels (undergoing same rate change)*/
                            
  /* Public parameters:                                             Typically */
  double factor,             /* Input rate divided by output rate.            */
  double bits,               /* Required bit-accuracy (pass + stop)  16|20|28 */
  double phase,              /* Linear/minimum etc. filter phase.       50    */
  double bw_pc,              /* Pass-band % (0dB pt.) to preserve.   91.3|98.4*/
  double anti_aliasing_pc,   /* % bandwidth without aliasing            100   */
  rolloff_t rolloff,         /* Pass-band roll-off                    small   */
  sox_bool maintain_3dB_pt,  /*                                        true   */
                            
  /* Primarily for test/development purposes:                                 */
  sox_bool use_hi_prec_clock,/* Increase irrational ratio accuracy.   false   */
  int interpolator,          /* Force a particular coef interpolator.   -1    */
  int max_coefs_size,        /* k bytes of coefs to try to keep below.  400   */
  sox_bool noSmallIntOpt)    /* Disable small integer optimisations.  false   */
{
  double att = (bits + 1) * linear_to_dB(2.), attArb = att;    /* pass + stop */
  double tbw0 = 1 - bw_pc / 100, Fs_a = 2 - anti_aliasing_pc / 100;
  double arbM = factor, tbw_tighten = 1;
  int n = 0, i, preL = 1, preM = 1, shift = 0, arbL = 1, postL = 1, postM = 1;
  sox_bool upsample = sox_false, rational = sox_false, iOpt = !noSmallIntOpt;
  int mode = rolloff > rolloff_small? factor > 1 || bw_pc > LOW_Q_BW0_PC :
    ceil(2 + (bits - 17) / 4);
  stage_t * s;

  assert(factor > 0);
  assert(!bits || (15 <= bits && bits <= 33));
  assert(0 <= phase && phase <= 100);
  assert(53 <= bw_pc && bw_pc <= 100);
  assert(85 <= anti_aliasing_pc && anti_aliasing_pc <= 100);

  p->factor = factor;
  if (bits) while (!n++) {                               /* Determine stages: */
    int try, L, M, x, maxL = interpolator > 0? 1 : mode? 2048 :
      ceil(max_coefs_size * 1000. / (U100_l * sizeof(sample_t)));
    double d, epsilon = 0, frac;
    upsample = arbM < 1;
    for (i = arbM * .5, shift = 0; i >>= 1; arbM *= .5, ++shift);
    preM = upsample || (arbM > 1.5 && arbM < 2);
    postM = 1 + (arbM > 1 && preM), arbM /= postM;
    preL = 1 + (!preM && arbM < 2) + (upsample && mode), arbM *= preL;
    if ((frac = arbM - (int)arbM))
      epsilon = fabs((uint32_t)(frac * MULT32 + .5) / (frac * MULT32) - 1);
    for (i = 1, rational = !frac; i <= maxL && !rational; ++i) {
      d = frac * i, try = d + .5;
      if ((rational = fabs(try / d - 1) <= epsilon)) {    /* No long doubles! */
        if (try == i)
          arbM = ceil(arbM), shift += arbM > 2, arbM /= 1 + (arbM > 2);
        else arbM = i * (int)arbM + try, arbL = i;
      }
    }
    L = preL * arbL, M = arbM * postM, x = (L|M)&1, L >>= !x, M >>= !x;
    if (iOpt && postL == 1 && (d = preL * arbL / arbM) > 4 && d != 5) {
      for (postL = 4, i = d / 16; i >>= 1; postL <<= 1);
      arbM = arbM * postL / arbL / preL, arbL = 1, n = 0;
    } else if (rational && (max(L, M) < 3 + 2 * iOpt || L * M < 6 * iOpt))
      preL = L, preM = M, arbM = arbL = postM = 1;
    if (!mode && (!rational || !n))
      ++mode, n = 0;
  }

  p->num_stages = shift + have_pre_stage + have_arb_stage + have_post_stage;

  if (!p->num_stages)
    return;

  p->stages = calloc(p->num_stages + 1, sizeof(*p->stages));
  for (i = 0; i < p->num_stages; ++i)
    p->stages[i].shared = shared;

  if ((n = p->num_stages) > 1) {                              /* Att. budget: */
    if (have_arb_stage)
      att += linear_to_dB(2.), attArb = att, --n; 
    att += linear_to_dB((double)n);
  }

  for (n = 0; n + 1u < array_length(half_firs) && att > half_firs[n].att; ++n);
  for (i = 0, s = p->stages; i < shift; ++i, ++s) {
    s->fn = half_firs[n].fn;
    s->pre_post = 4 * half_firs[n].num_coefs;
    s->preload = s->pre = s->pre_post >> 1;
  }

  if (have_pre_stage) {
    if (maintain_3dB_pt && have_post_stage) {    /* Trans. bands overlapping. */
      double tbw3 = tbw0 * TO_3dB(att);               /* TODO: consider Fs_a. */
      double x = ((2.1429e-4 - 5.2083e-7 * att) * att - .015863) * att + 3.95;
      x = att * pow((tbw0 - tbw3) / (postM / (factor * postL) - 1 + tbw0), x);
      if (x > .035) {
        tbw_tighten = ((4.3074e-3 - 3.9121e-4 * x) * x - .040009) * x + 1.0014;
        lsx_debug("x=%g tbw_tighten=%g", x, tbw_tighten);
      }
    }
    dft_stage_init(0, 1 - tbw0 * tbw_tighten, Fs_a, preM? max(preL, preM) :
        arbM / arbL, att, phase, &pre_stage, preL, max(preM, 1));
  }

  if (!bits) {                                  /* Quick and dirty arb stage: */
    arb_stage.fn = cubic_stage_fn;
    arb_stage.step.all = arbM * MULT32 + .5;
    arb_stage.pre_post = max(3, arb_stage.step.parts.integer);
    arb_stage.preload = arb_stage.pre = 1;
    arb_stage.out_in_ratio = MULT32 * arbL / arb_stage.step.all;
  }
  else if (have_arb_stage) {                     /* Higher quality arb stage: */
    poly_fir_t const * f = &poly_firs[6*(upsample + !!preM) + mode - !upsample];
    int order, num_coefs = f->interp[0].scalar, phase_bits, phases, coefs_size;
    double x = .5, at, Fp, Fs, Fn, mult = upsample? 1 : arbL / arbM;
    poly_fir1_t const * f1;

    Fn = !upsample && preM? x = arbM / arbL : 1;
    Fp = !preM? mult : mode? .5 : 1;
    Fs = 2 - Fp;           /* Ignore Fs_a; it would have little benefit here. */
    Fp *= 1 - tbw0;
    if (rolloff > rolloff_small && mode)
      Fp = !preM? mult * .5 - .125 : mult * .05 + .1;
    else if (rolloff == rolloff_small)
      Fp = Fs - (Fs - .148 * x - Fp * .852) * (.00813 * bits + .973);

    i = (interpolator < 0? !rational : max(interpolator, !rational)) - 1;
    do {
      f1 = &f->interp[++i];
      assert(f1->fn);
      if (i)
        arbM /= arbL, arbL = 1, rational = sox_false;
      phase_bits = ceil(f1->scalar + log(mult)/log(2.));
      phases = !rational? (1 << phase_bits) : arbL;
      if (!f->interp[0].scalar) {
        int phases0 = max(phases, 19), n0 = 0;
        lsx_design_lpf(Fp, Fs, -Fn, attArb, &n0, phases0, f->beta);
        num_coefs = n0 / phases0 + 1, num_coefs += num_coefs & !preM;
      }
      if ((num_coefs & 1) && rational && (arbL & 1))
        phases <<= 1, arbL <<= 1, arbM *= 2;
      at = arbL * .5 * (num_coefs & 1);
      order = i + (i && mode > 4);
      coefs_size = num_coefs4 * phases * (order + 1) * sizeof(sample_t);
    } while (interpolator < 0 && i < 2 && f->interp[i+1].fn &&
        coefs_size / 1000 > max_coefs_size);

    if (!arb_stage.shared->poly_fir_coefs) {
      int num_taps = num_coefs * phases - 1;
      raw_coef_t * coefs = lsx_design_lpf(
          Fp, Fs, Fn, attArb, &num_taps, phases, f->beta);
      arb_stage.shared->poly_fir_coefs = prepare_coefs(
          coefs, num_coefs, phases, order, 1);
      lsx_debug("fir_len=%i phases=%i coef_interp=%i size=%s",
          num_coefs, phases, order, lsx_sigfigs3((double)coefs_size));
      free(coefs);
    }
    arb_stage.fn = f1->fn;
    arb_stage.pre_post = num_coefs4 - 1;
    arb_stage.preload = (num_coefs - 1) >> 1;
    arb_stage.n = num_coefs4;
    arb_stage.phase_bits = phase_bits;
    arb_stage.L = arbL;
    arb_stage.use_hi_prec_clock = mode > 1 && use_hi_prec_clock && !rational;
    if (arb_stage.use_hi_prec_clock) {
      arb_stage.at.hi_prec_clock = at;
      arb_stage.step.hi_prec_clock = arbM;
      arb_stage.out_in_ratio = arbL / arb_stage.step.hi_prec_clock;
    } else {
      arb_stage.at.all = at * MULT32 + .5;
      arb_stage.step.all = arbM * MULT32 + .5;
      arb_stage.out_in_ratio = MULT32 * arbL / arb_stage.step.all;
    }
  }

  if (have_post_stage)
    dft_stage_init(1, 1 - (1 - (1 - tbw0) *
        (upsample? factor * postL / postM : 1)) * tbw_tighten, Fs_a,
        (double)max(postL, postM), att, phase, &post_stage, postL, postM);

  for (i = 0, s = p->stages; i < p->num_stages; ++i, ++s) {
    fifo_create(&s->fifo, (int)sizeof(sample_t));
    memset(fifo_reserve(&s->fifo, s->preload), 0, sizeof(sample_t)*s->preload);
    lsx_debug("%5i|%-5i preload=%i remL=%i",
        s->pre, s->pre_post - s->pre, s->preload, s->remL);
  }
  fifo_create(&s->fifo, (int)sizeof(sample_t));
}

static void rate_process(rate_t * p)
{
  stage_t * stage = p->stages;
  int i;

  for (i = 0; i < p->num_stages; ++i, ++stage)
    stage->fn(stage, &(stage+1)->fifo);
}

static sample_t * rate_input(rate_t * p, sample_t const * samples, size_t n)
{
  p->samples_in += n;
  return fifo_write(&p->stages[0].fifo, (int)n, samples);
}

static sample_t const * rate_output(rate_t * p, sample_t * samples, size_t * n)
{
  fifo_t * fifo = &p->stages[p->num_stages].fifo;
  p->samples_out += *n = min(*n, (size_t)fifo_occupancy(fifo));
  return fifo_read(fifo, (int)*n, samples);
}

static void rate_flush(rate_t * p)
{
  fifo_t * fifo = &p->stages[p->num_stages].fifo;
  uint64_t samples_out = p->samples_in / p->factor + .5;
  size_t remaining = samples_out > p->samples_out ?
      (size_t)(samples_out - p->samples_out) : 0;
  sample_t * buff = calloc(1024, sizeof(*buff));

  if (remaining > 0) {
    while ((size_t)fifo_occupancy(fifo) < remaining) {
      rate_input(p, buff, (size_t) 1024);
      rate_process(p);
    }
    fifo_trim_to(fifo, (int)remaining);
    p->samples_in = 0;
  }
  free(buff);
}

static void rate_close(rate_t * p)
{
  rate_shared_t *shared;
  int i;

  if (!p->num_stages)
    return;

  shared = p->stages[0].shared;

  for (i = 0; i <= p->num_stages; ++i)
    fifo_delete(&p->stages[i].fifo);
  free(shared->dft_filter[0].coefs);
  free(shared->dft_filter[1].coefs);
  free(shared->poly_fir_coefs);
  memset(shared, 0, sizeof(*shared));
  free(p->stages);
}

/*------------------------------- SoX Wrapper --------------------------------*/

typedef struct {
  sox_rate_t      out_rate;
  int             rolloff, coef_interp, max_coefs_size;
  double          bit_depth, phase, bw_0dB_pc, anti_aliasing_pc;
  sox_bool        use_hi_prec_clock, noIOpt, given_0dB_pt;
  rate_t          rate;
  rate_shared_t   shared, * shared_ptr;
} priv_t;

static int create(sox_effect_t * effp, int argc, char **argv)
{
  priv_t * p = (priv_t *) effp->priv;
  int c, quality;
  char * dummy_p, * found_at;
  char const * opts = "+i:c:b:B:A:p:Q:R:d:MILafnost" "qlmghevu";
  char const * qopts = strchr(opts, 'q');
  double rej = 0, bw_3dB_pc = 0;
  sox_bool allow_aliasing = sox_false;
  lsx_getopt_t optstate;
  lsx_getopt_init(argc, argv, opts, NULL, lsx_getopt_flag_none, 1, &optstate);

  p->coef_interp = quality = -1;
  p->rolloff = rolloff_small;
  p->phase = 50;
  p->max_coefs_size = 400;
  p->shared_ptr = &p->shared;

  while ((c = lsx_getopt(&optstate)) != -1) switch (c) {
    GETOPT_NUMERIC(optstate, 'i', coef_interp, -1, 2)
    GETOPT_NUMERIC(optstate, 'c', max_coefs_size, 100, INT_MAX)
    GETOPT_NUMERIC(optstate, 'p', phase, 0, 100)
    GETOPT_NUMERIC(optstate, 'B', bw_0dB_pc, 53, 99.5)
    GETOPT_NUMERIC(optstate, 'A', anti_aliasing_pc, 85, 100)
    GETOPT_NUMERIC(optstate, 'd', bit_depth, 15, 33)
    GETOPT_LOCAL_NUMERIC(optstate, 'b', bw_3dB_pc, 74, 99.7)
    GETOPT_LOCAL_NUMERIC(optstate, 'R', rej, 90, 200)
    GETOPT_LOCAL_NUMERIC(optstate, 'Q', quality, 0, 7)
    case 'M': p->phase =  0; break;
    case 'I': p->phase = 25; break;
    case 'L': p->phase = 50; break;
    case 'a': allow_aliasing = sox_true; break;
    case 'f': p->rolloff = rolloff_none; break;
    case 'n': p->noIOpt = sox_true; break;
    case 's': bw_3dB_pc = 99; break;
    case 't': p->use_hi_prec_clock = sox_true; break;
    default:
      if ((found_at = strchr(qopts, c)))
        quality = found_at - qopts;
      else {
        lsx_fail("unknown option `-%c'", optstate.opt);
        return lsx_usage(effp);
      }
  }
  argc -= optstate.ind, argv += optstate.ind;

  if ((unsigned)quality < 2 && (p->bw_0dB_pc || bw_3dB_pc || p->phase != 50 ||
        allow_aliasing || rej || p->bit_depth || p->anti_aliasing_pc)) {
    lsx_fail("override options not allowed with this quality level");
    return SOX_EOF;
  }
  if (quality < 0 && rej == 0 && p->bit_depth == 0)
    quality = 4;
  if (rej)
    p->bit_depth = rej / linear_to_dB(2.);
  else {
    if (quality >= 0) {
      p->bit_depth = quality? 16 + 4 * max(quality - 3, 0) : 0;
      if (quality <= 2)
        p->rolloff = rolloff_medium;
    }
    rej = p->bit_depth * linear_to_dB(2.);
  }

  if (bw_3dB_pc && p->bw_0dB_pc) {
    lsx_fail("conflicting bandwidth options");
    return SOX_EOF;
  }
  allow_aliasing |= p->anti_aliasing_pc != 0;
  if (!bw_3dB_pc && !p->bw_0dB_pc)
    p->bw_0dB_pc = quality == 1? LOW_Q_BW0_PC : 100 - 5 / TO_3dB(rej);
  else if (bw_3dB_pc && bw_3dB_pc < 85 && allow_aliasing) {
    lsx_fail("minimum allowed 3dB bandwidth with aliasing is %g%%", 85.);
    return SOX_EOF;
  }
  else if (p->bw_0dB_pc && p->bw_0dB_pc < 74 && allow_aliasing) {
    lsx_fail("minimum allowed bandwidth with aliasing is %g%%", 74.);
    return SOX_EOF;
  }
  if (bw_3dB_pc)
    p->bw_0dB_pc = 100 - (100 - bw_3dB_pc) / TO_3dB(rej);
  else {
    bw_3dB_pc = 100 - (100 - p->bw_0dB_pc) * TO_3dB(rej);
    p->given_0dB_pt = sox_true;
  }
  p->anti_aliasing_pc = p->anti_aliasing_pc? p->anti_aliasing_pc :
    allow_aliasing? bw_3dB_pc : 100;

  if (argc) {
    if ((p->out_rate = lsx_parse_frequency(*argv, &dummy_p)) <= 0 || *dummy_p)
      return lsx_usage(effp);
    argc--; argv++;
    effp->out_signal.rate = p->out_rate;
  }
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  double out_rate = p->out_rate != 0 ? p->out_rate : effp->out_signal.rate;

  if (effp->in_signal.rate == out_rate)
    return SOX_EFF_NULL;

  if (effp->in_signal.mult)
    *effp->in_signal.mult *= .705; /* 1/(2/sinc(pi/3)-1); see De Soras 4.1.2 */

  effp->out_signal.channels = effp->in_signal.channels;
  effp->out_signal.rate = out_rate;
  rate_init(&p->rate, p->shared_ptr, effp->in_signal.rate/out_rate,p->bit_depth,
      p->phase, p->bw_0dB_pc, p->anti_aliasing_pc, p->rolloff, !p->given_0dB_pt,
      p->use_hi_prec_clock, p->coef_interp, p->max_coefs_size, p->noIOpt);

  if (!p->rate.num_stages) {
    lsx_warn("input and output rates too close, skipping resampling");
    return SOX_EFF_NULL;
  }

  return SOX_SUCCESS;
}

static int flow(sox_effect_t * effp, const sox_sample_t * ibuf,
                sox_sample_t * obuf, size_t * isamp, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t odone = *osamp;

  sample_t const * s = rate_output(&p->rate, NULL, &odone);
  lsx_save_samples(obuf, s, odone, &effp->clips);

  if (*isamp && odone < *osamp) {
    sample_t * t = rate_input(&p->rate, NULL, *isamp);
    lsx_load_samples(t, ibuf, *isamp);
    rate_process(&p->rate);
  }
  else *isamp = 0;
  *osamp = odone;
  return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  static size_t isamp = 0;
  rate_flush(&p->rate);
  return flow(effp, 0, obuf, &isamp, osamp);
}

static int stop(sox_effect_t * effp)
{
  priv_t * p = (priv_t *) effp->priv;
  rate_close(&p->rate);
  return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_rate_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "rate", 0, SOX_EFF_RATE, create, start, flow, drain, stop, 0, sizeof(priv_t)
  };
  static char const * lines[] = {
    "[-q|-l|-m|-h|-v] [override-options] RATE[k]",
    "                    BAND-",
    "     QUALITY        WIDTH  REJ dB   TYPICAL USE",
    " -q  quick          n/a  ~30 @ Fs/4 playback on ancient hardware",
    " -l  low            80%     100     playback on old hardware",
    " -m  medium         95%     100     audio playback",
    " -h  high (default) 95%     125     16-bit mastering (use with dither)",
    " -v  very high      95%     175     24-bit mastering",
    "              OVERRIDE OPTIONS (only with -m, -h, -v)",
    " -M/-I/-L     Phase response = minimum/intermediate/linear(default)",
    " -s           Steep filter (band-width = 99%)",
    " -a           Allow aliasing above the pass-band",
    " -b 74-99.7   Any band-width %",
    " -p 0-100     Any phase response (0 = minimum, 25 = intermediate,",
    "              50 = linear, 100 = maximum)",
  };
  static char * usage;
  handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));
  return &handler;
}
