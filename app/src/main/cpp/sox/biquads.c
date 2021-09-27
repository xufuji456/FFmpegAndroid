/* libSoX Biquad filter effects   (c) 2006-8 robs@users.sourceforge.net
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
 *
 *
 * 2-pole filters designed by Robert Bristow-Johnson <rbj@audioimagination.com>
 *   see https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
 *
 * 1-pole filters based on code (c) 2000 Chris Bagwell <cbagwell@sprynet.com>
 *   Algorithms: Recursive single pole low/high pass filter
 *   Reference: The Scientist and Engineer's Guide to Digital Signal Processing
 *
 *   low-pass: output[N] = input[N] * A + output[N-1] * B
 *     X = exp(-2.0 * pi * Fc)
 *     A = 1 - X
 *     B = X
 *     Fc = cutoff freq / sample rate
 *
 *     Mimics an RC low-pass filter:
 *
 *     ---/\/\/\/\----------->
 *                   |
 *                  --- C
 *                  ---
 *                   |
 *                   |
 *                   V
 *
 *   high-pass: output[N] = A0 * input[N] + A1 * input[N-1] + B1 * output[N-1]
 *     X  = exp(-2.0 * pi * Fc)
 *     A0 = (1 + X) / 2
 *     A1 = -(1 + X) / 2
 *     B1 = X
 *     Fc = cutoff freq / sample rate
 *
 *     Mimics an RC high-pass filter:
 *
 *         || C
 *     ----||--------->
 *         ||    |
 *               <
 *               > R
 *               <
 *               |
 *               V
 */


#include "biquad.h"
#include <assert.h>
#include <string.h>

typedef biquad_t priv_t;


static int hilo1_getopts(sox_effect_t * effp, int argc, char **argv) {
  return lsx_biquad_getopts(effp, argc, argv, 1, 1, 0, 1, 2, "",
      *effp->handler.name == 'l'? filter_LPF_1 : filter_HPF_1);
}


static int hilo2_getopts(sox_effect_t * effp, int argc, char **argv) {
  priv_t * p = (priv_t *)effp->priv;
  if (argc > 1 && strcmp(argv[1], "-1") == 0)
    return hilo1_getopts(effp, argc - 1, argv + 1);
  if (argc > 1 && strcmp(argv[1], "-2") == 0)
    ++argv, --argc;
  p->width = sqrt(0.5); /* Default to Butterworth */
  return lsx_biquad_getopts(effp, argc, argv, 1, 2, 0, 1, 2, "qohk",
      *effp->handler.name == 'l'? filter_LPF : filter_HPF);
}


static int bandpass_getopts(sox_effect_t * effp, int argc, char **argv) {
  filter_t type = filter_BPF;
  if (argc > 1 && strcmp(argv[1], "-c") == 0)
    ++argv, --argc, type = filter_BPF_CSG;
  return lsx_biquad_getopts(effp, argc, argv, 2, 2, 0, 1, 2, "hkqob", type);
}


static int bandrej_getopts(sox_effect_t * effp, int argc, char **argv) {
  return lsx_biquad_getopts(effp, argc, argv, 2, 2, 0, 1, 2, "hkqob", filter_notch);
}


static int allpass_getopts(sox_effect_t * effp, int argc, char **argv) {
  filter_t type = filter_APF;
  int m;
  if (argc > 1 && strcmp(argv[1], "-1") == 0)
    ++argv, --argc, type = filter_AP1;
  else if (argc > 1 && strcmp(argv[1], "-2") == 0)
    ++argv, --argc, type = filter_AP2;
  m = 1 + (type == filter_APF);
  return lsx_biquad_getopts(effp, argc, argv, m, m, 0, 1, 2, "hkqo", type);
}


static int tone_getopts(sox_effect_t * effp, int argc, char **argv) {
  priv_t * p = (priv_t *)effp->priv;
  p->width = 0.5;
  p->fc = *effp->handler.name == 'b'? 100 : 3000;
  return lsx_biquad_getopts(effp, argc, argv, 1, 3, 1, 2, 0, "shkqo",
      *effp->handler.name == 'b'?  filter_lowShelf: filter_highShelf);
}


