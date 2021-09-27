/* libSoX Ogg Vorbis sound format handler
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

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>

#define DEF_BUF_LEN 4096

#define BUF_ERROR -1
#define BUF_EOF  0
#define BUF_DATA 1

#define HEADER_ERROR 0
#define HEADER_OK   1

/* Private data for Ogg Vorbis file */
typedef struct {
  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;

  vorbis_dsp_state vd;
  vorbis_block vb;
  vorbis_info vi;
} vorbis_enc_t;

typedef struct {
  /* Decoding data */
  OggVorbis_File *vf;
  char *buf;
  size_t buf_len;
  size_t start;
  size_t end;     /* Unsent data samples in buf[start] through buf[end-1] */
  int current_section;
  int eof;

  vorbis_enc_t *vorbis_enc_data;
} priv_t;

/******** Callback functions used in ov_open_callbacks ************/

static size_t callback_read(void* ptr, size_t size, size_t nmemb, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  size_t ret = lsx_readbuf(ft, ptr, size * nmemb);
  return ret / size;
}

static int callback_seek(void* ft_data, ogg_int64_t off, int whence)
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

static long callback_tell(void* ft_data)
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
  vorbis_info *vi;
  vorbis_comment *vc;
  int i;

  ov_callbacks callbacks = {
    callback_read,
    callback_seek,
    callback_close,
    callback_tell
  };

  /* Allocate space for decoding structure */
  vb->vf = lsx_malloc(sizeof(OggVorbis_File));

  /* Init the decoder */
  if (ov_open_callbacks(ft, vb->vf, NULL, (size_t) 0, callbacks) < 0) {
    lsx_fail_errno(ft, SOX_EHDR, "Input not an Ogg Vorbis audio stream");
    return (SOX_EOF);
  }

  /* Get info about the Ogg Vorbis stream */
  vi = ov_info(vb->vf, -1);
  vc = ov_comment(vb->vf, -1);

  /* Record audio info */
  ft->signal.rate = vi->rate;
  ft->encoding.encoding = SOX_ENCODING_VORBIS;
  ft->signal.channels = vi->channels;

  /* ov_pcm_total doesn't work on non-seekable files so
   * skip that step in that case.  Also, it reports
   * "frame"-ish results so we must * channels.
   */
  if (ft->seekable)
    ft->signal.length = ov_pcm_total(vb->vf, -1) * ft->signal.channels;

  /* Record comments */
  for (i = 0; i < vc->comments; i++)
    sox_append_comment(&ft->oob.comments, vc->user_comments[i]);

  /* Setup buffer */
  vb->buf_len = DEF_BUF_LEN;
  vb->buf_len -= vb->buf_len % (vi->channels*2); /* 2 bytes per sample */
  vb->buf = lsx_calloc(vb->buf_len, sizeof(char));
  vb->start = vb->end = 0;

  /* Fill in other info */
  vb->eof = 0;
  vb->current_section = -1;

  return (SOX_SUCCESS);
}


/* Refill the buffer with samples.  Returns BUF_EOF if the end of the
 * vorbis data was reached while the buffer was being filled,
 * BUF_ERROR is something bad happens, and BUF_DATA otherwise */
