/* libSoX internal DSP functions.
 * All public functions & data are prefixed with lsx_ .
 *
 * Copyright (c) 2008,2012 robs@users.sourceforge.net
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

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include "sox_i.h"
#include <assert.h>
#include <string.h>

/* Concurrent Control with "Readers" and "Writers", P.J. Courtois et al, 1971:*/

#if defined HAVE_OPENMP

typedef struct {
  int readcount, writecount; /* initial value = 0 */
  omp_lock_t mutex_1, mutex_2, mutex_3, w, r; /* initial value = 1 */
} ccrw2_t; /* Problem #2: `writers-preference' */

#define ccrw2_become_reader(p) do {\
  omp_set_lock(&p.mutex_3);\
    omp_set_lock(&p.r);\
      omp_set_lock(&p.mutex_1);\
        if (++p.readcount == 1) omp_set_lock(&p.w);\
      omp_unset_lock(&p.mutex_1);\
    omp_unset_lock(&p.r);\
  omp_unset_lock(&p.mutex_3);\
} while (0)
#define ccrw2_cease_reading(p) do {\
  omp_set_lock(&p.mutex_1);\
    if (!--p.readcount) omp_unset_lock(&p.w);\
  omp_unset_lock(&p.mutex_1);\
} while (0)
#define ccrw2_become_writer(p) do {\
  omp_set_lock(&p.mutex_2);\
    if (++p.writecount == 1) omp_set_lock(&p.r);\
  omp_unset_lock(&p.mutex_2);\
  omp_set_lock(&p.w);\
} while (0)
#define ccrw2_cease_writing(p) do {\
  omp_unset_lock(&p.w);\
  omp_set_lock(&p.mutex_2);\
    if (!--p.writecount) omp_unset_lock(&p.r);\
  omp_unset_lock(&p.mutex_2);\
} while (0)
#define ccrw2_init(p) do {\
  omp_init_lock(&p.mutex_1);\
  omp_init_lock(&p.mutex_2);\
  omp_init_lock(&p.mutex_3);\
  omp_init_lock(&p.w);\
  omp_init_lock(&p.r);\
} while (0)
#define ccrw2_clear(p) do {\
  omp_destroy_lock(&p.r);\
  omp_destroy_lock(&p.w);\
  omp_destroy_lock(&p.mutex_3);\
  omp_destroy_lock(&p.mutex_2);\
  omp_destroy_lock(&p.mutex_1);\
} while (0)

#else

#define ccrw2_become_reader(x) (void)0
#define ccrw2_cease_reading(x) (void)0
#define ccrw2_become_writer(x) (void)0
#define ccrw2_cease_writing(x) (void)0
#define ccrw2_init(x) (void)0
#define ccrw2_clear(x) (void)0

#endif /* HAVE_OPENMP */

/* Numerical Recipes cubic spline: */

