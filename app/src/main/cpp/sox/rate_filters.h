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

static const sample_t half_fir_coefs_8[] = {
  0.3115465451887802, -0.08734497241282892, 0.03681452335604365,
  -0.01518925831569441, 0.005454118437408876, -0.001564400922162005,
  0.0003181701445034203, -3.48001341225749e-5,
};
#define FUNCTION h8
#define CONVOLVE _ _ _ _ _ _ _ _
#define h8_l 8
#define COEFS half_fir_coefs_8
#include "rate_half_fir.h"

static const sample_t half_fir_coefs_9[] = {
  0.3122703613711853, -0.08922155288172305, 0.03913974805854332,
  -0.01725059723447163, 0.006858970092378141, -0.002304518467568703,
  0.0006096426006051062, -0.0001132393923815236, 1.119795386287666e-5,
};
#define FUNCTION h9
#define CONVOLVE _ _ _ _ _ _ _ _ _
#define h9_l 9
#define COEFS half_fir_coefs_9
#include "rate_half_fir.h"

static const sample_t half_fir_coefs_10[] = {
  0.3128545521327376, -0.09075671986104322, 0.04109637155154835,
  -0.01906629512749895, 0.008184039342054333, -0.0030766775017262,
  0.0009639607022414314, -0.0002358552746579827, 4.025184282444155e-5,
  -3.629779111541012e-6,
};
#define FUNCTION h10
#define CONVOLVE _ _ _ _ _ _ _ _ _ _
#define h10_l 10
#define COEFS half_fir_coefs_10
#include "rate_half_fir.h"

static const sample_t half_fir_coefs_11[] = {
  0.3133358837508807, -0.09203588680609488, 0.04276515428384758,
  -0.02067356614745591, 0.00942253142371517, -0.003856330993895144,
  0.001363470684892284, -0.0003987400965541919, 9.058629923971627e-5,
  -1.428553070915318e-5, 1.183455238783835e-6,
};
#define FUNCTION h11
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _
#define h11_l 11
#define COEFS half_fir_coefs_11
#include "rate_half_fir.h"

static const sample_t half_fir_coefs_12[] = {
  0.3137392991811407, -0.0931182192961332, 0.0442050575271454,
  -0.02210391200618091, 0.01057473015666001, -0.00462766983973885,
  0.001793630226239453, -0.0005961819959665878, 0.0001631475979359577,
  -3.45557865639653e-5, 5.06188341942088e-6, -3.877010943315563e-7,
};
#define FUNCTION h12
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _ _
#define h12_l 12
#define COEFS half_fir_coefs_12
#include "rate_half_fir.h"

static const sample_t half_fir_coefs_13[] = {
  0.3140822554324578, -0.0940458550886253, 0.04545990399121566,
  -0.02338339450796002, 0.01164429409071052, -0.005380686021429845,
  0.002242915773871009, -0.000822047600000082, 0.0002572510962395222,
  -6.607320708956279e-5, 1.309926399120154e-5, -1.790719575255006e-6,
  1.27504961098836e-7,
};
#define FUNCTION h13
#define CONVOLVE _ _ _ _ _ _ _ _ _ _ _ _ _
#define h13_l 13
#define COEFS half_fir_coefs_13
#include "rate_half_fir.h"

static struct {int num_coefs; stage_fn_t fn; float att;} const half_firs[] = {
  { 8, h8 , 136.51},
  { 9, h9 , 152.32},
  {10, h10, 168.07},
  {11, h11, 183.78},
  {12, h12, 199.44},
  {13, h13, 212.75},
};

#define HI_PREC_CLOCK

#define VAR_LENGTH p->n
#define VAR_CONVOLVE while (j < FIR_LENGTH) _
#define VAR_POLY_PHASE_BITS p->phase_bits

#define FUNCTION vpoly0
#define FIR_LENGTH VAR_LENGTH
#define CONVOLVE VAR_CONVOLVE
#include "rate_poly_fir0.h"

#define FUNCTION vpoly1
#define COEF_INTERP 1
#define PHASE_BITS VAR_POLY_PHASE_BITS
#define FIR_LENGTH VAR_LENGTH
#define CONVOLVE VAR_CONVOLVE
#include "rate_poly_fir.h"

