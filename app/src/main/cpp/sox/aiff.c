/* libSoX SGI/Amiga AIFF format.
 * Copyright 1991-2007 Guido van Rossum And Sundry Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Guido van Rossum And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * Used by SGI on 4D/35 and Indigo.
 * This is a subformat of the EA-IFF-85 format.
 * This is related to the IFF format used by the Amiga.
 * But, apparently, not the same.
 * Also AIFF-C format output that is defined in DAVIC 1.4 Part 9 Annex B
 * (usable for japanese-data-broadcasting, specified by ARIB STD-B24.)
 */

#include "sox_i.h"
#include "aiff.h"
#include "id3.h"

#include <time.h>      /* for time stamping comments */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

/* forward declarations */
static double read_ieee_extended(sox_format_t *);
static int aiffwriteheader(sox_format_t *, uint64_t);
static int aifcwriteheader(sox_format_t *, uint64_t);
static void write_ieee_extended(sox_format_t *, double);
static double ConvertFromIeeeExtended(unsigned char*);
static void ConvertToIeeeExtended(double, char *);
static int textChunk(char **text, char *chunkDescription, sox_format_t * ft);
static int commentChunk(char **text, char *chunkDescription, sox_format_t * ft);
static void reportInstrument(sox_format_t * ft);

int lsx_aiffstartread(sox_format_t * ft)
{
  char buf[5];
  uint32_t totalsize;
  uint32_t chunksize;
  unsigned short channels = 0;
  sox_encoding_t enc = SOX_ENCODING_SIGN2;
  uint32_t frames;
  unsigned short bits = 0;
  double rate = 0.0;
  uint32_t offset = 0;
  uint32_t blocksize = 0;
  int foundcomm = 0, foundmark = 0, foundinstr = 0, is_sowt = 0;
  struct mark {
    unsigned short id;
    uint32_t position;
    char name[40];
  } marks[32];
  unsigned short looptype;
  int i, j;
  unsigned short nmarks = 0;
  unsigned short sustainLoopBegin = 0, sustainLoopEnd = 0,
                 releaseLoopBegin = 0, releaseLoopEnd = 0;
  off_t seekto = 0;
  size_t ssndsize = 0;
  char *annotation;
  char *author;
  char *copyright;
  char *nametext;

  uint8_t trash8;
  uint16_t trash16;
  uint32_t trash32;

  int rc;

  /* FORM chunk */
  if (lsx_reads(ft, buf, (size_t)4) == SOX_EOF || strncmp(buf, "FORM", (size_t)4) != 0) {
    lsx_fail_errno(ft,SOX_EHDR,"AIFF header does not begin with magic word `FORM'");
    return(SOX_EOF);
  }
  lsx_readdw(ft, &totalsize);
  if (lsx_reads(ft, buf, (size_t)4) == SOX_EOF || (strncmp(buf, "AIFF", (size_t)4) != 0 &&
        strncmp(buf, "AIFC", (size_t)4) != 0)) {
    lsx_fail_errno(ft,SOX_EHDR,"AIFF `FORM' chunk does not specify `AIFF' or `AIFC' as type");
    return(SOX_EOF);
  }


  /* Skip everything but the COMM chunk and the SSND chunk */
  /* The SSND chunk must be the last in the file */
  while (1) {
    if (lsx_reads(ft, buf, (size_t)4) == SOX_EOF) {
      if (seekto > 0)
        break;
      else {
        lsx_fail_errno(ft,SOX_EHDR,"Missing SSND chunk in AIFF file");
        return(SOX_EOF);
      }
    }
    if (strncmp(buf, "COMM", (size_t)4) == 0) {
      /* COMM chunk */
      lsx_readdw(ft, &chunksize);
      lsx_readw(ft, &channels);
      lsx_readdw(ft, &frames);
      lsx_readw(ft, &bits);
      rate = read_ieee_extended(ft);
      chunksize -= 18;
      if (chunksize > 0) {
        lsx_reads(ft, buf, (size_t)4);
        chunksize -= 4;
        if (strncmp(buf, "sowt", (size_t)4) == 0) {
          /* CD audio as read on Mac OS machines */
          /* Need to endian swap all the data */
          is_sowt = 1;
        }
        else if (strncmp(buf, "fl32", (size_t)4) == 0 ||
            strncmp(buf, "FL32", (size_t)4) == 0) {
          enc = SOX_ENCODING_FLOAT;
          if (bits != 32) {
            lsx_fail_errno(ft, SOX_EHDR,
              "Sample size of %u is not consistent with `fl32' compression type", bits);
            return SOX_EOF;
          }
        }
        else if (strncmp(buf, "fl64", (size_t)4) == 0 ||
            strncmp(buf, "FL64", (size_t)4) == 0) {
          enc = SOX_ENCODING_FLOAT;
          if (bits != 64) {
            lsx_fail_errno(ft, SOX_EHDR,
              "Sample size of %u is not consistent with `fl64' compression type", bits);
            return SOX_EOF;
          }
        }
        else if (strncmp(buf, "NONE", (size_t)4) != 0 &&
            strncmp(buf, "twos", (size_t)4) != 0) {
          buf[4] = 0;
          lsx_fail_errno(ft, SOX_EHDR, "Unsupported AIFC compression type `%s'", buf);
          return(SOX_EOF);
        }
      }
      while(chunksize-- > 0)
        lsx_readb(ft, &trash8);
      foundcomm = 1;
    }
    else if (strncmp(buf, "SSND", (size_t)4) == 0) {
      /* SSND chunk */
      lsx_readdw(ft, &chunksize);
      lsx_readdw(ft, &offset);
      lsx_readdw(ft, &blocksize);
      chunksize -= 8;
      ssndsize = chunksize;
      /* word-align chunksize in case it wasn't
       * done by writing application already.
       */
      chunksize += (chunksize % 2);
      /* if can't seek, just do sound now */
      if (!ft->seekable)
        break;
      /* else, seek to end of sound and hunt for more */
      seekto = lsx_tell(ft);
      lsx_seeki(ft, (off_t)chunksize, SEEK_CUR);
    }
    else if (strncmp(buf, "MARK", (size_t)4) == 0) {
      /* MARK chunk */
      lsx_readdw(ft, &chunksize);
      if (chunksize >= sizeof(nmarks)) {
        lsx_readw(ft, &nmarks);
        chunksize -= sizeof(nmarks);
      }
      else nmarks = 0;

      /* Some programs like to always have a MARK chunk
       * but will set number of marks to 0 and force
       * software to detect and ignore it.
       */
      if (nmarks == 0)
        foundmark = 0;
      else
        foundmark = 1;

      /* Make sure its not larger then we support */
      if (nmarks > 32)
        nmarks = 32;

      for(i = 0; i < nmarks && chunksize; i++) {
        unsigned char len, read_len, tmp_c;

        if (chunksize < 6)
          break;
        lsx_readw(ft, &(marks[i].id));
        lsx_readdw(ft, &(marks[i].position));
        chunksize -= 6;
        /* If error reading length then
         * don't try to read more bytes
         * based on that value.
         */
        if (lsx_readb(ft, &len) != SOX_SUCCESS)
          break;
        --chunksize;
        if (len > chunksize)
          len = chunksize;
        read_len = len;
        if (read_len > 39)
          read_len = 39;
        for(j = 0; j < len && chunksize; j++) {
          lsx_readb(ft, &tmp_c);
          if (j < read_len)
            marks[i].name[j] = tmp_c;
          chunksize--;
        }
        marks[i].name[read_len] = 0;
        if ((len & 1) == 0 && chunksize) {
          chunksize--;
          lsx_readb(ft, &trash8);
        }
      }
      /* HA HA!  Sound Designer (and others) makes */
      /* bogus files. It spits out bogus chunksize */
      /* for MARK field */
      while(chunksize-- > 0)
        lsx_readb(ft, &trash8);
    }
    else if (strncmp(buf, "INST", (size_t)4) == 0) {
      /* INST chunk */
      lsx_readdw(ft, &chunksize);
      lsx_readsb(ft, &(ft->oob.instr.MIDInote));
      lsx_readb(ft, &trash8);
      lsx_readsb(ft, &(ft->oob.instr.MIDIlow));
      lsx_readsb(ft, &(ft->oob.instr.MIDIhi));
      /* Low  velocity */
      lsx_readb(ft, &trash8);
      /* Hi  velocity */
      lsx_readb(ft, &trash8);
      lsx_readw(ft, &trash16);/* gain */
      lsx_readw(ft, &looptype); /* sustain loop */
      ft->oob.loops[0].type = looptype;
      lsx_readw(ft, &sustainLoopBegin); /* begin marker */
      lsx_readw(ft, &sustainLoopEnd);    /* end marker */
      lsx_readw(ft, &looptype); /* release loop */
      ft->oob.loops[1].type = looptype;
      lsx_readw(ft, &releaseLoopBegin);  /* begin marker */
      lsx_readw(ft, &releaseLoopEnd);    /* end marker */

      foundinstr = 1;
    }
    else if (strncmp(buf, "APPL", (size_t)4) == 0) {
      lsx_readdw(ft, &chunksize);
      /* word-align chunksize in case it wasn't
       * done by writing application already.
       */
      chunksize += (chunksize % 2);
      while(chunksize-- > 0)
        lsx_readb(ft, &trash8);
    }
    else if (strncmp(buf, "ALCH", (size_t)4) == 0) {
      /* I think this is bogus and gets grabbed by APPL */
      /* INST chunk */
      lsx_readdw(ft, &trash32);                /* ENVS - jeez! */
      lsx_readdw(ft, &chunksize);
      while(chunksize-- > 0)
        lsx_readb(ft, &trash8);
    }
    else if (strncmp(buf, "ANNO", (size_t)4) == 0) {
      rc = textChunk(&annotation, "Annotation:", ft);
      if (rc) {
        /* Fail already called in function */
        return(SOX_EOF);
      }
      if (annotation)
        sox_append_comments(&ft->oob.comments, annotation);
      free(annotation);
    }
    else if (strncmp(buf, "COMT", (size_t)4) == 0) {
      char *comment = NULL;
      rc = commentChunk(&comment, "Comment:", ft);
      if (rc) {
        /* Fail already called in function */
        return(SOX_EOF);
      }
      if (comment)
        sox_append_comments(&ft->oob.comments, comment);
      free(comment);
    }
    else if (strncmp(buf, "AUTH", (size_t)4) == 0) {
      /* Author chunk */
      rc = textChunk(&author, "Author:", ft);
      if (rc) {
        /* Fail already called in function */
        return(SOX_EOF);
      }
      free(author);
    }
    else if (strncmp(buf, "NAME", (size_t)4) == 0) {
      /* Name chunk */
      rc = textChunk(&nametext, "Name:", ft);
      if (rc) {
        /* Fail already called in function */
        return(SOX_EOF);
      }
      free(nametext);
    }
    else if (strncmp(buf, "(c) ", (size_t)4) == 0) {
      /* Copyright chunk */
      rc = textChunk(&copyright, "Copyright:", ft);
      if (rc) {
        /* Fail already called in function */
        return(SOX_EOF);
      }
      free(copyright);
    }
    else if (strncmp(buf, "ID3 ", 4) == 0) {
      off_t offs;
      lsx_readdw(ft, &chunksize);
      offs = lsx_tell(ft);
      lsx_id3_read_tag(ft, 0);
      lsx_seeki(ft, offs + chunksize, SEEK_SET);
    }
    else {
      if (lsx_eof(ft))
        break;
      buf[4] = 0;
      lsx_debug("AIFFstartread: ignoring `%s' chunk", buf);
      lsx_readdw(ft, &chunksize);
      if (lsx_eof(ft))
        break;
      /* account for padding after odd-sized chunks */
      chunksize += chunksize & 1;
      /* Skip the chunk using lsx_readb() so we may read
         from a pipe */
      while (chunksize-- > 0) {
        if (lsx_readb(ft, &trash8) == SOX_EOF)
          break;
      }
    }
    if (lsx_eof(ft))
      break;
  }

  /*
   * if a pipe, we lose all chunks after sound.
   * Like, say, instrument loops.
   */
  if (ft->seekable) {
    if (seekto > 0)
      lsx_seeki(ft, seekto, SEEK_SET);
    else {
      lsx_fail_errno(ft,SOX_EOF,"AIFF: no sound data on input file");
      return(SOX_EOF);
    }
  }
  /* SSND chunk just read */
  if (blocksize != 0)
    lsx_warn("AIFF header has invalid blocksize.  Ignoring but expect a premature EOF");

  ssndsize -= offset;
  while (offset-- > 0) {
    if (lsx_readb(ft, &trash8) == SOX_EOF) {
      lsx_fail_errno(ft,errno,"unexpected EOF while skipping AIFF offset");
      return(SOX_EOF);
    }
  }

  if (foundcomm) {
    if      (bits <=  8) bits = 8;
    else if (bits <= 16) bits = 16;
    else if (bits <= 24) bits = 24;
    else if (bits <= 32) bits = 32;
    else if (bits == 64 && enc == SOX_ENCODING_FLOAT) /* no-op */;
    else {
      lsx_fail_errno(ft,SOX_EFMT,"unsupported sample size in AIFF header: %d", bits);
      return(SOX_EOF);
    }
  } else  {
    if ((ft->signal.channels == SOX_UNSPEC)
        || (ft->signal.rate == SOX_UNSPEC)
        || (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
        || (ft->encoding.bits_per_sample == 0)) {
      lsx_report("You must specify # channels, sample rate, signed/unsigned,");
      lsx_report("and 8/16 on the command line.");
      lsx_fail_errno(ft,SOX_EFMT,"Bogus AIFF file: no COMM section.");
      return(SOX_EOF);
    }

  }
  ssndsize /= bits >> 3;

  /* Cope with 'sowt' CD tracks as read on Macs */
  if (is_sowt)
    ft->encoding.reverse_bytes = !ft->encoding.reverse_bytes;

  if (foundmark && !foundinstr) {
    lsx_debug("Ignoring MARK chunk since no INSTR found.");
    foundmark = 0;
  }
  if (!foundmark && foundinstr) {
    lsx_debug("Ignoring INSTR chunk since no MARK found.");
    foundinstr = 0;
  }
  if (foundmark && foundinstr) {
    int i2;
    int slbIndex = 0, sleIndex = 0;
    int rlbIndex = 0, rleIndex = 0;

    /* find our loop markers and save their marker indexes */
    for(i2 = 0; i2 < nmarks; i2++) {
      if(marks[i2].id == sustainLoopBegin)
        slbIndex = i2;
      if(marks[i2].id == sustainLoopEnd)
        sleIndex = i2;
      if(marks[i2].id == releaseLoopBegin)
        rlbIndex = i2;
      if(marks[i2].id == releaseLoopEnd)
        rleIndex = i2;
    }

    ft->oob.instr.nloops = 0;
    if (ft->oob.loops[0].type != 0) {
      ft->oob.loops[0].start = marks[slbIndex].position;
      ft->oob.loops[0].length =
        marks[sleIndex].position - marks[slbIndex].position;
      /* really the loop count should be infinite */
      ft->oob.loops[0].count = 1;
      ft->oob.instr.loopmode = SOX_LOOP_SUSTAIN_DECAY | ft->oob.loops[0].type;
      ft->oob.instr.nloops++;
    }
    if (ft->oob.loops[1].type != 0) {
      ft->oob.loops[1].start = marks[rlbIndex].position;
      ft->oob.loops[1].length =
        marks[rleIndex].position - marks[rlbIndex].position;
      /* really the loop count should be infinite */
      ft->oob.loops[1].count = 1;
      ft->oob.instr.loopmode = SOX_LOOP_SUSTAIN_DECAY | ft->oob.loops[1].type;
      ft->oob.instr.nloops++;
    }
  }
  reportInstrument(ft);

  return lsx_check_read_params(
      ft, channels, rate, enc, bits, (uint64_t)ssndsize, sox_false);
}

/* print out the MIDI key allocations, loop points, directions etc */
static void reportInstrument(sox_format_t * ft)
{
  unsigned loopNum;

  if(ft->oob.instr.nloops > 0)
    lsx_report("AIFF Loop markers:");
  for(loopNum  = 0; loopNum < ft->oob.instr.nloops; loopNum++) {
    if (ft->oob.loops[loopNum].count) {
      lsx_report("Loop %d: start: %6lu", loopNum, (unsigned long)ft->oob.loops[loopNum].start);
      lsx_report(" end:   %6lu",
              (unsigned long)(ft->oob.loops[loopNum].start + ft->oob.loops[loopNum].length));
      lsx_report(" count: %6d", ft->oob.loops[loopNum].count);
      lsx_report(" type:  ");
      switch(ft->oob.loops[loopNum].type & ~SOX_LOOP_SUSTAIN_DECAY) {
      case 0: lsx_report("off"); break;
      case 1: lsx_report("forward"); break;
      case 2: lsx_report("forward/backward"); break;
      }
    }
  }
  lsx_report("Unity MIDI Note: %d", ft->oob.instr.MIDInote);
  lsx_report("Low   MIDI Note: %d", ft->oob.instr.MIDIlow);
  lsx_report("High  MIDI Note: %d", ft->oob.instr.MIDIhi);
}

/* Process a text chunk, allocate memory, display it if verbose and return */
static int textChunk(char **text, char *chunkDescription, sox_format_t * ft)
{
  uint32_t chunksize0;
  size_t chunksize;
  lsx_readdw(ft, &chunksize0);
  chunksize = chunksize0;

  /* allocate enough memory to hold the text including a terminating \0 */
  if (chunksize != SOX_SIZE_MAX)
    *text = lsx_malloc((size_t)chunksize+1);
  else
    *text = lsx_malloc((size_t)chunksize);

  if (lsx_readbuf(ft, *text, (size_t) chunksize) != chunksize)
  {
    lsx_fail_errno(ft,SOX_EOF,"AIFF: Unexpected EOF in %s header", chunkDescription);
    return(SOX_EOF);
  }
  if (chunksize != SOX_SIZE_MAX)
    *(*text + chunksize) = '\0';
  else
    *(*text + chunksize-1) = '\0';
  if (chunksize % 2)
  {
    /* Read past pad byte */
    char c;
    if (lsx_readbuf(ft, &c, (size_t)1) != 1)
    {
      lsx_fail_errno(ft,SOX_EOF,"AIFF: Unexpected EOF in %s header", chunkDescription);
      return(SOX_EOF);
    }
  }
  lsx_debug("%-10s   \"%s\"", chunkDescription, *text);
  return(SOX_SUCCESS);
}

/* Comment lengths are words, not double words, and we can have several, so
   we use a special function, not textChunk().;
 */
static int commentChunk(char **text, char *chunkDescription, sox_format_t * ft)
{
  uint32_t chunksize;
  unsigned short numComments;
  uint32_t timeStamp;
  unsigned short markerId;
  unsigned short totalCommentLength = 0;
  unsigned int totalReadLength = 0;
  unsigned int commentIndex;

  lsx_readdw(ft, &chunksize);
  lsx_readw(ft, &numComments);
  totalReadLength += 2; /* chunksize doesn't count */
  for(commentIndex = 0; commentIndex < numComments; commentIndex++) {
    unsigned short commentLength;

    lsx_readdw(ft, &timeStamp);
    lsx_readw(ft, &markerId);
    lsx_readw(ft, &commentLength);
    if (((size_t)totalCommentLength) + commentLength > USHRT_MAX) {
        lsx_fail_errno(ft,SOX_EOF,"AIFF: Comment too long in %s header", chunkDescription);
        return(SOX_EOF);
    }
    totalCommentLength += commentLength;
    /* allocate enough memory to hold the text including a terminating \0 */
    if(commentIndex == 0) {
      *text = lsx_malloc((size_t) totalCommentLength + 1);
    }
    else {
      *text = lsx_realloc(*text, (size_t) totalCommentLength + 1);
    }

    if (lsx_readbuf(ft, *text + totalCommentLength - commentLength, (size_t) commentLength) != commentLength) {
        lsx_fail_errno(ft,SOX_EOF,"AIFF: Unexpected EOF in %s header", chunkDescription);
        return(SOX_EOF);
    }
    *(*text + totalCommentLength) = '\0';
    totalReadLength += totalCommentLength + 4 + 2 + 2; /* include header */
    if (commentLength % 2) {
        /* Read past pad byte */
        char c;
        if (lsx_readbuf(ft, &c, (size_t)1) != 1) {
            lsx_fail_errno(ft,SOX_EOF,"AIFF: Unexpected EOF in %s header", chunkDescription);
            return(SOX_EOF);
        }
        totalReadLength += 1;
    }
  }
  lsx_debug("%-10s   \"%s\"", chunkDescription, *text);
  /* make sure we read the whole chunk */
  if (totalReadLength < chunksize) {
       size_t i;
       char c;
       for (i=0; i < chunksize - totalReadLength; i++ )
           lsx_readbuf(ft, &c, (size_t)1);
  }
  return(SOX_SUCCESS);
}

int lsx_aiffstopread(sox_format_t * ft)
{
        char buf[5];
        uint32_t chunksize;
        uint8_t trash;

        if (!ft->seekable)
        {
            while (! lsx_eof(ft))
            {
                if (lsx_readbuf(ft, buf, (size_t)4) != 4)
                        break;

                lsx_readdw(ft, &chunksize);
                if (lsx_eof(ft))
                        break;
                buf[4] = '\0';
                lsx_warn("Ignoring AIFF tail chunk: `%s', %u bytes long",
                        buf, chunksize);
                if (! strcmp(buf, "MARK") || ! strcmp(buf, "INST"))
                        lsx_warn("       You're stripping MIDI/loop info!");
                while (chunksize-- > 0)
                {
                        if (lsx_readb(ft, &trash) == SOX_EOF)
                                break;
                }
            }
        }
        return SOX_SUCCESS;
}

/* When writing, the header is supposed to contain the number of
   samples and data bytes written.
   Since we don't know how many samples there are until we're done,
   we first write the header with an very large number,
   and at the end we rewind the file and write the header again
   with the right number.  This only works if the file is seekable;
   if it is not, the very large size remains in the header.
   Strictly spoken this is not legal, but the playaiff utility
   will still be able to play the resulting file. */

int lsx_aiffstartwrite(sox_format_t * ft)
{
        int rc;

        /* Needed because lsx_rawwrite() */
        rc = lsx_rawstartwrite(ft);
        if (rc)
            return rc;

        /* Compute the "very large number" so that a maximum number
           of samples can be transmitted through a pipe without the
           risk of causing overflow when calculating the number of bytes.
           At 48 kHz, 16 bits stereo, this gives ~3 hours of audio.
           Sorry, the AIFF format does not provide for an indefinite
           number of samples. */
        return(aiffwriteheader(ft, (uint64_t) 0x7f000000 / ((ft->encoding.bits_per_sample>>3)*ft->signal.channels)));
}

int lsx_aiffstopwrite(sox_format_t * ft)
{
        /* If we've written an odd number of bytes, write a padding
           NUL */
        if (ft->olength % 2 == 1 && ft->encoding.bits_per_sample == 8 && ft->signal.channels == 1)
        {
            sox_sample_t buf = 0;
            lsx_rawwrite(ft, &buf, (size_t) 1);
        }

        if (!ft->seekable)
        {
            lsx_fail_errno(ft,SOX_EOF,"Non-seekable file.");
            return(SOX_EOF);
        }
        if (lsx_seeki(ft, (off_t)0, SEEK_SET) != 0)
        {
                lsx_fail_errno(ft,errno,"can't rewind output file to rewrite AIFF header");
                return(SOX_EOF);
        }
        return(aiffwriteheader(ft, ft->olength / ft->signal.channels));
}

static int aiffwriteheader(sox_format_t * ft, uint64_t nframes)
{
        int hsize =
                8 /*COMM hdr*/ + 18 /*COMM chunk*/ +
                8 /*SSND hdr*/ + 12 /*SSND chunk*/;
        unsigned bits = 0;
        unsigned i;
        uint64_t size;
        size_t padded_comment_size = 0, comment_size = 0;
        size_t comment_chunk_size = 0;
        char * comment = lsx_cat_comments(ft->oob.comments);

        /* MARK and INST chunks */
        if (ft->oob.instr.nloops) {
          hsize += 8 /* MARK hdr */ + 2 + 16*ft->oob.instr.nloops;
          hsize += 8 /* INST hdr */ + 20; /* INST chunk */
        }

        if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
            ft->encoding.bits_per_sample == 8)
                bits = 8;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 16)
                bits = 16;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 24)
                bits = 24;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 32)
                bits = 32;
        else
        {
                lsx_fail_errno(ft,SOX_EFMT,"unsupported output encoding/size for AIFF header");
                return(SOX_EOF);
        }

        /* COMT comment chunk -- holds comments text with a timestamp and marker id */
        /* We calculate the comment_chunk_size if we will be writing a comment */
        if (ft->oob.comments)
        {
          comment_size = strlen(comment);
          /* Must put an even number of characters out.
           * True 68k processors OS's seem to require this.
           */
          padded_comment_size = ((comment_size % 2) == 0) ?
                                comment_size : comment_size + 1;
          /* one comment, timestamp, marker ID and text count */
          comment_chunk_size = (2 + 4 + 2 + 2 + padded_comment_size);
          hsize += 8 /* COMT hdr */ + comment_chunk_size;
        }

        lsx_writes(ft, "FORM"); /* IFF header */
        /* file size */
        size = hsize + nframes * (ft->encoding.bits_per_sample >> 3) * ft->signal.channels;
        if (size > UINT_MAX)
        {
            lsx_warn("file size too big for accurate AIFF header");
            size = UINT_MAX;
        }
        lsx_writedw(ft, (unsigned)size);
        lsx_writes(ft, "AIFF"); /* File type */

        /* Now we write the COMT comment chunk using the precomputed sizes */
        if (ft->oob.comments)
        {
          lsx_writes(ft, "COMT");
          lsx_writedw(ft, (unsigned) comment_chunk_size);

          /* one comment */
          lsx_writew(ft, 1);

          /* time stamp of comment, Unix knows of time from 1/1/1970,
             Apple knows time from 1/1/1904 */
          lsx_writedw(ft, (unsigned)((sox_globals.repeatable? 0 : time(NULL)) + 2082844800));

          /* A marker ID of 0 indicates the comment is not associated
             with a marker */
          lsx_writew(ft, 0);

          /* now write the count and the bytes of text */
          lsx_writew(ft, (unsigned) padded_comment_size);
          lsx_writes(ft, comment);
          if (comment_size != padded_comment_size)
                lsx_writes(ft, " ");
        }
        free(comment);

        /* COMM chunk -- describes encoding (and #frames) */
        lsx_writes(ft, "COMM");
        lsx_writedw(ft, 18); /* COMM chunk size */
        lsx_writew(ft, ft->signal.channels); /* nchannels */
        lsx_writedw(ft, (unsigned) nframes); /* number of frames */
        lsx_writew(ft, bits); /* sample width, in bits */
        write_ieee_extended(ft, (double)ft->signal.rate);

        /* MARK chunk -- set markers */
        if (ft->oob.instr.nloops) {
                lsx_writes(ft, "MARK");
                if (ft->oob.instr.nloops > 2)
                        ft->oob.instr.nloops = 2;
                lsx_writedw(ft, 2 + 16u*ft->oob.instr.nloops);
                lsx_writew(ft, ft->oob.instr.nloops);

                for(i = 0; i < ft->oob.instr.nloops; i++) {
                        unsigned start = ft->oob.loops[i].start > UINT_MAX
                            ? UINT_MAX
                            : ft->oob.loops[i].start;
                        unsigned end = ft->oob.loops[i].start + ft->oob.loops[i].length > UINT_MAX
                            ? UINT_MAX
                            : ft->oob.loops[i].start + ft->oob.loops[i].length;
                        lsx_writew(ft, i + 1);
                        lsx_writedw(ft, start);
                        lsx_writeb(ft, 0);
                        lsx_writeb(ft, 0);
                        lsx_writew(ft, i*2 + 1);
                        lsx_writedw(ft, end);
                        lsx_writeb(ft, 0);
                        lsx_writeb(ft, 0);
                }

                lsx_writes(ft, "INST");
                lsx_writedw(ft, 20);
                /* random MIDI shit that we default on */
                lsx_writeb(ft, (uint8_t)ft->oob.instr.MIDInote);
                lsx_writeb(ft, 0);                       /* detune */
                lsx_writeb(ft, (uint8_t)ft->oob.instr.MIDIlow);
                lsx_writeb(ft, (uint8_t)ft->oob.instr.MIDIhi);
                lsx_writeb(ft, 1);                       /* low velocity */
                lsx_writeb(ft, 127);                     /* hi  velocity */
                lsx_writew(ft, 0);                               /* gain */

                /* sustain loop */
                lsx_writew(ft, ft->oob.loops[0].type);
                lsx_writew(ft, 1);                               /* marker 1 */
                lsx_writew(ft, 3);                               /* marker 3 */
                /* release loop, if there */
                if (ft->oob.instr.nloops == 2) {
                        lsx_writew(ft, ft->oob.loops[1].type);
                        lsx_writew(ft, 2);                       /* marker 2 */
                        lsx_writew(ft, 4);                       /* marker 4 */
                } else {
                        lsx_writew(ft, 0);                       /* no release loop */
                        lsx_writew(ft, 0);
                        lsx_writew(ft, 0);
                }
        }

        /* SSND chunk -- describes data */
        lsx_writes(ft, "SSND");
        /* chunk size */
        lsx_writedw(ft, (unsigned) (8 + nframes * ft->signal.channels * (ft->encoding.bits_per_sample >> 3)));
        lsx_writedw(ft, 0); /* offset */
        lsx_writedw(ft, 0); /* block size */
        return(SOX_SUCCESS);
}