void lsx_prepare_spline3(double const * x, double const * y, int n,
    double start_1d, double end_1d, double * y_2d)
{
  double p, qn, sig, un, * u = lsx_malloc((n - 1) * sizeof(*u));
  int i;

  if (start_1d == HUGE_VAL)
    y_2d[0] = u[0] = 0;      /* Start with natural spline or */
  else {                     /* set the start first derivative */
    y_2d[0] = -.5;
    u[0] = (3 / (x[1] - x[0])) * ((y[1] - y[0]) / (x[1] - x[0]) - start_1d);
  }

  for (i = 1; i < n - 1; ++i) {
    sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
    p = sig * y_2d[i - 1] + 2;
    y_2d[i] = (sig - 1) / p;
    u[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) -
           (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
    u[i] = (6 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
  }
  if (end_1d == HUGE_VAL)
    qn = un = 0;             /* End with natural spline or */
  else {                     /* set the end first derivative */
    qn = .5;
    un = 3 / (x[n - 1] - x[n - 2]) * (end_1d - (y[n - 1] - y[n - 2]) / (x[n - 1] - x[n - 2]));
  }
  y_2d[n - 1] = (un - qn * u[n - 2]) / (qn * y_2d[n - 2] + 1);
  for (i = n - 2; i >= 0; --i)
    y_2d[i] = y_2d[i] * y_2d[i + 1] + u[i];
  free(u);
}

double lsx_spline3(double const * x, double const * y, double const * y_2d,
    int n, double x1)
{
  int     t, i[2] = {0, 0};
  double  d, a, b;

  for (i[1] = n - 1; i[1] - i[0] > 1; t = (i[1] + i[0]) >> 1, i[x[t] > x1] = t);
  d = x[i[1]] - x[i[0]];
  assert(d != 0);
  a = (x[i[1]] - x1) / d;
  b = (x1 - x[i[0]]) / d;
  return a * y[i[0]] + b * y[i[1]] +
    ((a * a * a - a) * y_2d[i[0]] + (b * b * b - b) * y_2d[i[1]]) * d * d / 6;
}

double lsx_bessel_I_0(double x)
{
  double term = 1, sum = 1, last_sum, x2 = x / 2;
  int i = 1;
  do {
    double y = x2 / i++;
    last_sum = sum, sum += term *= y * y;
  } while (sum != last_sum);
  return sum;
}

int lsx_set_dft_length(int num_taps) /* Set to 4 x nearest power of 2 */
{      /* or half of that if danger of causing too many cache misses. */
  int min = sox_globals.log2_dft_min_size;
  double d = log((double)num_taps) / log(2.);
  return 1 << range_limit((int)(d + 2.77), min, max((int)(d + 1.77), 17));
}

#include "fft4g.h"
static int * lsx_fft_br;
static double * lsx_fft_sc;
static int fft_len = -1;
#if defined HAVE_OPENMP
static ccrw2_t fft_cache_ccrw;
#endif

void init_fft_cache(void)
{
  assert(lsx_fft_br == NULL);
  assert(lsx_fft_sc == NULL);
  assert(fft_len == -1);
  ccrw2_init(fft_cache_ccrw);
  fft_len = 0;
}

void clear_fft_cache(void)
{
  assert(fft_len >= 0);
  ccrw2_clear(fft_cache_ccrw);
  free(lsx_fft_br);
  free(lsx_fft_sc);
  lsx_fft_sc = NULL;
  lsx_fft_br = NULL;
  fft_len = -1;
}

static sox_bool update_fft_cache(int len)
{
  assert(lsx_is_power_of_2(len));
  assert(fft_len >= 0);
  ccrw2_become_reader(fft_cache_ccrw);
  if (len > fft_len) {
    ccrw2_cease_reading(fft_cache_ccrw);
    ccrw2_become_writer(fft_cache_ccrw);
    if (len > fft_len) {
      int old_n = fft_len;
      fft_len = len;
      lsx_fft_br = lsx_realloc(lsx_fft_br, dft_br_len(fft_len) * sizeof(*lsx_fft_br));
      lsx_fft_sc = lsx_realloc(lsx_fft_sc, dft_sc_len(fft_len) * sizeof(*lsx_fft_sc));
      if (!old_n)
        lsx_fft_br[0] = 0;
      return sox_true;
    }
    ccrw2_cease_writing(fft_cache_ccrw);
    ccrw2_become_reader(fft_cache_ccrw);
  }
  return sox_false;
}

static void done_with_fft_cache(sox_bool is_writer)
{
  if (is_writer)
    ccrw2_cease_writing(fft_cache_ccrw);
  else ccrw2_cease_reading(fft_cache_ccrw);
}

void lsx_safe_rdft(int len, int type, double * d)
{
  sox_bool is_writer = update_fft_cache(len);
  lsx_rdft(len, type, d, lsx_fft_br, lsx_fft_sc);
  done_with_fft_cache(is_writer);
}

void lsx_safe_cdft(int len, int type, double * d)
{
  sox_bool is_writer = update_fft_cache(len);
  lsx_cdft(len, type, d, lsx_fft_br, lsx_fft_sc);
  done_with_fft_cache(is_writer);
}

void lsx_power_spectrum(int n, double const * in, double * out)
{
  int i;
  double * work = lsx_memdup(in, n * sizeof(*work));
  lsx_safe_rdft(n, 1, work);
  out[0] = sqr(work[0]);
  for (i = 2; i < n; i += 2)
    out[i >> 1] = sqr(work[i]) + sqr(work[i + 1]);
  out[i >> 1] = sqr(work[1]);
  free(work);
}

void lsx_power_spectrum_f(int n, float const * in, float * out)
{
  int i;
  double * work = lsx_malloc(n * sizeof(*work));
  for (i = 0; i< n; ++i) work[i] = in[i];
  lsx_safe_rdft(n, 1, work);
  out[0] = sqr(work[0]);
  for (i = 2; i < n; i += 2)
    out[i >> 1] = sqr(work[i]) + sqr(work[i + 1]);
  out[i >> 1] = sqr(work[1]);
  free(work);
}

void lsx_apply_hann_f(float h[], const int num_points)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    double x = 2 * M_PI * i / m;
    h[i] *= .5 - .5 * cos(x);
  }
}

