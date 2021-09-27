/* File format: AMR-WB   (c) 2007 robs@users.sourceforge.net
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
 
/*
 * In order to use the AMR format with SoX, you need to have an
 * AMR library installed at SoX build time. The SoX build system
 * recognizes the AMR implementations available from
 * http://opencore-amr.sourceforge.net/
 */

#include "sox_i.h"

/* Common definitions: */

static const uint8_t amrwb_block_size[] = {18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1};
static char const amrwb_magic[] = "#!AMR-WB\n";
#define amr_block_size amrwb_block_size
#define amr_magic amrwb_magic
#define amr_priv_t amrwb_priv_t
#define amr_opencore_funcs amrwb_opencore_funcs
#define amr_vo_funcs amrwb_vo_funcs

#define AMR_CODED_MAX       61 /* NB_SERIAL_MAX */
#define AMR_ENCODING        SOX_ENCODING_AMR_WB
#define AMR_FORMAT_FN       lsx_amr_wb_format_fn
#define AMR_FRAME           320 /* L_FRAME16k */
#define AMR_MODE_MAX        8
#define AMR_NAMES           "amr-wb", "awb"
#define AMR_RATE            16000
#define AMR_DESC            "3GPP Adaptive Multi Rate Wide-Band (AMR-WB) lossy speech compressor"

/* OpenCore definitions: */

#ifdef DL_OPENCORE_AMRWB
  #define AMR_OC_FUNC  LSX_DLENTRY_DYNAMIC
#else
  #define AMR_OC_FUNC  LSX_DLENTRY_STATIC
#endif

#if defined(HAVE_OPENCORE_AMRWB_DEC_IF_H) || defined(DL_OPENCORE_AMRWB)
  #define AMR_OPENCORE 1
  #define AMR_OPENCORE_ENABLE_ENCODE 0
#endif

#define AMR_OPENCORE_FUNC_ENTRIES(f,x) \
  AMR_OC_FUNC(f,x, void*, D_IF_init,   (void)) \
  AMR_OC_FUNC(f,x, void,  D_IF_decode, (void* state, const unsigned char* in, short* out, int bfi)) \
  AMR_OC_FUNC(f,x, void,  D_IF_exit,   (void* state)) \

#define AmrDecoderInit() \
  D_IF_init()
#define AmrDecoderDecode(state, in, out, bfi) \
  D_IF_decode(state, in, out, bfi)
#define AmrDecoderExit(state) \
  D_IF_exit(state)

#define AMR_OPENCORE_DESC "amr-wb OpenCore library"
static const char* const amr_opencore_library_names[] =
{
#ifdef DL_OPENCORE_AMRWB
  "libopencore-amrwb",
  "libopencore-amrwb-0",
#endif
  NULL
};

/* VO definitions: */

#ifdef DL_VO_AMRWBENC
  #define AMR_VO_FUNC  LSX_DLENTRY_DYNAMIC
#else
  #define AMR_VO_FUNC  LSX_DLENTRY_STATIC
#endif

#if defined(HAVE_VO_AMRWBENC_ENC_IF_H) || defined(DL_VO_AMRWBENC)
  #define AMR_VO 1
#endif

#define AMR_VO_FUNC_ENTRIES(f,x) \
  AMR_VO_FUNC(f,x, void*, E_IF_init,     (void)) \
  AMR_VO_FUNC(f,x, int,   E_IF_encode,(void* state, int16_t mode, int16_t* in, uint8_t* out, int16_t dtx)) \
  AMR_VO_FUNC(f,x, void,  E_IF_exit,     (void* state)) \

#define AmrEncoderInit() \
  E_IF_init()
#define AmrEncoderEncode(state, mode, in, out, forceSpeech) \
  E_IF_encode(state, mode, in, out, forceSpeech)
#define AmrEncoderExit(state) \
  E_IF_exit(state)

#define AMR_VO_DESC "amr-wb VisualOn library"
static const char* const amr_vo_library_names[] =
{
#ifdef DL_VO_AMRWBENC
  "libvo-amrwbenc",
  "libvo-amrwbenc-0",
#endif
  NULL
};

#include "amr.h"