static int equalizer_getopts(sox_effect_t * effp, int argc, char **argv) {
  return lsx_biquad_getopts(effp, argc, argv, 3, 3, 0, 1, 2, "qohk", filter_peakingEQ);
}


static int band_getopts(sox_effect_t * effp, int argc, char **argv) {
  filter_t type = filter_BPF_SPK;
  if (argc > 1 && strcmp(argv[1], "-n") == 0)
    ++argv, --argc, type = filter_BPF_SPK_N;
  return lsx_biquad_getopts(effp, argc, argv, 1, 2, 0, 1, 2, "hkqo", type);
}


static int deemph_getopts(sox_effect_t * effp, int argc, char **argv) {
  return lsx_biquad_getopts(effp, argc, argv, 0, 0, 0, 1, 2, "s", filter_deemph);
}


static int riaa_getopts(sox_effect_t * effp, int argc, char **argv) {
  priv_t * p = (priv_t *)effp->priv;
  p->filter_type = filter_riaa;
  (void)argv;
  return --argc? lsx_usage(effp) : SOX_SUCCESS;
}


static void make_poly_from_roots(
    double const * roots, size_t num_roots, double * poly)
{
  size_t i, j;
  poly[0] = 1;
  poly[1] = -roots[0];
  memset(poly + 2, 0, (num_roots + 1 - 2) * sizeof(*poly));
  for (i = 1; i < num_roots; ++i)
    for (j = num_roots; j > 0; --j)
      poly[j] -= poly[j - 1] * roots[i];
}

