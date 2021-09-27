/* MP3 support for SoX
 *
 * Uses libmad for MP3 decoding
 * libmp3lame for MP3 encoding
 * and libtwolame for MP2 encoding
 *
 * Written by Fabrizio Gennari <fabrizio.ge@tiscali.it>
 *
 * The decoding part is based on the decoder-tutorial program madlld
 * written by Bertrand Petit <madlld@phoe.fmug.org>,
 */

#include "sox_i.h"
#include <string.h>

#if defined(HAVE_LAME_LAME_H) || defined(HAVE_LAME_H) || defined(DL_LAME)
#define HAVE_LAME 1
#endif

#if defined(HAVE_TWOLAME_H) || defined(DL_TWOLAME)
  #define HAVE_TWOLAME 1
#endif

#if defined(HAVE_MAD_H) || defined(HAVE_LAME) || defined(HAVE_TWOLAME)

#ifdef HAVE_MAD_H
#include <mad.h>
#endif

#if defined(HAVE_LAME_LAME_H)
#include <lame/lame.h>
#elif defined(HAVE_LAME_H)
#include <lame.h>
#elif defined(DL_LAME)
typedef struct lame_global_struct lame_global_flags;
typedef enum {
  vbr_off=0,
  vbr_default=4
} vbr_mode;
#endif

#if defined(HAVE_ID3TAG) && (defined(HAVE_IO_H) || defined(HAVE_UNISTD_H))
#define USING_ID3TAG 1
#endif

#ifdef USING_ID3TAG
  #include <id3tag.h>
  #include "id3.h"
#if defined(HAVE_UNISTD_H)
  #include <unistd.h>
#elif defined(HAVE_IO_H)
  #include <io.h>
#endif
#else
  #define ID3_TAG_FLAG_FOOTERPRESENT 0x10
#endif

#ifdef HAVE_TWOLAME_H
  #include <twolame.h>
#endif

/* Under Windows, importing data from DLLs is a dicey proposition. This is true
 * when using dlopen, but also true if linking directly against the DLL if the
 * header does not mark the data as __declspec(dllexport), which mad.h does not.
 * Sidestep the issue by defining our own mad_timer_zero. This is needed because
 * mad_timer_zero is used in some of the mad.h macros.
 */
#ifdef HAVE_MAD_H
#define mad_timer_zero mad_timer_zero_stub
static mad_timer_t const mad_timer_zero_stub = {0, 0};
#endif

#define MAXFRAMESIZE 2880
#define ID3PADDING 128

/* LAME takes float values as input. */
#define MP3_LAME_PRECISION 24

/* MAD returns values with MAD_F_FRACBITS (28) bits of precision, though it's
   not certain that all of them are meaningful. Default to 16 bits to
   align with most users expectation of output file should be 16 bits. */
#define MP3_MAD_PRECISION  16

static const char* const mad_library_names[] =
{
#ifdef DL_MAD
    "libmad",
    "libmad-0",
    "cygmad-0",
#endif
    NULL
};

#ifdef DL_MAD
  #define MAD_FUNC LSX_DLENTRY_DYNAMIC
#else
  #define MAD_FUNC LSX_DLENTRY_STATIC
#endif

#define MAD_FUNC_ENTRIES(f,x) \
  MAD_FUNC(f,x, void, mad_stream_buffer, (struct mad_stream *, unsigned char const *, unsigned long)) \
  MAD_FUNC(f,x, void, mad_stream_skip, (struct mad_stream *, unsigned long)) \
  MAD_FUNC(f,x, int, mad_stream_sync, (struct mad_stream *)) \
  MAD_FUNC(f,x, void, mad_stream_init, (struct mad_stream *)) \
  MAD_FUNC(f,x, void, mad_frame_init, (struct mad_frame *)) \
  MAD_FUNC(f,x, void, mad_synth_init, (struct mad_synth *)) \
  MAD_FUNC(f,x, int, mad_frame_decode, (struct mad_frame *, struct mad_stream *)) \
  MAD_FUNC(f,x, void, mad_timer_add, (mad_timer_t *, mad_timer_t)) \
  MAD_FUNC(f,x, void, mad_synth_frame, (struct mad_synth *, struct mad_frame const *)) \
  MAD_FUNC(f,x, char const *, mad_stream_errorstr, (struct mad_stream const *)) \
  MAD_FUNC(f,x, void, mad_frame_finish, (struct mad_frame *)) \
  MAD_FUNC(f,x, void, mad_stream_finish, (struct mad_stream *)) \
  MAD_FUNC(f,x, unsigned long, mad_bit_read, (struct mad_bitptr *, unsigned int)) \
  MAD_FUNC(f,x, int, mad_header_decode, (struct mad_header *, struct mad_stream *)) \
  MAD_FUNC(f,x, void, mad_header_init, (struct mad_header *)) \
  MAD_FUNC(f,x, signed long, mad_timer_count, (mad_timer_t, enum mad_units)) \
  MAD_FUNC(f,x, void, mad_timer_multiply, (mad_timer_t *, signed long))

static const char* const lame_library_names[] =
{
#ifdef DL_LAME
  "libmp3lame",
  "libmp3lame-0",
  "lame-enc",
  "cygmp3lame-0",
#endif
  NULL
};

#ifdef DL_LAME
  #define LAME_FUNC           LSX_DLENTRY_DYNAMIC
#else /* DL_LAME */
  #define LAME_FUNC           LSX_DLENTRY_STATIC
#endif /* DL_LAME */

