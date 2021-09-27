/* libSoX Opus-in-Ogg sound format handler
 * Copyright (C) 2013 John Stumpo <stump@jstump.com>
 *
 * Largely based on vorbis.c:
 * libSoX Ogg Vorbis sound format handler
 * Copyright 2001, Stan Seibert <indigo@aztec.asu.edu>
 *
 * Portions from oggenc, (c) Michael Smith <msmith@labyrinth.net.au>,
 * ogg123, (c) Kenneth Arnold <kcarnold@yahoo.com>, and
 * libvorbisfile (c) Xiphophorus Company
 *
 * May 9, 2001 - Stan Seibert (indigo@aztec.asu.edu)
 * Ogg Vorbis handler initially written.
 *
 * July 5, 1991 - Skeleton file
 * Copyright 1991 Lance Norskog And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Lance Norskog And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <opusfile.h>

#define DEF_BUF_LEN 4096

#define BUF_ERROR -1
#define BUF_EOF  0
#define BUF_DATA 1

typedef struct {
  /* Decoding data */
  OggOpusFile *of;
  char *buf;
  size_t buf_len;
  size_t start;
  size_t end;     /* Unsent data samples in buf[start] through buf[end-1] */
  int current_section;
  int eof;
} priv_t;

/******** Callback functions used in op_open_callbacks ************/

static int callback_read(void* ft_data, unsigned char* ptr, int nbytes)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  return lsx_readbuf(ft, ptr, (size_t)nbytes);
}

static int callback_seek(void* ft_data, opus_int64 off, int whence)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  int ret = ft->seekable ? lsx_seeki(ft, (off_t)off, whence) : -1;

  if (ret == EBADF)
    ret = -1;
  return ret;
}

static int callback_close(void* ft_data UNUSED)
{
  /* Do nothing so sox can close the file for us */
  return 0;
}

static opus_int64 callback_tell(void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  return lsx_tell(ft);
}

/********************* End callbacks *****************************/


/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int startread(sox_format_t * ft)
{
  priv_t * vb = (priv_t *) ft->priv;
  const OpusTags *ot;
  int i;

  OpusFileCallbacks callbacks = {
    callback_read,
    callback_seek,
    callback_tell,
    callback_close
  };

  /* Init the decoder */
  vb->of = op_open_callbacks(ft, &callbacks, NULL, (size_t) 0, NULL);
  if (vb->of == NULL) {
    lsx_fail_errno(ft, SOX_EHDR, "Input not an Ogg Opus audio stream");
    return (SOX_EOF);
  }

  /* Get info about the Opus stream */
  ot = op_tags(vb->of, -1);

  /* Record audio info */
  ft->signal.rate = 48000;  /* libopusfile always uses 48 kHz */
  ft->encoding.encoding = SOX_ENCODING_OPUS;
  ft->signal.channels = op_channel_count(vb->of, -1);

  /* op_pcm_total doesn't work on non-seekable files so
   * skip that step in that case.  Also, it reports
   * "frame"-ish results so we must * channels.
   */
  if (ft->seekable)
    ft->signal.length = op_pcm_total(vb->of, -1) * ft->signal.channels;

  /* Record comments */
  for (i = 0; i < ot->comments; i++)
    sox_append_comment(&ft->oob.comments, ot->user_comments[i]);

  /* Setup buffer */
  vb->buf_len = DEF_BUF_LEN;
  vb->buf_len -= vb->buf_len % (ft->signal.channels*2); /* 2 bytes per sample */
  vb->buf = lsx_calloc(vb->buf_len, sizeof(char));
  vb->start = vb->end = 0;

  /* Fill in other info */
  vb->eof = 0;
  vb->current_section = -1;

  return (SOX_SUCCESS);
}


/* Refill the buffer with samples.  Returns BUF_EOF if the end of the
 * Opus data was reached while the buffer was being filled,
 * BUF_ERROR is something bad happens, and BUF_DATA otherwise */
static int refill_buffer(sox_format_t * ft)
{
  priv_t * vb = (priv_t *) ft->priv;
  int num_read;

  if (vb->start == vb->end)     /* Samples all played */
    vb->start = vb->end = 0;

  while (vb->end < vb->buf_len) {
    num_read = op_read(vb->of, (opus_int16*) (vb->buf + vb->end),
        (int) ((vb->buf_len - vb->end) / sizeof(opus_int16)),
        &vb->current_section);
    if (num_read == 0)
      return (BUF_EOF);
    else if (num_read == OP_HOLE)
      lsx_warn("Warning: hole in stream; probably harmless");
    else if (num_read < 0)
      return (BUF_ERROR);
    else
      vb->end += num_read * sizeof(opus_int16) * ft->signal.channels;
  }
  return (BUF_DATA);
}


/*
 * Read up to len samples from file.
 * Convert to signed longs.
 * Place in buf[].
 * Return number of samples read.
 */

static size_t read_samples(sox_format_t * ft, sox_sample_t * buf, size_t len)
{
  priv_t * vb = (priv_t *) ft->priv;
  size_t i;
  int ret;
  sox_sample_t l;


  for (i = 0; i < len; i++) {
    if (vb->start == vb->end) {
      if (vb->eof)
        break;
      ret = refill_buffer(ft);
      if (ret == BUF_EOF || ret == BUF_ERROR) {
        vb->eof = 1;
        if (vb->end == 0)
          break;
      }
    }

    l = (vb->buf[vb->start + 1] << 24)
        | (0xffffff & (vb->buf[vb->start] << 16));
    *(buf + i) = l;
    vb->start += 2;
  }
  return i;
}

/*
 * Do anything required when you stop reading samples.
 * Don't close input file!
 */
static int stopread(sox_format_t * ft)
{
  priv_t * vb = (priv_t *) ft->priv;

  free(vb->buf);
  op_free(vb->of);

  return (SOX_SUCCESS);
}

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * vb = (priv_t *) ft->priv;

  return op_pcm_seek(vb->of, (opus_int64)(offset / ft->signal.channels))? SOX_EOF:SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(opus)
{
  static const char *const names[] = {"opus", NULL};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Xiph.org's Opus lossy compression", names, 0,
    startread, read_samples, stopread,
    NULL, NULL, NULL,
    seek, NULL, NULL, sizeof(priv_t)
  };
  return &handler;
}
