/* adpcm.c  codex functions for MS_ADPCM data
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

/*
 * November 22, 1999
 *  specs I've seen are unclear about ADPCM supporting more than 2 channels,
 *  but these routines support more channels in a manner which looks (IMHO)
 *  like the most natural extension.
 *
 *  Remark: code still turbulent, encoding very new.
 *
 */

#include "sox_i.h"
#include "adpcm.h"

#include <sys/types.h>
#include <stdio.h>

typedef struct {
        sox_sample_t  step;      /* step size */
        short coef[2];
} MsState_t;

#define lsbshortldi(x,p) { (x)=((short)((int)(p)[0] + ((int)(p)[1]<<8))); (p) += 2; }

/*
 * Lookup tables for MS ADPCM format
 */

/* these are step-size adjust factors, where
 * 1.0 is scaled to 0x100
 */
static const
sox_sample_t stepAdjustTable[] = {
        230, 230, 230, 230, 307, 409, 512, 614,
        768, 614, 512, 409, 307, 230, 230, 230
};

/* TODO : The first 7 lsx_ms_adpcm_i_coef sets are always hardcoded and must
   appear in the actual WAVE file.  They should be read in
   in case a sound program added extras to the list. */

const short lsx_ms_adpcm_i_coef[7][2] = {
                        { 256,   0},
                        { 512,-256},
                        {   0,   0},
                        { 192,  64},
                        { 240,   0},
                        { 460,-208},
                        { 392,-232}
};

extern void *lsx_ms_adpcm_alloc(unsigned chans)
{
        return lsx_malloc(chans * sizeof(MsState_t));
}

static inline sox_sample_t AdpcmDecode(sox_sample_t c, MsState_t *state,
                               sox_sample_t sample1, sox_sample_t sample2)
{
        sox_sample_t vlin;
        sox_sample_t sample;
        sox_sample_t step;

        /** Compute next step value **/
        step = state->step;
        {
                sox_sample_t nstep;
                nstep = (stepAdjustTable[c] * step) >> 8;
                state->step = (nstep < 16)? 16:nstep;
        }

        /** make linear prediction for next sample **/
        vlin =
                        ((sample1 * state->coef[0]) +
                         (sample2 * state->coef[1])) >> 8;
        /** then add the code*step adjustment **/
        c -= (c & 0x08) << 1;
        sample = (c * step) + vlin;

        if (sample > 0x7fff) sample = 0x7fff;
        else if (sample < -0x8000) sample = -0x8000;

        return (sample);
}

/* lsx_ms_adpcm_block_expand_i() outputs interleaved samples into one output buffer */
const char *lsx_ms_adpcm_block_expand_i(
        void *priv,
        unsigned chans,          /* total channels             */
        int nCoef,
        const short *coef,
        const unsigned char *ibuff,/* input buffer[blockAlign]   */
        SAMPL *obuff,       /* output samples, n*chans    */
        int n               /* samples to decode PER channel */
)
{
  const unsigned char *ip;
  unsigned ch;
  const char *errmsg = NULL;
  MsState_t *state = priv;  /* One decompressor state for each channel */

  /* Read the four-byte header for each channel */
  ip = ibuff;
  for (ch = 0; ch < chans; ch++) {
    unsigned char bpred = *ip++;
    if (bpred >= nCoef) {
      errmsg = "MSADPCM bpred >= nCoef, arbitrarily using 0\n";
      bpred = 0;
    }
    state[ch].coef[0] = coef[(int)bpred*2+0];
    state[ch].coef[1] = coef[(int)bpred*2+1];
  }

  for (ch = 0; ch < chans; ch++)
    lsbshortldi(state[ch].step, ip);

  /* sample1's directly into obuff */
  for (ch = 0; ch < chans; ch++)
    lsbshortldi(obuff[chans+ch], ip);

  /* sample2's directly into obuff */
  for (ch = 0; ch < chans; ch++)
    lsbshortldi(obuff[ch], ip);

  {
    unsigned ch2;
    unsigned char b;
    short *op, *top, *tmp;

    /* already have 1st 2 samples from block-header */
    op = obuff + 2*chans;
    top = obuff + n*chans;

    ch2 = 0;
    while (op < top) { /*** N.B. Without int casts, crashes on 64-bit arch ***/
      b = *ip++;
      tmp = op;
      *op++ = AdpcmDecode(b >> 4, state+ch2, tmp[-(int)chans], tmp[-(int)(2*chans)]);
      if (++ch2 == chans) ch2 = 0;
      tmp = op;
      *op++ = AdpcmDecode(b&0x0f, state+ch2, tmp[-(int)chans], tmp[-(int)(2*chans)]);
      if (++ch2 == chans) ch2 = 0;
    }
  }
  return errmsg;
}

