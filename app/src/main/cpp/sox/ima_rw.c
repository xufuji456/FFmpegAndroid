/* libSoX ima_rw.c -- codex utilities for WAV_FORMAT_IMA_ADPCM
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

#include "sox_i.h"
#include "ima_rw.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
/*
 *
 * Lookup tables for IMA ADPCM format
 *
 */
#define ISSTMAX 88

static const int imaStepSizeTable[ISSTMAX + 1] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34,
        37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494,
        544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
        1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
        11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
        27086, 29794, 32767
};

#define imaStateAdjust(c) (((c)<4)? -1:(2*(c)-6))
/* +0 - +3, decrease step size */
/* +4 - +7, increase step size */
/* -0 - -3, decrease step size */
/* -4 - -7, increase step size */

static unsigned char imaStateAdjustTable[ISSTMAX+1][8];

void lsx_ima_init_table(void)
{
        int i,j,k;
        for (i=0; i<=ISSTMAX; i++) {
                for (j=0; j<8; j++) {
                        k = i + imaStateAdjust(j);
                        if (k<0) k=0;
                        else if (k>ISSTMAX) k=ISSTMAX;
                        imaStateAdjustTable[i][j] = k;
                }
        }
}

static void ImaExpandS(
        unsigned ch,             /* channel number to decode, REQUIRE 0 <= ch < chans  */
        unsigned chans,          /* total channels             */
        const unsigned char *ibuff,/* input buffer[blockAlign]   */
        SAMPL *obuff,       /* obuff[n] will be output samples */
        int n,              /* samples to decode PER channel, REQUIRE n % 8 == 1  */
        unsigned o_inc           /* index difference between successive output samples */
)
{
        const unsigned char *ip;
        int i_inc;
        SAMPL *op;
        int i, val, state;

        ip = ibuff + 4*ch;     /* input pointer to 4-byte block state-initializer   */
        i_inc = 4*(chans-1);   /* amount by which to incr ip after each 4-byte read */
        val = (short)(ip[0] + (ip[1]<<8)); /* need cast for sign-extend */
        state = ip[2];
        if (state > ISSTMAX) {
                lsx_warn("IMA_ADPCM block ch%d initial-state (%d) out of range", ch, state);
                state = 0;
        }
        /* specs say to ignore ip[3] , but write it as 0 */
        ip += 4+i_inc;

        op = obuff;
        *op = val;      /* 1st output sample for this channel */
        op += o_inc;

        for (i = 1; i < n; i++) {
                int step,dp,c,cm;

                if (i&1) {         /* 1st of pair */
                        cm = *ip & 0x0f;
                } else {
                        cm = (*ip++)>>4;
                        if ((i&7) == 0)  /* ends the 8-sample input block for this channel */
                                ip += i_inc;   /* skip ip for next group */
                }

                step = imaStepSizeTable[state];
                /* Update the state for the next sample */
                c = cm & 0x07;
                state = imaStateAdjustTable[state][c];

                dp = 0;
                if (c & 4) dp += step;
                step = step >> 1;
                if (c & 2) dp += step;
                step = step >> 1;
                if (c & 1) dp += step;
                step = step >> 1;
                dp += step;

                if (c != cm) {
                        val -= dp;
                        if (val<-0x8000) val = -0x8000;
                } else {
                        val += dp;
                        if (val>0x7fff) val = 0x7fff;
                }
                *op = val;
                op += o_inc;
        }
        return;
}

/* lsx_ima_block_expand_i() outputs interleaved samples into one output buffer */
void lsx_ima_block_expand_i(
        unsigned chans,          /* total channels             */
        const unsigned char *ibuff,/* input buffer[blockAlign]   */
        SAMPL *obuff,       /* output samples, n*chans    */
        int n               /* samples to decode PER channel, REQUIRE n % 8 == 1  */
)
{
        unsigned ch;
        for (ch=0; ch<chans; ch++)
                ImaExpandS(ch, chans, ibuff, obuff+ch, n, chans);
}

