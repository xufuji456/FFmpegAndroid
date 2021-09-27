/* libSoX test code    copyright (c) 2006 robs@users.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef NDEBUG /* Enable assert always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif
#include <assert.h>
#include <math.h>
#include "sox.h"

#define TEST_UINT(bits) \
  uint##bits = 0; \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample == SOX_SAMPLE_MIN); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == 0 && clips == 0); \
 \
  uint##bits = 1; \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample > SOX_SAMPLE_MIN && sample < 0); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == 1 && clips == 0); \
 \
  uint##bits = SOX_INT_MAX(bits); \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample * SOX_INT_MAX(bits) == SOX_UNSIGNED_TO_SAMPLE(bits,1)); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == SOX_INT_MAX(bits) && clips == 0); \
 \
  sample =SOX_UNSIGNED_TO_SAMPLE(bits,1)+SOX_UNSIGNED_TO_SAMPLE(bits,SOX_INT_MAX(bits))/2; \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == 1 && clips == 0); \
 \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,1)+SOX_UNSIGNED_TO_SAMPLE(bits,SOX_INT_MAX(bits))/2-1; \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == 0 && clips == 0); \
 \
  uint##bits = (0^SOX_INT_MIN(bits)); \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample == 0); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == (0^SOX_INT_MIN(bits)) && clips == 0); \
 \
  uint##bits = ((0^SOX_INT_MIN(bits))+1); \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample > 0 && sample < SOX_SAMPLE_MAX); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == ((0^SOX_INT_MIN(bits))+1) && clips == 0); \
 \
  uint##bits = SOX_UINT_MAX(bits); \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,uint##bits); \
  assert(sample == SOX_INT_MAX(bits) * SOX_UNSIGNED_TO_SAMPLE(bits,((0^SOX_INT_MIN(bits))+1))); \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == SOX_UINT_MAX(bits) && clips == 0); \
 \
  sample =SOX_UNSIGNED_TO_SAMPLE(bits,SOX_UINT_MAX(bits))+SOX_UNSIGNED_TO_SAMPLE(bits,((0^SOX_INT_MIN(bits))+1))/2-1; \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == SOX_UINT_MAX(bits) && clips == 0); \
 \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,SOX_UINT_MAX(bits))+SOX_UNSIGNED_TO_SAMPLE(bits,((0^SOX_INT_MIN(bits))+1))/2; \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == SOX_UINT_MAX(bits) && --clips == 0); \
 \
  sample = SOX_SAMPLE_MAX; \
  uint##bits = SOX_SAMPLE_TO_UNSIGNED(bits,sample, clips); \
  assert(uint##bits == SOX_UINT_MAX(bits) && --clips == 0); \

#define TEST_SINT(bits) \
  int##bits = SOX_INT_MIN(bits); \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample == SOX_SAMPLE_MIN); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = SOX_INT_MIN(bits)+1; \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample > SOX_SAMPLE_MIN && sample < 0); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = SOX_UINT_MAX(bits) /* i.e. -1 */; \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample * SOX_INT_MAX(bits) == SOX_SIGNED_TO_SAMPLE(bits,SOX_INT_MIN(bits)+1)); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = SOX_INT_MIN(bits)+1; \
  sample =SOX_UNSIGNED_TO_SAMPLE(bits,1)+SOX_UNSIGNED_TO_SAMPLE(bits,SOX_INT_MAX(bits))/2; \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = SOX_INT_MIN(bits); \
  sample = SOX_UNSIGNED_TO_SAMPLE(bits,1)+SOX_UNSIGNED_TO_SAMPLE(bits,SOX_INT_MAX(bits))/2-1; \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = 0; \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample == 0); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = 1; \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample > 0 && sample < SOX_SAMPLE_MAX); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  int##bits = SOX_INT_MAX(bits); \
  sample = SOX_SIGNED_TO_SAMPLE(bits,int##bits); \
  assert(sample == SOX_INT_MAX(bits) * SOX_SIGNED_TO_SAMPLE(bits,1)); \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  sample =SOX_UNSIGNED_TO_SAMPLE(bits,SOX_UINT_MAX(bits))+SOX_UNSIGNED_TO_SAMPLE(bits,((0^SOX_INT_MIN(bits))+1))/2-1; \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && clips == 0); \
 \
  sample =SOX_UNSIGNED_TO_SAMPLE(bits,SOX_UINT_MAX(bits))+SOX_UNSIGNED_TO_SAMPLE(bits,((0^SOX_INT_MIN(bits))+1))/2; \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && --clips == 0); \
 \
  sample = SOX_SAMPLE_MAX; \
  int##bits##_2 = SOX_SAMPLE_TO_SIGNED(bits,sample, clips); \
  assert(int##bits##_2 == int##bits && --clips == 0);

int main(void)
{
  sox_int8_t int8;
  sox_int16_t int16;
  sox_int24_t int24;

  sox_uint8_t uint8;
  sox_uint16_t uint16;
  sox_uint24_t uint24;

  sox_int8_t int8_2;
  sox_int16_t int16_2;
  sox_int24_t int24_2;

  sox_sample_t sample;
  size_t clips = 0;

  double d;
  SOX_SAMPLE_LOCALS;

  TEST_UINT(8)
  TEST_UINT(16)
  TEST_UINT(24)

  TEST_SINT(8)
  TEST_SINT(16)
  TEST_SINT(24)

  d = -1.000000001;
  sample = SOX_FLOAT_64BIT_TO_SAMPLE(d, clips);
  assert(sample == SOX_SAMPLE_MIN && --clips == 0);

  d = -1;
  sample = SOX_FLOAT_64BIT_TO_SAMPLE(d, clips);
  assert(sample == SOX_SAMPLE_MIN && clips == 0);
  d = SOX_SAMPLE_TO_FLOAT_64BIT(sample,clips);
  assert(d == -1 && clips == 0);

  d = 1;
  sample = SOX_FLOAT_64BIT_TO_SAMPLE(d, clips);
  assert(sample == SOX_SAMPLE_MAX && clips == 0);
  d = SOX_SAMPLE_TO_FLOAT_64BIT(sample,clips);
  assert(fabs(d - 1) < 1e-9 && clips == 0);

  d = 1.0000000001;
  sample = SOX_FLOAT_64BIT_TO_SAMPLE(d, clips);
  assert(sample == SOX_SAMPLE_MAX && --clips == 0);

  return 0;
}