int lsx_aifcstartwrite(sox_format_t * ft)
{
        int rc;

        /* Needed because lsx_rawwrite() */
        rc = lsx_rawstartwrite(ft);
        if (rc)
            return rc;

        /* Compute the "very large number" so that a maximum number
           of samples can be transmitted through a pipe without the
           risk of causing overflow when calculating the number of bytes.
           At 48 kHz, 16 bits stereo, this gives ~3 hours of music.
           Sorry, the AIFC format does not provide for an "infinite"
           number of samples. */
        return(aifcwriteheader(ft, (uint64_t) 0x7f000000 / ((ft->encoding.bits_per_sample >> 3)*ft->signal.channels)));
}

int lsx_aifcstopwrite(sox_format_t * ft)
{
        /* If we've written an odd number of bytes, write a padding
           NUL */
        if (ft->olength % 2 == 1 && ft->encoding.bits_per_sample == 8 && ft->signal.channels == 1)
        {
            sox_sample_t buf = 0;
            lsx_rawwrite(ft, &buf, (size_t) 1);
        }

        if (!ft->seekable)
        {
            lsx_fail_errno(ft,SOX_EOF,"Non-seekable file.");
            return(SOX_EOF);
        }
        if (lsx_seeki(ft, (off_t)0, SEEK_SET) != 0)
        {
                lsx_fail_errno(ft,errno,"can't rewind output file to rewrite AIFC header");
                return(SOX_EOF);
        }
        return(aifcwriteheader(ft, ft->olength / ft->signal.channels));
}

