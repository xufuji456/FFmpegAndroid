/* File format: AMR   (c) 2007 robs@users.sourceforge.net
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

#include <string.h>
#include <math.h>

#ifdef AMR_OPENCORE

LSX_DLENTRIES_TO_FUNCTIONS(AMR_OPENCORE_FUNC_ENTRIES)

typedef struct amr_opencore_funcs {
  LSX_DLENTRIES_TO_PTRS(AMR_OPENCORE_FUNC_ENTRIES, amr_dl);
} amr_opencore_funcs;

#endif /* AMR_OPENCORE */

#ifdef AMR_VO

LSX_DLENTRIES_TO_FUNCTIONS(AMR_VO_FUNC_ENTRIES)

typedef struct amr_vo_funcs {
  LSX_DLENTRIES_TO_PTRS(AMR_VO_FUNC_ENTRIES, amr_dl);
} amr_vo_funcs;

#endif /* AMR_VO */

#define AMR_CALL(p, func, args) ((p)->opencore.func args)

#ifdef AMR_VO
  #define AMR_CALL_ENCODER(p, func, args) ((p)->vo.func args)
#else
  #define AMR_CALL_ENCODER(p, func, args) ((p)->opencore.func args)
#endif

typedef struct amr_priv_t {
  void* state;
  unsigned mode;
  size_t pcm_index;
#ifdef AMR_OPENCORE
  amr_opencore_funcs opencore;
#endif /* AMR_OPENCORE */
#ifdef AMR_VO
  amr_vo_funcs vo;
#endif /* AMR_VO */
  short pcm[AMR_FRAME];
} priv_t;

#ifdef AMR_OPENCORE
static size_t decode_1_frame(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t n;
  uint8_t coded[AMR_CODED_MAX];

  if (lsx_readbuf(ft, &coded[0], (size_t)1) != 1)
    return AMR_FRAME;
  n = amr_block_size[(coded[0] >> 3) & 0x0F];
  if (!n) {
    lsx_fail("invalid block type");
    return AMR_FRAME;
  }
  n--;
  if (lsx_readbuf(ft, &coded[1], n) != n)
    return AMR_FRAME;
  AMR_CALL(p, AmrDecoderDecode, (p->state, coded, p->pcm, 0));
  return 0;
}
#endif

static int openlibrary(priv_t* p, int encoding)
{
  int open_library_result;

  (void)encoding;
#ifdef AMR_OPENCORE
  if (AMR_OPENCORE_ENABLE_ENCODE || !encoding)
  {
    LSX_DLLIBRARY_TRYOPEN(
      0,
      &p->opencore,
      amr_dl,
      AMR_OPENCORE_FUNC_ENTRIES,
      AMR_OPENCORE_DESC,
      amr_opencore_library_names,
      open_library_result);
    if (!open_library_result)
      return SOX_SUCCESS;
    lsx_fail("Unable to open " AMR_OPENCORE_DESC);
    return SOX_EOF;
  }
#endif /* AMR_OPENCORE */

#ifdef AMR_VO
  if (encoding) {
    LSX_DLLIBRARY_TRYOPEN(
        0,
        &p->vo,
        amr_dl,
        AMR_VO_FUNC_ENTRIES,
        AMR_VO_DESC,
        amr_vo_library_names,
        open_library_result);
    if (!open_library_result)
      return SOX_SUCCESS;
    lsx_fail("Unable to open " AMR_VO_DESC);
  }
#endif /* AMR_VO */

  return SOX_EOF;
}

static void closelibrary(priv_t* p)
{
#ifdef AMR_OPENCORE
  LSX_DLLIBRARY_CLOSE(&p->opencore, amr_dl);
#endif
#ifdef AMR_VO
  LSX_DLLIBRARY_CLOSE(&p->vo, amr_dl);
#endif
}

#ifdef AMR_OPENCORE
static size_t amr_duration_frames(sox_format_t * ft)
{
  off_t      frame_size, data_start_offset = lsx_tell(ft);
  size_t     frames;
  uint8_t    coded;

  for (frames = 0; lsx_readbuf(ft, &coded, (size_t)1) == 1; ++frames) {
    frame_size = amr_block_size[coded >> 3 & 15];
    if (!frame_size) {
      lsx_fail("invalid block type");
      break;
    }
    if (lsx_seeki(ft, frame_size - 1, SEEK_CUR)) {
      lsx_fail("seek");
      break;
    }
  }
  lsx_debug("frames=%lu", (unsigned long)frames);
  lsx_seeki(ft, data_start_offset, SEEK_SET);
  return frames;
}
#endif

