/* adpcm.h  codex functions for MS_ADPCM data
 *          (hopefully) provides interoperability with
 *          Microsoft's ADPCM format, but, as usual,
 *          see LACK-OF-WARRANTY information below.
 *
 *      Copyright (C) 1999 Stanley J. Brooks <stabro@megsinet.net>
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
 */
#include "sox.h"

#ifndef SAMPL
#define SAMPL short
#endif

/* default coef sets */
extern const short lsx_ms_adpcm_i_coef[7][2];

extern void *lsx_ms_adpcm_alloc(unsigned chans);

/* lsx_ms_adpcm_block_expand_i() outputs interleaved samples into one output buffer */
extern const char *lsx_ms_adpcm_block_expand_i(
	void *priv,
	unsigned chans,          /* total channels             */
	int nCoef,
	const short *coef,
	const unsigned char *ibuff,/* input buffer[blockAlign]   */
	SAMPL *obuff,       /* output samples, n*chans    */
	int n               /* samples to decode PER channel, REQUIRE n % 8 == 1  */
);

extern void lsx_ms_adpcm_block_mash_i(
	unsigned chans,          /* total channels */
	const SAMPL *ip,    /* ip[n*chans] is interleaved input samples */
	int n,              /* samples to encode PER channel, REQUIRE */
	int *st,            /* input/output steps, 16<=st[i] */
	unsigned char *obuff,      /* output buffer[blockAlign] */
	int blockAlign      /* >= 7*chans + n/2          */
);

/* Some helper functions for computing samples/block and blockalign */

/*
 * lsx_ms_adpcm_samples_in(dataLen, chans, blockAlign, samplesPerBlock)
 *  returns the number of samples/channel which would be
 *  in the dataLen, given the other parameters ...
 *  if input samplesPerBlock is 0, then returns the max
 *  samplesPerBlock which would go into a block of size blockAlign
 *  Yes, it is confusing usage.
 */
extern size_t lsx_ms_adpcm_samples_in(
	size_t dataLen,
	size_t chans,
	size_t blockAlign,
	size_t samplesPerBlock
);

/*
 * size_t lsx_ms_adpcm_bytes_per_block(chans, samplesPerBlock)
 *   return minimum blocksize which would be required
 *   to encode number of chans with given samplesPerBlock
 */
extern size_t lsx_ms_adpcm_bytes_per_block(
	size_t chans,
	size_t samplesPerBlock
);