static int aifcwriteheader(sox_format_t * ft, uint64_t nframes)
{
        unsigned hsize;
        unsigned bits = 0;
        uint64_t size;
        char *ctype = NULL, *cname = NULL;
        unsigned cname_len = 0, comm_len = 0, comm_padding = 0;

        if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
            ft->encoding.bits_per_sample == 8)
                bits = 8;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 16)
                bits = 16;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 24)
                bits = 24;
        else if (ft->encoding.encoding == SOX_ENCODING_SIGN2 &&
                 ft->encoding.bits_per_sample == 32)
                bits = 32;
        else if (ft->encoding.encoding == SOX_ENCODING_FLOAT &&
                 ft->encoding.bits_per_sample == 32)
                bits = 32;
        else if (ft->encoding.encoding == SOX_ENCODING_FLOAT &&
                 ft->encoding.bits_per_sample == 64)
                bits = 64;
        else
        {
                lsx_fail_errno(ft,SOX_EFMT,"unsupported output encoding/size for AIFC header");
                return(SOX_EOF);
        }

        /* calculate length of COMM chunk (without header) */
        switch (ft->encoding.encoding) {
          case SOX_ENCODING_SIGN2:
            ctype = "NONE";
            cname = "not compressed";
            break;
          case SOX_ENCODING_FLOAT:
            if (bits == 32) {
              ctype = "fl32";
              cname = "32-bit floating point";
            } else {
              ctype = "fl64";
              cname = "64-bit floating point";
            }
            break;
          default: /* can't happen */
            break;
        }
        cname_len = strlen(cname);
        comm_len = 18+4+1+cname_len;
        comm_padding = comm_len%2;

        hsize = 12 /*FVER*/ + 8 /*COMM hdr*/ + comm_len+comm_padding /*COMM chunk*/ +
                8 /*SSND hdr*/ + 12 /*SSND chunk*/;

        lsx_writes(ft, "FORM"); /* IFF header */
        /* file size */
        size = hsize + nframes * (ft->encoding.bits_per_sample >> 3) * ft->signal.channels;
        if (size > UINT_MAX)
        {
            lsx_warn("file size too big for accurate AIFC header");
            size = UINT_MAX;
        }
        lsx_writedw(ft, (unsigned)size);
        lsx_writes(ft, "AIFC"); /* File type */

        /* FVER chunk */
        lsx_writes(ft, "FVER");
        lsx_writedw(ft, 4); /* FVER chunk size */
        lsx_writedw(ft, 0xa2805140); /* version_date(May23,1990,2:40pm) */

        /* COMM chunk -- describes encoding (and #frames) */
        lsx_writes(ft, "COMM");
        lsx_writedw(ft, comm_len+comm_padding); /* COMM chunk size */
        lsx_writew(ft, ft->signal.channels); /* nchannels */
        lsx_writedw(ft, (unsigned) nframes); /* number of frames */
        lsx_writew(ft, bits); /* sample width, in bits */
        write_ieee_extended(ft, (double)ft->signal.rate);

        lsx_writes(ft, ctype); /*compression_type*/
        lsx_writeb(ft, cname_len);
        lsx_writes(ft, cname);
        if (comm_padding)
          lsx_writeb(ft, 0);

        /* SSND chunk -- describes data */
        lsx_writes(ft, "SSND");
        /* chunk size */
        lsx_writedw(ft, (unsigned) (8 + nframes * ft->signal.channels * (ft->encoding.bits_per_sample >> 3)));
        lsx_writedw(ft, 0); /* offset */
        lsx_writedw(ft, 0); /* block size */

        /* Any Private chunks shall appear after the required chunks (FORM,FVER,COMM,SSND) */
        return(SOX_SUCCESS);
}

