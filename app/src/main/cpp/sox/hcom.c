/* libSoX Macintosh HCOM format.
 * These are really FSSD type files with Huffman compression,
 * in MacBinary format.
 * TODO: make the MacBinary format optional (so that .data files
 * are also acceptable).  (How to do this on output?)
 *
 * September 25, 1991
 * Copyright 1991 Guido van Rossum And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Guido van Rossum And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * April 28, 1998 - Chris Bagwell (cbagwell@sprynet.com)
 *
 *  Rearranged some functions so that they are declared before they are
 *  used, clearing up some compiler warnings.  Because these functions
 *  passed floats, it helped some dumb compilers pass stuff on the
 *  stack correctly.
 *
 */

#include "sox_i.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* FIXME: eliminate these 2 functions */

static void put32_be(unsigned char **p, int32_t val)
{
  *(*p)++ = (val >> 24) & 0xff;
  *(*p)++ = (val >> 16) & 0xff;
  *(*p)++ = (val >> 8) & 0xff;
  *(*p)++ = val & 0xff;
}

static void put16_be(unsigned char **p, int val)
{
  *(*p)++ = (val >> 8) & 0xff;
  *(*p)++ = val & 0xff;
}

/* Dictionary entry for Huffman (de)compression */
typedef struct {
        long frequ;
        short dict_leftson;
        short dict_rightson;
} dictent;

typedef struct {
  /* Static data from the header */
  dictent *dictionary;
  int32_t checksum;
  int deltacompression;
  /* Engine state */
  long huffcount;
  long cksum;
  int dictentry;
  int nrbits;
  uint32_t current;
  short sample;
  /* Dictionary */
  dictent *de;
  int32_t new_checksum;
  int nbits;
  int32_t curword;

  /* Private data used by writer */
  unsigned char *data;          /* Buffer allocated with lsx_malloc */
  size_t size;               /* Size of allocated buffer */
  size_t pos;                /* Where next byte goes */
} priv_t;

static int dictvalid(int n, int size, int left, int right)
{
        if (n > 0 && left < 0)
                return 1;

        return (unsigned)left < size && (unsigned)right < size;
}

static int startread(sox_format_t * ft)
{
        priv_t *p = (priv_t *) ft->priv;
        int i;
        char buf[5];
        uint32_t datasize, rsrcsize;
        uint32_t huffcount, checksum, compresstype, divisor;
        unsigned short dictsize;
        int rc;


        /* Skip first 65 bytes of header */
        rc = lsx_skipbytes(ft, (size_t) 65);
        if (rc)
            return rc;

        /* Check the file type (bytes 65-68) */
        if (lsx_reads(ft, buf, (size_t)4) == SOX_EOF || strncmp(buf, "FSSD", (size_t)4) != 0)
        {
                lsx_fail_errno(ft,SOX_EHDR,"Mac header type is not FSSD");
                return (SOX_EOF);
        }

        /* Skip to byte 83 */
        rc = lsx_skipbytes(ft, (size_t) 83-69);
        if (rc)
            return rc;

        /* Get essential numbers from the header */
        lsx_readdw(ft, &datasize); /* bytes 83-86 */
        lsx_readdw(ft, &rsrcsize); /* bytes 87-90 */

        /* Skip the rest of the header (total 128 bytes) */
        rc = lsx_skipbytes(ft, (size_t) 128-91);
        if (rc != 0)
            return rc;

        /* The data fork must contain a "HCOM" header */
        if (lsx_reads(ft, buf, (size_t)4) == SOX_EOF || strncmp(buf, "HCOM", (size_t)4) != 0)
        {
                lsx_fail_errno(ft,SOX_EHDR,"Mac data fork is not HCOM");
                return (SOX_EOF);
        }

        /* Then follow various parameters */
        lsx_readdw(ft, &huffcount);
        lsx_readdw(ft, &checksum);
        lsx_readdw(ft, &compresstype);
        if (compresstype > 1)
        {
                lsx_fail_errno(ft,SOX_EHDR,"Bad compression type in HCOM header");
                return (SOX_EOF);
        }
        lsx_readdw(ft, &divisor);
        if (divisor == 0 || divisor > 4)
        {
                lsx_fail_errno(ft,SOX_EHDR,"Bad sampling rate divisor in HCOM header");
                return (SOX_EOF);
        }
        lsx_readw(ft, &dictsize);

        /* Translate to sox parameters */
        ft->encoding.encoding = SOX_ENCODING_HCOM;
        ft->encoding.bits_per_sample = 8;
        ft->signal.rate = 22050 / divisor;
        ft->signal.channels = 1;
        ft->signal.length = huffcount;

        /* Allocate memory for the dictionary */
        p->dictionary = lsx_malloc(511 * sizeof(dictent));

        /* Read dictionary */
        for(i = 0; i < dictsize; i++) {
                lsx_readsw(ft, &(p->dictionary[i].dict_leftson));
                lsx_readsw(ft, &(p->dictionary[i].dict_rightson));
                lsx_debug("%d %d",
                       p->dictionary[i].dict_leftson,
                       p->dictionary[i].dict_rightson);
                if (!dictvalid(i, dictsize, p->dictionary[i].dict_leftson,
                               p->dictionary[i].dict_rightson)) {
                        lsx_fail_errno(ft, SOX_EHDR, "Invalid dictionary");
                        return SOX_EOF;
                }
        }
        rc = lsx_skipbytes(ft, (size_t) 1); /* skip pad byte */
        if (rc)
            return rc;

        /* Initialized the decompression engine */
        p->checksum = checksum;
        p->deltacompression = compresstype;
        if (!p->deltacompression)
                lsx_debug("HCOM data using value compression");
        p->huffcount = huffcount;
        p->cksum = 0;
        p->dictentry = 0;
        p->nrbits = -1; /* Special case to get first byte */

        return (SOX_SUCCESS);
}