void lsx_apply_hann(double h[], const int num_points)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    double x = 2 * M_PI * i / m;
    h[i] *= .5 - .5 * cos(x);
  }
}

void lsx_apply_hamming(double h[], const int num_points)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    double x = 2 * M_PI * i / m;
    h[i] *= .53836 - .46164 * cos(x);
  }
}

void lsx_apply_bartlett(double h[], const int num_points)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    h[i] *= 2. / m * (m / 2. - fabs(i - m / 2.));
  }
}

void lsx_apply_blackman(double h[], const int num_points, double alpha /*.16*/)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    double x = 2 * M_PI * i / m;
    h[i] *= (1 - alpha) *.5 - .5 * cos(x) + alpha * .5 * cos(2 * x);
  }
}

void lsx_apply_blackman_nutall(double h[], const int num_points)
{
  int i, m = num_points - 1;
  for (i = 0; i < num_points; ++i) {
    double x = 2 * M_PI * i / m;
    h[i] *= .3635819 - .4891775 * cos(x) + .1365995 * cos(2 * x) - .0106411 * cos(3 * x);
  }
}

double lsx_kaiser_beta(double att, double tr_bw)
{
  if (att >= 60) {
    static const double coefs[][4] = {
      {-6.784957e-10,1.02856e-05,0.1087556,-0.8988365+.001},
      {-6.897885e-10,1.027433e-05,0.10876,-0.8994658+.002},
      {-1.000683e-09,1.030092e-05,0.1087677,-0.9007898+.003},
      {-3.654474e-10,1.040631e-05,0.1087085,-0.8977766+.006},
      {8.106988e-09,6.983091e-06,0.1091387,-0.9172048+.015},
      {9.519571e-09,7.272678e-06,0.1090068,-0.9140768+.025},
      {-5.626821e-09,1.342186e-05,0.1083999,-0.9065452+.05},
      {-9.965946e-08,5.073548e-05,0.1040967,-0.7672778+.085},
      {1.604808e-07,-5.856462e-05,0.1185998,-1.34824+.1},
      {-1.511964e-07,6.363034e-05,0.1064627,-0.9876665+.18},
    };
    double realm = log(tr_bw/.0005)/log(2.);
    double const * c0 = coefs[range_limit(  (int)realm, 0, (int)array_length(coefs)-1)];
    double const * c1 = coefs[range_limit(1+(int)realm, 0, (int)array_length(coefs)-1)];
    double b0 = ((c0[0]*att + c0[1])*att + c0[2])*att + c0[3];
    double b1 = ((c1[0]*att + c1[1])*att + c1[2])*att + c1[3];
    return b0 + (b1 - b0) * (realm - (int)realm);
  }
  if (att > 50   ) return .1102 * (att - 8.7);
  if (att > 20.96) return .58417 * pow(att -20.96, .4) + .07886 * (att - 20.96);
  return 0;
}

void lsx_apply_kaiser(double h[], const int num_points, double beta)
{
  int i, m = num_points - 1;
  for (i = 0; i <= m; ++i) {
    double x = 2. * i / m - 1;
    h[i] *= lsx_bessel_I_0(beta * sqrt(1 - x * x)) / lsx_bessel_I_0(beta);
  }
}

void lsx_apply_dolph(double h[], const int N, double att)
{
  double b = cosh(acosh(pow(10., att/20)) / (N-1)), sum, t, c, norm = 0;
  int i, j;
  for (c = 1 - 1 / (b*b), i = (N-1) / 2; i >= 0; --i) {
    for (sum = !i, b = t = j = 1; j <= i && sum != t; b *= (i-j) * (1./j), ++j)
      t = sum, sum += (b *= c * (N - i - j) * (1./j));
    sum /= (N - 1 - i), sum /= (norm = norm? norm : sum);
    h[i] *= sum, h[N - 1 - i] *= sum;
  }
}

