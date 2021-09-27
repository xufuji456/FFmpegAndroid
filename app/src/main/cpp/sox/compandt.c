/* libSoX Compander Transfer Function: (c) 2007 robs@users.sourceforge.net
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

#include "sox_i.h"

#include "compandt.h"
#include <string.h>

#define LOG_TO_LOG10(x) ((x) * 20 / M_LN10)

sox_bool lsx_compandt_show(sox_compandt_t * t, sox_plot_t plot)
{
  int i;

  for (i = 1; t->segments[i-1].x; ++i)
    lsx_debug("TF: %g %g %g %g",
       LOG_TO_LOG10(t->segments[i].x),
       LOG_TO_LOG10(t->segments[i].y),
       LOG_TO_LOG10(t->segments[i].a),
       LOG_TO_LOG10(t->segments[i].b));

  if (plot == sox_plot_octave) {
    printf(
      "%% GNU Octave file (may also work with MATLAB(R) )\n"
      "in=linspace(-99.5,0,200);\n"
      "out=[");
    for (i = -199; i <= 0; ++i) {
      double in = i/2.;
      double in_lin = pow(10., in/20);
      printf("%g ", in + 20 * log10(lsx_compandt(t, in_lin)));
    }
    printf(
      "];\n"
      "plot(in,out)\n"
      "title('SoX effect: compand')\n"
      "xlabel('Input level (dB)')\n"
      "ylabel('Output level (dB)')\n"
      "grid on\n"
      "disp('Hit return to continue')\n"
      "pause\n");
    return sox_false;
  }
  if (plot == sox_plot_gnuplot) {
    printf(
      "# gnuplot file\n"
      "set title 'SoX effect: compand'\n"
      "set xlabel 'Input level (dB)'\n"
      "set ylabel 'Output level (dB)'\n"
      "set grid xtics ytics\n"
      "set key off\n"
      "plot '-' with lines\n");
    for (i = -199; i <= 0; ++i) {
      double in = i/2.;
      double in_lin = pow(10., in/20);
      printf("%g %g\n", in, in + 20 * log10(lsx_compandt(t, in_lin)));
    }
    printf(
      "e\n"
      "pause -1 'Hit return to continue'\n");
    return sox_false;
  }
  return sox_true;
}

static void prepare_transfer_fn(sox_compandt_t * t)
{
  int i;
  double radius = t->curve_dB * M_LN10 / 20;

  for (i = 0; !i || t->segments[i-2].x; i += 2) {
    t->segments[i].y += t->outgain_dB;
    t->segments[i].x *= M_LN10 / 20; /* Convert to natural logs */
    t->segments[i].y *= M_LN10 / 20;
  }

#define line1 t->segments[i - 4]
#define curve t->segments[i - 3]
#define line2 t->segments[i - 2]
#define line3 t->segments[i - 0]
  for (i = 4; t->segments[i - 2].x; i += 2) {
    double x, y, cx, cy, in1, in2, out1, out2, theta, len, r;

    line1.a = 0;
    line1.b = (line2.y - line1.y) / (line2.x - line1.x);

    line2.a = 0;
    line2.b = (line3.y - line2.y) / (line3.x - line2.x);

    theta = atan2(line2.y - line1.y, line2.x - line1.x);
    len = sqrt(pow(line2.x - line1.x, 2.) + pow(line2.y - line1.y, 2.));
    r = min(radius, len);
    curve.x = line2.x - r * cos(theta);
    curve.y = line2.y - r * sin(theta);

    theta = atan2(line3.y - line2.y, line3.x - line2.x);
    len = sqrt(pow(line3.x - line2.x, 2.) + pow(line3.y - line2.y, 2.));
    r = min(radius, len / 2);
    x = line2.x + r * cos(theta);
    y = line2.y + r * sin(theta);

    cx = (curve.x + line2.x + x) / 3;
    cy = (curve.y + line2.y + y) / 3;

    line2.x = x;
    line2.y = y;

    in1 = cx - curve.x;
    out1 = cy - curve.y;
    in2 = line2.x - curve.x;
    out2 = line2.y - curve.y;
    curve.a = (out2/in2 - out1/in1) / (in2-in1);
    curve.b = out1/in1 - curve.a*in1;
  }