static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  double w0, A, alpha, mult;

  if (p->filter_type == filter_deemph) { /* See deemph.plt for documentation */
    if (effp->in_signal.rate == 44100) {
      p->fc    = 5283;
      p->width = 0.4845;
      p->gain  = -9.477;
    }
    else if (effp->in_signal.rate == 48000) {
      p->fc    = 5356;
      p->width = 0.479;
      p->gain  = -9.62;
    }
    else {
      lsx_fail("sample rate must be 44100 (audio-CD) or 48000 (DAT)");
      return SOX_EOF;
    }
  }

  w0 = 2 * M_PI * p->fc / effp->in_signal.rate;
  A  = exp(p->gain / 40 * log(10.));
  alpha = 0, mult = dB_to_linear(max(p->gain, 0));

  if (w0 > M_PI) {
    lsx_fail("frequency must be less than half the sample-rate (Nyquist rate)");
    return SOX_EOF;
  }

  /* Set defaults: */
  p->b0 = p->b1 = p->b2 = p->a1 = p->a2 = 0;
  p->a0 = 1;

  if (p->width) switch (p->width_type) {
    case width_slope:
      alpha = sin(w0)/2 * sqrt((A + 1/A)*(1/p->width - 1) + 2);
      break;

    case width_Q:
      alpha = sin(w0)/(2*p->width);
      break;

    case width_bw_oct:
      alpha = sin(w0)*sinh(log(2.)/2 * p->width * w0/sin(w0));
      break;

    case width_bw_Hz:
      alpha = sin(w0)/(2*p->fc/p->width);
      break;

    case width_bw_kHz: assert(0); /* Shouldn't get here */

    case width_bw_old:
      alpha = tan(M_PI * p->width / effp->in_signal.rate);
      break;
  }
  switch (p->filter_type) {
    case filter_LPF: /* H(s) = 1 / (s^2 + s/Q + 1) */
      p->b0 =  (1 - cos(w0))/2;
      p->b1 =   1 - cos(w0);
      p->b2 =  (1 - cos(w0))/2;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_HPF: /* H(s) = s^2 / (s^2 + s/Q + 1) */
      p->b0 =  (1 + cos(w0))/2;
      p->b1 = -(1 + cos(w0));
      p->b2 =  (1 + cos(w0))/2;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_BPF_CSG: /* H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q) */
      p->b0 =   sin(w0)/2;
      p->b1 =   0;
      p->b2 =  -sin(w0)/2;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_BPF: /* H(s) = (s/Q) / (s^2 + s/Q + 1)      (constant 0 dB peak gain) */
      p->b0 =   alpha;
      p->b1 =   0;
      p->b2 =  -alpha;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_notch: /* H(s) = (s^2 + 1) / (s^2 + s/Q + 1) */
      p->b0 =   1;
      p->b1 =  -2*cos(w0);
      p->b2 =   1;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_APF: /* H(s) = (s^2 - s/Q + 1) / (s^2 + s/Q + 1) */
      p->b0 =   1 - alpha;
      p->b1 =  -2*cos(w0);
      p->b2 =   1 + alpha;
      p->a0 =   1 + alpha;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha;
      break;

    case filter_peakingEQ: /* H(s) = (s^2 + s*(A/Q) + 1) / (s^2 + s/(A*Q) + 1) */
      if (A == 1)
        return SOX_EFF_NULL;
      p->b0 =   1 + alpha*A;
      p->b1 =  -2*cos(w0);
      p->b2 =   1 - alpha*A;
      p->a0 =   1 + alpha/A;
      p->a1 =  -2*cos(w0);
      p->a2 =   1 - alpha/A;
      break;

    case filter_lowShelf: /* H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1) */
      if (A == 1)
        return SOX_EFF_NULL;
      p->b0 =    A*( (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha );
      p->b1 =  2*A*( (A-1) - (A+1)*cos(w0)                   );
      p->b2 =    A*( (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha );
      p->a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha;
      p->a1 =   -2*( (A-1) + (A+1)*cos(w0)                   );
      p->a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha;
      break;

    case filter_deemph: /* Falls through to high-shelf... */

    case filter_highShelf: /* H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A) */
      if (!A)
        return SOX_EFF_NULL;
      p->b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha );
      p->b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   );
      p->b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha );
      p->a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
      p->a1 =    2*( (A-1) - (A+1)*cos(w0)                   );
      p->a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
      break;

    case filter_LPF_1: /* single-pole */
      p->a1 = -exp(-w0);
      p->b0 = 1 + p->a1;
      break;

    case filter_HPF_1: /* single-pole */
      p->a1 = -exp(-w0);
      p->b0 = (1 - p->a1)/2;
      p->b1 = -p->b0;
      break;

    case filter_BPF_SPK: case filter_BPF_SPK_N: {
      double bw_Hz;
      if (!p->width)
        p->width = p->fc / 2;
      bw_Hz = p->width_type == width_Q?  p->fc / p->width :
        p->width_type == width_bw_Hz? p->width :
        p->fc * (pow(2., p->width) - 1) * pow(2., -0.5 * p->width); /* bw_oct */
      #include "band.h" /* Has different licence */
      break;
    }

    case filter_AP1:     /* Experimental 1-pole all-pass from Tom Erbe @ UCSD */
      p->b0 = exp(-w0);
      p->b1 = -1;
      p->a1 = -exp(-w0);
      break;

    case filter_AP2:     /* Experimental 2-pole all-pass from Tom Erbe @ UCSD */
      p->b0 = 1 - sin(w0);
      p->b1 = -2 * cos(w0);
      p->b2 = 1 + sin(w0);
      p->a0 = 1 + sin(w0);
      p->a1 = -2 * cos(w0);
      p->a2 = 1 - sin(w0);
      break;

    case filter_riaa: /* http://www.dsprelated.com/showmessage/73300/3.php */
      if (effp->in_signal.rate == 44100) {
        static const double zeros[] = {-0.2014898, 0.9233820};
        static const double poles[] = {0.7083149, 0.9924091};
        make_poly_from_roots(zeros, (size_t)2, &p->b0);
        make_poly_from_roots(poles, (size_t)2, &p->a0);
      }
      else if (effp->in_signal.rate == 48000) {
        static const double zeros[] = {-0.1766069, 0.9321590};
        static const double poles[] = {0.7396325, 0.9931330};
        make_poly_from_roots(zeros, (size_t)2, &p->b0);
        make_poly_from_roots(poles, (size_t)2, &p->a0);
      }
      else if (effp->in_signal.rate == 88200) {
        static const double zeros[] = {-0.1168735, 0.9648312};
        static const double poles[] = {0.8590646, 0.9964002};
        make_poly_from_roots(zeros, (size_t)2, &p->b0);
        make_poly_from_roots(poles, (size_t)2, &p->a0);
      }
      else if (effp->in_signal.rate == 96000) {
        static const double zeros[] = {-0.1141486, 0.9676817};
        static const double poles[] = {0.8699137, 0.9966946};
        make_poly_from_roots(zeros, (size_t)2, &p->b0);
        make_poly_from_roots(poles, (size_t)2, &p->a0);
      }
      else if (effp->in_signal.rate == 192000) {
        static const double zeros[] = {-0.1040610965, 0.9837523263};
        static const double poles[] = {0.9328992971, 0.9983633125};
        make_poly_from_roots(zeros, (size_t)2, &p->b0);
        make_poly_from_roots(poles, (size_t)2, &p->a0);
      }
      else {
        lsx_fail("Sample rate must be 44.1k, 48k, 88.2k, 96k, or 192k");
        return SOX_EOF;
      }
      { /* Normalise to 0dB at 1kHz (Thanks to Glenn Davis) */
        double y = 2 * M_PI * 1000 / effp->in_signal.rate;
        double b_re = p->b0 + p->b1 * cos(-y) + p->b2 * cos(-2 * y);
        double a_re = p->a0 + p->a1 * cos(-y) + p->a2 * cos(-2 * y);
        double b_im = p->b1 * sin(-y) + p->b2 * sin(-2 * y);
        double a_im = p->a1 * sin(-y) + p->a2 * sin(-2 * y);
        double g = 1 / sqrt((sqr(b_re) + sqr(b_im)) / (sqr(a_re) + sqr(a_im)));
        p->b0 *= g; p->b1 *= g; p->b2 *= g;
      }
      mult = (p->b0 + p->b1 + p->b2) / (p->a0 + p->a1 + p->a2);
      lsx_debug("gain=%f", linear_to_dB(mult));
      break;
  }
  if (effp->in_signal.mult)
    *effp->in_signal.mult /= mult;
  return lsx_biquad_start(effp);
}


