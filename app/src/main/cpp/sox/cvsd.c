/*      libSoX CVSD (Continuously Variable Slope Delta modulation)
 *      conversion routines
 *
 *      The CVSD format is described in the MIL Std 188 113, which is
 *      available from http://bbs.itsi.disa.mil:5580/T3564
 *
 *      Copyright (C) 1996
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
 *
 * Change History:
 *
 * June 1, 1998 - Chris Bagwell (cbagwell@sprynet.com)
 *   Fixed compile warnings reported by Kjetil Torgrim Homme
 *   <kjetilho@ifi.uio.no>
 *
 * June 20, 2006 - Kimberly Rockwell (pyxis13317 (at) yahoo.com)
 *   Speed optimization: Unrolled float_conv() loop in seperate
 *     functions for encoding and decoding.  15% speed up decoding.
 *
 * Aug. 24, 2009 - P. Chaintreuil (sox-cvsd-peep (at) parallaxshift.com)
 *   Speed optimization: Replaced calls to memmove() with a 
 *     mirrored circular buffer.  This doubles the size of the
 *     dec.output_filter (48 -> 96 floats) and enc.input_filter
 *     (16 -> 32 floats), but keeps the memory from having to
 *     be copied so many times.  56% speed increase decoding;
 *     less than 5% encoding speed increase.
 */

#include "sox_i.h"
#include "cvsd.h"
#include "cvsdfilt.h"

#include <string.h>
#include <time.h>

/* ---------------------------------------------------------------------- */
/*
 * private data structures
 */

typedef cvsd_priv_t priv_t;

static int debug_count = 0;

/* ---------------------------------------------------------------------- */

/* This float_conv() function is not used as more specialized/optimized
 * versions exist below.  However, those new versions are tied to
 * very percise filters defined in cvsdfilt.h.  If those are modified
 * or different filters are found to be required, this function may
 * be needed.  Thus I leave it here for possible future use, but commented
 * out to avoid compiler warnings about it not being used.
static float float_conv(float const *fp1, float const *fp2,int n)
{
        float res = 0;
        for(; n > 0; n--)
                res += (*fp1++) * (*fp2++);
        return res;
}
*/

static float float_conv_enc(float const *fp1, float const *fp2)
{
    /* This is a specialzed version of float_conv() for encoding
     * which simply assumes a CVSD_ENC_FILTERLEN (16) length of
     * the two arrays and unrolls that loop.
     *
     * fp1 should be the enc.input_filter array and must be 
     * CVSD_ENC_FILTERLEN (16) long.
     *
     * fp2 should be one of the enc_filter_xx_y() tables listed
     * in cvsdfilt.h.  At minimum, fp2 must be CVSD_ENC_FILTERLEN
     * (16) entries long.
     */
    float res = 0;

    /* unrolling loop */ 
    res += fp1[0] * fp2[0];
    res += fp1[1] * fp2[1];
    res += fp1[2] * fp2[2];
    res += fp1[3] * fp2[3];
    res += fp1[4] * fp2[4];
    res += fp1[5] * fp2[5];
    res += fp1[6] * fp2[6];
    res += fp1[7] * fp2[7];
    res += fp1[8] * fp2[8];
    res += fp1[9] * fp2[9];
    res += fp1[10] * fp2[10];
    res += fp1[11] * fp2[11];
    res += fp1[12] * fp2[12];
    res += fp1[13] * fp2[13];
    res += fp1[14] * fp2[14];
    res += fp1[15] * fp2[15];

    return res;
}