double * lsx_make_lpf(int num_taps, double Fc, double beta, double rho,
    double scale, sox_bool dc_norm)
{
  int i, m = num_taps - 1;
  double * h = calloc(num_taps, sizeof(*h)), sum = 0;
  double mult = scale / lsx_bessel_I_0(beta), mult1 = 1 / (.5 * m + rho);
  assert(Fc >= 0 && Fc <= 1);
  lsx_debug("make_lpf(n=%i Fc=%.7g β=%g ρ=%g dc-norm=%i scale=%g)", num_taps, Fc, beta, rho, dc_norm, scale);

  if (!h)
    return NULL;

  for (i = 0; i <= m / 2; ++i) {
    double z = i - .5 * m, x = z * M_PI, y = z * mult1;
    h[i] = x? sin(Fc * x) / x : Fc;
    sum += h[i] *= lsx_bessel_I_0(beta * sqrt(1 - y * y)) * mult;
    if (m - i != i)
      sum += h[m - i] = h[i];
  }
  for (i = 0; dc_norm && i < num_taps; ++i) h[i] *= scale / sum;
  return h;
}

void lsx_kaiser_params(double att, double Fc, double tr_bw, double * beta, int * num_taps)
{
  *beta = *beta < 0? lsx_kaiser_beta(att, tr_bw * .5 / Fc): *beta;
  att = att < 60? (att - 7.95) / (2.285 * M_PI * 2) :
    ((.0007528358-1.577737e-05**beta)**beta+.6248022)**beta+.06186902;
  *num_taps = !*num_taps? ceil(att/tr_bw + 1) : *num_taps;
}

double * lsx_design_lpf(
    double Fp,      /* End of pass-band */
    double Fs,      /* Start of stop-band */
    double Fn,      /* Nyquist freq; e.g. 0.5, 1, PI */
    double att,     /* Stop-band attenuation in dB */
    int * num_taps, /* 0: value will be estimated */
    int k,          /* >0: number of phases; <0: num_taps ≡ 1 (mod -k) */
    double beta)    /* <0: value will be estimated */
{
  int n = *num_taps, phases = max(k, 1), modulo = max(-k, 1);
  double tr_bw, Fc, rho = phases == 1? .5 : att < 120? .63 : .75;

  Fp /= fabs(Fn), Fs /= fabs(Fn);        /* Normalise to Fn = 1 */
  tr_bw = .5 * (Fs - Fp); /* Transition band-width: 6dB to stop points */
  tr_bw /= phases, Fs /= phases;
  tr_bw = min(tr_bw, .5 * Fs);
  Fc = Fs - tr_bw;
  assert(Fc - tr_bw >= 0);
  lsx_kaiser_params(att, Fc, tr_bw, &beta, num_taps);
  if (!n)
    *num_taps = phases > 1? *num_taps / phases * phases + phases - 1 : (*num_taps + modulo - 2) / modulo * modulo + 1;
  return Fn < 0? 0 : lsx_make_lpf(
      *num_taps, Fc, beta, rho, (double)phases, sox_false);
}

static double safe_log(double x)
{
  assert(x >= 0);
  if (x)
    return log(x);
  lsx_debug("log(0)");
  return -26;
}