static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
        register priv_t *p = (priv_t *) ft->priv;
        int done = 0;
        unsigned char sample_rate;

        if (p->nrbits < 0) {
                /* The first byte is special */
                if (p->huffcount == 0)
                        return 0; /* Don't know if this can happen... */
                if (lsx_readb(ft, &sample_rate) == SOX_EOF)
                {
                        return (0);
                }
                p->sample = sample_rate;
                *buf++ = SOX_UNSIGNED_8BIT_TO_SAMPLE(p->sample,);
                p->huffcount--;
                p->nrbits = 0;
                done++;
                len--;
                if (len == 0)
                        return done;
        }

        while (p->huffcount > 0) {
                if(p->nrbits == 0) {
                        lsx_readdw(ft, &(p->current));
                        if (lsx_eof(ft))
                        {
                                lsx_fail_errno(ft,SOX_EOF,"unexpected EOF in HCOM data");
                                return (0);
                        }
                        p->cksum += p->current;
                        p->nrbits = 32;
                }
                if(p->current & 0x80000000) {
                        p->dictentry =
                                p->dictionary[p->dictentry].dict_rightson;
                } else {
                        p->dictentry =
                                p->dictionary[p->dictentry].dict_leftson;
                }
                p->current = p->current << 1;
                p->nrbits--;
                if(p->dictionary[p->dictentry].dict_leftson < 0) {
                        short datum;
                        datum = p->dictionary[p->dictentry].dict_rightson;
                        if (!p->deltacompression)
                                p->sample = 0;
                        p->sample = (p->sample + datum) & 0xff;
                        p->huffcount--;
                        *buf++ = SOX_UNSIGNED_8BIT_TO_SAMPLE(p->sample,);
                        p->dictentry = 0;
                        done++;
                        len--;
                        if (len == 0)
                                break;
                }
        }

        return done;
}

static int stopread(sox_format_t * ft)
{
        register priv_t *p = (priv_t *) ft->priv;

        if (p->huffcount != 0)
        {
                lsx_fail_errno(ft,SOX_EFMT,"not all HCOM data read");
                return (SOX_EOF);
        }
        if(p->cksum != p->checksum)
        {
                lsx_fail_errno(ft,SOX_EFMT,"checksum error in HCOM data");
                return (SOX_EOF);
        }
        free(p->dictionary);
        p->dictionary = NULL;
        return (SOX_SUCCESS);
}

#define BUFINCR (10*BUFSIZ)

