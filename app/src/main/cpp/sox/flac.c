/* libSoX file format: FLAC   (c) 2006-7 robs@users.sourceforge.net
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

#include <string.h>
/* Next line for systems that don't define off_t when you #include
   stdio.h; apparently OS/2 has this bug */
#include <sys/types.h>

#include <FLAC/all.h>

#define MAX_COMPRESSION 8


typedef struct {
  /* Info: */
  unsigned bits_per_sample;
  unsigned channels;
  unsigned sample_rate;
  uint64_t total_samples;

  /* Decode buffer: */
  sox_sample_t *req_buffer; /* this may be on the stack */
  size_t number_of_requested_samples;
  sox_sample_t *leftover_buf; /* heap */
  unsigned number_of_leftover_samples;

  FLAC__StreamDecoder * decoder;
  FLAC__bool eof;
  sox_bool seek_pending;
  uint64_t seek_offset;

  /* Encode buffer: */
  FLAC__int32 * decoded_samples;
  unsigned number_of_samples;

  FLAC__StreamEncoder * encoder;
  FLAC__StreamMetadata * metadata[2];
  unsigned num_metadata;
} priv_t;


static FLAC__StreamDecoderReadStatus decoder_read_callback(FLAC__StreamDecoder const* decoder UNUSED, FLAC__byte buffer[], size_t* bytes, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  if(*bytes > 0) {
    *bytes = lsx_readbuf(ft, buffer, *bytes);
    if(lsx_error(ft))
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    else if(*bytes == 0)
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    else
      return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }
  else
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

static FLAC__StreamDecoderSeekStatus decoder_seek_callback(FLAC__StreamDecoder const* decoder UNUSED, FLAC__uint64 absolute_byte_offset, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  if(lsx_seeki(ft, (off_t)absolute_byte_offset, SEEK_SET) < 0)
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  else
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus decoder_tell_callback(FLAC__StreamDecoder const* decoder UNUSED, FLAC__uint64* absolute_byte_offset, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  off_t pos;
  if((pos = lsx_tell(ft)) < 0)
    return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
  else {
    *absolute_byte_offset = (FLAC__uint64)pos;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
  }
}

static FLAC__StreamDecoderLengthStatus decoder_length_callback(FLAC__StreamDecoder const* decoder UNUSED, FLAC__uint64* stream_length, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  *stream_length = lsx_filelength(ft);
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool decoder_eof_callback(FLAC__StreamDecoder const* decoder UNUSED, void* ft_data)
{
  sox_format_t* ft = (sox_format_t*)ft_data;
  return lsx_eof(ft) ? 1 : 0;
}

static void decoder_metadata_callback(FLAC__StreamDecoder const * const flac, FLAC__StreamMetadata const * const metadata, void * const client_data)
{
  sox_format_t * ft = (sox_format_t *) client_data;
  priv_t * p = (priv_t *)ft->priv;

  (void) flac;

  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
    p->bits_per_sample = metadata->data.stream_info.bits_per_sample;
    p->channels = metadata->data.stream_info.channels;
    p->sample_rate = metadata->data.stream_info.sample_rate;
    p->total_samples = metadata->data.stream_info.total_samples;
  }
  else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
    const FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
    size_t i;

    if (vc->num_comments == 0)
      return;

    if (ft->oob.comments != NULL) {
      lsx_warn("multiple Vorbis comment block ignored");
      return;
    }

    for (i = 0; i < vc->num_comments; ++i)
      if (vc->comments[i].entry)
        sox_append_comment(&ft->oob.comments, (char const *) vc->comments[i].entry);
  }
}



static void decoder_error_callback(FLAC__StreamDecoder const * const flac, FLAC__StreamDecoderErrorStatus const status, void * const client_data)
{
  sox_format_t * ft = (sox_format_t *) client_data;

  (void) flac;

  lsx_fail_errno(ft, SOX_EINVAL, "%s", FLAC__StreamDecoderErrorStatusString[status]);
}



static FLAC__StreamDecoderWriteStatus decoder_write_callback(FLAC__StreamDecoder const * const flac, FLAC__Frame const * const frame, FLAC__int32 const * const buffer[], void * const client_data)
{
  sox_format_t * ft = (sox_format_t *) client_data;
  priv_t * p = (priv_t *)ft->priv;
  sox_sample_t * dst = p->req_buffer;
  unsigned channel;
  unsigned nsamples = frame->header.blocksize;
  unsigned sample = 0;
  size_t actual = nsamples * p->channels;

  (void) flac;

  if (frame->header.bits_per_sample != p->bits_per_sample || frame->header.channels != p->channels || frame->header.sample_rate != p->sample_rate) {
    lsx_fail_errno(ft, SOX_EINVAL, "FLAC ERROR: parameters differ between frame and header");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }
  if (dst == NULL) {
    lsx_warn("FLAC ERROR: entered write callback without a buffer (SoX bug)");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }

  /* FLAC may give us too much data, prepare the leftover buffer */
  if (actual > p->number_of_requested_samples) {
    size_t to_stash = actual - p->number_of_requested_samples;

    p->leftover_buf = lsx_malloc(to_stash * sizeof(sox_sample_t));
    p->number_of_leftover_samples = to_stash;
    nsamples = p->number_of_requested_samples / p->channels;

    p->req_buffer += p->number_of_requested_samples;
    p->number_of_requested_samples = 0;
  } else {
    p->req_buffer += actual;
    p->number_of_requested_samples -= actual;
  }

leftover_copy:

  for (; sample < nsamples; sample++) {
    for (channel = 0; channel < p->channels; channel++) {
      FLAC__int32 d = buffer[channel][sample];
      switch (p->bits_per_sample) {
      case  8: *dst++ = SOX_SIGNED_8BIT_TO_SAMPLE(d,); break;
      case 16: *dst++ = SOX_SIGNED_16BIT_TO_SAMPLE(d,); break;
      case 24: *dst++ = SOX_SIGNED_24BIT_TO_SAMPLE(d,); break;
      case 32: *dst++ = SOX_SIGNED_32BIT_TO_SAMPLE(d,); break;
      }
    }
  }

  /* copy into the leftover buffer if we've prepared it */
  if (sample < frame->header.blocksize) {
    nsamples = frame->header.blocksize;
    dst = p->leftover_buf;
    goto leftover_copy;
  }

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



static int start_read(sox_format_t * const ft)
{
  priv_t * p = (priv_t *)ft->priv;
  lsx_debug("API version %u", FLAC_API_VERSION_CURRENT);
  p->decoder = FLAC__stream_decoder_new();
  if (p->decoder == NULL) {
    lsx_fail_errno(ft, SOX_ENOMEM, "FLAC ERROR creating the decoder instance");
    return SOX_EOF;
  }

  FLAC__stream_decoder_set_md5_checking(p->decoder, sox_true);
  FLAC__stream_decoder_set_metadata_respond_all(p->decoder);
  if (FLAC__stream_decoder_init_stream(
      p->decoder,
      decoder_read_callback,
      ft->seekable ? decoder_seek_callback : NULL,
      ft->seekable ? decoder_tell_callback : NULL,
      ft->seekable ? decoder_length_callback : NULL,
      ft->seekable ? decoder_eof_callback : NULL,
      decoder_write_callback,
      decoder_metadata_callback,
      decoder_error_callback,
      ft) != FLAC__STREAM_DECODER_INIT_STATUS_OK){
    lsx_fail_errno(ft, SOX_EHDR, "FLAC ERROR initialising decoder");
    return SOX_EOF;
  }

  if (!FLAC__stream_decoder_process_until_end_of_metadata(p->decoder)) {
    lsx_fail_errno(ft, SOX_EHDR, "FLAC ERROR whilst decoding metadata");
    return SOX_EOF;
  }

  if (FLAC__stream_decoder_get_state(p->decoder) > FLAC__STREAM_DECODER_END_OF_STREAM) {
    lsx_fail_errno(ft, SOX_EHDR, "FLAC ERROR during metadata decoding");
    return SOX_EOF;
  }

  ft->encoding.encoding = SOX_ENCODING_FLAC;
  ft->signal.rate = p->sample_rate;
  ft->encoding.bits_per_sample = p->bits_per_sample;
  ft->signal.channels = p->channels;
  ft->signal.length = p->total_samples * p->channels;
  return SOX_SUCCESS;
}


static size_t read_samples(sox_format_t * const ft, sox_sample_t * sampleBuffer, size_t const requested)
{
  priv_t * p = (priv_t *)ft->priv;
  size_t prev_requested;

  if (p->seek_pending) {
    p->seek_pending = sox_false; 

    /* discard leftover decoded data */
    free(p->leftover_buf);
    p->leftover_buf = NULL;
    p->number_of_leftover_samples = 0;

    p->req_buffer = sampleBuffer;
    p->number_of_requested_samples = requested;

    /* calls decoder_write_callback */
    if (!FLAC__stream_decoder_seek_absolute(p->decoder, (FLAC__uint64)(p->seek_offset / ft->signal.channels))) {
      p->req_buffer = NULL;
      return 0;
    }
  } else if (p->number_of_leftover_samples > 0) {

    /* small request, no need to decode more samples since we have leftovers */
    if (requested < p->number_of_leftover_samples) {
      size_t req_bytes = requested * sizeof(sox_sample_t);

      memcpy(sampleBuffer, p->leftover_buf, req_bytes);
      p->number_of_leftover_samples -= requested;
      memmove(p->leftover_buf, (char *)p->leftover_buf + req_bytes,
              (size_t)p->number_of_leftover_samples * sizeof(sox_sample_t));
      return requested;
    }

    /* first, give them all of our leftover data: */
    memcpy(sampleBuffer, p->leftover_buf,
           p->number_of_leftover_samples * sizeof(sox_sample_t));

    p->req_buffer = sampleBuffer + p->number_of_leftover_samples;
    p->number_of_requested_samples = requested - p->number_of_leftover_samples;

    free(p->leftover_buf);
    p->leftover_buf = NULL;
    p->number_of_leftover_samples = 0;

    /* continue invoking decoder below */
  } else {
    p->req_buffer = sampleBuffer;
    p->number_of_requested_samples = requested;
  }

  /* invoke the decoder, calls decoder_write_callback */
  while ((prev_requested = p->number_of_requested_samples) && !p->eof) {
    if (!FLAC__stream_decoder_process_single(p->decoder))
      break; /* error, but maybe got earlier in the loop, though */

    /* number_of_requested_samples decrements as the decoder progresses */
    if (p->number_of_requested_samples == prev_requested)
      p->eof = sox_true;
  }
  p->req_buffer = NULL;

  return requested - p->number_of_requested_samples;
}



static int stop_read(sox_format_t * const ft)
{
  priv_t * p = (priv_t *)ft->priv;
  if (!FLAC__stream_decoder_finish(p->decoder) && p->eof)
    lsx_warn("decoder MD5 checksum mismatch.");
  FLAC__stream_decoder_delete(p->decoder);

  free(p->leftover_buf);
  p->leftover_buf = NULL;
  p->number_of_leftover_samples = 0;
  return SOX_SUCCESS;
}



static FLAC__StreamEncoderWriteStatus flac_stream_encoder_write_callback(FLAC__StreamEncoder const * const flac, const FLAC__byte buffer[], size_t const bytes, unsigned const samples, unsigned const current_frame, void * const client_data)
{
  sox_format_t * const ft = (sox_format_t *) client_data;
  (void) flac, (void) samples, (void) current_frame;

  return lsx_writebuf(ft, buffer, bytes) == bytes ? FLAC__STREAM_ENCODER_WRITE_STATUS_OK : FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}



static void flac_stream_encoder_metadata_callback(FLAC__StreamEncoder const * encoder, FLAC__StreamMetadata const * metadata, void * client_data)
{
  (void) encoder, (void) metadata, (void) client_data;
}



static FLAC__StreamEncoderSeekStatus flac_stream_encoder_seek_callback(FLAC__StreamEncoder const * encoder, FLAC__uint64 absolute_byte_offset, void * client_data)
{
  sox_format_t * const ft = (sox_format_t *) client_data;
  (void) encoder;
  if (!ft->seekable)
    return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
  else if (lsx_seeki(ft, (off_t)absolute_byte_offset, SEEK_SET) != SOX_SUCCESS)
    return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
  else
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}



static FLAC__StreamEncoderTellStatus flac_stream_encoder_tell_callback(FLAC__StreamEncoder const * encoder, FLAC__uint64 * absolute_byte_offset, void * client_data)
{
  sox_format_t * const ft = (sox_format_t *) client_data;
  off_t pos;
  (void) encoder;
  if (!ft->seekable)
    return FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED;
  else if ((pos = lsx_tell(ft)) < 0)
    return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
  else {
    *absolute_byte_offset = (FLAC__uint64)pos;
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
  }
}



static int start_write(sox_format_t * const ft)
{
  priv_t * p = (priv_t *)ft->priv;
  FLAC__StreamEncoderInitStatus status;
  unsigned compression_level = MAX_COMPRESSION; /* Default to "best" */

  if (ft->encoding.compression != HUGE_VAL) {
    compression_level = ft->encoding.compression;
    if (compression_level != ft->encoding.compression ||
        compression_level > MAX_COMPRESSION) {
      lsx_fail_errno(ft, SOX_EINVAL,
                 "FLAC compression level must be a whole number from 0 to %i",
                 MAX_COMPRESSION);
      return SOX_EOF;
    }
  }

  p->encoder = FLAC__stream_encoder_new();
  if (p->encoder == NULL) {
    lsx_fail_errno(ft, SOX_ENOMEM, "FLAC ERROR creating the encoder instance");
    return SOX_EOF;
  }

  p->bits_per_sample = ft->encoding.bits_per_sample;
  ft->signal.precision = ft->encoding.bits_per_sample;

  lsx_report("encoding at %i bits per sample", p->bits_per_sample);

  FLAC__stream_encoder_set_channels(p->encoder, ft->signal.channels);
  FLAC__stream_encoder_set_bits_per_sample(p->encoder, p->bits_per_sample);
  FLAC__stream_encoder_set_sample_rate(p->encoder, (unsigned)(ft->signal.rate + .5));

  { /* Check if rate is streamable: */
    static const unsigned streamable_rates[] =
      {8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000};
    size_t i;
    sox_bool streamable = sox_false;
    for (i = 0; !streamable && i < array_length(streamable_rates); ++i)
       streamable = (streamable_rates[i] == ft->signal.rate);
    if (!streamable) {
      lsx_report("non-standard rate; output may not be streamable");
      FLAC__stream_encoder_set_streamable_subset(p->encoder, sox_false);
    }
  }

#if FLAC_API_VERSION_CURRENT >= 10
  FLAC__stream_encoder_set_compression_level(p->encoder, compression_level);
#else
  {
    static struct {
      unsigned blocksize;
      FLAC__bool do_exhaustive_model_search;
      FLAC__bool do_mid_side_stereo;
      FLAC__bool loose_mid_side_stereo;
      unsigned max_lpc_order;
      unsigned max_residual_partition_order;
      unsigned min_residual_partition_order;
    } const options[MAX_COMPRESSION + 1] = {
      {1152, sox_false, sox_false, sox_false, 0, 2, 2},
      {1152, sox_false, sox_true, sox_true, 0, 2, 2},
      {1152, sox_false, sox_true, sox_false, 0, 3, 0},
      {4608, sox_false, sox_false, sox_false, 6, 3, 3},
      {4608, sox_false, sox_true, sox_true, 8, 3, 3},
      {4608, sox_false, sox_true, sox_false, 8, 3, 3},
      {4608, sox_false, sox_true, sox_false, 8, 4, 0},
      {4608, sox_true, sox_true, sox_false, 8, 6, 0},
      {4608, sox_true, sox_true, sox_false, 12, 6, 0},
    };
#define SET_OPTION(x) do {\
  lsx_report(#x" = %i", options[compression_level].x); \
  FLAC__stream_encoder_set_##x(p->encoder, options[compression_level].x);\
} while (0)
    SET_OPTION(blocksize);
    SET_OPTION(do_exhaustive_model_search);
    SET_OPTION(max_lpc_order);
    SET_OPTION(max_residual_partition_order);
    SET_OPTION(min_residual_partition_order);
    if (ft->signal.channels == 2) {
      SET_OPTION(do_mid_side_stereo);
      SET_OPTION(loose_mid_side_stereo);
    }
#undef SET_OPTION
  }
#endif

  if (ft->signal.length != 0) {
    FLAC__stream_encoder_set_total_samples_estimate(p->encoder, (FLAC__uint64)(ft->signal.length / ft->signal.channels));

    p->metadata[p->num_metadata] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
    if (p->metadata[p->num_metadata] == NULL) {
      lsx_fail_errno(ft, SOX_ENOMEM, "FLAC ERROR creating the encoder seek table template");
      return SOX_EOF;
    }
    {
      if (!FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(p->metadata[p->num_metadata], (unsigned)(10 * ft->signal.rate + .5), (FLAC__uint64)(ft->signal.length/ft->signal.channels))) {
        lsx_fail_errno(ft, SOX_ENOMEM, "FLAC ERROR creating the encoder seek table points");
        return SOX_EOF;
      }
    }
    p->metadata[p->num_metadata]->is_last = sox_false; /* the encoder will set this for us */
    ++p->num_metadata;
  }

  if (ft->oob.comments) {     /* Make the comment structure */
    FLAC__StreamMetadata_VorbisComment_Entry entry;
    int i;

    p->metadata[p->num_metadata] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
    for (i = 0; ft->oob.comments[i]; ++i) {
      static const char prepend[] = "Comment=";
      char * text = lsx_calloc(strlen(prepend) + strlen(ft->oob.comments[i]) + 1, sizeof(*text));
      /* Prepend `Comment=' if no field-name already in the comment */
      if (!strchr(ft->oob.comments[i], '='))
        strcpy(text, prepend);
      entry.entry = (FLAC__byte *) strcat(text, ft->oob.comments[i]);
      entry.length = strlen(text);
      FLAC__metadata_object_vorbiscomment_append_comment(p->metadata[p->num_metadata], entry, /*copy= */ sox_true);
      free(text);
    }
    ++p->num_metadata;
  }

  if (p->num_metadata)
    FLAC__stream_encoder_set_metadata(p->encoder, p->metadata, p->num_metadata);

  status = FLAC__stream_encoder_init_stream(p->encoder, flac_stream_encoder_write_callback,
      flac_stream_encoder_seek_callback, flac_stream_encoder_tell_callback, flac_stream_encoder_metadata_callback, ft);

  if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
    lsx_fail_errno(ft, SOX_EINVAL, "%s", FLAC__StreamEncoderInitStatusString[status]);
    return SOX_EOF;
  }
  return SOX_SUCCESS;
}



static size_t write_samples(sox_format_t * const ft, sox_sample_t const * const sampleBuffer, size_t const len)
{
  priv_t * p = (priv_t *)ft->priv;
  unsigned i;

  /* allocate or grow buffer */
  if (p->number_of_samples < len) {
    p->number_of_samples = len;
    free(p->decoded_samples);
    p->decoded_samples = lsx_malloc(p->number_of_samples * sizeof(FLAC__int32));
  }

  for (i = 0; i < len; ++i) {
    SOX_SAMPLE_LOCALS;
    long pcm = SOX_SAMPLE_TO_SIGNED_32BIT(sampleBuffer[i], ft->clips);
    p->decoded_samples[i] = pcm >> (32 - p->bits_per_sample);
    switch (p->bits_per_sample) {
      case  8: p->decoded_samples[i] =
          SOX_SAMPLE_TO_SIGNED_8BIT(sampleBuffer[i], ft->clips);
        break;
      case 16: p->decoded_samples[i] =
          SOX_SAMPLE_TO_SIGNED_16BIT(sampleBuffer[i], ft->clips);
        break;
      case 24: p->decoded_samples[i] = /* sign extension: */
          SOX_SAMPLE_TO_SIGNED_24BIT(sampleBuffer[i],ft->clips) << 8;
        p->decoded_samples[i] >>= 8;
        break;
      case 32: p->decoded_samples[i] =
          SOX_SAMPLE_TO_SIGNED_32BIT(sampleBuffer[i],ft->clips);
        break;
    }
  }
  FLAC__stream_encoder_process_interleaved(p->encoder, p->decoded_samples, (unsigned) len / ft->signal.channels);
  return FLAC__stream_encoder_get_state(p->encoder) == FLAC__STREAM_ENCODER_OK ? len : 0;
}



static int stop_write(sox_format_t * const ft)
{
  priv_t * p = (priv_t *)ft->priv;
  FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(p->encoder);
  unsigned i;

  FLAC__stream_encoder_finish(p->encoder);
  FLAC__stream_encoder_delete(p->encoder);
  for (i = 0; i < p->num_metadata; ++i)
    FLAC__metadata_object_delete(p->metadata[i]);
  free(p->decoded_samples);
  if (state != FLAC__STREAM_ENCODER_OK) {
    lsx_fail_errno(ft, SOX_EINVAL, "FLAC ERROR: failed to encode to end of stream");
    return SOX_EOF;
  }
  return SOX_SUCCESS;
}



static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t * p = (priv_t *)ft->priv;
  p->seek_offset = offset;
  p->seek_pending = sox_true;
  return ft->mode == 'r' ? SOX_SUCCESS : SOX_EOF;
}



LSX_FORMAT_HANDLER(flac)
{
  static char const * const names[] = {"flac", NULL};
  static unsigned const encodings[] = {SOX_ENCODING_FLAC, 8, 16, 24, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Free Lossless Audio CODEC compressed audio", names, 0,
    start_read, read_samples, stop_read,
    start_write, write_samples, stop_write,
    seek, encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
