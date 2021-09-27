/*
 * libsndio sound handler
 *
 * Copyright (c) 2009 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "sox_i.h"
#include <string.h>
#include <sndio.h>

struct sndio_priv {
  struct sio_hdl *hdl;             /* handle to speak to libsndio */
  struct sio_par par;              /* current device parameters */
#define SNDIO_BUFSZ 0x1000
  unsigned char buf[SNDIO_BUFSZ];  /* temp buffer for converions */
};

/*
 * convert ``count'' samples from sox encoding to sndio encoding
 */
static void encode(struct sio_par *par,
    sox_sample_t const *idata, unsigned char *odata, unsigned count)
{
  int obnext, osnext, s, osigbit;
  unsigned oshift, obps, i;

  obps = par->bps;
  osigbit = par->sig ? 0 : 1 << (par->bits - 1);
  oshift = 32 - (par->msb ? par->bps * 8 : par->bits);
  if (par->le) {
    obnext = 1;
    osnext = 0;
  } else {
    odata += par->bps - 1;
    obnext = -1;
    osnext = 2 * par->bps;
  }
  for (; count > 0; count--) {
    s = (*idata++ >> oshift) ^ osigbit;
    for (i = obps; i > 0; i--) {
      *odata = (unsigned char)s;
      s >>= 8;
      odata += obnext;
    }
    odata += osnext;
  }
}

/*
 * convert ``count'' samples from sndio encoding to sox encoding
 */
static void decode(struct sio_par *par,
    unsigned char *idata, sox_sample_t *odata, unsigned count)
{
  unsigned ishift, ibps, i;
  int s = 0xdeadbeef, ibnext, isnext, isigbit;

  ibps = par->bps;
  isigbit = par->sig ? 0 : 1 << (par->bits - 1);
  ishift = 32 - (par->msb ? par->bps * 8 : par->bits);
  if (par->le) {
    idata += par->bps - 1;
    ibnext = -1;
    isnext = 2 * par->bps;
  } else {
    ibnext = 1;
    isnext = 0;
  }
  for (; count > 0; count--) {
    for (i = ibps; i > 0; i--) {
      s <<= 8;
      s |= *idata;
      idata += ibnext;
    }
    idata += isnext;
    *odata++ = (s ^ isigbit) << ishift;
  }
}

static int startany(sox_format_t *ft, unsigned mode)
{
  struct sndio_priv *p = (struct sndio_priv *)ft->priv;
  struct sio_par reqpar;
  char *device;

  device = ft->filename;
  if (strcmp("default", device) == 0)
    device = NULL;

  p->hdl = sio_open(device, mode, 0);
  if (p->hdl == NULL)
    return SOX_EOF;
  /*
   * set specified parameters, leaving others to the defaults
   */
  sio_initpar(&reqpar);
  if (ft->signal.rate > 0)
    reqpar.rate = ft->signal.rate;
  if (ft->signal.channels > 0) {
    if (mode == SIO_PLAY)
      reqpar.pchan = ft->signal.channels;
    else
      reqpar.rchan = ft->signal.channels;
  }
  switch (ft->encoding.encoding) {
  case SOX_ENCODING_SIGN2:
    reqpar.sig = 1;
    break;
  case SOX_ENCODING_UNSIGNED:
    reqpar.sig = 0;
    break;
  default:
    break;  /* use device default */
  }
  if (ft->encoding.bits_per_sample > 0)
    reqpar.bits = ft->encoding.bits_per_sample;
  else if (ft->signal.precision > 0)
    reqpar.bits = ft->signal.precision;
  else
    reqpar.bits = SOX_DEFAULT_PRECISION;
  reqpar.bps = (reqpar.bits + 7) / 8;
  reqpar.msb = 1;
  if (ft->encoding.reverse_bytes != sox_option_default) {
    reqpar.le = SIO_LE_NATIVE;
    if (ft->encoding.reverse_bytes)
      reqpar.le = !reqpar.le;
  }
  if (!sio_setpar(p->hdl, &reqpar) ||
      !sio_getpar(p->hdl, &p->par))
    goto failed;
  ft->signal.channels = (mode == SIO_PLAY) ? p->par.pchan : p->par.rchan;
  ft->signal.precision = p->par.bits;
  ft->signal.rate = p->par.rate;
  ft->encoding.encoding = p->par.sig ? SOX_ENCODING_SIGN2 : SOX_ENCODING_UNSIGNED;
  ft->encoding.bits_per_sample = p->par.bps * 8;
  ft->encoding.reverse_bytes = SIO_LE_NATIVE ? !p->par.le : p->par.le;
  ft->encoding.reverse_nibbles = sox_option_no;
  ft->encoding.reverse_bits = sox_option_no;

  if (!sio_start(p->hdl))
    goto failed;
  return SOX_SUCCESS;
 failed:
  sio_close(p->hdl);
  return SOX_EOF;
}

static int stopany(sox_format_t *ft)
{
  sio_close(((struct sndio_priv *)ft->priv)->hdl);
  return SOX_SUCCESS;
}

static int startread(sox_format_t *ft)
{
  return startany(ft, SIO_REC);
}

static int startwrite(sox_format_t *ft)
{
  return startany(ft, SIO_PLAY);
}

static size_t readsamples(sox_format_t *ft, sox_sample_t *buf, size_t len)
{
  struct sndio_priv *p = (struct sndio_priv *)ft->priv;
  unsigned char partial[4];
  unsigned cpb, cc, pc;
  size_t todo, n;

  pc = 0;
  todo = len * p->par.bps;
  cpb = SNDIO_BUFSZ - (SNDIO_BUFSZ % p->par.bps);
  while (todo > 0) {
    memcpy(p->buf, partial, (size_t)pc);
    cc = cpb - pc;
    if (cc > todo)
      cc = todo;
    n = sio_read(p->hdl, p->buf + pc, (size_t)cc);
    if (n == 0 && sio_eof(p->hdl))
      break;
    n += pc;
    pc = n % p->par.bps;
    n -= pc;
    memcpy(partial, p->buf + n, (size_t)pc);
    decode(&p->par, p->buf, buf, (unsigned)(n / p->par.bps));
    buf += n / p->par.bps;
    todo -= n;
  }
  return len - todo / p->par.bps;
}

static size_t writesamples(sox_format_t *ft, const sox_sample_t *buf, size_t len)
{
  struct sndio_priv *p = (struct sndio_priv *)ft->priv;
  unsigned sc, spb;
  size_t n, todo;

  todo = len;
  spb = SNDIO_BUFSZ / p->par.bps;
  while (todo > 0) {
    sc = spb;
    if (sc > todo)
      sc = todo;
    encode(&p->par, buf, p->buf, sc);
    n = sio_write(p->hdl, p->buf, (size_t)(sc * p->par.bps));
    if (n == 0 && sio_eof(p->hdl))
      break;
    n /= p->par.bps;
    todo -= n;
    buf += n;
  }
  return len - todo;
}

LSX_FORMAT_HANDLER(sndio)
{
  static char const * const names[] = {"sndio", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 32, 24, 16, 8, 0,
    SOX_ENCODING_UNSIGNED, 32, 24, 16, 8, 0,
    0
  };
  static sox_format_handler_t const handler = {
    SOX_LIB_VERSION_CODE,
    "libsndio device driver",
    names,
    SOX_FILE_DEVICE | SOX_FILE_NOSTDIO,
    startread, readsamples, stopany,
    startwrite, writesamples, stopany,
    NULL, write_encodings, NULL,
    sizeof(struct sndio_priv)
  };
  return &handler;
}