#undef line1
#undef curve
#undef line2
#undef line3
  t->segments[i - 3].x = 0;
  t->segments[i - 3].y = t->segments[i - 2].y;

  t->in_min_lin = exp(t->segments[1].x);
  t->out_min_lin= exp(t->segments[1].y);
}

static sox_bool parse_transfer_value(char const * text, double * value)
{
  char dummy;     /* To check for extraneous chars. */

  if (!text) {
    lsx_fail("syntax error trying to read transfer function value");
    return sox_false;
  }
  if (!strcmp(text, "-inf"))
    *value = -20 * log10(-(double)SOX_SAMPLE_MIN);
  else if (sscanf(text, "%lf %c", value, &dummy) != 1) {
    lsx_fail("syntax error trying to read transfer function value");
    return sox_false;
  }
  else if (*value > 0) {
    lsx_fail("transfer function values are relative to maximum volume so can't exceed 0dB");
    return sox_false;
  }
  return sox_true;
}

sox_bool lsx_compandt_parse(sox_compandt_t * t, char * points, char * gain)
{
  char const * text = points;
  unsigned i, j, num, pairs, commas = 0;
  char dummy;     /* To check for extraneous chars. */

  if (sscanf(points, "%lf %c", &t->curve_dB, &dummy) == 2 && dummy == ':')
    points = strchr(points, ':') + 1;
  else t->curve_dB = 0;
  t->curve_dB = max(t->curve_dB, .01);

  while (*text) commas += *text++ == ',';
  pairs = 1 + commas / 2;
  ++pairs;    /* allow room for extra pair at the beginning */
  pairs *= 2; /* allow room for the auto-curves */
  ++pairs;    /* allow room for 0,0 at end */
  t->segments = lsx_calloc(pairs, sizeof(*t->segments));

#define s(n) t->segments[2*((n)+1)]
  for (i = 0, text = strtok(points, ","); text != NULL; ++i) {
    if (!parse_transfer_value(text, &s(i).x))
      return sox_false;
    if (i && s(i-1).x > s(i).x) {
      lsx_fail("transfer function input values must be strictly increasing");
      return sox_false;
    }
    if (i || (commas & 1)) {
      text = strtok(NULL, ",");
      if (!parse_transfer_value(text, &s(i).y))
        return sox_false;
      s(i).y -= s(i).x;
    }
    text = strtok(NULL, ",");
  }
  num = i;

  if (num == 0 || s(num-1).x) /* Add 0,0 if necessary */
    ++num;
#undef s

  if (gain && sscanf(gain, "%lf %c", &t->outgain_dB, &dummy) != 1) {
    lsx_fail("syntax error trying to read post-processing gain value");
    return sox_false;
  }

#define s(n) t->segments[2*(n)]
  s(0).x = s(1).x - 2 * t->curve_dB; /* Add a tail off segment at the start */
  s(0).y = s(1).y;
  ++num;

  for (i = 2; i < num; ++i) { /* Join adjacent colinear segments */
    double g1 = (s(i-1).y - s(i-2).y) * (s(i-0).x - s(i-1).x);
    double g2 = (s(i-0).y - s(i-1).y) * (s(i-1).x - s(i-2).x);
    if (fabs(g1 - g2)) /* fabs stops epsilon problems */
      continue;
    --num;
    for (j = --i; j < num; ++j)
      s(j) = s(j+1);
  }
#undef s

  prepare_transfer_fn(t);
  return sox_true;
}

void lsx_compandt_kill(sox_compandt_t * p)
{
  free(p->segments);
}

