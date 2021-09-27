/* libSoX Biquad filter common definitions (c) 2006-7 robs@users.sourceforge.net
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

#ifndef biquad_included
#define biquad_included

#define LSX_EFF_ALIAS
#include "sox_i.h"

typedef enum {
  filter_LPF,
  filter_HPF,
  filter_BPF_CSG,
  filter_BPF,
  filter_notch,
  filter_APF,
  filter_peakingEQ,
  filter_lowShelf,
  filter_highShelf,
  filter_LPF_1,
  filter_HPF_1,
  filter_BPF_SPK,
  filter_BPF_SPK_N,
  filter_AP1,
  filter_AP2,
  filter_deemph,
  filter_riaa
} filter_t;

typedef enum {
  width_bw_Hz,
  width_bw_kHz,
  /* The old, non-RBJ, non-freq-warped band-pass/reject response;
   * leaving here for now just in case anybody misses it: */
  width_bw_old,
  width_bw_oct,
  width_Q,
  width_slope
} width_t;

/* Private data for the biquad filter effects */
typedef struct {
  double gain;             /* For EQ filters */
  double fc;               /* Centre/corner/cutoff frequency */
  double width;            /* Filter width; interpreted as per width_type */
  width_t width_type;

  filter_t filter_type;

  double b0, b1, b2;       /* Filter coefficients */
  double a0, a1, a2;       /* Filter coefficients */

  sox_sample_t i1, i2;     /* Filter memory */
  double      o1, o2;      /* Filter memory */
} biquad_t;

int lsx_biquad_getopts(sox_effect_t * effp, int n, char **argv,
    int min_args, int max_args, int fc_pos, int width_pos, int gain_pos,
    char const * allowed_width_types, filter_t filter_type);
int lsx_biquad_start(sox_effect_t * effp);
int lsx_biquad_flow(sox_effect_t * effp, const sox_sample_t *ibuf, sox_sample_t *obuf,
                        size_t *isamp, size_t *osamp);

#endif
