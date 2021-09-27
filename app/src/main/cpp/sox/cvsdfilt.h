/*      libSoX CVSD (Continuously Variable Slope Delta modulation)
 *      conversion routines
 *
 *      The CVSD format is described in the MIL Std 188 113, which is
 *      available from http://bbs.itsi.disa.mil:5580/T3564
 *
 *	Copyright (C) 1996
 *      Thomas Sailer (sailer@ife.ee.ethz.ch) (HB9JNX/AE4WA)
 *      Swiss Federal Institute of Technology, Electronics Lab
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

static const float dec_filter_16[48] = {
	       0.001102,       0.001159,       0.000187,      -0.000175,
	       0.002097,       0.006543,       0.009384,       0.008004,
	       0.006562,       0.013569,       0.030745,       0.047053,
	       0.050491,       0.047388,       0.062171,       0.109115,
	       0.167120,       0.197144,       0.195471,       0.222098,
	       0.354745,       0.599184,       0.849632,       0.956536,
	       0.849632,       0.599184,       0.354745,       0.222098,
	       0.195471,       0.197144,       0.167120,       0.109115,
	       0.062171,       0.047388,       0.050491,       0.047053,
	       0.030745,       0.013569,       0.006562,       0.008004,
	       0.009384,       0.006543,       0.002097,      -0.000175,
	       0.000187,       0.001159,       0.001102,       0.000000
};

/* ---------------------------------------------------------------------- */

static const float dec_filter_32[48] = {
	       0.001950,       0.004180,       0.006331,       0.007907,
	       0.008510,       0.008342,       0.008678,       0.011827,
	       0.020282,       0.035231,       0.055200,       0.075849,
	       0.091585,       0.098745,       0.099031,       0.101287,
	       0.120058,       0.170672,       0.262333,       0.392047,
	       0.542347,       0.684488,       0.786557,       0.823702,
	       0.786557,       0.684488,       0.542347,       0.392047,
	       0.262333,       0.170672,       0.120058,       0.101287,
	       0.099031,       0.098745,       0.091585,       0.075849,
	       0.055200,       0.035231,       0.020282,       0.011827,
	       0.008678,       0.008342,       0.008510,       0.007907,
	       0.006331,       0.004180,       0.001950,      -0.000000
};

/* ---------------------------------------------------------------------- */

static const float enc_filter_16_0[16] = {
	      -0.000362,       0.004648,       0.001381,       0.008312,
	       0.041490,      -0.001410,       0.124061,       0.247446,
	      -0.106761,      -0.236326,      -0.023798,      -0.023506,
	      -0.030097,       0.001493,      -0.005363,      -0.001672
};

static const float enc_filter_16_1[16] = {
	       0.001672,       0.005363,      -0.001493,       0.030097,
	       0.023506,       0.023798,       0.236326,       0.106761,
	      -0.247446,      -0.124061,       0.001410,      -0.041490,
	      -0.008312,      -0.001381,      -0.004648,       0.000362
};

static const float *enc_filter_16[2] = {
	enc_filter_16_0, enc_filter_16_1
};

/* ---------------------------------------------------------------------- */

static const float enc_filter_32_0[16] = {
	      -0.000289,       0.002112,       0.001421,       0.002235,
	       0.021003,       0.001237,       0.047132,       0.129636,
	      -0.027328,      -0.126462,      -0.021456,      -0.008069,
	      -0.017959,       0.000301,      -0.002538,      -0.001278
};

static const float enc_filter_32_1[16] = {
	      -0.000010,       0.002787,       0.000055,       0.006813,
	       0.020249,      -0.000995,       0.077912,       0.112870,
	      -0.076980,      -0.106971,      -0.005096,      -0.015449,
	      -0.012591,       0.000813,      -0.003003,      -0.000527
};

static const float enc_filter_32_2[16] = {
	       0.000527,       0.003003,      -0.000813,       0.012591,
	       0.015449,       0.005096,       0.106971,       0.076980,
	      -0.112870,      -0.077912,       0.000995,      -0.020249,
	      -0.006813,      -0.000055,      -0.002787,       0.000010
};

static const float enc_filter_32_3[16] = {
	       0.001278,       0.002538,      -0.000301,       0.017959,
	       0.008069,       0.021456,       0.126462,       0.027328,
	      -0.129636,      -0.047132,      -0.001237,      -0.021003,
	      -0.002235,      -0.001421,      -0.002112,       0.000289
};

static const float *enc_filter_32[4] = {
	enc_filter_32_0, enc_filter_32_1, enc_filter_32_2, enc_filter_32_3
};

/* ---------------------------------------------------------------------- */