static float float_conv_dec(float const *fp1, float const *fp2)
{
    /* This is a specialzed version of float_conv() for decoding
     * which assumes a specific length and structure to the data
     * in fp2.
     *
     * fp1 should be the dec.output_filter array and must be 
     * CVSD_DEC_FILTERLEN (48) long.
     *
     * fp2 should be one of the dec_filter_xx() tables listed
     * in cvsdfilt.h.  fp2 is assumed to be CVSD_DEC_FILTERLEN
     * (48) entries long, is assumed to have 0.0 in the last
     * entry, and is a symmetrical mirror around fp2[23] (ie,
     * fp2[22] == fp2[24], fp2[0] == fp2[47], etc).
     */
    float res = 0;

    /* unrolling loop, also taking advantage of the symmetry
    * of the sampling rate array*/
    res += (fp1[0] + fp1[46]) * fp2[0];
    res += (fp1[1] + fp1[45]) * fp2[1];
    res += (fp1[2] + fp1[44]) * fp2[2];
    res += (fp1[3] + fp1[43]) * fp2[3];
    res += (fp1[4] + fp1[42]) * fp2[4];
    res += (fp1[5] + fp1[41]) * fp2[5];
    res += (fp1[6] + fp1[40]) * fp2[6];
    res += (fp1[7] + fp1[39]) * fp2[7];
    res += (fp1[8] + fp1[38]) * fp2[8];
    res += (fp1[9] + fp1[37]) * fp2[9];
    res += (fp1[10] + fp1[36]) * fp2[10];
    res += (fp1[11] + fp1[35]) * fp2[11];
    res += (fp1[12] + fp1[34]) * fp2[12];
    res += (fp1[13] + fp1[33]) * fp2[13];
    res += (fp1[14] + fp1[32]) * fp2[14];
    res += (fp1[15] + fp1[31]) * fp2[15];
    res += (fp1[16] + fp1[30]) * fp2[16];
    res += (fp1[17] + fp1[29]) * fp2[17];
    res += (fp1[18] + fp1[28]) * fp2[18];
    res += (fp1[19] + fp1[27]) * fp2[19];
    res += (fp1[20] + fp1[26]) * fp2[20];
    res += (fp1[21] + fp1[25]) * fp2[21];
    res += (fp1[22] + fp1[24]) * fp2[22];
    res += (fp1[23]) * fp2[23];

    return res;
}

/* ---------------------------------------------------------------------- */
/*
 * some remarks about the implementation of the CVSD decoder
 * the principal integrator is integrated into the output filter
 * to achieve this, the coefficients of the output filter are multiplied
 * with (1/(1-1/z)) in the initialisation code.
 * the output filter must have a sharp zero at f=0 (i.e. the sum of the
 * filter parameters must be zero). This prevents an accumulation of
 * DC voltage at the principal integration.
 */
/* ---------------------------------------------------------------------- */

static void cvsdstartcommon(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;

        p->cvsd_rate = (ft->signal.rate <= 24000) ? 16000 : 32000;
        ft->signal.rate = 8000;
        ft->signal.channels = 1;
        lsx_rawstart(ft, sox_true, sox_false, sox_true, SOX_ENCODING_CVSD, 1);
        /*
         * initialize the decoder
         */
        p->com.overload = 0x5;
        p->com.mla_int = 0;
        /*
         * timeconst = (1/e)^(200 / SR) = exp(-200/SR)
         * SR is the sampling rate
         */
        p->com.mla_tc0 = exp((-200.0)/((float)(p->cvsd_rate)));
        /*
         * phase_inc = 32000 / SR
         */
        p->com.phase_inc = 32000 / p->cvsd_rate;
        /*
         * initialize bit shift register
         */
        p->bit.shreg = p->bit.cnt = 0;
        p->bit.mask = 1;
        /*
         * count the bytes written
         */
        p->bytes_written = 0;
        p->com.v_min = 1;
        p->com.v_max = -1;
        lsx_report("cvsd: bit rate %dbit/s, bits from %s", p->cvsd_rate,
               ft->encoding.reverse_bits ? "msb to lsb" : "lsb to msb");
}

/* ---------------------------------------------------------------------- */

