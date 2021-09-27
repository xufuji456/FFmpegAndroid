/* libSoX ima_rw.h -- codex utilities for WAV_FORMAT_IMA_ADPCM
 * Copyright (C) 1999 Stanley J. Brooks <stabro@megsinet.net>
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

#include "sox.h"

#ifndef SAMPL
#define SAMPL short
#endif

/*
 * call lsx_ima_init_table() before any other Ima* functions,
 * to create the fast lookup tables
 */
extern void lsx_ima_init_table(void);

/* lsx_ima_block_expand_i() outputs interleaved samples into one output buffer */
extern void lsx_ima_block_expand_i(
	unsigned chans,          /* total channels             */
	const unsigned char *ibuff,/* input buffer[blockAlign]   */
	SAMPL *obuff,       /* output samples, n*chans    */
	int n               /* samples to decode PER channel, REQUIRE n % 8 == 1  */
);

/* lsx_ima_block_expand_m() outputs non-interleaved samples into chan separate output buffers */
extern void lsx_ima_block_expand_m(
	unsigned chans,          /* total channels             */
	const unsigned char *ibuff,/* input buffer[blockAlign]   */
	SAMPL **obuffs,     /* chan output sample buffers, each takes n samples */
	int n               /* samples to decode PER channel, REQUIRE n % 8 == 1  */
);

/* mash one block.  if you want to use opt>0, 9 is a reasonable value */
extern void lsx_ima_block_mash_i(
	unsigned chans,          /* total channels */
	const SAMPL *ip,    /* ip[] is interleaved input samples */
	int n,              /* samples to encode PER channel, REQUIRE n % 8 == 1 */
	int *st,            /* input/output state[chans], REQUIRE 0 <= st[ch] <= ISSTMAX */
	unsigned char *obuff, /* output buffer[blockAlign] */
	int opt             /* non-zero allows some cpu-intensive code to improve output */
);

/* Some helper functions for computing samples/block and blockalign */

/*
 * lsx_ima_samples_in(dataLen, chans, blockAlign, samplesPerBlock)
 *  returns the number of samples/channel which would go
 *  in the dataLen, given the other parameters ...
 *  if input samplesPerBlock is 0, then returns the max
 *  samplesPerBlock which would go into a block of size blockAlign
 *  Yes, it is confusing usage.
 */
extern size_t lsx_ima_samples_in(
	size_t dataLen,
	size_t chans,
	size_t blockAlign,
	size_t samplesPerBlock
);

/*
 * size_t lsx_ima_bytes_per_block(chans, samplesPerBlock)
 *   return minimum blocksize which would be required
 *   to encode number of chans with given samplesPerBlock
 */
extern size_t lsx_ima_bytes_per_block(
	size_t chans,
	size_t samplesPerBlock
);

