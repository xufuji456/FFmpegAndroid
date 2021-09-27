/* libSoX Biquad filter common functions   (c) 2006-7 robs@users.sourceforge.net
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

#include "biquad.h"
#include <string.h>

typedef biquad_t priv_t;

static char const * const width_str[] = {
  "band-width(Hz)",
  "band-width(kHz)",
  "band-width(Hz, no warp)", /* deprecated */
  "band-width(octaves)",
  "Q",
  "slope",
};
static char const all_width_types[] = "hkboqs";


int lsx_biquad_getopts(sox_effect_t * effp, int argc, char **argv,
    int min_args, int max_args, int fc_pos, int width_pos, int gain_pos,
    char const * allowed_width_types, filter_t filter_type)
{
  priv_t * p = (priv_t *)effp->priv;
  char width_type = *allowed_width_types;
  char dummy, * dummy_p;     /* To check for extraneous chars. */
  --argc, ++argv;

  p->filter_type = filter_type;
  if (argc < min_args || argc > max_args ||
      (argc > fc_pos    && ((p->fc = lsx_parse_frequency(argv[fc_pos], &dummy_p)) <= 0 || *dummy_p)) ||
      (argc > width_pos && ((unsigned)(sscanf(argv[width_pos], "%lf%c %c", &p->width, &width_type, &dummy)-1) > 1 || p->width <= 0)) ||
      (argc > gain_pos  && sscanf(argv[gain_pos], "%lf %c", &p->gain, &dummy) != 1) ||
      !strchr(allowed_width_types, width_type) || (width_type == 's' && p->width > 1))
    return lsx_usage(effp);
  p->width_type = strchr(all_width_types, width_type) - all_width_types;
  if ((size_t)p->width_type >= strlen(all_width_types))
    p->width_type = 0;
  if (p->width_type == width_bw_kHz) {
    p->width *= 1000;
    p->width_type = width_bw_Hz;
  }
  return SOX_SUCCESS;
}


static int start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;
  /* Simplify: */
  p->b2 /= p->a0;
  p->b1 /= p->a0;
  p->b0 /= p->a0;
  p->a2 /= p->a0;
  p->a1 /= p->a0;

  p->o2 = p->o1 = p->i2 = p->i1 = 0;
  return SOX_SUCCESS;
}


int lsx_biquad_start(sox_effect_t * effp)
{
  priv_t * p = (priv_t *)effp->priv;

  start(effp);

  if (effp->global_info->plot == sox_plot_octave) {
    printf(
      "%% GNU Octave file (may also work with MATLAB(R) )\n"
      "Fs=%g;minF=10;maxF=Fs/2;\n"
      "sweepF=logspace(log10(minF),log10(maxF),200);\n"
      "[h,w]=freqz([%.15e %.15e %.15e],[1 %.15e %.15e],sweepF,Fs);\n"
      "semilogx(w,20*log10(h))\n"
      "title('SoX effect: %s gain=%g frequency=%g %s=%g (rate=%g)')\n"
      "xlabel('Frequency (Hz)')\n"
      "ylabel('Amplitude Response (dB)')\n"
      "axis([minF maxF -35 25])\n"
      "grid on\n"
      "disp('Hit return to continue')\n"
      "pause\n"
      , effp->in_signal.rate, p->b0, p->b1, p->b2, p->a1, p->a2
      , effp->handler.name, p->gain, p->fc, width_str[p->width_type], p->width
      , effp->in_signal.rate);
    return SOX_EOF;
  }
  if (effp->global_info->plot == sox_plot_gnuplot) {
    printf(
      "# gnuplot file\n"
      "set title 'SoX effect: %s gain=%g frequency=%g %s=%g (rate=%g)'\n"
      "set xlabel 'Frequency (Hz)'\n"
      "set ylabel 'Amplitude Response (dB)'\n"
      "Fs=%g\n"
      "b0=%.15e; b1=%.15e; b2=%.15e; a1=%.15e; a2=%.15e\n"
      "o=2*pi/Fs\n"
      "H(f)=sqrt((b0*b0+b1*b1+b2*b2+2.*(b0*b1+b1*b2)*cos(f*o)+2.*(b0*b2)*cos(2.*f*o))/(1.+a1*a1+a2*a2+2.*(a1+a1*a2)*cos(f*o)+2.*a2*cos(2.*f*o)))\n"
      "set logscale x\n"
      "set samples 250\n"
      "set grid xtics ytics\n"
      "set key off\n"
      "plot [f=10:Fs/2] [-35:25] 20*log10(H(f))\n"
      "pause -1 'Hit return to continue'\n"
      , effp->handler.name, p->gain, p->fc, width_str[p->width_type], p->width
      , effp->in_signal.rate, effp->in_signal.rate
      , p->b0, p->b1, p->b2, p->a1, p->a2);
    return SOX_EOF;
  }
  if (effp->global_info->plot == sox_plot_data) {
    printf("# SoX effect: %s gain=%g frequency=%g %s=%g (rate=%g)\n"
      "# IIR filter\n"
      "# rate: %g\n"
      "# name: b\n"
      "# type: matrix\n"
      "# rows: 3\n"
      "# columns: 1\n"
      "%24.16e\n%24.16e\n%24.16e\n"
      "# name: a\n"
      "# type: matrix\n"
      "# rows: 3\n"
      "# columns: 1\n"
      "%24.16e\n%24.16e\n%24.16e\n"
      , effp->handler.name, p->gain, p->fc, width_str[p->width_type], p->width
      , effp->in_signal.rate, effp->in_signal.rate
      , p->b0, p->b1, p->b2, 1. /* a0 */, p->a1, p->a2);
    return SOX_EOF;
  }
  return SOX_SUCCESS;
}


int lsx_biquad_flow(sox_effect_t * effp, const sox_sample_t *ibuf,
    sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
  priv_t * p = (priv_t *)effp->priv;
  size_t len = *isamp = *osamp = min(*isamp, *osamp);
  while (len--) {
    double o0 = *ibuf*p->b0 + p->i1*p->b1 + p->i2*p->b2 - p->o1*p->a1 - p->o2*p->a2;
    p->i2 = p->i1, p->i1 = *ibuf++;
    p->o2 = p->o1, p->o1 = o0;
    *obuf++ = SOX_ROUND_CLIP_COUNT(o0, effp->clips);
  }
  return SOX_SUCCESS;
}

static int create(sox_effect_t * effp, int argc, char * * argv)
{
  priv_t             * p = (priv_t *)effp->priv;
  double             * d = &p->b0;
  char               c;

  --argc, ++argv;
  if (argc == 6)
    for (; argc && sscanf(*argv, "%lf%c", d, &c) == 1; --argc, ++argv, ++d);
  return argc? lsx_usage(effp) : SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_biquad_effect_fn(void)
{
  static sox_effect_handler_t handler = {
    "biquad", "b0 b1 b2 a0 a1 a2", 0,
    create, lsx_biquad_start, lsx_biquad_flow, NULL, NULL, NULL, sizeof(priv_t)
  };
  return &handler;
}