#define LAME_FUNC_ENTRIES(f,x) \
  LAME_FUNC(f,x, lame_global_flags*, lame_init, (void)) \
  LAME_FUNC(f,x, int, lame_set_errorf, (lame_global_flags *, void (*)(const char *, va_list))) \
  LAME_FUNC(f,x, int, lame_set_debugf, (lame_global_flags *, void (*)(const char *, va_list))) \
  LAME_FUNC(f,x, int, lame_set_msgf, (lame_global_flags *, void (*)(const char *, va_list))) \
  LAME_FUNC(f,x, int, lame_set_num_samples, (lame_global_flags *, unsigned long)) \
  LAME_FUNC(f,x, int, lame_get_num_channels, (const lame_global_flags *)) \
  LAME_FUNC(f,x, int, lame_set_num_channels, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_set_in_samplerate, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_set_out_samplerate, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_set_bWriteVbrTag, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_set_brate, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_set_quality, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, vbr_mode, lame_get_VBR, (const lame_global_flags *)) \
  LAME_FUNC(f,x, int, lame_set_VBR, (lame_global_flags *, vbr_mode)) \
  LAME_FUNC(f,x, int, lame_set_VBR_q, (lame_global_flags *, int)) \
  LAME_FUNC(f,x, int, lame_init_params, (lame_global_flags *)) \
  LAME_FUNC(f,x, int, lame_encode_buffer_float, (lame_global_flags *, const float[], const float[], const int, unsigned char *, const int)) \
  LAME_FUNC(f,x, int, lame_encode_flush, (lame_global_flags *, unsigned char *, int)) \
  LAME_FUNC(f,x, int, lame_close, (lame_global_flags *)) \
  LAME_FUNC(f,x, size_t, lame_get_lametag_frame, (const lame_global_flags *, unsigned char*, size_t)) \
  LAME_FUNC(f,x, void, id3tag_init, (lame_global_flags *)) \
  LAME_FUNC(f,x, void, id3tag_set_title, (lame_global_flags *, const char* title)) \
  LAME_FUNC(f,x, void, id3tag_set_artist, (lame_global_flags *, const char* artist)) \
  LAME_FUNC(f,x, void, id3tag_set_album, (lame_global_flags *, const char* album)) \
  LAME_FUNC(f,x, void, id3tag_set_year, (lame_global_flags *, const char* year)) \
  LAME_FUNC(f,x, void, id3tag_set_comment, (lame_global_flags *, const char* comment)) \
  LAME_FUNC(f,x, int, id3tag_set_track, (lame_global_flags *, const char* track)) \
  LAME_FUNC(f,x, int, id3tag_set_genre, (lame_global_flags *, const char* genre)) \
  LAME_FUNC(f,x, size_t, id3tag_set_pad, (lame_global_flags *, size_t)) \
  LAME_FUNC(f,x, size_t, lame_get_id3v2_tag, (lame_global_flags *, unsigned char*, size_t)) \
  LAME_FUNC(f,x, int, id3tag_set_fieldvalue, (lame_global_flags *, const char *))

#ifdef HAVE_TWOLAME
static const char* const twolame_library_names[] =
{
#ifdef DL_TWOLAME
  "libtwolame",
  "libtwolame-0",
#endif
  NULL
};
#endif

#ifdef DL_TWOLAME
  #define TWOLAME_FUNC LSX_DLENTRY_DYNAMIC
#else
  #define TWOLAME_FUNC LSX_DLENTRY_STATIC
#endif

#define TWOLAME_FUNC_ENTRIES(f,x) \
  TWOLAME_FUNC(f,x, twolame_options*, twolame_init, (void)) \
  TWOLAME_FUNC(f,x, int, twolame_get_num_channels, (twolame_options*)) \
  TWOLAME_FUNC(f,x, int, twolame_set_num_channels, (twolame_options*, int)) \
  TWOLAME_FUNC(f,x, int, twolame_set_in_samplerate, (twolame_options *, int)) \
  TWOLAME_FUNC(f,x, int, twolame_set_out_samplerate, (twolame_options *, int)) \
  TWOLAME_FUNC(f,x, int, twolame_set_brate, (twolame_options *, int)) \
  TWOLAME_FUNC(f,x, int, twolame_init_params, (twolame_options *)) \
  TWOLAME_FUNC(f,x, int, twolame_encode_buffer_float32_interleaved, (twolame_options *, const float [], int, unsigned char *, int)) \
  TWOLAME_FUNC(f,x, int, twolame_encode_flush, (twolame_options *, unsigned char *, int)) \
  TWOLAME_FUNC(f,x, void, twolame_close, (twolame_options **))

/* Private data */
typedef struct mp3_priv_t {
  unsigned char *mp3_buffer;
  size_t mp3_buffer_size;

#ifdef HAVE_MAD_H
  struct mad_stream       Stream;
  struct mad_frame        Frame;
  struct mad_synth        Synth;
  mad_timer_t             Timer;
  ptrdiff_t               cursamp;
  size_t                  FrameCount;
  LSX_DLENTRIES_TO_PTRS(MAD_FUNC_ENTRIES, mad_dl);
#endif /*HAVE_MAD_H*/

#if defined(HAVE_LAME) || defined(HAVE_TWOLAME)
  float *pcm_buffer;
  size_t pcm_buffer_size;
  char mp2;
#endif

#ifdef HAVE_LAME
  lame_global_flags *gfp;
  uint64_t num_samples;
  int vbr_tag;
  LSX_DLENTRIES_TO_PTRS(LAME_FUNC_ENTRIES, lame_dl);
#endif

#ifdef HAVE_TWOLAME
  twolame_options *opt;
  LSX_DLENTRIES_TO_PTRS(TWOLAME_FUNC_ENTRIES, twolame_dl);
#endif
} priv_t;

#ifdef HAVE_MAD_H

/* This function merges the functions tagtype() and id3_tag_query()
   from MAD's libid3tag, so we don't have to link to it
   Returns 0 if the frame is not an ID3 tag, tag length if it is */