static int startwrite(sox_format_t * ft)
{
  priv_t * p = (priv_t *) ft->priv;

  p->size = BUFINCR;
  p->pos = 0;
  p->data = lsx_malloc(p->size);
  return SOX_SUCCESS;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
  priv_t *p = (priv_t *) ft->priv;
  sox_sample_t datum;
  size_t i;

  if (len == 0)
    return 0;

  if (p->pos == INT32_MAX)
    return SOX_EOF;

  if (p->pos + len > INT32_MAX) {
    lsx_warn("maximum file size exceeded");
    len = INT32_MAX - p->pos;
  }

  if (p->pos + len > p->size) {
    p->size = ((p->pos + len) / BUFINCR + 1) * BUFINCR;
    p->data = lsx_realloc(p->data, p->size);
  }

  for (i = 0; i < len; i++) {
    SOX_SAMPLE_LOCALS;
    datum = *buf++;
    p->data[p->pos++] = SOX_SAMPLE_TO_UNSIGNED_8BIT(datum, ft->clips);
  }

  return len;
}

static void makecodes(int e, int c, int s, int b, dictent newdict[511], long codes[256], long codesize[256])
{
  assert(b);                    /* Prevent stack overflow */
  if (newdict[e].dict_leftson < 0) {
    codes[newdict[e].dict_rightson] = c;
    codesize[newdict[e].dict_rightson] = s;
  } else {
    makecodes(newdict[e].dict_leftson, c, s + 1, b << 1, newdict, codes, codesize);
    makecodes(newdict[e].dict_rightson, c + b, s + 1, b << 1, newdict, codes, codesize);
  }
}

static void putcode(sox_format_t * ft, long codes[256], long codesize[256], unsigned c, unsigned char **df)
{
  priv_t *p = (priv_t *) ft->priv;
  long code, size;
  int i;

  code = codes[c];
  size = codesize[c];
  for(i = 0; i < size; i++) {
    p->curword <<= 1;
    if (code & 1)
      p->curword += 1;
    p->nbits++;
    if (p->nbits == 32) {
      put32_be(df, p->curword);
      p->new_checksum += p->curword;
      p->nbits = 0;
      p->curword = 0;
    }
    code >>= 1;
  }
}