static int refill_buffer(priv_t * vb)
{
  int num_read;

  if (vb->start == vb->end)     /* Samples all played */
    vb->start = vb->end = 0;

  while (vb->end < vb->buf_len) {
    num_read = ov_read(vb->vf, vb->buf + vb->end,
        (int) (vb->buf_len - vb->end), 0, 2, 1, &vb->current_section);
    if (num_read == 0)
      return (BUF_EOF);
    else if (num_read == OV_HOLE)
      lsx_warn("Warning: hole in stream; probably harmless");
    else if (num_read < 0)
      return (BUF_ERROR);
    else
      vb->end += num_read;
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
      ret = refill_buffer(vb);
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
  ov_clear(vb->vf);

  return (SOX_SUCCESS);
}

/* Write a page of ogg data to a file.  Taken directly from encode.c in
 * oggenc.   Returns the number of bytes written. */
static int oe_write_page(ogg_page * page, sox_format_t * ft)
{
  int written;

  written = lsx_writebuf(ft, page->header, (size_t) page->header_len);
  written += lsx_writebuf(ft, page->body, (size_t) page->body_len);

  return written;
}

/* Write out the header packets.  Derived mostly from encode.c in oggenc.
 * Returns HEADER_OK if the header can be written, HEADER_ERROR otherwise. */
static int write_vorbis_header(sox_format_t * ft, vorbis_enc_t * ve)
{
  ogg_packet header_main;
  ogg_packet header_comments;
  ogg_packet header_codebooks;
  vorbis_comment vc;
  int i, ret = HEADER_OK;

  memset(&vc, 0, sizeof(vc));
  vc.comments = sox_num_comments(ft->oob.comments);
  if (vc.comments) {     /* Make the comment structure */
    vc.comment_lengths = lsx_calloc((size_t)vc.comments, sizeof(*vc.comment_lengths));
    vc.user_comments = lsx_calloc((size_t)vc.comments, sizeof(*vc.user_comments));
    for (i = 0; i < vc.comments; ++i) {
      static const char prepend[] = "Comment=";
      char * text = lsx_calloc(strlen(prepend) + strlen(ft->oob.comments[i]) + 1, sizeof(*text));
      /* Prepend `Comment=' if no field-name already in the comment */
      if (!strchr(ft->oob.comments[i], '='))
        strcpy(text, prepend);
      vc.user_comments[i] = strcat(text, ft->oob.comments[i]);
      vc.comment_lengths[i] = strlen(text);
    }
  }
  if (vorbis_analysis_headerout(    /* Build the packets */
      &ve->vd, &vc, &header_main, &header_comments, &header_codebooks) < 0) {
      ret = HEADER_ERROR;
      goto cleanup;
  }

  ogg_stream_packetin(&ve->os, &header_main);   /* And stream them out */
  ogg_stream_packetin(&ve->os, &header_comments);
  ogg_stream_packetin(&ve->os, &header_codebooks);

  while (ogg_stream_flush(&ve->os, &ve->og) && ret == HEADER_OK)
    if (!oe_write_page(&ve->og, ft))
      ret = HEADER_ERROR;
cleanup:
  for (i = 0; i < vc.comments; ++i)
    free(vc.user_comments[i]);
  free(vc.user_comments);
  free(vc.comment_lengths);
  return ret;
}

static int startwrite(sox_format_t * ft)
{
  priv_t * vb = (priv_t *) ft->priv;
  vorbis_enc_t *ve;
  long rate;
  double quality = 3;           /* Default compression quality gives ~112kbps */

  ft->encoding.encoding = SOX_ENCODING_VORBIS;

  /* Allocate memory for all of the structures */
  ve = vb->vorbis_enc_data = lsx_malloc(sizeof(vorbis_enc_t));

  vorbis_info_init(&ve->vi);

  /* TODO */
  rate = ft->signal.rate;
  if (rate)
    lsx_fail_errno(ft, SOX_EHDR,
      "Error setting-up Ogg Vorbis encoder; check sample-rate & # of channels");

  /* Use encoding to average bit rate of VBR as specified by the -C option */
  if (ft->encoding.compression != HUGE_VAL) {
    if (ft->encoding.compression < -1 || ft->encoding.compression > 10) {
      lsx_fail_errno(ft, SOX_EINVAL,
                     "Vorbis compression quality nust be between -1 and 10");
      return SOX_EOF;
    }
    quality = ft->encoding.compression;
  }

  if (vorbis_encode_init_vbr(&ve->vi, ft->signal.channels, ft->signal.rate + .5, quality / 10))
  {
    lsx_fail_errno(ft, SOX_EFMT, "libVorbis cannot encode this sample-rate or # of channels");
    return SOX_EOF;
  }

  vorbis_analysis_init(&ve->vd, &ve->vi);
  vorbis_block_init(&ve->vd, &ve->vb);

  ogg_stream_init(&ve->os, INT_MAX & (int)RANQD1);  /* Random serial number */

  if (write_vorbis_header(ft, ve) == HEADER_ERROR) {
    lsx_fail_errno(ft, SOX_EHDR,
                   "Error writing header for Ogg Vorbis audio stream");
    return (SOX_EOF);
  }

  return (SOX_SUCCESS);
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t * buf,
                        size_t len)
{
  priv_t * vb = (priv_t *) ft->priv;
  vorbis_enc_t *ve = vb->vorbis_enc_data;
  size_t samples = len / ft->signal.channels;
  float **buffer = vorbis_analysis_buffer(&ve->vd, (int) samples);
  size_t i, j;
  int ret;
  int eos = 0;

  /* Copy samples into vorbis buffer */
  for (i = 0; i < samples; i++)
    for (j = 0; j < ft->signal.channels; j++)
      buffer[j][i] = buf[i * ft->signal.channels + j]
          / ((float) SOX_SAMPLE_MAX);

  vorbis_analysis_wrote(&ve->vd, (int) samples);

  while (vorbis_analysis_blockout(&ve->vd, &ve->vb) == 1) {
    /* Do the main analysis, creating a packet */
    vorbis_analysis(&ve->vb, &ve->op);
    vorbis_bitrate_addblock(&ve->vb);

    /* Add packet to bitstream */
    while (vorbis_bitrate_flushpacket(&ve->vd, &ve->op)) {
      ogg_stream_packetin(&ve->os, &ve->op);

      /* If we've gone over a page boundary, we can do actual
       * output, so do so (for however many pages are available)
       */

      while (!eos) {
        int result = ogg_stream_pageout(&ve->os, &ve->og);

        if (!result)
          break;

        ret = oe_write_page(&ve->og, ft);
        if (!ret)
          return 0;

        if (ogg_page_eos(&ve->og))
          eos = 1;
      }
    }
  }

  return (len);
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * vb = (priv_t *) ft->priv;
  vorbis_enc_t *ve = vb->vorbis_enc_data;

  /* Close out the remaining data */
  write_samples(ft, NULL, (size_t) 0);

  ogg_stream_clear(&ve->os);
  vorbis_block_clear(&ve->vb);
  vorbis_dsp_clear(&ve->vd);
  vorbis_info_clear(&ve->vi);

  return (SOX_SUCCESS);
}

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * vb = (priv_t *) ft->priv;

  return ov_pcm_seek(vb->vf, (ogg_int64_t)(offset / ft->signal.channels))? SOX_EOF:SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(vorbis)
{
  static const char *names[] = {"vorbis", "ogg", NULL};
  static const unsigned encodings[] = {SOX_ENCODING_VORBIS, 0, 0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Xiph.org's ogg-vorbis lossy compression", names, 0,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    seek, encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