static int startread(sox_format_t * ft)
{
#if !defined(AMR_OPENCORE)
  lsx_fail_errno(ft, SOX_EOF, "SoX was compiled without AMR-WB decoding support.");
  return SOX_EOF;
#else
  priv_t * p = (priv_t *)ft->priv;
  char buffer[sizeof(amr_magic) - 1];
  int open_library_result;

  if (lsx_readchars(ft, buffer, sizeof(buffer)))
    return SOX_EOF;
  if (memcmp(buffer, amr_magic, sizeof(buffer))) {
    lsx_fail_errno(ft, SOX_EHDR, "invalid magic number");
    return SOX_EOF;
  }

  open_library_result = openlibrary(p, 0);
  if (open_library_result != SOX_SUCCESS)
    return open_library_result;

  p->pcm_index = AMR_FRAME;
  p->state = AMR_CALL(p, AmrDecoderInit, ());
  if (!p->state)
  {
      closelibrary(p);
      lsx_fail("AMR decoder failed to initialize.");
      return SOX_EOF;
  }

  ft->signal.rate = AMR_RATE;
  ft->encoding.encoding = AMR_ENCODING;
  ft->signal.channels = 1;
  ft->signal.length = ft->signal.length != SOX_IGNORE_LENGTH && ft->seekable?
    (size_t)(amr_duration_frames(ft) * .02 * ft->signal.rate +.5) : SOX_UNSPEC;
  return SOX_SUCCESS;
#endif
}

#ifdef AMR_OPENCORE

static size_t read_samples(sox_format_t * ft, sox_sample_t * buf, size_t len)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t done;

  for (done = 0; done < len; done++) {
    if (p->pcm_index >= AMR_FRAME)
      p->pcm_index = decode_1_frame(ft);
    if (p->pcm_index >= AMR_FRAME)
      break;
    *buf++ = SOX_SIGNED_16BIT_TO_SAMPLE(p->pcm[p->pcm_index++], ft->clips);
  }
  return done;
}

static int stopread(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  AMR_CALL(p, AmrDecoderExit, (p->state));
  closelibrary(p);
  return SOX_SUCCESS;
}

#else

#define read_samples NULL
#define stopread NULL

#endif

static int startwrite(sox_format_t * ft)
{
#if !defined(AMR_VO) && !AMR_OPENCORE_ENABLE_ENCODE
  lsx_fail_errno(ft, SOX_EOF, "SoX was compiled without AMR-WB encoding support.");
  return SOX_EOF;
#else
  priv_t * p = (priv_t *)ft->priv;
  int open_library_result;

  if (ft->encoding.compression != HUGE_VAL) {
    p->mode = (unsigned)ft->encoding.compression;
    if (p->mode != ft->encoding.compression || p->mode > AMR_MODE_MAX) {
      lsx_fail_errno(ft, SOX_EINVAL, "compression level must be a whole number from 0 to %i", AMR_MODE_MAX);
      return SOX_EOF;
    }
  }
  else p->mode = 0;

  open_library_result = openlibrary(p, 1);
  if (open_library_result != SOX_SUCCESS)
    return open_library_result;

  p->state = AMR_CALL_ENCODER(p, AmrEncoderInit, ());
  if (!p->state)
  {
      closelibrary(p);
      lsx_fail("AMR encoder failed to initialize.");
      return SOX_EOF;
  }

  lsx_writes(ft, amr_magic);
  p->pcm_index = 0;
  return SOX_SUCCESS;
#endif
}

#if defined(AMR_VO) || AMR_OPENCORE_ENABLE_ENCODE

static sox_bool encode_1_frame(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  uint8_t coded[AMR_CODED_MAX];
  int n = AMR_CALL_ENCODER(p, AmrEncoderEncode, (p->state, p->mode, p->pcm, coded, 1));
  sox_bool result = lsx_writebuf(ft, coded, (size_t) (size_t) (unsigned)n) == (unsigned)n;
  if (!result)
    lsx_fail_errno(ft, errno, "write error");
  return result;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t * buf, size_t len)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t done;

  for (done = 0; done < len; ++done) {
    SOX_SAMPLE_LOCALS;
    p->pcm[p->pcm_index++] = SOX_SAMPLE_TO_SIGNED_16BIT(*buf++, ft->clips);
    if (p->pcm_index == AMR_FRAME) {
      p->pcm_index = 0;
      if (!encode_1_frame(ft))
        return 0;
    }
  }
  return done;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  int result = SOX_SUCCESS;

  if (p->pcm_index) {
    do {
      p->pcm[p->pcm_index++] = 0;
    } while (p->pcm_index < AMR_FRAME);
    if (!encode_1_frame(ft))
      result = SOX_EOF;
  }
  AMR_CALL_ENCODER(p, AmrEncoderExit, (p->state));
  return result;
}

#else

#define write_samples NULL
#define stopwrite NULL

#endif /* defined(AMR_VO) || AMR_OPENCORE_ENABLE_ENCODE */

sox_format_handler_t const * AMR_FORMAT_FN(void);
sox_format_handler_t const * AMR_FORMAT_FN(void)
{
  static char const * const names[] = {AMR_NAMES, NULL};
  static sox_rate_t   const write_rates[] = {AMR_RATE, 0};
  static unsigned const write_encodings[] = {AMR_ENCODING, 0, 0};
  static sox_format_handler_t handler = {
    SOX_LIB_VERSION_CODE,
    AMR_DESC,
    names, SOX_FILE_MONO,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    NULL, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
