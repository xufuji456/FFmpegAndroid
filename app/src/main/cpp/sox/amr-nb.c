/* File format: AMR-NB   (c) 2007 robs@users.sourceforge.net
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

enum amrnb_mode { amrnb_mode_dummy };

static const unsigned amrnb_block_size[] = {13, 14, 16, 18, 20, 21, 27, 32, 6, 0, 0, 0, 0, 0, 0, 1};
static char const amrnb_magic[] = "#!AMR\n";
#define amr_block_size amrnb_block_size
#define amr_magic amrnb_magic
#define amr_priv_t amrnb_priv_t
#define amr_opencore_funcs amrnb_opencore_funcs
#define amr_gp3_funcs amrnb_gp3_funcs

#define AMR_CODED_MAX       32                  /* max coded size */
#define AMR_ENCODING        SOX_ENCODING_AMR_NB
#define AMR_FORMAT_FN       lsx_amr_nb_format_fn
#define AMR_FRAME           160                 /* 20ms @ 8kHz */
#define AMR_MODE_MAX        7
#define AMR_NAMES           "amr-nb", "anb"
#define AMR_RATE            8000
#define AMR_DESC            "3GPP Adaptive Multi Rate Narrow-Band (AMR-NB) lossy speech compressor"

#ifdef DL_OPENCORE_AMRNB
  #define AMR_FUNC  LSX_DLENTRY_DYNAMIC
#else
  #define AMR_FUNC  LSX_DLENTRY_STATIC
#endif /* DL_AMRNB */

/* OpenCore definitions: */

#define AMR_OPENCORE 1
#define AMR_OPENCORE_ENABLE_ENCODE 1

#define AMR_OPENCORE_FUNC_ENTRIES(f,x) \
  AMR_FUNC(f,x, void*, Encoder_Interface_init,   (int dtx)) \
  AMR_FUNC(f,x, int,   Encoder_Interface_Encode, (void* state, enum amrnb_mode mode, const short* in, unsigned char* out, int forceSpeech)) \
  AMR_FUNC(f,x, void,  Encoder_Interface_exit,   (void* state)) \
  AMR_FUNC(f,x, void*, Decoder_Interface_init,   (void)) \
  AMR_FUNC(f,x, void,  Decoder_Interface_Decode, (void* state, const unsigned char* in, short* out, int bfi)) \
  AMR_FUNC(f,x, void,  Decoder_Interface_exit,   (void* state)) \

#define AmrEncoderInit() \
  Encoder_Interface_init(1)
#define AmrEncoderEncode(state, mode, in, out, forceSpeech) \
  Encoder_Interface_Encode(state, mode, in, out, forceSpeech)
#define AmrEncoderExit(state) \
  Encoder_Interface_exit(state)
#define AmrDecoderInit() \
  Decoder_Interface_init()
#define AmrDecoderDecode(state, in, out, bfi) \
  Decoder_Interface_Decode(state, in, out, bfi)
#define AmrDecoderExit(state) \
  Decoder_Interface_exit(state)

#define AMR_OPENCORE_DESC "amr-nb OpenCore library"
static const char* const amr_opencore_library_names[] =
{
#ifdef DL_OPENCORE_AMRNB
  "libopencore-amrnb",
  "libopencore-amrnb-0",
#endif
  NULL
};

#include "amr.h"