static int AdpcmMashS(
        unsigned ch,              /* channel number to encode, REQUIRE 0 <= ch < chans  */
        unsigned chans,           /* total channels */
        SAMPL v[2],          /* values to use as starting 2 */
        const short coef[2],/* lin predictor coeffs */
        const SAMPL *ibuff,  /* ibuff[] is interleaved input samples */
        int n,               /* samples to encode PER channel */
        int *iostep,         /* input/output step, REQUIRE 16 <= *st <= 0x7fff */
        unsigned char *obuff /* output buffer[blockAlign], or NULL for no output  */
)
{
        const SAMPL *ip, *itop;
        unsigned char *op;
        int ox = 0;      /*  */
        int d, v0, v1, step;
        double d2;       /* long long is okay also, speed abt the same */

        ip = ibuff + ch;       /* point ip to 1st input sample for this channel */
        itop = ibuff + n*chans;
        v0 = v[0];
        v1 = v[1];
        d = *ip - v1; ip += chans; /* 1st input sample for this channel */
        d2 = d*d;  /* d2 will be sum of squares of errors, given input v0 and *st */
        d = *ip - v0; ip += chans; /* 2nd input sample for this channel */
        d2 += d*d;

        step = *iostep;

        op = obuff;            /* output pointer (or NULL) */
        if (op) {              /* NULL means don't output, just compute the rms error */
                op += chans;         /* skip bpred indices */
                op += 2*ch;          /* channel's stepsize */
                op[0] = step; op[1] = step>>8;
                op += 2*chans;       /* skip to v0 */
                op[0] = v0; op[1] = v0>>8;
                op += 2*chans;       /* skip to v1 */
                op[0] = v1; op[1] = v1>>8;
                op = obuff+7*chans;  /* point to base of output nibbles */
                ox = 4*ch;
        }
        for (; ip < itop; ip+=chans) {
                int vlin,d3,dp,c;

          /* make linear prediction for next sample */
                vlin = (v0 * coef[0] + v1 * coef[1]) >> 8;
                d3 = *ip - vlin;  /* difference between linear prediction and current sample */
                dp = d3 + (step<<3) + (step>>1);
                c = 0;
                if (dp>0) {
                        c = dp/step;
                        if (c>15) c = 15;
                }
                c -= 8;
                dp = c * step;   /* quantized estimate of samp - vlin */
                c &= 0x0f;       /* mask to 4 bits */

                v1 = v0; /* shift history */
                v0 = vlin + dp;
                if (v0<-0x8000) v0 = -0x8000;
                else if (v0>0x7fff) v0 = 0x7fff;

                d3 = *ip - v0;
                d2 += d3*d3; /* update square-error */

                if (op) {   /* if we want output, put it in proper place */
                        op[ox>>3] |= (ox&4)? c:(c<<4);
                        ox += 4*chans;
                        lsx_debug_more("%.1x",c);

                }

                /* Update the step for the next sample */
                step = (stepAdjustTable[c] * step) >> 8;
                if (step < 16) step = 16;

        }
        if (op) lsx_debug_more("\n");
        d2 /= n; /* be sure it's non-negative */
        lsx_debug_more("ch%d: st %d->%d, d %.1f\n", ch, *iostep, step, sqrt(d2));
        *iostep = step;
        return (int) sqrt(d2);
}