/* lsx_ima_block_expand_m() outputs non-interleaved samples into chan separate output buffers */
void lsx_ima_block_expand_m(
        unsigned chans,          /* total channels             */
        const unsigned char *ibuff,/* input buffer[blockAlign]   */
        SAMPL **obuffs,     /* chan output sample buffers, each takes n samples */
        int n               /* samples to decode PER channel, REQUIRE n % 8 == 1  */
)
{
        unsigned ch;
        for (ch=0; ch<chans; ch++)
                ImaExpandS(ch, chans, ibuff, obuffs[ch], n, 1);
}

static int ImaMashS(
        unsigned ch,             /* channel number to encode, REQUIRE 0 <= ch < chans  */
        unsigned chans,          /* total channels */
        int v0,           /* value to use as starting prediction0 */
        const SAMPL *ibuff, /* ibuff[] is interleaved input samples */
        int n,              /* samples to encode PER channel, REQUIRE n % 8 == 1 */
        int *st,            /* input/output state, REQUIRE 0 <= *st <= ISSTMAX */
        unsigned char *obuff /* output buffer[blockAlign], or NULL for no output  */
)
{
        const SAMPL *ip, *itop;
        unsigned char *op;
        int o_inc = 0;      /* set 0 only to shut up gcc's 'might be uninitialized' */
        int i, val;
        int state;
        double d2;  /* long long is okay also, speed abt the same */

        ip = ibuff + ch;       /* point ip to 1st input sample for this channel */
        itop = ibuff + n*chans;
        val = *ip - v0; ip += chans;/* 1st input sample for this channel */
        d2 = val*val;/* d2 will be sum of squares of errors, given input v0 and *st */
        val = v0;

        op = obuff;            /* output pointer (or NULL) */
        if (op) {              /* NULL means don't output, just compute the rms error */
                op += 4*ch;          /* where to put this channel's 4-byte block state-initializer */
                o_inc = 4*(chans-1); /* amount by which to incr op after each 4-byte written */
                *op++ = val; *op++ = val>>8;
                *op++ = *st; *op++ = 0; /* they could have put a mid-block state-correction here  */
                op += o_inc;            /* _sigh_   NEVER waste a byte.      It's a rule!         */
        }

        state = *st;

        for (i = 0; ip < itop; ip+=chans) {
                int step,d,dp,c;

                d = *ip - val;  /* difference between last prediction and current sample */

                step = imaStepSizeTable[state];
                c = (abs(d)<<2)/step;
                if (c > 7) c = 7;
                /* Update the state for the next sample */
                state = imaStateAdjustTable[state][c];

                if (op) {   /* if we want output, put it in proper place */
                        int cm = c;
                        if (d<0) cm |= 8;
                        if (i&1) {       /* odd numbered output */
                                *op++ |= (cm<<4);
                                if (i == 7)    /* ends the 8-sample output block for this channel */
                                        op += o_inc; /* skip op for next group */
                        } else {
                                *op = cm;
                        }
                        i = (i+1) & 0x07;
                }

                dp = 0;
                if (c & 4) dp += step;
                step = step >> 1;
                if (c & 2) dp += step;
                step = step >> 1;
                if (c & 1) dp += step;
                step = step >> 1;
                dp += step;

                if (d<0) {
                        val -= dp;
                        if (val<-0x8000) val = -0x8000;
                } else {
                        val += dp;
                        if (val>0x7fff) val = 0x7fff;
                }

                {
                        int x = *ip - val;
                        d2 += x*x;
                }

        }
        d2 /= n; /* be sure it's non-negative */
        *st = state;
        return (int) sqrt(d2);
}