#define BIQUAD_EFFECT(name,group,usage,flags) \
sox_effect_handler_t const * lsx_##name##_effect_fn(void) { \
  static sox_effect_handler_t handler = { \
    #name, usage, flags, \
    group##_getopts, start, lsx_biquad_flow, 0, 0, 0, sizeof(biquad_t)\
  }; \
  return &handler; \
}

BIQUAD_EFFECT(highpass,  hilo2,    "[-1|-2] frequency [width[q|o|h|k](0.707q)]", 0)
BIQUAD_EFFECT(lowpass,   hilo2,    "[-1|-2] frequency [width[q|o|h|k]](0.707q)", 0)
BIQUAD_EFFECT(bandpass,  bandpass, "[-c] frequency width[h|k|q|o]", 0)
BIQUAD_EFFECT(bandreject,bandrej,  "frequency width[h|k|q|o]", 0)
BIQUAD_EFFECT(allpass,   allpass,  "frequency width[h|k|q|o]", 0)
BIQUAD_EFFECT(bass,      tone,     "gain [frequency(100) [width[s|h|k|q|o]](0.5s)]", 0)
BIQUAD_EFFECT(treble,    tone,     "gain [frequency(3000) [width[s|h|k|q|o]](0.5s)]", 0)
BIQUAD_EFFECT(equalizer, equalizer,"frequency width[q|o|h|k] gain", 0)
BIQUAD_EFFECT(band,      band,     "[-n] center [width[h|k|q|o]]", 0)
BIQUAD_EFFECT(deemph,    deemph,   NULL, 0)
BIQUAD_EFFECT(riaa,      riaa,     NULL, 0)