void lsx_fir_to_phase(double * * h, int * len, int * post_len, double phase)
{
  double * pi_wraps, * work, phase1 = (phase > 50 ? 100 - phase : phase) / 50;
  int i, work_len, begin, end, imp_peak = 0, peak = 0;
  double imp_sum = 0, peak_imp_sum = 0;
  double prev_angle2 = 0, cum_2pi = 0, prev_angle1 = 0, cum_1pi = 0;

  for (i = *len, work_len = 2 * 2 * 8; i > 1; work_len <<= 1, i >>= 1);

  work = lsx_calloc((size_t)work_len + 2, sizeof(*work)); /* +2: (UN)PACK */
  pi_wraps = lsx_malloc((((size_t)work_len + 2) / 2) * sizeof(*pi_wraps));

  memcpy(work, *h, *len * sizeof(*work));
  lsx_safe_rdft(work_len, 1, work); /* Cepstral: */
  LSX_UNPACK(work, work_len);

  for (i = 0; i <= work_len; i += 2) {
    double angle = atan2(work[i + 1], work[i]);
    double detect = 2 * M_PI;
    double delta = angle - prev_angle2;
    double adjust = detect * ((delta < -detect * .7) - (delta > detect * .7));
    prev_angle2 = angle;
    cum_2pi += adjust;
    angle += cum_2pi;
    detect = M_PI;
    delta = angle - prev_angle1;
    adjust = detect * ((delta < -detect * .7) - (delta > detect * .7));
    prev_angle1 = angle;
    cum_1pi += fabs(adjust); /* fabs for when 2pi and 1pi have combined */
    pi_wraps[i >> 1] = cum_1pi;

    work[i] = safe_log(sqrt(sqr(work[i]) + sqr(work[i + 1])));
    work[i + 1] = 0;
  }
  LSX_PACK(work, work_len);
  lsx_safe_rdft(work_len, -1, work);
  for (i = 0; i < work_len; ++i) work[i] *= 2. / work_len;

  for (i = 1; i < work_len / 2; ++i) { /* Window to reject acausal components */
    work[i] *= 2;
    work[i + work_len / 2] = 0;
  }
  lsx_safe_rdft(work_len, 1, work);

  for (i = 2; i < work_len; i += 2) /* Interpolate between linear & min phase */
    work[i + 1] = phase1 * i / work_len * pi_wraps[work_len >> 1] +
        (1 - phase1) * (work[i + 1] + pi_wraps[i >> 1]) - pi_wraps[i >> 1];

  work[0] = exp(work[0]), work[1] = exp(work[1]);
  for (i = 2; i < work_len; i += 2) {
    double x = exp(work[i]);
    work[i    ] = x * cos(work[i + 1]);
    work[i + 1] = x * sin(work[i + 1]);
  }

  lsx_safe_rdft(work_len, -1, work);
  for (i = 0; i < work_len; ++i) work[i] *= 2. / work_len;

  /* Find peak pos. */
  for (i = 0; i <= (int)(pi_wraps[work_len >> 1] / M_PI + .5); ++i) {
    imp_sum += work[i];
    if (fabs(imp_sum) > fabs(peak_imp_sum)) {
      peak_imp_sum = imp_sum;
      peak = i;
    }
    if (work[i] > work[imp_peak]) /* For debug check only */
      imp_peak = i;
  }
  while (peak && fabs(work[peak-1]) > fabs(work[peak]) && work[peak-1] * work[peak] > 0)
    --peak;

  if (!phase1)
    begin = 0;
  else if (phase1 == 1)
    begin = peak - *len / 2;
  else {
    begin = (.997 - (2 - phase1) * .22) * *len + .5;
    end   = (.997 + (0 - phase1) * .22) * *len + .5;
    begin = peak - (begin & ~3);
    end   = peak + 1 + ((end + 3) & ~3);
    *len = end - begin;
    *h = lsx_realloc(*h, *len * sizeof(**h));
  }
  for (i = 0; i < *len; ++i) (*h)[i] =
    work[(begin + (phase > 50 ? *len - 1 - i : i) + work_len) & (work_len - 1)];
  *post_len = phase > 50 ? peak - begin : begin + *len - (peak + 1);

  lsx_debug("nPI=%g peak-sum@%i=%g (val@%i=%g); len=%i post=%i (%g%%)",
      pi_wraps[work_len >> 1] / M_PI, peak, peak_imp_sum, imp_peak,
      work[imp_peak], *len, *post_len, 100 - 100. * *post_len / (*len - 1));
  free(pi_wraps), free(work);
}

