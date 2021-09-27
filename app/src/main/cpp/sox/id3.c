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

#include "sox_i.h"
#include "id3.h"

#ifdef HAVE_ID3TAG

#include <id3tag.h>

static char const * id3tagmap[][2] =
{
  {"TIT2", "Title"},
  {"TPE1", "Artist"},
  {"TALB", "Album"},
  {"TCOM", "Composer"},
  {"TRCK", "Tracknumber"},
  {"TDRC", "Year"},
  {"TCON", "Genre"},
  {"COMM", "Comment"},
  {"TPOS", "Discnumber"},
  {NULL, NULL}
};

static id3_utf8_t * utf8_id3tag_findframe(
    struct id3_tag * tag, const char * const frameid, unsigned index)
{
  struct id3_frame const * frame = id3_tag_findframe(tag, frameid, index);
  if (frame) {
    unsigned nfields = frame->nfields;

    while (nfields--) {
      union id3_field const *field = id3_frame_field(frame, nfields);
      int ftype = id3_field_type(field);
      const id3_ucs4_t *ucs4 = NULL;
      unsigned nstrings;

      switch (ftype) {
      case ID3_FIELD_TYPE_STRING:
        ucs4 = id3_field_getstring(field);
        break;

      case ID3_FIELD_TYPE_STRINGFULL:
        ucs4 = id3_field_getfullstring(field);
        break;

      case ID3_FIELD_TYPE_STRINGLIST:
        nstrings = id3_field_getnstrings(field);
        while (nstrings--) {
          ucs4 = id3_field_getstrings(field, nstrings);
          if (ucs4)
            break;
        }
        break;
      }

      if (ucs4)
        return id3_ucs4_utf8duplicate(ucs4); /* Must call free() on this */
    }
  }
  return NULL;
}

struct tag_info_node
{
    struct tag_info_node * next;
    off_t start;
    off_t end;
};

struct tag_info {
  sox_format_t * ft;
  struct tag_info_node * head;
  struct id3_tag * tag;
};

static int add_tag(struct tag_info * info)
{
  struct tag_info_node * current;
  off_t start, end;
  id3_byte_t query[ID3_TAG_QUERYSIZE];
  id3_byte_t * buffer;
  long size;
  int result = 0;

  /* Ensure we're at the start of a valid tag and get its size. */
  if (ID3_TAG_QUERYSIZE != lsx_readbuf(info->ft, query, ID3_TAG_QUERYSIZE) ||
      !(size = id3_tag_query(query, ID3_TAG_QUERYSIZE))) {
    return 0;
  }
  if (size < 0) {
    if (0 != lsx_seeki(info->ft, size, SEEK_CUR) ||
        ID3_TAG_QUERYSIZE != lsx_readbuf(info->ft, query, ID3_TAG_QUERYSIZE) ||
        (size = id3_tag_query(query, ID3_TAG_QUERYSIZE)) <= 0) {
      return 0;
    }
  }

  /* Don't read a tag more than once. */
  start = lsx_tell(info->ft);
  end = start + size;
  for (current = info->head; current; current = current->next) {
    if (start == current->start && end == current->end) {
      return 1;
    } else if (start < current->end && current->start < end) {
      return 0;
    }
  }

  buffer = lsx_malloc((size_t)size);
  if (!buffer) {
    return 0;
  }
  memcpy(buffer, query, ID3_TAG_QUERYSIZE);
  if ((unsigned long)size - ID3_TAG_QUERYSIZE ==
      lsx_readbuf(info->ft, buffer + ID3_TAG_QUERYSIZE, (size_t)size - ID3_TAG_QUERYSIZE)) {
    struct id3_tag * tag = id3_tag_parse(buffer, (size_t)size);
    if (tag) {
      current = lsx_malloc(sizeof(struct tag_info_node));
      if (current) {
        current->next = info->head;
        current->start = start;
        current->end = end;
        info->head = current;
        if (info->tag && (info->tag->extendedflags & ID3_TAG_EXTENDEDFLAG_TAGISANUPDATE)) {
          struct id3_frame * frame;
          unsigned i;
          for (i = 0; (frame = id3_tag_findframe(tag, NULL, i)); i++) {
            id3_tag_attachframe(info->tag, frame);
          }
          id3_tag_delete(tag);
        } else {
          if (info->tag) {
            id3_tag_delete(info->tag);
          }
          info->tag = tag;
        }
      }
    }
  }
  free(buffer);
  return result;
}

void lsx_id3_read_tag(sox_format_t * ft, sox_bool search)
{
  struct tag_info   info;
  id3_utf8_t        * utf8;
  int               i;
  int               has_id3v1 = 0;

  info.ft = ft;
  info.head = NULL;
  info.tag = NULL;

  /*
  We look for:
  ID3v1 at end (EOF - 128).
  ID3v2 at start.
  ID3v2 at end (but before ID3v1 from end if there was one).
  */

  if (search) {
    if (0 == lsx_seeki(ft, -128, SEEK_END)) {
      has_id3v1 =
        add_tag(&info) &&
        1 == ID3_TAG_VERSION_MAJOR(id3_tag_version(info.tag));
    }
    if (0 == lsx_seeki(ft, 0, SEEK_SET)) {
      add_tag(&info);
    }
    if (0 == lsx_seeki(ft, has_id3v1 ? -138 : -10, SEEK_END)) {
      add_tag(&info);
    }
  } else {
    add_tag(&info);
  }

  if (info.tag && info.tag->frames) {
    for (i = 0; id3tagmap[i][0]; ++i) {
      if ((utf8 = utf8_id3tag_findframe(info.tag, id3tagmap[i][0], 0))) {
        char * comment = lsx_malloc(strlen(id3tagmap[i][1]) + 1 + strlen((char *)utf8) + 1);
        sprintf(comment, "%s=%s", id3tagmap[i][1], utf8);
        sox_append_comment(&ft->oob.comments, comment);
        free(comment);
        free(utf8);
      }
    }
    if ((utf8 = utf8_id3tag_findframe(info.tag, "TLEN", 0))) {
      unsigned long tlen = strtoul((char *)utf8, NULL, 10);
      if (tlen > 0 && tlen < ULONG_MAX) {
        ft->signal.length= tlen; /* In ms; convert to samples later */
        lsx_debug("got exact duration from ID3 TLEN");
      }
      free(utf8);
    }
  }
  while (info.head) {
    struct tag_info_node * head = info.head;
    info.head = head->next;
    free(head);
  }
  if (info.tag) {
    id3_tag_delete(info.tag);
  }
}

#else

/* Stub for format modules */
void lsx_id3_read_tag(sox_format_t *ft, sox_bool search) { }

#endif