#define FUNCTION vpoly2
#define COEF_INTERP 2
#define PHASE_BITS VAR_POLY_PHASE_BITS
#define FIR_LENGTH VAR_LENGTH
#define CONVOLVE VAR_CONVOLVE
#include "rate_poly_fir.h"

#define FUNCTION vpoly3
#define COEF_INTERP 3
#define PHASE_BITS VAR_POLY_PHASE_BITS
#define FIR_LENGTH VAR_LENGTH
#define CONVOLVE VAR_CONVOLVE
#include "rate_poly_fir.h"

#undef HI_PREC_CLOCK

#define U100_l 42
#define poly_fir_convolve_U100 _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
#define FUNCTION U100_0
#define FIR_LENGTH U100_l
#define CONVOLVE poly_fir_convolve_U100
#include "rate_poly_fir0.h"

#define u100_l 11
#define poly_fir_convolve_u100 _ _ _ _ _ _ _ _ _ _ _
#define FUNCTION u100_0
#define FIR_LENGTH u100_l
#define CONVOLVE poly_fir_convolve_u100
#include "rate_poly_fir0.h"

#define FUNCTION u100_1
#define COEF_INTERP 1
#define PHASE_BITS 8
#define FIR_LENGTH u100_l
#define CONVOLVE poly_fir_convolve_u100
#include "rate_poly_fir.h"
#define u100_1_b 8

#define FUNCTION u100_2
#define COEF_INTERP 2
#define PHASE_BITS 6
#define FIR_LENGTH u100_l
#define CONVOLVE poly_fir_convolve_u100
#include "rate_poly_fir.h"
#define u100_2_b 6

typedef struct {float scalar; stage_fn_t fn;} poly_fir1_t;
typedef struct {float beta; poly_fir1_t interp[3];} poly_fir_t;

static poly_fir_t const poly_firs[] = {
  {-1, {{0, vpoly0}, { 7.2, vpoly1}, {5.0, vpoly2}}}, 
  {-1, {{0, vpoly0}, { 9.4, vpoly1}, {6.7, vpoly2}}}, 
  {-1, {{0, vpoly0}, {12.4, vpoly1}, {7.8, vpoly2}}}, 
  {-1, {{0, vpoly0}, {13.6, vpoly1}, {9.3, vpoly2}}}, 
  {-1, {{0, vpoly0}, {10.5, vpoly2}, {8.4, vpoly3}}}, 
  {-1, {{0, vpoly0}, {11.85,vpoly2}, {9.0, vpoly3}}}, 
 
  {-1, {{0, vpoly0}, { 8.0, vpoly1}, {5.3, vpoly2}}}, 
  {-1, {{0, vpoly0}, { 8.6, vpoly1}, {5.7, vpoly2}}}, 
  {-1, {{0, vpoly0}, {10.6, vpoly1}, {6.75,vpoly2}}}, 
  {-1, {{0, vpoly0}, {12.6, vpoly1}, {8.6, vpoly2}}}, 
  {-1, {{0, vpoly0}, { 9.6, vpoly2}, {7.6, vpoly3}}}, 
  {-1, {{0, vpoly0}, {11.4, vpoly2}, {8.65,vpoly3}}}, 
               
  {10.62, {{U100_l, U100_0}, {0, 0}, {0, 0}}}, 
  {11.28, {{u100_l, u100_0}, {u100_1_b, u100_1}, {u100_2_b, u100_2}}}, 
  {-1, {{0, vpoly0}, {   9, vpoly1}, {  6, vpoly2}}}, 
  {-1, {{0, vpoly0}, {  11, vpoly1}, {  7, vpoly2}}}, 
  {-1, {{0, vpoly0}, {  13, vpoly1}, {  8, vpoly2}}}, 
  {-1, {{0, vpoly0}, {  10, vpoly2}, {  8, vpoly3}}}, 
  {-1, {{0, vpoly0}, {  12, vpoly2}, {  9, vpoly3}}}, 
};
