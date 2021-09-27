/* libSoX file format: WavPack   (c) 2008 robs@users.sourceforge.net
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

#ifdef HAVE_WAVPACK_H
#define HAVE_WAVPACK 1
#endif

#ifdef HAVE_WAVPACK

#ifdef HAVE_WAVPACK_H
#include <wavpack.h>
#else
#include <wavpack/wavpack.h>
#endif

typedef struct {
  WavpackContext * codec;
  size_t first_block_size;
} priv_t;

static int32_t ft_read_b_buf(void * ft, void * buf, int32_t len) {
  return (int32_t)lsx_read_b_buf((sox_format_t *)ft, buf, (size_t)len);}
static uint32_t ft_tell(void * ft) {
  return lsx_tell((sox_format_t *)ft);}
static int ft_seek_abs(void * ft, uint32_t offset) {
  return lsx_seeki((sox_format_t *)ft, (off_t)offset, SEEK_SET);}
static int ft_seek_rel(void * ft, int32_t offset, int mode) {
  return lsx_seeki((sox_format_t *)ft, (off_t)offset, mode);}
static int ft_unreadb(void * ft, int b) {
  return lsx_unreadb((sox_format_t *)ft, (unsigned)b);}
static uint32_t ft_filelength(void * ft) {
  return (uint32_t)lsx_filelength((sox_format_t *)ft);}
static int ft_is_seekable(void *ft) {
  return ((sox_format_t *)ft)->seekable;}
static int32_t ft_write_b_buf(void * ft, void * buf, int32_t len) {
  priv_t * p = (priv_t *)((sox_format_t *)ft)->priv;
  if (!p->first_block_size)
    p->first_block_size = len;
  return (int32_t)lsx_write_b_buf((sox_format_t *)ft, buf, (size_t)len);}

static WavpackStreamReader io_fns = {
  ft_read_b_buf, ft_tell, ft_seek_abs, ft_seek_rel,
  ft_unreadb, ft_filelength, ft_is_seekable, ft_write_b_buf
};

static int start_read(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  char msg[80];

  p->codec = WavpackOpenFileInputEx(&io_fns, ft, NULL, msg, OPEN_NORMALIZE, 0);
  if (!p->codec) {
    lsx_fail_errno(ft, SOX_EHDR, "%s", msg);
    return SOX_EOF;
  }
  ft->encoding.bits_per_sample = WavpackGetBytesPerSample(p->codec) << 3;
  ft->signal.channels   = WavpackGetNumChannels(p->codec);
  if (WavpackGetSampleRate(p->codec) && ft->signal.rate && ft->signal.rate != WavpackGetSampleRate(p->codec))
    lsx_warn("`%s': overriding sample rate", ft->filename);
  else ft->signal.rate = WavpackGetSampleRate(p->codec);

  ft->signal.length = (uint64_t)WavpackGetNumSamples(p->codec) * ft->signal.channels;
  ft->encoding.encoding = (WavpackGetMode(p->codec) & MODE_FLOAT)?
    SOX_ENCODING_WAVPACKF : SOX_ENCODING_WAVPACK;
  return SOX_SUCCESS;
}

static size_t read_samples(sox_format_t * ft, sox_sample_t * buf, size_t len)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t i, actual = WavpackUnpackSamples(p->codec, buf, (uint32_t) len / ft->signal.channels) * ft->signal.channels;
  for (i = 0; i < actual; ++i) switch (ft->encoding.bits_per_sample) {
    SOX_SAMPLE_LOCALS;
    case  8: buf[i] = SOX_SIGNED_8BIT_TO_SAMPLE(buf[i],); break;
    case 16: buf[i] = SOX_SIGNED_16BIT_TO_SAMPLE(buf[i],); break;
    case 24: buf[i] = SOX_SIGNED_24BIT_TO_SAMPLE(buf[i],); break;
    case 32: buf[i] = ft->encoding.encoding == SOX_ENCODING_WAVPACKF?
      SOX_FLOAT_32BIT_TO_SAMPLE(*(float *)&buf[i], ft->clips) :
      SOX_SIGNED_32BIT_TO_SAMPLE(buf[i],);
      break;
  }
  return actual;
}

static int stop_read(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  WavpackCloseFile(p->codec);
  return SOX_SUCCESS;
}

static int start_write(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  WavpackConfig config;
  uint64_t size64;

  p->codec = WavpackOpenFileOutput(ft_write_b_buf, ft, NULL);
  if (!p->codec) {
    lsx_fail_errno(ft, SOX_ENOMEM, "WavPack error creating output instance");
    return SOX_EOF;
  }
  memset(&config, 0, sizeof(config));
  config.bytes_per_sample  = ft->encoding.bits_per_sample >> 3;
  config.bits_per_sample   = ft->encoding.bits_per_sample;
  config.channel_mask      = ft->signal.channels == 1? 4 :
      ft->signal.channels == 2? 3 : (1 << ft->signal.channels) - 1;
  config.num_channels      = ft->signal.channels;
  config.sample_rate       = (int32_t)(ft->signal.rate + .5);
  config.flags = CONFIG_VERY_HIGH_FLAG;
  size64 = ft->signal.length / ft->signal.channels;
  if (!WavpackSetConfiguration(p->codec, &config, size64 && size64 <= UINT_MAX ? (uint32_t)size64 : (uint32_t)-1)) {
    lsx_fail_errno(ft, SOX_EHDR, "%s", WavpackGetErrorMessage(p->codec));
    return SOX_EOF;
  }
  WavpackPackInit(p->codec);
  return SOX_SUCCESS;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t * buf, size_t len)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t i;
  int32_t * obuf = lsx_malloc(len * sizeof(*obuf));
  int result;

  for (i = 0; i < len; ++i) switch (ft->encoding.bits_per_sample) {
    SOX_SAMPLE_LOCALS;
    case  8: obuf[i] = SOX_SAMPLE_TO_SIGNED_8BIT(buf[i], ft->clips); break;
    case 16: obuf[i] = SOX_SAMPLE_TO_SIGNED_16BIT(buf[i], ft->clips); break;
    case 24: obuf[i] = SOX_SAMPLE_TO_SIGNED_24BIT(buf[i], ft->clips) << 8;
             obuf[i] >>= 8; break;
    case 32: obuf[i] = ft->encoding.encoding == SOX_ENCODING_WAVPACKF?
      SOX_SAMPLE_TO_SIGNED_24BIT(*(float *)&buf[i], ft->clips) :
      SOX_SAMPLE_TO_SIGNED_32BIT(buf[i], ft->clips);
      break;
  }
  result = WavpackPackSamples(p->codec, obuf, (uint32_t) len / ft->signal.channels);
  free(obuf);
  return result? len : 0;
}

static int stop_write(sox_format_t * ft)
{
  priv_t * p = (priv_t *)ft->priv;
  WavpackFlushSamples(p->codec);
  if (!WavpackFlushSamples(p->codec)) {
    lsx_fail_errno(ft, SOX_EINVAL, "%s", WavpackGetErrorMessage(p->codec));
    return SOX_EOF;
  }
  if (ft->seekable && WavpackGetNumSamples(p->codec) != WavpackGetSampleIndex(p->codec) && p->first_block_size >= 4) {
    char * buf = lsx_malloc(p->first_block_size);
    lsx_rewind(ft);
    lsx_readchars(ft, buf, p->first_block_size);
    if (!memcmp(buf, "wvpk", (size_t)4)) {
      WavpackUpdateNumSamples(p->codec, buf);
      lsx_rewind(ft);
      lsx_writebuf(ft, buf, p->first_block_size);
    }
    free(buf);
  }
  p->codec = WavpackCloseFile(p->codec);
  return SOX_SUCCESS;
}

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * p = (priv_t *)ft->priv;

  return WavpackSeekSample(p->codec, (uint32_t) (offset / ft->signal.channels))? SOX_SUCCESS : SOX_EOF;
}

LSX_FORMAT_HANDLER(wavpack)
{
  static char const * const names[] = {"wv", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_WAVPACK, 8, 16, 24, 32, 0,
    SOX_ENCODING_WAVPACKF, 32, 0,
    0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Lossless, lossy, and hybrid audio compression",
    names, 0,
    start_read, read_samples, stop_read,
    start_write, write_samples, stop_write,
    seek, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}

#endif /* HAVE_WAVPACK */