void lsx_plot_fir(double * h, int num_points, sox_rate_t rate, sox_plot_t type, char const * title, double y1, double y2)
{
  int i, N = lsx_set_dft_length(num_points);
  if (type == sox_plot_gnuplot) {
    double * h1 = lsx_calloc(N, sizeof(*h1));
    double * H = lsx_malloc((N / 2 + 1) * sizeof(*H));
    memcpy(h1, h, sizeof(*h1) * num_points);
    lsx_power_spectrum(N, h1, H);
    printf(
        "# gnuplot file\n"
        "set title '%s'\n"
        "set xlabel 'Frequency (Hz)'\n"
        "set ylabel 'Amplitude Response (dB)'\n"
        "set grid xtics ytics\n"
        "set key off\n"
        "plot '-' with lines\n"
        , title);
    for (i = 0; i <= N/2; ++i)
      printf("%g %g\n", i * rate / N, 10 * log10(H[i]));
    printf(
        "e\n"
        "pause -1 'Hit return to continue'\n");
    free(H);
    free(h1);
  }
  else if (type == sox_plot_octave) {
    printf("%% GNU Octave file (may also work with MATLAB(R) )\nb=[");
    for (i = 0; i < num_points; ++i)
      printf("%24.16e\n", h[i]);
    printf("];\n"
        "[h,w]=freqz(b,1,%i);\n"
        "plot(%g*w/pi,20*log10(h))\n"
        "title('%s')\n"
        "xlabel('Frequency (Hz)')\n"
        "ylabel('Amplitude Response (dB)')\n"
        "grid on\n"
        "axis([0 %g %g %g])\n"
        "disp('Hit return to continue')\n"
        "pause\n"
        , N, rate * .5, title, rate * .5, y1, y2);
  }
  else if (type == sox_plot_data) {
    printf("# %s\n"
        "# FIR filter\n"
        "# rate: %g\n"
        "# name: b\n"
        "# type: matrix\n"
        "# rows: %i\n"
        "# columns: 1\n", title, rate, num_points);
    for (i = 0; i < num_points; ++i)
      printf("%24.16e\n", h[i]);
  }
}

#if HAVE_FENV_H
  #include <fenv.h>
  #if defined FE_INVALID
    #if HAVE_LRINT && LONG_MAX == 2147483647
      #define lrint32 lrint
    #elif defined __GNUC__ && defined __x86_64__
      #define lrint32 lrint32
      static __inline sox_int32_t lrint32(double input) {
        sox_int32_t result;
        __asm__ __volatile__("fistpl %0": "=m"(result): "t"(input): "st");
        return result;
      }
    #endif
  #endif
#endif

#if defined lrint32
#define _ dest[i] = lrint32(src[i]), ++i,
#pragma STDC FENV_ACCESS ON

static void rint_clip(sox_sample_t * const dest, double const * const src,
    size_t i, size_t const n, sox_uint64_t * const clips)
{
  for (; i < n; ++i) {
    dest[i] = lrint32(src[i]);
    if (fetestexcept(FE_INVALID)) {
      feclearexcept(FE_INVALID);
      dest[i] = src[i] > 0? SOX_SAMPLE_MAX : SOX_SAMPLE_MIN;
      ++*clips;
    }
  }
}

void lsx_save_samples(sox_sample_t * const dest, double const * const src,
    size_t const n, sox_uint64_t * const clips)
{
  size_t i;
  feclearexcept(FE_INVALID);
  for (i = 0; i < (n & ~7);) {
    _ _ _ _ _ _ _ _ 0;
    if (fetestexcept(FE_INVALID)) {
      feclearexcept(FE_INVALID);
      rint_clip(dest, src, i - 8, i, clips);
    }
  }
  rint_clip(dest, src, i, n, clips);
}

void lsx_load_samples(double * const dest, sox_sample_t const * const src,
    size_t const n)
{
  size_t i;
  for (i = 0; i < n; ++i)
    dest[i] = src[i];
}

#pragma STDC FENV_ACCESS OFF
#undef _
#else

void lsx_save_samples(sox_sample_t * const dest, double const * const src,
    size_t const n, sox_uint64_t * const clips)
{
  SOX_SAMPLE_LOCALS;
  size_t i;
  for (i = 0; i < n; ++i)
    dest[i] = SOX_FLOAT_64BIT_TO_SAMPLE(src[i], *clips);
}

void lsx_load_samples(double * const dest, sox_sample_t const * const src,
    size_t const n)
{
  size_t i;
  for (i = 0; i < n; ++i)
    dest[i] = SOX_SAMPLE_TO_FLOAT_64BIT(src[i],);
}

#endif