static void compress(sox_format_t * ft, unsigned char **df, int32_t *dl)
{
  priv_t *p = (priv_t *) ft->priv;
  int samplerate;
  unsigned char *datafork = *df;
  unsigned char *ddf, *dfp;
  short dictsize;
  int frequtable[256];
  long codes[256], codesize[256];
  dictent newdict[511];
  int i, sample, j, k, d, l, frequcount;
  int64_t csize;

  sample = *datafork;
  memset(frequtable, 0, sizeof(frequtable));
  memset(codes, 0, sizeof(codes));
  memset(codesize, 0, sizeof(codesize));
  memset(newdict, 0, sizeof(newdict));

  for (i = 1; i < *dl; i++) {
    d = (datafork[i] - (sample & 0xff)) & 0xff; /* creates absolute entries LMS */
    sample = datafork[i];
    datafork[i] = d;
    assert(d >= 0 && d <= 255); /* check our table is accessed correctly */
    frequtable[d]++;
  }
  p->de = newdict;
  for (i = 0; i < 256; i++)
    if (frequtable[i] != 0) {
      p->de->frequ = -frequtable[i];
      p->de->dict_leftson = -1;
      p->de->dict_rightson = i;
      p->de++;
    }
  frequcount = p->de - newdict;
  for (i = 0; i < frequcount; i++) {
    for (j = i + 1; j < frequcount; j++) {
      if (newdict[i].frequ > newdict[j].frequ) {
        k = newdict[i].frequ;
        newdict[i].frequ = newdict[j].frequ;
        newdict[j].frequ = k;
        k = newdict[i].dict_leftson;
        newdict[i].dict_leftson = newdict[j].dict_leftson;
        newdict[j].dict_leftson = k;
        k = newdict[i].dict_rightson;
        newdict[i].dict_rightson = newdict[j].dict_rightson;
        newdict[j].dict_rightson = k;
      }
    }
  }
  while (frequcount > 1) {
    j = frequcount - 1;
    p->de->frequ = newdict[j - 1].frequ;
    p->de->dict_leftson = newdict[j - 1].dict_leftson;
    p->de->dict_rightson = newdict[j - 1].dict_rightson;
    l = newdict[j - 1].frequ + newdict[j].frequ;
    for (i = j - 2; i >= 0 && l < newdict[i].frequ; i--)
      newdict[i + 1] = newdict[i];
    i = i + 1;
    newdict[i].frequ = l;
    newdict[i].dict_leftson = j;
    newdict[i].dict_rightson = p->de - newdict;
    p->de++;
    frequcount--;
  }
  dictsize = p->de - newdict;
  makecodes(0, 0, 0, 1, newdict, codes, codesize);
  csize = 0;
  for (i = 0; i < 256; i++)
    csize += frequtable[i] * codesize[i];
  l = (((csize + 31) >> 5) << 2) + 24 + dictsize * 4;
  lsx_debug("  Original size: %6d bytes", *dl);
  lsx_debug("Compressed size: %6d bytes", l);
  datafork = lsx_malloc((size_t)l);
  ddf = datafork + 22;
  for(i = 0; i < dictsize; i++) {
    put16_be(&ddf, newdict[i].dict_leftson);
    put16_be(&ddf, newdict[i].dict_rightson);
  }
  *ddf++ = 0;
  *ddf++ = *(*df)++;
  p->new_checksum = 0;
  p->nbits = 0;
  p->curword = 0;
  for (i = 1; i < *dl; i++)
    putcode(ft, codes, codesize, *(*df)++, &ddf);
  if (p->nbits != 0) {
    codes[0] = 0;
    codesize[0] = 32 - p->nbits;
    putcode(ft, codes, codesize, 0, &ddf);
  }
  memcpy(datafork, "HCOM", (size_t)4);
  dfp = datafork + 4;
  put32_be(&dfp, *dl);
  put32_be(&dfp, p->new_checksum);
  put32_be(&dfp, 1);
  samplerate = 22050 / ft->signal.rate + .5;
  put32_be(&dfp, samplerate);
  put16_be(&dfp, dictsize);
  *df = datafork;               /* reassign passed pointer to new datafork */
  *dl = l;                      /* and its compressed length */
}

/* End of hcom utility routines */

static int stopwrite(sox_format_t * ft)
{
  priv_t *p = (priv_t *) ft->priv;
  unsigned char *compressed_data = p->data;
  int32_t compressed_len = p->pos;
  int rc = SOX_SUCCESS;

  /* Compress it all at once */
  if (compressed_len) {
    compress(ft, &compressed_data, &compressed_len);
    free(p->data);
  }

  /* Write the header */
  lsx_writebuf(ft, "\000\001A", (size_t) 3); /* Dummy file name "A" */
  lsx_padbytes(ft, (size_t) 65-3);
  lsx_writes(ft, "FSSD");
  lsx_padbytes(ft, (size_t) 83-69);
  lsx_writedw(ft, (unsigned) compressed_len); /* compressed_data size */
  lsx_writedw(ft, 0); /* rsrc size */
  lsx_padbytes(ft, (size_t) 128 - 91);
  if (lsx_error(ft)) {
    lsx_fail_errno(ft, errno, "write error in HCOM header");
    rc = SOX_EOF;
  } else if (lsx_writebuf(ft, compressed_data, compressed_len) != compressed_len) {
    /* Write the compressed_data fork */
    lsx_fail_errno(ft, errno, "can't write compressed HCOM data");
    rc = SOX_EOF;
  }
  free(compressed_data);

  if (rc == SOX_SUCCESS)
    /* Pad the compressed_data fork to a multiple of 128 bytes */
    lsx_padbytes(ft, 128u - (compressed_len % 128));

  return rc;
}

LSX_FORMAT_HANDLER(hcom)
{
  static char const * const names[]       = {"hcom", NULL};
  static sox_rate_t   const write_rates[] = {22050,22050/2,22050/3,22050/4.,0};
  static unsigned     const write_encodings[] = {
    SOX_ENCODING_HCOM, 8, 0, 0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Mac FSSD files with Huffman compression",
    names, SOX_FILE_BIG_END|SOX_FILE_MONO,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    NULL, write_encodings, write_rates, sizeof(priv_t)
  };
  return &handler;
}