/* mash one channel... if you want to use opt>0, 9 is a reasonable value */
inline static void ImaMashChannel(
        unsigned ch,             /* channel number to encode, REQUIRE 0 <= ch < chans  */
        unsigned chans,          /* total channels */
        const SAMPL *ip,    /* ip[] is interleaved input samples */
        int n,              /* samples to encode PER channel, REQUIRE n % 8 == 1 */
        int *st,            /* input/output state, REQUIRE 0 <= *st <= ISSTMAX */
        unsigned char *obuff, /* output buffer[blockAlign] */
        int opt             /* non-zero allows some cpu-intensive code to improve output */
)
{
        int snext;
        int s0,d0;

        s0 = *st;
        if (opt>0) {
                int low,hi,w;
                int low0,hi0;
                snext = s0;
                d0 = ImaMashS(ch, chans, ip[ch], ip,n,&snext, NULL);

                w = 0;
                low=hi=s0;
                low0 = low-opt; if (low0<0) low0=0;
                hi0 = hi+opt; if (hi0>ISSTMAX) hi0=ISSTMAX;
                while (low>low0 || hi<hi0) {
                        if (!w && low>low0) {
                                int d2;
                                snext = --low;
                                d2 = ImaMashS(ch, chans, ip[ch], ip,n,&snext, NULL);
                                if (d2<d0) {
                                        d0=d2; s0=low;
                                        low0 = low-opt; if (low0<0) low0=0;
                                        hi0 = low+opt; if (hi0>ISSTMAX) hi0=ISSTMAX;
                                }
                        }
                        if (w && hi<hi0) {
                                int d2;
                                snext = ++hi;
                                d2 = ImaMashS(ch, chans, ip[ch], ip,n,&snext, NULL);
                                if (d2<d0) {
                                        d0=d2; s0=hi;
                                        low0 = hi-opt; if (low0<0) low0=0;
                                        hi0 = hi+opt; if (hi0>ISSTMAX) hi0=ISSTMAX;
                                }
                        }
                        w=1-w;
                }
                *st = s0;
        }
        ImaMashS(ch, chans, ip[ch], ip,n,st, obuff);
}

/* mash one block.  if you want to use opt>0, 9 is a reasonable value */
void lsx_ima_block_mash_i(
        unsigned chans,          /* total channels */
        const SAMPL *ip,    /* ip[] is interleaved input samples */
        int n,              /* samples to encode PER channel, REQUIRE n % 8 == 1 */
        int *st,            /* input/output state, REQUIRE 0 <= *st <= ISSTMAX */
        unsigned char *obuff, /* output buffer[blockAlign] */
        int opt             /* non-zero allows some cpu-intensive code to improve output */
)
{
        unsigned ch;
        for (ch=0; ch<chans; ch++)
                ImaMashChannel(ch, chans, ip, n, st+ch, obuff, opt);
}

/*
 * lsx_ima_samples_in(dataLen, chans, blockAlign, samplesPerBlock)
 *  returns the number of samples/channel which would go
 *  in the dataLen, given the other parameters ...
 *  if input samplesPerBlock is 0, then returns the max
 *  samplesPerBlock which would go into a block of size blockAlign
 *  Yes, it is confusing.
 */
size_t lsx_ima_samples_in(
  size_t dataLen,
  size_t chans,
  size_t blockAlign,
  size_t samplesPerBlock
)
{
  size_t m, n;

  if (samplesPerBlock) {
    n = (dataLen / blockAlign) * samplesPerBlock;
    m = (dataLen % blockAlign);
  } else {
    n = 0;
    m = blockAlign;
  }
  if (m >= (size_t)4*chans) {
    m -= 4*chans;    /* number of bytes beyond block-header */
    m /= 4*chans;    /* number of 4-byte blocks/channel beyond header */
    m = 8*m + 1;     /* samples/chan beyond header + 1 in header */
    if (samplesPerBlock && m > samplesPerBlock) m = samplesPerBlock;
    n += m;
  }
  return n;
  /*wSamplesPerBlock = ((wBlockAlign - 4*wChannels)/(4*wChannels))*8 + 1;*/
}

/*
 * size_t lsx_ima_bytes_per_block(chans, samplesPerBlock)
 *   return minimum blocksize which would be required
 *   to encode number of chans with given samplesPerBlock
 */
size_t lsx_ima_bytes_per_block(
  size_t chans,
  size_t samplesPerBlock
)
{
  size_t n;
  /* per channel, ima has blocks of len 4, the 1st has 1st sample, the others
   * up to 8 samples per block,
   * so number of later blocks is (nsamp-1 + 7)/8, total blocks/chan is
   * (nsamp-1+7)/8 + 1 = (nsamp+14)/8
   */
  n = ((size_t)samplesPerBlock + 14)/8 * 4 * chans;
  return n;
}