static double read_ieee_extended(sox_format_t * ft)
{
        unsigned char buf[10];
        if (lsx_readbuf(ft, buf, (size_t)10) != 10)
        {
                lsx_fail_errno(ft,SOX_EOF,"EOF while reading IEEE extended number");
                return(SOX_EOF);
        }
        return ConvertFromIeeeExtended(buf);
}

static void write_ieee_extended(sox_format_t * ft, double x)
{
        char buf[10];
        ConvertToIeeeExtended(x, buf);
        lsx_debug_more("converted %g to %o %o %o %o %o %o %o %o %o %o",
                x,
                buf[0], buf[1], buf[2], buf[3], buf[4],
                buf[5], buf[6], buf[7], buf[8], buf[9]);
        (void)lsx_writebuf(ft, buf, (size_t) 10);
}


/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 *
 * All rights reserved.
 *
 * Warranty Information
 *  Even though Apple has reviewed this software, Apple makes no warranty
 *  or representation, either express or implied, with respect to this
 *  software, its quality, accuracy, merchantability, or fitness for a
 *  particular purpose.  As a result, this software is provided "as is,"
 *  and you, its user, are assuming the entire risk as to its quality
 *  and accuracy.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#define FloatToUnsigned(f) ((uint32_t)(((int32_t)(f - 2147483648.0)) + 2147483647) + 1)