static int tagtype(const unsigned char *data, size_t length)
{
    if (length >= 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
    {
        return 128; /* ID3V1 */
    }

    if (length >= 10 &&
        (data[0] == 'I' && data[1] == 'D' && data[2] == '3') &&
        data[3] < 0xff && data[4] < 0xff &&
        data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
    {     /* ID3V2 */
        unsigned char flags;
        unsigned int size;
        flags = data[5];
        size = 10 + (data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9];
        if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
            size += 10;
        for (; size < length && !data[size]; ++size);  /* Consume padding */
        return size;
    }

    return 0;
}

#endif /*HAVE_MAD_H*/

#include "mp3-util.h"

#ifdef HAVE_MAD_H

/*
 * (Re)fill the stream buffer that is to be decoded.  If any data
 * still exists in the buffer then they are first shifted to be
 * front of the stream buffer.
 */
static int sox_mp3_input(sox_format_t * ft)
{
    priv_t *p = (priv_t *) ft->priv;
    size_t bytes_read;
    size_t remaining;

    remaining = p->Stream.bufend - p->Stream.next_frame;

    /* libmad does not consume all the buffer it's given. Some
     * data, part of a truncated frame, is left unused at the
     * end of the buffer. That data must be put back at the
     * beginning of the buffer and taken in account for
     * refilling the buffer. This means that the input buffer
     * must be large enough to hold a complete frame at the
     * highest observable bit-rate (currently 448 kb/s).
     * TODO: Is 2016 bytes the size of the largest frame?
     * (448000*(1152/32000))/8
     */
    memmove(p->mp3_buffer, p->Stream.next_frame, remaining);

    bytes_read = lsx_readbuf(ft, p->mp3_buffer+remaining,
                            p->mp3_buffer_size-remaining);
    if (bytes_read == 0)
    {
        return SOX_EOF;
    }

    p->mad_stream_buffer(&p->Stream, p->mp3_buffer, bytes_read+remaining);
    p->Stream.error = 0;

    return SOX_SUCCESS;
}

/* Attempts to read an ID3 tag at the current location in stream and
 * consume it all.  Returns SOX_EOF if no tag is found.  Its up to
 * caller to recover.
 * */
static int sox_mp3_inputtag(sox_format_t * ft)
{
    priv_t *p = (priv_t *) ft->priv;
    int rc = SOX_EOF;
    size_t remaining;
    size_t tagsize;


    /* FIXME: This needs some more work if we are to ever
     * look at the ID3 frame.  This is because the Stream
     * may not be able to hold the complete ID3 frame.
     * We should consume the whole frame inside tagtype()
     * instead of outside of tagframe().  That would support
     * recovering when Stream contains less then 8-bytes (header)
     * and also when ID3v2 is bigger then Stream buffer size.
     * Need to pass in stream so that buffer can be
     * consumed as well as letting additional data to be
     * read in.
     */
    remaining = p->Stream.bufend - p->Stream.next_frame;
    if ((tagsize = tagtype(p->Stream.this_frame, remaining)))
    {
        p->mad_stream_skip(&p->Stream, tagsize);
        rc = SOX_SUCCESS;
    }

    /* We know that a valid frame hasn't been found yet
     * so help libmad out and go back into frame seek mode.
     * This is true whether an ID3 tag was found or not.
     */
    p->mad_stream_sync(&p->Stream);

    return rc;
}

static sox_bool sox_mp3_vbrtag(sox_format_t *ft)
{
    priv_t *p = ft->priv;
    struct mad_bitptr *anc = &p->Stream.anc_ptr;

    if (p->Frame.header.layer != MAD_LAYER_III)
        return sox_false;

    if (p->Stream.anc_bitlen < 32)
        return sox_false;

    if (!memcmp(anc->byte, "Xing", 4) ||
        !memcmp(anc->byte, "Info", 4))
        return sox_true;

    return sox_false;
}

static int startread(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;
  size_t ReadSize;
  sox_bool ignore_length = ft->signal.length == SOX_IGNORE_LENGTH;
  int open_library_result;

  LSX_DLLIBRARY_OPEN(
      p,
      mad_dl,
      MAD_FUNC_ENTRIES,
      "MAD decoder library",
      mad_library_names,
      open_library_result);
  if (open_library_result)
    return SOX_EOF;

  p->mp3_buffer_size = sox_globals.bufsiz;
  p->mp3_buffer = lsx_malloc(p->mp3_buffer_size);

  ft->signal.length = SOX_UNSPEC;
  if (ft->seekable) {
#ifdef USING_ID3TAG
    lsx_id3_read_tag(ft, sox_true);
    lsx_rewind(ft);
    if (!ft->signal.length)
#endif
      if (!ignore_length)
        ft->signal.length = mp3_duration(ft);
  }

  p->mad_stream_init(&p->Stream);
  p->mad_frame_init(&p->Frame);
  p->mad_synth_init(&p->Synth);
  mad_timer_reset(&p->Timer);

  ft->encoding.encoding = SOX_ENCODING_MP3;

  /* Decode at least one valid frame to find out the input
   * format.  The decoded frame will be saved off so that it
   * can be processed later.
   */
  ReadSize = lsx_readbuf(ft, p->mp3_buffer, p->mp3_buffer_size);
  if (ReadSize != p->mp3_buffer_size && lsx_error(ft))
    return SOX_EOF;

  p->mad_stream_buffer(&p->Stream, p->mp3_buffer, ReadSize);

  /* Find a valid frame before starting up.  This makes sure
   * that we have a valid MP3 and also skips past ID3v2 tags
   * at the beginning of the audio file.
   */
  p->Stream.error = 0;
  while (p->mad_frame_decode(&p->Frame,&p->Stream))
  {
      /* check whether input buffer needs a refill */
      if (p->Stream.error == MAD_ERROR_BUFLEN)
      {
          if (sox_mp3_input(ft) == SOX_EOF)
              return SOX_EOF;

          continue;
      }

      /* Consume any ID3 tags */
      sox_mp3_inputtag(ft);

      /* FIXME: We should probably detect when we've read
       * a bunch of non-ID3 data and still haven't found a
       * frame.  In that case we can abort early without
       * scanning the whole file.
       */
      p->Stream.error = 0;
  }

  if (p->Stream.error)
  {
      lsx_fail_errno(ft,SOX_EOF,"No valid MP3 frame found");
      return SOX_EOF;
  }

  switch(p->Frame.header.mode)
  {
      case MAD_MODE_SINGLE_CHANNEL:
      case MAD_MODE_DUAL_CHANNEL:
      case MAD_MODE_JOINT_STEREO:
      case MAD_MODE_STEREO:
          ft->signal.channels = MAD_NCHANNELS(&p->Frame.header);
          break;
      default:
          lsx_fail_errno(ft, SOX_EFMT, "Cannot determine number of channels");
          return SOX_EOF;
  }

  ft->signal.precision = MP3_MAD_PRECISION;
  ft->signal.rate=p->Frame.header.samplerate;
  if (ignore_length)
    ft->signal.length = SOX_UNSPEC;
  else {
    ft->signal.length *= ft->signal.channels;  /* Keep separate from line above! */
  }

  if (!sox_mp3_vbrtag(ft))
      p->Stream.next_frame = p->Stream.this_frame;

  p->mad_frame_init(&p->Frame);
  sox_mp3_input(ft);

  p->cursamp = 0;

  return SOX_SUCCESS;
}

/*
 * Read up to len samples from p->Synth
 * If needed, read some more MP3 data, decode them and synth them
 * Place in buf[].
 * Return number of samples read.
 */
static size_t sox_mp3read(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
    priv_t *p = (priv_t *) ft->priv;
    size_t donow,i,done=0;
    mad_fixed_t sample;
    size_t chan;

    do {
        size_t x = (p->Synth.pcm.length - p->cursamp)*ft->signal.channels;
        donow=min(len, x);
        i=0;
        while(i<donow){
            for(chan=0;chan<ft->signal.channels;chan++){
                sample=p->Synth.pcm.samples[chan][p->cursamp];
                if (sample < -MAD_F_ONE)
                    sample=-MAD_F_ONE;
                else if (sample >= MAD_F_ONE)
                    sample=MAD_F_ONE-1;
                *buf++=(sox_sample_t)(sample<<(32-1-MAD_F_FRACBITS));
                i++;
            }
            p->cursamp++;
        };

        len-=donow;
        done+=donow;

        if (len==0) break;

        /* check whether input buffer needs a refill */
        if (p->Stream.error == MAD_ERROR_BUFLEN)
        {
            if (sox_mp3_input(ft) == SOX_EOF) {
                lsx_debug("sox_mp3_input EOF");
                break;
            }
        }

        if (p->mad_frame_decode(&p->Frame,&p->Stream))
        {
            if(MAD_RECOVERABLE(p->Stream.error))
            {
                sox_mp3_inputtag(ft);
                continue;
            }
            else
            {
                if (p->Stream.error == MAD_ERROR_BUFLEN)
                    continue;
                else
                {
                    lsx_report("unrecoverable frame level error (%s).",
                              p->mad_stream_errorstr(&p->Stream));
                    break;
                }
            }
        }
        p->FrameCount++;
        p->mad_timer_add(&p->Timer,p->Frame.header.duration);
        p->mad_synth_frame(&p->Synth,&p->Frame);
        p->cursamp=0;
    } while(1);

    return done;
}

static int stopread(sox_format_t * ft)
{
  priv_t *p=(priv_t*) ft->priv;

  mad_synth_finish(&p->Synth);
  p->mad_frame_finish(&p->Frame);
  p->mad_stream_finish(&p->Stream);

  free(p->mp3_buffer);
  LSX_DLLIBRARY_CLOSE(p, mad_dl);
  return SOX_SUCCESS;
}

static int sox_mp3seek(sox_format_t * ft, uint64_t offset)
{
  priv_t   * p = (priv_t *) ft->priv;
  size_t   initial_bitrate = p->Frame.header.bitrate;
  size_t   tagsize = 0, consumed = 0;
  sox_bool vbr = sox_false; /* Variable Bit Rate */
  sox_bool depadded = sox_false;
  uint64_t to_skip_samples = 0;

  /* Reset all */
  lsx_rewind(ft);
  mad_timer_reset(&p->Timer);
  p->FrameCount = 0;

  /* They where opened in startread */
  mad_synth_finish(&p->Synth);
  p->mad_frame_finish(&p->Frame);
  p->mad_stream_finish(&p->Stream);

  p->mad_stream_init(&p->Stream);
  p->mad_frame_init(&p->Frame);
  p->mad_synth_init(&p->Synth);

  offset /= ft->signal.channels;
  to_skip_samples = offset;

  while(sox_true) {  /* Read data from the MP3 file */
    size_t padding = 0;
    size_t read;
    size_t leftover = p->Stream.bufend - p->Stream.next_frame;

    memmove(p->mp3_buffer, p->Stream.this_frame, leftover);
    read = lsx_readbuf(ft, p->mp3_buffer + leftover, p->mp3_buffer_size - leftover);
    if (read == 0) {
      lsx_debug("seek failure. unexpected EOF (frames=%" PRIuPTR " leftover=%" PRIuPTR ")", p->FrameCount, leftover);
      break;
    }
    for (; !depadded && padding < read && !p->mp3_buffer[padding]; ++padding);
    depadded = sox_true;
    p->mad_stream_buffer(&p->Stream, p->mp3_buffer + padding, leftover + read - padding);

    while (sox_true) {  /* Decode frame headers */
      static unsigned short samples;
      p->Stream.error = MAD_ERROR_NONE;

      /* Not an audio frame */
      if (p->mad_header_decode(&p->Frame.header, &p->Stream) == -1) {
        if (p->Stream.error == MAD_ERROR_BUFLEN)
          break;  /* Normal behaviour; get some more data from the file */
        if (!MAD_RECOVERABLE(p->Stream.error)) {
          lsx_warn("unrecoverable MAD error");
          break;
        }
        if (p->Stream.error == MAD_ERROR_LOSTSYNC) {
          unsigned available = (p->Stream.bufend - p->Stream.this_frame);
          tagsize = tagtype(p->Stream.this_frame, (size_t) available);
          if (tagsize) {   /* It's some ID3 tags, so just skip */
            if (tagsize >= available) {
              lsx_seeki(ft, (off_t)(tagsize - available), SEEK_CUR);
              depadded = sox_false;
            }
            p->mad_stream_skip(&p->Stream, min(tagsize, available));
          }
          else lsx_warn("MAD lost sync");
        }
        else lsx_warn("recoverable MAD error");
        continue;
      }

      consumed += p->Stream.next_frame - p->Stream.this_frame;
      vbr      |= (p->Frame.header.bitrate != initial_bitrate);

      samples = 32 * MAD_NSBSAMPLES(&p->Frame.header);

      p->FrameCount++;
      p->mad_timer_add(&p->Timer, p->Frame.header.duration);

      if(to_skip_samples <= samples)
      {
        p->mad_frame_decode(&p->Frame,&p->Stream);
        p->mad_synth_frame(&p->Synth, &p->Frame);
        p->cursamp = to_skip_samples;
        return SOX_SUCCESS;
      }
      else to_skip_samples -= samples;

      /* If not VBR, we can extrapolate frame size */
      if (p->FrameCount == 64 && !vbr) {
        p->FrameCount = offset / samples;
        to_skip_samples = offset % samples;

        if (SOX_SUCCESS != lsx_seeki(ft, (off_t)(p->FrameCount * consumed / 64 + tagsize), SEEK_SET))
          return SOX_EOF;

        /* Reset Stream for refilling buffer */
        p->mad_stream_finish(&p->Stream);
        p->mad_stream_init(&p->Stream);
        break;
      }
    }
  };

  return SOX_EOF;
}
#else /* !HAVE_MAD_H */
static int startread(sox_format_t * ft)
{
  lsx_fail_errno(ft,SOX_EOF,"SoX was compiled without MP3 decoding support");
  return SOX_EOF;
}
#define sox_mp3read NULL
#define stopread NULL
#define sox_mp3seek NULL
#endif /*HAVE_MAD_H*/

#ifdef HAVE_LAME

/* Adapters for lame message callbacks: */

static void errorf(const char* fmt, va_list va)
{
  sox_globals.subsystem=__FILE__;
  if (sox_globals.output_message_handler)
    (*sox_globals.output_message_handler)(1,sox_globals.subsystem,fmt,va);
  return;
}

static void debugf(const char* fmt, va_list va)
{
  sox_globals.subsystem=__FILE__;
  if (sox_globals.output_message_handler)
    (*sox_globals.output_message_handler)(4,sox_globals.subsystem,fmt,va);
  return;
}

static void msgf(const char* fmt, va_list va)
{
  sox_globals.subsystem=__FILE__;
  if (sox_globals.output_message_handler)
    (*sox_globals.output_message_handler)(3,sox_globals.subsystem,fmt,va);
  return;
}

/* These functions are considered optional. If they aren't present in the
   library, the stub versions defined here will be used instead. */

UNUSED static void id3tag_init_stub(lame_global_flags * gfp UNUSED)
  { return; }
UNUSED static void id3tag_set_title_stub(lame_global_flags * gfp UNUSED, const char* title UNUSED)
  { return; }
UNUSED static void id3tag_set_artist_stub(lame_global_flags * gfp UNUSED, const char* artist UNUSED)
  { return; }
UNUSED static void id3tag_set_album_stub(lame_global_flags * gfp UNUSED, const char* album UNUSED)
  { return; }
UNUSED static void id3tag_set_year_stub(lame_global_flags * gfp UNUSED, const char* year UNUSED)
  { return; }
UNUSED static void id3tag_set_comment_stub(lame_global_flags * gfp UNUSED, const char* comment UNUSED)
  { return; }
UNUSED static void id3tag_set_track_stub(lame_global_flags * gfp UNUSED, const char* track UNUSED)
  { return; }
UNUSED static int id3tag_set_genre_stub(lame_global_flags * gfp UNUSED, const char* genre UNUSED)
  { return 0; }
UNUSED static size_t id3tag_set_pad_stub(lame_global_flags * gfp UNUSED, size_t n UNUSED)
  { return 0; }
UNUSED static size_t lame_get_id3v2_tag_stub(lame_global_flags * gfp UNUSED, unsigned char * buffer UNUSED, size_t size UNUSED)
  { return 0; }
UNUSED static int id3tag_set_fieldvalue_stub(lame_global_flags * gfp UNUSED, const char *fieldvalue UNUSED)
  { return 0; }

static int get_id3v2_tag_size(sox_format_t * ft)
{
  size_t bytes_read;
  int id3v2_size;
  unsigned char id3v2_header[10];

  if (lsx_seeki(ft, (off_t)0, SEEK_SET) != 0) {
    lsx_warn("cannot update id3 tag - failed to seek to beginning");
    return SOX_EOF;
  }

  /* read 10 bytes in case there's an ID3 version 2 header here */
  bytes_read = lsx_readbuf(ft, id3v2_header, sizeof(id3v2_header));
  if (bytes_read != sizeof(id3v2_header)) {
    lsx_warn("cannot update id3 tag - failed to read id3 header");
    return SOX_EOF;      /* not readable, maybe opened Write-Only */
  }

  /* does the stream begin with the ID3 version 2 file identifier? */
  if (!strncmp((char *) id3v2_header, "ID3", (size_t)3)) {
    /* the tag size (minus the 10-byte header) is encoded into four
     * bytes where the most significant bit is clear in each byte */
    id3v2_size = (((id3v2_header[6] & 0x7f) << 21)
                    | ((id3v2_header[7] & 0x7f) << 14)
                    | ((id3v2_header[8] & 0x7f) << 7)
                    | (id3v2_header[9] & 0x7f))
        + sizeof(id3v2_header);
  } else {
    /* no ID3 version 2 tag in this stream */
    id3v2_size = 0;
  }
  return id3v2_size;
}

static void rewrite_id3v2_tag(sox_format_t * ft, size_t id3v2_size, uint64_t num_samples)
{
  priv_t *p = (priv_t *)ft->priv;
  size_t new_size;
  unsigned char * buffer;

  if (LSX_DLFUNC_IS_STUB(p, lame_get_id3v2_tag))
  {
    if (p->num_samples)
      lsx_warn("cannot update track length info - tag update not supported with this version of LAME. Track length will be incorrect.");
    else
      lsx_report("cannot update track length info - tag update not supported with this version of LAME. Track length will be unspecified.");
    return;
  }

  buffer = lsx_malloc(id3v2_size);
  if (!buffer)
  {
    lsx_warn("cannot update track length info - failed to allocate buffer");
    return;
  }

  if (num_samples > ULONG_MAX)
  {
    lsx_warn("cannot accurately update track length info - file is too long");
    num_samples = 0;
  }
  p->lame_set_num_samples(p->gfp, (unsigned long)num_samples);
  lsx_debug("updated MP3 TLEN to %lu samples", (unsigned long)num_samples);

  new_size = p->lame_get_id3v2_tag(p->gfp, buffer, id3v2_size);

  if (new_size != id3v2_size && new_size-ID3PADDING <= id3v2_size) {
    p->id3tag_set_pad(p->gfp, ID3PADDING + id3v2_size - new_size);
    new_size = p->lame_get_id3v2_tag(p->gfp, buffer, id3v2_size);
  }

  if (new_size != id3v2_size) {
    if (LSX_DLFUNC_IS_STUB(p, id3tag_set_pad))
    {
      if (p->num_samples)
        lsx_warn("cannot update track length info - tag size adjustment not supported with this version of LAME. Track length will be invalid.");
      else
        lsx_report("cannot update track length info - tag size adjustment not supported with this version of LAME. Track length will be unspecified.");
    }
    else
      lsx_warn("cannot update track length info - failed to adjust tag size");
  } else {
    lsx_seeki(ft, (off_t)0, SEEK_SET);
    /* Overwrite the Id3v2 tag (this time TLEN should be accurate) */
    if (lsx_writebuf(ft, buffer, id3v2_size) != 1) {
      lsx_debug("Rewrote Id3v2 tag (%" PRIuPTR " bytes)", id3v2_size);
    }
  }

  free(buffer);
}

static void rewrite_tags(sox_format_t * ft, uint64_t num_samples)
{
  priv_t *p = (priv_t *)ft->priv;

  off_t file_size;
  int id3v2_size;

  if (lsx_seeki(ft, (off_t)0, SEEK_END)) {
    lsx_warn("cannot update tags - seek to end failed");
    return;
  }

  /* Get file size */
  file_size = lsx_tell(ft);

  if (file_size == 0) {
    lsx_warn("cannot update tags - file size is 0");
    return;
  }

  id3v2_size = get_id3v2_tag_size(ft);
  if (id3v2_size > 0 && num_samples != p->num_samples) {
    rewrite_id3v2_tag(ft, id3v2_size, num_samples);
  }

  if (p->vbr_tag) {
    size_t lametag_size;
    uint8_t buffer[MAXFRAMESIZE];

    if (lsx_seeki(ft, (off_t)id3v2_size, SEEK_SET)) {
      lsx_warn("cannot write VBR tag - seek to tag block failed");
      return;
    }

    lametag_size = p->lame_get_lametag_frame(p->gfp, buffer, sizeof(buffer));
    if (lametag_size > sizeof(buffer)) {
      lsx_warn("cannot write VBR tag - VBR tag too large for buffer");
      return;
    }

    if (lametag_size < 1) {
      return;
    }

    if (lsx_writebuf(ft, buffer, lametag_size) != lametag_size) {
      lsx_warn("cannot write VBR tag - VBR tag write failed");
    } else {
      lsx_debug("rewrote VBR tag (%" PRIuPTR " bytes)", lametag_size);
    }
  }
}

#endif /* HAVE_LAME */

#if defined(HAVE_LAME) || defined(HAVE_TWOLAME)

#define LAME_BUFFER_SIZE(num_samples) (((num_samples) + 3) / 4 * 5 + 7200)

static int startwrite(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;
  int openlibrary_result;
  int fail = 0;

  if (ft->encoding.encoding != SOX_ENCODING_MP3) {
    if(ft->encoding.encoding != SOX_ENCODING_UNKNOWN)
      lsx_report("Encoding forced to MP2/MP3");
    ft->encoding.encoding = SOX_ENCODING_MP3;
  }

  if(strchr(ft->filetype, '2'))
      p->mp2 = 1;

  if (p->mp2) {
#ifdef HAVE_TWOLAME
    LSX_DLLIBRARY_OPEN(
        p,
        twolame_dl,
        TWOLAME_FUNC_ENTRIES,
        "Twolame encoder library",
        twolame_library_names,
        openlibrary_result);
#else
    lsx_fail_errno(ft,SOX_EOF,"SoX was compiled without MP2 encoding support");
    return SOX_EOF;
#endif
  } else {
#ifdef HAVE_LAME
    LSX_DLLIBRARY_OPEN(
        p,
        lame_dl,
        LAME_FUNC_ENTRIES,
        "LAME encoder library",
        lame_library_names,
        openlibrary_result);
#else
    lsx_fail_errno(ft,SOX_EOF,"SoX was compiled without MP3 encoding support");
    return SOX_EOF;
#endif
  }
  if (openlibrary_result)
    return SOX_EOF;

  p->mp3_buffer_size = LAME_BUFFER_SIZE(sox_globals.bufsiz / max(ft->signal.channels, 1));
  p->mp3_buffer = lsx_malloc(p->mp3_buffer_size);

  p->pcm_buffer_size = sox_globals.bufsiz * sizeof(float);
  p->pcm_buffer = lsx_malloc(p->pcm_buffer_size);

  if (p->mp2) {
#ifdef HAVE_TWOLAME
    p->opt = p->twolame_init();

    if (p->opt == NULL){
      lsx_fail_errno(ft,SOX_EOF,"Initialization of Twolame library failed");
      return(SOX_EOF);
    }
#endif
  } else {
#ifdef HAVE_LAME
    p->gfp = p->lame_init();

    if (p->gfp == NULL){
      lsx_fail_errno(ft,SOX_EOF,"Initialization of LAME library failed");
      return(SOX_EOF);
    }

    /* First set message callbacks so we don't miss any messages: */
    p->lame_set_errorf(p->gfp,errorf);
    p->lame_set_debugf(p->gfp,debugf);
    p->lame_set_msgf  (p->gfp,msgf);

    p->num_samples = ft->signal.length == SOX_IGNORE_LENGTH ? 0 : ft->signal.length / max(ft->signal.channels, 1);
    p->lame_set_num_samples(p->gfp, p->num_samples > ULONG_MAX ? 0 : (unsigned long)p->num_samples);
#endif
  }

  ft->signal.precision = MP3_LAME_PRECISION;

  if (ft->signal.channels != SOX_ENCODING_UNKNOWN) {
    if (p->mp2) {
#ifdef HAVE_TWOLAME
      fail = (p->twolame_set_num_channels(p->opt,(int)ft->signal.channels) != 0);
#endif
    } else {
#ifdef HAVE_LAME
      fail = (p->lame_set_num_channels(p->gfp,(int)ft->signal.channels) < 0);
#endif
    }
    if (fail) {
      lsx_fail_errno(ft,SOX_EOF,"Unsupported number of channels");
      return(SOX_EOF);
    }
  }
  else {
    if (p->mp2) {
#ifdef HAVE_TWOLAME
      ft->signal.channels = p->twolame_get_num_channels(p->opt); /* Twolame default */
#endif
    } else {
#ifdef HAVE_LAME
      ft->signal.channels = p->lame_get_num_channels(p->gfp); /* LAME default */
#endif
    }
  }

  if (p->mp2) {
#ifdef HAVE_TWOLAME
    p->twolame_set_in_samplerate(p->opt,(int)ft->signal.rate);
    p->twolame_set_out_samplerate(p->opt,(int)ft->signal.rate);
#endif
  } else {
#ifdef HAVE_LAME
    p->lame_set_in_samplerate(p->gfp,(int)ft->signal.rate);
    p->lame_set_out_samplerate(p->gfp,(int)ft->signal.rate);
#endif
  }

  if (!p->mp2) {
#ifdef HAVE_LAME
    if (!LSX_DLFUNC_IS_STUB(p, id3tag_init))
      write_comments(ft);
#endif
  }

  /* The primary parameter to the LAME encoder is the bit rate. If the
   * value of encoding.compression is a positive integer, it's taken as
   * the bitrate in kbps (that is if you specify 128, it use 128 kbps).
   *
   * The second most important parameter is probably "quality" (really
   * performance), which allows balancing encoding speed vs. quality.
   * In LAME, 0 specifies highest quality but is very slow, while
   * 9 selects poor quality, but is fast. (5 is the default and 2 is
   * recommended as a good trade-off for high quality encodes.)
   *
   * Because encoding.compression is a float, the fractional part is used
   * to select quality. 128.2 selects 128 kbps encoding with a quality
   * of 2. There is one problem with this approach. We need 128 to specify
   * 128 kbps encoding with default quality, so .0 means use default. Instead
   * of .0 you have to use .01 to specify the highest quality (128.01).
   *
   * LAME uses bitrate to specify a constant bitrate, but higher quality
   * can be achieved using Variable Bit Rate (VBR). VBR quality (really
   * size) is selected using a number from 0 to 9. Use a value of 0 for high
   * quality, larger files, and 9 for smaller files of lower quality. 4 is
   * the default.
   *
   * In order to squeeze the selection of VBR into the encoding.compression
   * float we use negative numbers to select VRR. -4.2 would select default
   * VBR encoding (size) with high quality (speed). One special case is 0,
   * which is a valid VBR encoding parameter but not a valid bitrate.
   * Compression value of 0 is always treated as a high quality vbr, as a
   * result both -0.2 and 0.2 are treated as highest quality VBR (size) and
   * high quality (speed).
   *
   * Note: It would have been nice to simply use low values, 0-9, to trigger
   * VBR mode, but 8 kbps is a valid bit rate, so negative values were
   * used instead.
  */

  lsx_debug("-C option is %f", ft->encoding.compression);

  if (ft->encoding.compression == HUGE_VAL) {
    /* Do nothing, use defaults: */
    lsx_report("using %s encoding defaults", p->mp2? "MP2" : "MP3");
  } else {
    double abs_compression = fabs(ft->encoding.compression);
    double floor_compression = floor(abs_compression);
    double fraction_compression = abs_compression - floor_compression;
    int bitrate_q = (int)floor_compression;
    int encoder_q =
        fraction_compression == 0.0
        ? -1
        : (int)(fraction_compression * 10.0 + 0.5);

    if (ft->encoding.compression < 0.5) {
      if (p->mp2) {
        lsx_fail_errno(ft,SOX_EOF,"Variable bitrate encoding not supported for MP2 audio");
        return(SOX_EOF);
      }
#ifdef HAVE_LAME
      if (p->lame_get_VBR(p->gfp) == vbr_off)
        p->lame_set_VBR(p->gfp, vbr_default);

      if (ft->seekable) {
        p->vbr_tag = 1;
      } else {
        lsx_warn("unable to write VBR tag because we can't seek");
      }

      if (p->lame_set_VBR_q(p->gfp, bitrate_q) < 0)
      {
        lsx_fail_errno(ft, SOX_EOF,
          "lame_set_VBR_q(%d) failed (should be between 0 and 9)",
          bitrate_q);
        return(SOX_EOF);
      }
      lsx_report("lame_set_VBR_q(%d)", bitrate_q);
#endif
    } else {
      if (p->mp2) {
#ifdef HAVE_TWOLAME
        fail = (p->twolame_set_brate(p->opt, bitrate_q) != 0);
#endif
      } else {
#ifdef HAVE_LAME
        fail = (p->lame_set_brate(p->gfp, bitrate_q) < 0);
#endif
      }
      if (fail) {
        lsx_fail_errno(ft, SOX_EOF,
          "%slame_set_brate(%d) failed", p->mp2? "two" : "", bitrate_q);
        return(SOX_EOF);
      }
      lsx_report("(two)lame_set_brate(%d)", bitrate_q);
    }

    /* Set Quality */

    if (encoder_q < 0 || p->mp2) {
      /* use default quality value */
      lsx_report("using %s default quality", p->mp2? "MP2" : "MP3");
    } else {
#ifdef HAVE_LAME
      if (p->lame_set_quality(p->gfp, encoder_q) < 0) {
        lsx_fail_errno(ft, SOX_EOF,
          "lame_set_quality(%d) failed", encoder_q);
        return(SOX_EOF);
      }
      lsx_report("lame_set_quality(%d)", encoder_q);
#endif
    }
  }

  if (!p->mp2) {
#ifdef HAVE_LAME
    p->lame_set_bWriteVbrTag(p->gfp, p->vbr_tag);
#endif
  }

  if (p->mp2) {
#ifdef HAVE_TWOLAME
    fail = (p->twolame_init_params(p->opt) != 0);
#endif
  } else {
#ifdef HAVE_LAME
    fail = (p->lame_init_params(p->gfp) < 0);
#endif
  }
  if (fail) {
    lsx_fail_errno(ft,SOX_EOF,"%s initialization failed", p->mp2? "Twolame" : "LAME");
    return(SOX_EOF);
  }

  return(SOX_SUCCESS);
}

#define MP3_SAMPLE_TO_FLOAT(d) ((float)(32768*SOX_SAMPLE_TO_FLOAT_32BIT(d,)))

static size_t sox_mp3write(sox_format_t * ft, const sox_sample_t *buf, size_t samp)
{
    priv_t *p = (priv_t *)ft->priv;
    size_t new_buffer_size;
    float *buffer_l, *buffer_r = NULL;
    int nsamples = samp/ft->signal.channels;
    int i,j;
    int written = 0;
    SOX_SAMPLE_LOCALS;

    new_buffer_size = samp * sizeof(float);
    if (p->pcm_buffer_size < new_buffer_size) {
      float *new_buffer = lsx_realloc(p->pcm_buffer, new_buffer_size);
      if (!new_buffer) {
        lsx_fail_errno(ft, SOX_ENOMEM, "Out of memory");
        return 0;
      }
      p->pcm_buffer_size = new_buffer_size;
      p->pcm_buffer = new_buffer;
    }

    buffer_l = p->pcm_buffer;

    if (p->mp2)
    {
        size_t s;
        for(s = 0; s < samp; s++)
            buffer_l[s] = SOX_SAMPLE_TO_FLOAT_32BIT(buf[s],);
    }
    else
    {
        if (ft->signal.channels == 2)
        {
            /* lame doesn't support interleaved samples for floats so we must break
             * them out into seperate buffers.
             */
            buffer_r = p->pcm_buffer + nsamples;
            j=0;
            for (i = 0; i < nsamples; i++)
            {
                buffer_l[i] = MP3_SAMPLE_TO_FLOAT(buf[j++]);
                buffer_r[i] = MP3_SAMPLE_TO_FLOAT(buf[j++]);
            }
        }
        else
        {
            j=0;
            for (i = 0; i < nsamples; i++) {
                buffer_l[i] = MP3_SAMPLE_TO_FLOAT(buf[j++]);
            }
        }
    }

    new_buffer_size = LAME_BUFFER_SIZE(nsamples);
    if (p->mp3_buffer_size < new_buffer_size) {
      unsigned char *new_buffer = lsx_realloc(p->mp3_buffer, new_buffer_size);
      if (!new_buffer) {
        lsx_fail_errno(ft, SOX_ENOMEM, "Out of memory");
        return 0;
      }
      p->mp3_buffer_size = new_buffer_size;
      p->mp3_buffer = new_buffer;
    }

    if(p->mp2) {
#ifdef HAVE_TWOLAME
        written = p->twolame_encode_buffer_float32_interleaved(p->opt, buffer_l,
                  nsamples, p->mp3_buffer, (int)p->mp3_buffer_size);
#endif
    } else {
#ifdef HAVE_LAME
        written = p->lame_encode_buffer_float(p->gfp, buffer_l, buffer_r,
                  nsamples, p->mp3_buffer, (int)p->mp3_buffer_size);
#endif
    }
    if (written < 0) {
        lsx_fail_errno(ft,SOX_EOF,"Encoding failed");
        return 0;
    }

    if (lsx_writebuf(ft, p->mp3_buffer, (size_t)written) < (size_t)written)
    {
        lsx_fail_errno(ft,SOX_EOF,"File write failed");
        return 0;
    }

    return samp;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;
  uint64_t num_samples = ft->olength == SOX_IGNORE_LENGTH ? 0 : ft->olength / max(ft->signal.channels, 1);
  int written = 0;

  if (p->mp2) {
#ifdef HAVE_TWOLAME
    written = p->twolame_encode_flush(p->opt, p->mp3_buffer, (int)p->mp3_buffer_size);
#endif
  } else {
#ifdef HAVE_LAME
    written = p->lame_encode_flush(p->gfp, p->mp3_buffer, (int)p->mp3_buffer_size);
#endif
  }
  if (written < 0)
    lsx_fail_errno(ft, SOX_EOF, "Encoding failed");
  else if (lsx_writebuf(ft, p->mp3_buffer, (size_t)written) < (size_t)written)
    lsx_fail_errno(ft, SOX_EOF, "File write failed");
  else if (!p->mp2) {
#ifdef HAVE_LAME
    if (ft->seekable && (num_samples != p->num_samples || p->vbr_tag))
      rewrite_tags(ft, num_samples);
#endif
  }

  free(p->mp3_buffer);
  free(p->pcm_buffer);

  if(p->mp2) {
#ifdef HAVE_TWOLAME
    p->twolame_close(&p->opt);
    LSX_DLLIBRARY_CLOSE(p, twolame_dl);
#endif
  } else {
#ifdef HAVE_LAME
    p->lame_close(p->gfp);
    LSX_DLLIBRARY_CLOSE(p, lame_dl);
#endif
  }
  return SOX_SUCCESS;
}

#else /* !(HAVE_LAME || HAVE_TWOLAME) */
static int startwrite(sox_format_t * ft UNUSED)
{
  lsx_fail_errno(ft,SOX_EOF,"SoX was compiled with neither MP2 nor MP3 encoding support");
  return SOX_EOF;
}
#define sox_mp3write NULL
#define stopwrite NULL
#endif /* HAVE_LAME || HAVE_TWOLAME */

LSX_FORMAT_HANDLER(mp3)
{
  static char const * const names[] = {"mp3", "mp2", "audio/mpeg", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_MP3, 0, 0};
  static sox_rate_t const write_rates[] = {
    8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "MPEG Layer 2/3 lossy audio compression", names, 0,
    startread, sox_mp3read, stopread,
    startwrite, sox_mp3write, stopwrite,
    sox_mp3seek, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
#endif /* defined(HAVE_MAD_H) || defined(HAVE_LAME) || defined(HAVE_TWOLAME) */
