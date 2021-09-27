/* libSoX Compander Transfer Function  (c) 2007 robs@users.sourceforge.net
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

#include <math.h>

typedef struct {
  struct sox_compandt_segment {
    double x, y;              /* 1st point in segment */
    double a, b;              /* Quadratic coeffecients for rest of segment */
  } * segments;
  double in_min_lin;
  double out_min_lin;
  double outgain_dB;        /* Post processor gain */
  double curve_dB;
} sox_compandt_t;

sox_bool lsx_compandt_parse(sox_compandt_t * t, char * points, char * gain);
sox_bool lsx_compandt_show(sox_compandt_t * t, sox_plot_t plot);
void    lsx_compandt_kill(sox_compandt_t * p);

/* Place in header to allow in-lining */
static double lsx_compandt(sox_compandt_t * t, double in_lin)
{
  struct sox_compandt_segment * s;
  double in_log, out_log;

  if (in_lin <= t->in_min_lin)
    return t->out_min_lin;

  in_log = log(in_lin);

  for (s = t->segments + 1; in_log > s[1].x; ++s);

  in_log -= s->x;
  out_log = s->y + in_log * (s->a * in_log + s->b);

  return exp(out_log);
}