static void ConvertToIeeeExtended(double num, char *bytes)
{
    int    sign;
    int expon;
    double fMant, fsMant;
    uint32_t hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);
            fsMant = floor(fMant);
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32);
            fsMant = floor(fMant);
            loMant = FloatToUnsigned(fsMant);
        }
    }

    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}


/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D
 */

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 *
 * All rights reserved.
 *
 * Warranty Information
 *  Even though Apple has reviewed this software, Apple makes no warranty
 *  or representation, either express or implied, with respect to this
 *  software, its quality, accuracy, merchantability, or fitness for a
 *  particular purpose.  As a result, this software is provided "as is,"
 *  and you, its user, are assuming the entire risk as to its quality
 *  and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the above warranty information.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#define UnsignedToFloat(u)         (((double)((int32_t)(u - 2147483647 - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

static double ConvertFromIeeeExtended(unsigned char *bytes)
{
    double    f;
    int    expon;
    uint32_t hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant    =    ((uint32_t)(bytes[2] & 0xFF) << 24)
            |    ((uint32_t)(bytes[3] & 0xFF) << 16)
            |    ((uint32_t)(bytes[4] & 0xFF) << 8)
            |    ((uint32_t)(bytes[5] & 0xFF));
    loMant    =    ((uint32_t)(bytes[6] & 0xFF) << 24)
            |    ((uint32_t)(bytes[7] & 0xFF) << 16)
            |    ((uint32_t)(bytes[8] & 0xFF) << 8)
            |    ((uint32_t)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) {    /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
            f += ldexp(UnsignedToFloat(loMant), expon-=32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}
