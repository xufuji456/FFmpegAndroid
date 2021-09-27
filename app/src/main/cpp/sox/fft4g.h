/* This library is free software; you can redistribute it and/or modify it
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

#define FFT4G_MAX_SIZE 262144

void lsx_cdft(int, int, double *, int *, double *);
void lsx_rdft(int, int, double *, int *, double *);
void lsx_ddct(int, int, double *, int *, double *);
void lsx_ddst(int, int, double *, int *, double *);
void lsx_dfct(int, double *, double *, int *, double *);
void lsx_dfst(int, double *, double *, int *, double *);

void lsx_cdft_f(int, int, float *, int *, float *);
void lsx_rdft_f(int, int, float *, int *, float *);
void lsx_ddct_f(int, int, float *, int *, float *);
void lsx_ddst_f(int, int, float *, int *, float *);
void lsx_dfct_f(int, float *, float *, int *, float *);
void lsx_dfst_f(int, float *, float *, int *, float *);

#define dft_br_len(l) (2 + (1 << (int)(log(l / 2 + .5) / log(2.)) / 2))
#define dft_sc_len(l) (l / 2)

/* Over-allocate h by 2 to use these macros */
#define LSX_PACK(h, n)   h[1] = h[n]
#define LSX_UNPACK(h, n) h[n] = h[1], h[n + 1] = h[1] = 0;