int lsx_cvsdstartread(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;
        float *fp1;
        int i;

        cvsdstartcommon(ft);

        p->com.mla_tc1 = 0.1 * (1 - p->com.mla_tc0);
        p->com.phase = 0;
        /*
         * initialize the output filter coeffs (i.e. multiply
         * the coeffs with (1/(1-1/z)) to achieve integration
         * this is now done in the filter parameter generation utility
         */
        /*
         * zero the filter
         */
        for(fp1 = p->c.dec.output_filter, i = CVSD_DEC_FILTERLEN*2; i > 0; i--)
                *fp1++ = 0;
        /* initialize mirror circular buffer offset to anything sane. */
        p->c.dec.offset = CVSD_DEC_FILTERLEN - 1;

        return (SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

int lsx_cvsdstartwrite(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;
        float *fp1;
        int i;

        cvsdstartcommon(ft);

        p->com.mla_tc1 = 0.1 * (1 - p->com.mla_tc0);
        p->com.phase = 4;
        /*
         * zero the filter
         */
        for(fp1 = p->c.enc.input_filter, i = CVSD_ENC_FILTERLEN*2; i > 0; i--)
                *fp1++ = 0;
        p->c.enc.recon_int = 0;
        /* initialize mirror circular buffer offset to anything sane. */
        p->c.enc.offset = CVSD_ENC_FILTERLEN - 1;

        return(SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

int lsx_cvsdstopwrite(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;

        if (p->bit.cnt) {
                lsx_writeb(ft, p->bit.shreg);
                p->bytes_written++;
        }
        lsx_debug("cvsd: min slope %f, max slope %f",
               p->com.v_min, p->com.v_max);

        return (SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

int lsx_cvsdstopread(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;

        lsx_debug("cvsd: min value %f, max value %f",
               p->com.v_min, p->com.v_max);

        return(SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

size_t lsx_cvsdread(sox_format_t * ft, sox_sample_t *buf, size_t nsamp)
{
        priv_t *p = (priv_t *) ft->priv;
        size_t done = 0;
        float oval;

        while (done < nsamp) {
                if (!p->bit.cnt) {
                        if (lsx_read_b_buf(ft, &(p->bit.shreg), (size_t) 1) != 1)
                                return done;
                        p->bit.cnt = 8;
                        p->bit.mask = 1;
                }
                /*
                 * handle one bit
                 */
                p->bit.cnt--;
                p->com.overload = ((p->com.overload << 1) |
                                   (!!(p->bit.shreg & p->bit.mask))) & 7;
                p->bit.mask <<= 1;
                p->com.mla_int *= p->com.mla_tc0;
                if ((p->com.overload == 0) || (p->com.overload == 7))
                        p->com.mla_int += p->com.mla_tc1;

                /* shift output filter window in mirror cirular buffer. */
                if (p->c.dec.offset != 0)
                        --p->c.dec.offset;
                else p->c.dec.offset = CVSD_DEC_FILTERLEN - 1;
                /* write into both halves of the mirror circular buffer */
                if (p->com.overload & 1)
                {
                        p->c.dec.output_filter[p->c.dec.offset] = p->com.mla_int;
                        p->c.dec.output_filter[p->c.dec.offset + CVSD_DEC_FILTERLEN] = p->com.mla_int;
                }
                else
                {
                        p->c.dec.output_filter[p->c.dec.offset] = -p->com.mla_int;
                        p->c.dec.output_filter[p->c.dec.offset + CVSD_DEC_FILTERLEN] = -p->com.mla_int;
                }

                /*
                 * check if the next output is due
                 */
                p->com.phase += p->com.phase_inc;
                if (p->com.phase >= 4) {
                        oval = float_conv_dec(
                                p->c.dec.output_filter + p->c.dec.offset,
                                (p->cvsd_rate < 24000) ?
                                dec_filter_16 : dec_filter_32);
                        lsx_debug_more("input %d %f\n", debug_count, p->com.mla_int);
                        lsx_debug_more("recon %d %f\n", debug_count, oval);
                        debug_count++;

                        if (oval > p->com.v_max)
                                p->com.v_max = oval;
                        if (oval < p->com.v_min)
                                p->com.v_min = oval;
                        *buf++ = (oval * ((float)SOX_SAMPLE_MAX));
                        done++;
                }
                p->com.phase &= 3;
        }
        return done;
}

/* ---------------------------------------------------------------------- */

size_t lsx_cvsdwrite(sox_format_t * ft, const sox_sample_t *buf, size_t nsamp)
{
        priv_t *p = (priv_t *) ft->priv;
        size_t done = 0;
        float inval;

        for(;;) {
                /*
                 * check if the next input is due
                 */
                if (p->com.phase >= 4) {
                        if (done >= nsamp)
                                return done;

                        /* shift input filter window in mirror cirular buffer. */
                        if (p->c.enc.offset != 0)
                                --p->c.enc.offset;
                        else p->c.enc.offset = CVSD_ENC_FILTERLEN - 1;

                        /* write into both halves of the mirror circular buffer */
                        p->c.enc.input_filter[p->c.enc.offset] = 
                                 p->c.enc.input_filter[p->c.enc.offset 
                                     + CVSD_ENC_FILTERLEN] = 
                                                (*buf++) /
                                                    ((float)SOX_SAMPLE_MAX);
                        done++;
                }
                p->com.phase &= 3;
                /* insert input filter here! */
                inval = float_conv_enc(
                                   p->c.enc.input_filter + p->c.enc.offset,
                                   (p->cvsd_rate < 24000) ?
                                   (enc_filter_16[(p->com.phase >= 2)]) :
                                   (enc_filter_32[p->com.phase]));
                /*
                 * encode one bit
                 */
                p->com.overload = (((p->com.overload << 1) |
                                    (inval >  p->c.enc.recon_int)) & 7);
                p->com.mla_int *= p->com.mla_tc0;
                if ((p->com.overload == 0) || (p->com.overload == 7))
                        p->com.mla_int += p->com.mla_tc1;
                if (p->com.mla_int > p->com.v_max)
                        p->com.v_max = p->com.mla_int;
                if (p->com.mla_int < p->com.v_min)
                        p->com.v_min = p->com.mla_int;
                if (p->com.overload & 1) {
                        p->c.enc.recon_int += p->com.mla_int;
                        p->bit.shreg |= p->bit.mask;
                } else
                        p->c.enc.recon_int -= p->com.mla_int;
                if ((++(p->bit.cnt)) >= 8) {
                        lsx_writeb(ft, p->bit.shreg);
                        p->bytes_written++;
                        p->bit.shreg = p->bit.cnt = 0;
                        p->bit.mask = 1;
                } else
                        p->bit.mask <<= 1;
                p->com.phase += p->com.phase_inc;
                lsx_debug_more("input %d %f\n", debug_count, inval);
                lsx_debug_more("recon %d %f\n", debug_count, p->c.enc.recon_int);
                debug_count++;
        }
}

/* ---------------------------------------------------------------------- */
/*
 * DVMS file header
 */

/* FIXME: eliminate these 4 functions */

static uint32_t get32_le(unsigned char **p)
{
  uint32_t val = (((*p)[3]) << 24) | (((*p)[2]) << 16) |
          (((*p)[1]) << 8) | (**p);
  (*p) += 4;
  return val;
}

static uint16_t get16_le(unsigned char **p)
{
  unsigned val = (((*p)[1]) << 8) | (**p);
  (*p) += 2;
  return val;
}

static void put32_le(unsigned char **p, uint32_t val)
{
  *(*p)++ = val & 0xff;
  *(*p)++ = (val >> 8) & 0xff;
  *(*p)++ = (val >> 16) & 0xff;
  *(*p)++ = (val >> 24) & 0xff;
}

static void put16_le(unsigned char **p, unsigned val)
{
  *(*p)++ = val & 0xff;
  *(*p)++ = (val >> 8) & 0xff;
}

struct dvms_header {
        char          Filename[14];
        unsigned      Id;
        unsigned      State;
        time_t        Unixtime;
        unsigned      Usender;
        unsigned      Ureceiver;
        size_t     Length;
        unsigned      Srate;
        unsigned      Days;
        unsigned      Custom1;
        unsigned      Custom2;
        char          Info[16];
        char          extend[64];
        unsigned      Crc;
};

#define DVMS_HEADER_LEN 120

/* ---------------------------------------------------------------------- */

static int dvms_read_header(sox_format_t * ft, struct dvms_header *hdr)
{
        unsigned char hdrbuf[DVMS_HEADER_LEN];
        unsigned char *pch = hdrbuf;
        int i;
        unsigned sum;

        if (lsx_readbuf(ft, hdrbuf, sizeof(hdrbuf)) != sizeof(hdrbuf))
        {
                return (SOX_EOF);
        }
        for(i = sizeof(hdrbuf), sum = 0; i > /*2*/3; i--) /* Deti bug */
                sum += *pch++;
        pch = hdrbuf;
        memcpy(hdr->Filename, pch, sizeof(hdr->Filename));
        pch += sizeof(hdr->Filename);
        hdr->Id = get16_le(&pch);
        hdr->State = get16_le(&pch);
        hdr->Unixtime = get32_le(&pch);
        hdr->Usender = get16_le(&pch);
        hdr->Ureceiver = get16_le(&pch);
        hdr->Length = get32_le(&pch);
        hdr->Srate = get16_le(&pch);
        hdr->Days = get16_le(&pch);
        hdr->Custom1 = get16_le(&pch);
        hdr->Custom2 = get16_le(&pch);
        memcpy(hdr->Info, pch, sizeof(hdr->Info));
        pch += sizeof(hdr->Info);
        memcpy(hdr->extend, pch, sizeof(hdr->extend));
        pch += sizeof(hdr->extend);
        hdr->Crc = get16_le(&pch);
        if (sum != hdr->Crc)
        {
                lsx_report("DVMS header checksum error, read %u, calculated %u",
                     hdr->Crc, sum);
                return (SOX_EOF);
        }
        return (SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

/*
 * note! file must be seekable
 */
static int dvms_write_header(sox_format_t * ft, struct dvms_header *hdr)
{
        unsigned char hdrbuf[DVMS_HEADER_LEN];
        unsigned char *pch = hdrbuf;
        unsigned char *pchs = hdrbuf;
        int i;
        unsigned sum;

        memcpy(pch, hdr->Filename, sizeof(hdr->Filename));
        pch += sizeof(hdr->Filename);
        put16_le(&pch, hdr->Id);
        put16_le(&pch, hdr->State);
        put32_le(&pch, (unsigned)hdr->Unixtime);
        put16_le(&pch, hdr->Usender);
        put16_le(&pch, hdr->Ureceiver);
        put32_le(&pch, (unsigned) hdr->Length);
        put16_le(&pch, hdr->Srate);
        put16_le(&pch, hdr->Days);
        put16_le(&pch, hdr->Custom1);
        put16_le(&pch, hdr->Custom2);
        memcpy(pch, hdr->Info, sizeof(hdr->Info));
        pch += sizeof(hdr->Info);
        memcpy(pch, hdr->extend, sizeof(hdr->extend));
        pch += sizeof(hdr->extend);
        for(i = sizeof(hdrbuf), sum = 0; i > /*2*/3; i--) /* Deti bug */
                sum += *pchs++;
        hdr->Crc = sum;
        put16_le(&pch, hdr->Crc);
        if (lsx_seeki(ft, (off_t)0, SEEK_SET) < 0)
        {
                lsx_report("seek failed\n: %s",strerror(errno));
                return (SOX_EOF);
        }
        if (lsx_writebuf(ft, hdrbuf, sizeof(hdrbuf)) != sizeof(hdrbuf))
        {
                lsx_report("%s",strerror(errno));
                return (SOX_EOF);
        }
        return (SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

static void make_dvms_hdr(sox_format_t * ft, struct dvms_header *hdr)
{
        priv_t *p = (priv_t *) ft->priv;
        size_t len;
        char * comment = lsx_cat_comments(ft->oob.comments);

        memset(hdr->Filename, 0, sizeof(hdr->Filename));
        len = strlen(ft->filename);
        if (len >= sizeof(hdr->Filename))
                len = sizeof(hdr->Filename)-1;
        memcpy(hdr->Filename, ft->filename, len);
        hdr->Id = hdr->State = 0;
        hdr->Unixtime = sox_globals.repeatable? 0 : time(NULL);
        hdr->Usender = hdr->Ureceiver = 0;
        hdr->Length = p->bytes_written;
        hdr->Srate = p->cvsd_rate/100;
        hdr->Days = hdr->Custom1 = hdr->Custom2 = 0;
        memset(hdr->Info, 0, sizeof(hdr->Info));
        len = strlen(comment);
        if (len >= sizeof(hdr->Info))
                len = sizeof(hdr->Info)-1;
        memcpy(hdr->Info, comment, len);
        memset(hdr->extend, 0, sizeof(hdr->extend));
        free(comment);
}

/* ---------------------------------------------------------------------- */

int lsx_dvmsstartread(sox_format_t * ft)
{
        struct dvms_header hdr;
        int rc;

        rc = dvms_read_header(ft, &hdr);
        if (rc){
            lsx_fail_errno(ft,SOX_EHDR,"unable to read DVMS header");
            return rc;
        }

        lsx_debug("DVMS header of source file \"%s\":", ft->filename);
        lsx_debug("  filename  \"%.14s\"", hdr.Filename);
        lsx_debug("  id        0x%x", hdr.Id);
        lsx_debug("  state     0x%x", hdr.State);
        lsx_debug("  time      %s", ctime(&hdr.Unixtime)); /* ctime generates lf */
        lsx_debug("  usender   %u", hdr.Usender);
        lsx_debug("  ureceiver %u", hdr.Ureceiver);
        lsx_debug("  length    %" PRIuPTR, hdr.Length);
        lsx_debug("  srate     %u", hdr.Srate);
        lsx_debug("  days      %u", hdr.Days);
        lsx_debug("  custom1   %u", hdr.Custom1);
        lsx_debug("  custom2   %u", hdr.Custom2);
        lsx_debug("  info      \"%.16s\"", hdr.Info);
        ft->signal.rate = (hdr.Srate < 240) ? 16000 : 32000;
        lsx_debug("DVMS rate %dbit/s using %gbit/s deviation %g%%",
               hdr.Srate*100, ft->signal.rate,
               ((ft->signal.rate - hdr.Srate*100) * 100) / ft->signal.rate);
        rc = lsx_cvsdstartread(ft);
        if (rc)
            return rc;

        return(SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

int lsx_dvmsstartwrite(sox_format_t * ft)
{
        struct dvms_header hdr;
        int rc;

        rc = lsx_cvsdstartwrite(ft);
        if (rc)
            return rc;

        make_dvms_hdr(ft, &hdr);
        rc = dvms_write_header(ft, &hdr);
        if (rc){
                lsx_fail_errno(ft,rc,"cannot write DVMS header");
            return rc;
        }

        if (!ft->seekable)
               lsx_warn("Length in output .DVMS header will wrong since can't seek to fix it");

        return(SOX_SUCCESS);
}

/* ---------------------------------------------------------------------- */

int lsx_dvmsstopwrite(sox_format_t * ft)
{
        struct dvms_header hdr;
        int rc;

        lsx_cvsdstopwrite(ft);
        if (!ft->seekable)
        {
            lsx_warn("File not seekable");
            return (SOX_EOF);
        }
        if (lsx_seeki(ft, (off_t)0, 0) != 0)
        {
                lsx_fail_errno(ft,errno,"Can't rewind output file to rewrite DVMS header.");
                return(SOX_EOF);
        }
        make_dvms_hdr(ft, &hdr);
        rc = dvms_write_header(ft, &hdr);
        if(rc){
            lsx_fail_errno(ft,rc,"cannot write DVMS header");
            return rc;
        }
        return rc;
}
