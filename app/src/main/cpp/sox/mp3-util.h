/* libSoX MP3 utilities  Copyright (c) 2007-9 SoX contributors
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

#include <sys/stat.h>

#if defined(HAVE_LAME)

static void write_comments(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;
  const char* comment;

  p->id3tag_init(p->gfp);
  p->id3tag_set_pad(p->gfp, (size_t)ID3PADDING);

  /* Note: id3tag_set_fieldvalue is not present in LAME 3.97, so we're using
     the 3.97-compatible methods for all of the tags that 3.97 supported. */
  /* FIXME: This is no more necessary, since support for LAME 3.97 has ended. */
  if ((comment = sox_find_comment(ft->oob.comments, "Title")))
    p->id3tag_set_title(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Artist")))
    p->id3tag_set_artist(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Album")))
    p->id3tag_set_album(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Tracknumber")))
    p->id3tag_set_track(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Year")))
    p->id3tag_set_year(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Comment")))
    p->id3tag_set_comment(p->gfp, comment);
  if ((comment = sox_find_comment(ft->oob.comments, "Genre")))
  {
    if (p->id3tag_set_genre(p->gfp, comment))
      lsx_warn("\"%s\" is not a recognized ID3v1 genre.", comment);
  }

  if ((comment = sox_find_comment(ft->oob.comments, "Discnumber")))
  {
    char* id3tag_buf = lsx_malloc(strlen(comment) + 6);
    if (id3tag_buf)
    {
      sprintf(id3tag_buf, "TPOS=%s", comment);
      p->id3tag_set_fieldvalue(p->gfp, id3tag_buf);
      free(id3tag_buf);
    }
  }
}

#endif /* HAVE_LAME */

#ifdef HAVE_MAD_H

static unsigned long xing_frames(priv_t * p, struct mad_bitptr ptr, unsigned bitlen)
{
  #define XING_MAGIC ( ('X' << 24) | ('i' << 16) | ('n' << 8) | 'g' )
  if (bitlen >= 96 && p->mad_bit_read(&ptr, 32) == XING_MAGIC &&
      (p->mad_bit_read(&ptr, 32) & 1 )) /* XING_FRAMES */
    return p->mad_bit_read(&ptr, 32);
  return 0;
}

static size_t mp3_duration(sox_format_t * ft)
{
  priv_t              * p = (priv_t *) ft->priv;
  struct mad_stream   mad_stream;
  struct mad_header   mad_header;
  struct mad_frame    mad_frame;
  size_t              initial_bitrate = 0; /* Initialised to prevent warning */
  size_t              tagsize = 0, consumed = 0, frames = 0;
  sox_bool            vbr = sox_false, depadded = sox_false;
  size_t              num_samples = 0;

  p->mad_stream_init(&mad_stream);
  p->mad_header_init(&mad_header);
  p->mad_frame_init(&mad_frame);

  do {  /* Read data from the MP3 file */
    int read, padding = 0;
    size_t leftover = mad_stream.bufend - mad_stream.next_frame;

    memmove(p->mp3_buffer, mad_stream.this_frame, leftover);
    read = lsx_readbuf(ft, p->mp3_buffer + leftover, p->mp3_buffer_size - leftover);
    if (read <= 0) {
      lsx_debug("got exact duration by scan to EOF (frames=%" PRIuPTR " leftover=%" PRIuPTR ")", frames, leftover);
      break;
    }
    for (; !depadded && padding < read && !p->mp3_buffer[padding]; ++padding);
    depadded = sox_true;
    p->mad_stream_buffer(&mad_stream, p->mp3_buffer + padding, leftover + read - padding);

    while (sox_true) {  /* Decode frame headers */
      mad_stream.error = MAD_ERROR_NONE;
      if (p->mad_header_decode(&mad_header, &mad_stream) == -1) {
        if (mad_stream.error == MAD_ERROR_BUFLEN)
          break;  /* Normal behaviour; get some more data from the file */
        if (!MAD_RECOVERABLE(mad_stream.error)) {
          lsx_warn("unrecoverable MAD error");
          break;
        }
        if (mad_stream.error == MAD_ERROR_LOSTSYNC) {
          unsigned available = (mad_stream.bufend - mad_stream.this_frame);
          tagsize = tagtype(mad_stream.this_frame, (size_t) available);
          if (tagsize) {   /* It's some ID3 tags, so just skip */
            if (tagsize >= available) {
              lsx_seeki(ft, (off_t)(tagsize - available), SEEK_CUR);
              depadded = sox_false;
            }
            p->mad_stream_skip(&mad_stream, min(tagsize, available));
          }
          else lsx_warn("MAD lost sync");
        }
        else lsx_warn("recoverable MAD error");
        continue; /* Not an audio frame */
      }

      num_samples += MAD_NSBSAMPLES(&mad_header) * 32;
      consumed += mad_stream.next_frame - mad_stream.this_frame;

      lsx_debug_more("bitrate=%lu", mad_header.bitrate);
      if (!frames) {
        initial_bitrate = mad_header.bitrate;

        /* Get the precise frame count from the XING header if present */
        mad_frame.header = mad_header;
        if (p->mad_frame_decode(&mad_frame, &mad_stream) == -1)
          if (!MAD_RECOVERABLE(mad_stream.error)) {
            lsx_warn("unrecoverable MAD error");
            break;
          }
        if ((frames = xing_frames(p, mad_stream.anc_ptr, mad_stream.anc_bitlen))) {
          num_samples *= frames;
          lsx_debug("got exact duration from XING frame count (%" PRIuPTR ")", frames);
          break;
        }
      }
      else vbr |= mad_header.bitrate != initial_bitrate;

      /* If not VBR, we can time just a few frames then extrapolate */
      if (++frames == 25 && !vbr) {
        double frame_size = (double) consumed / frames;
        size_t num_frames = (lsx_filelength(ft) - tagsize) / frame_size;
        num_samples = num_samples / frames * num_frames;
        lsx_debug("got approx. duration by CBR extrapolation");
        break;
      }
    }
  } while (mad_stream.error == MAD_ERROR_BUFLEN);

  p->mad_frame_finish(&mad_frame);
  mad_header_finish(&mad_header);
  p->mad_stream_finish(&mad_stream);
  lsx_rewind(ft);

  return num_samples;
}

#endif /* HAVE_MAD_H */