static inline void AdpcmMashChannel(
        unsigned ch,             /* channel number to encode, REQUIRE 0 <= ch < chans  */
        unsigned chans,          /* total channels */
        const SAMPL *ip,    /* ip[] is interleaved input samples */
        int n,              /* samples to encode PER channel, REQUIRE */
        int *st,            /* input/output steps, 16<=st[i] */
        unsigned char *obuff /* output buffer[blockAlign] */
)
{
        SAMPL v[2];
        int n0,s0,s1,ss,smin;
        int dmin,k,kmin;

        n0 = n/2; if (n0>32) n0=32;
        if (*st<16) *st = 16;
        v[1] = ip[ch];
        v[0] = ip[ch+chans];

        dmin = 0; kmin = 0; smin = 0;
        /* for each of 7 standard coeff sets, we try compression
         * beginning with last step-value, and with slightly
         * forward-adjusted step-value, taking best of the 14
         */
        for (k=0; k<7; k++) {
                int d0,d1;
                ss = s0 = *st;
                d0=AdpcmMashS(ch, chans, v, lsx_ms_adpcm_i_coef[k], ip, n, &ss, NULL); /* with step s0 */

                s1 = s0;
                AdpcmMashS(ch, chans, v, lsx_ms_adpcm_i_coef[k], ip, n0, &s1, NULL);
                lsx_debug_more(" s32 %d\n",s1);
                ss = s1 = (3*s0+s1)/4;
                d1=AdpcmMashS(ch, chans, v, lsx_ms_adpcm_i_coef[k], ip, n, &ss, NULL); /* with step s1 */
                if (!k || d0<dmin || d1<dmin) {
                        kmin = k;
                        if (d0<=d1) {
                                dmin = d0;
                                smin = s0;
                        }else{
                                dmin = d1;
                                smin = s1;
                        }
                }
        }
        *st = smin;
        lsx_debug_more("kmin %d, smin %5d, ",kmin,smin);
        AdpcmMashS(ch, chans, v, lsx_ms_adpcm_i_coef[kmin], ip, n, st, obuff);
        obuff[ch] = kmin;
}

void lsx_ms_adpcm_block_mash_i(
        unsigned chans,          /* total channels */
        const SAMPL *ip,    /* ip[n*chans] is interleaved input samples */
        int n,              /* samples to encode PER channel */
        int *st,            /* input/output steps, 16<=st[i] */
        unsigned char *obuff,      /* output buffer[blockAlign]     */
        int blockAlign      /* >= 7*chans + chans*(n-2)/2.0    */
)
{
        unsigned ch;
        unsigned char *p;

        lsx_debug_more("AdpcmMashI(chans %d, ip %p, n %d, st %p, obuff %p, bA %d)\n",
            chans, (void *)ip, n, (void *)st, obuff, blockAlign);

        for (p=obuff+7*chans; p<obuff+blockAlign; p++) *p=0;

        for (ch=0; ch<chans; ch++)
                AdpcmMashChannel(ch, chans, ip, n, st+ch, obuff);
}

/*
 * lsx_ms_adpcm_samples_in(dataLen, chans, blockAlign, samplesPerBlock)
 *  returns the number of samples/channel which would be
 *  in the dataLen, given the other parameters ...
 *  if input samplesPerBlock is 0, then returns the max
 *  samplesPerBlock which would go into a block of size blockAlign
 *  Yes, it is confusing usage.
 */
size_t lsx_ms_adpcm_samples_in(
        size_t dataLen,
        size_t chans,
        size_t blockAlign,
        size_t samplesPerBlock
){
        size_t m, n;

        if (samplesPerBlock) {
                n = (dataLen / blockAlign) * samplesPerBlock;
                m = (dataLen % blockAlign);
        } else {
                n = 0;
                m = blockAlign;
        }
        if (m >= (size_t)(7*chans)) {
                m -= 7*chans;          /* bytes beyond block-header */
                m = (2*m)/chans + 2;   /* nibbles/chans + 2 in header */
                if (samplesPerBlock && m > samplesPerBlock) m = samplesPerBlock;
                n += m;
        }
        return n;
}

size_t lsx_ms_adpcm_bytes_per_block(
        size_t chans,
        size_t samplesPerBlock
)
{
        size_t n;
        n = 7*chans;  /* header */
        if (samplesPerBlock > 2)
                n += (((size_t)samplesPerBlock-2)*chans + 1)/2;
        return n;
}

