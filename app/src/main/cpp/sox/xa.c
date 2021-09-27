/* libSoX xa.c  Support for Maxis .XA file format
 *
 *      Copyright (C) 2006 Dwayne C. Litzenberger <dlitz@dlitz.net>
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

/* Thanks to Valery V. Anisimovsky <samael@avn.mccme.ru> for the
 * "Maxis XA Audio File Format Description", dated 5-01-2002. */

#include "sox_i.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define HNIBBLE(byte) (((byte) >> 4) & 0xf)
#define LNIBBLE(byte) ((byte) & 0xf)

/* .xa file header */
typedef struct {
    char magic[4];  /* "XA\0\0", "XAI\0" (sound/speech), or "XAJ\0" (music) */
    uint32_t outSize;       /* decompressed size of the stream (in bytes) */

    /* WAVEFORMATEX structure for the decompressed data */
    uint16_t tag;           /* 0x0001 - PCM data */
    uint16_t channels;      /* number of channels */
    uint32_t sampleRate;    /* sample rate (samples/sec) */
    uint32_t avgByteRate;   /* sampleRate * align */
    uint16_t align;         /* bits / 8 * channels */
    uint16_t bits;          /* 8 or 16 */
} xa_header_t;

typedef struct {
    int32_t curSample;      /* current sample */
    int32_t prevSample;     /* previous sample */
    int32_t c1;
    int32_t c2;
    unsigned int shift;
} xa_state_t;

/* Private data for .xa file */
typedef struct {
    xa_header_t header;
    xa_state_t *state;
    unsigned int blockSize;
    unsigned int bufPos;    /* position within the current block */
    unsigned char *buf;     /* buffer for the current block */
    unsigned int bytesDecoded;  /* number of decompressed bytes read */
} priv_t;

/* coefficients for EA ADPCM */
static const int32_t EA_ADPCM_Table[]= {
    0, 240,  460,  392,
    0,   0, -208, -220,
    0,   1,    3,    4,
    7,   8,   10,   11,
    0,  -1,   -3,   -4
};

/* Clip sample to 16 bits */
static inline int32_t clip16(int32_t sample)
{
    if (sample > 32767) {
        return 32767;
    } else if (sample < -32768) {
        return -32768;
    } else {
        return sample;
    }
}

static int startread(sox_format_t * ft)
{
    priv_t * xa = (priv_t *) ft->priv;
    char *magic = xa->header.magic;

    /* Check for the magic value */
    if (lsx_readbuf(ft, xa->header.magic, (size_t)4) != 4 ||
        (memcmp("XA\0\0", xa->header.magic, (size_t)4) != 0 &&
         memcmp("XAI\0", xa->header.magic, (size_t)4) != 0 &&
         memcmp("XAJ\0", xa->header.magic, (size_t)4) != 0))
    {
        lsx_fail_errno(ft, SOX_EHDR, "XA: Header not found");
        return SOX_EOF;
    }

    /* Read the rest of the header */
    if (lsx_readdw(ft, &xa->header.outSize) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readw(ft, &xa->header.tag) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readw(ft, &xa->header.channels) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readdw(ft, &xa->header.sampleRate) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readdw(ft, &xa->header.avgByteRate) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readw(ft, &xa->header.align) != SOX_SUCCESS) return SOX_EOF;
    if (lsx_readw(ft, &xa->header.bits) != SOX_SUCCESS) return SOX_EOF;

    /* Output the data from the header */
    lsx_debug("XA Header:");
    lsx_debug(" szID:          %02x %02x %02x %02x  |%c%c%c%c|",
        magic[0], magic[1], magic[2], magic[3],
        (magic[0] >= 0x20 && magic[0] <= 0x7e) ? magic[0] : '.',
        (magic[1] >= 0x20 && magic[1] <= 0x7e) ? magic[1] : '.',
        (magic[2] >= 0x20 && magic[2] <= 0x7e) ? magic[2] : '.',
        (magic[3] >= 0x20 && magic[3] <= 0x7e) ? magic[3] : '.');
    lsx_debug(" dwOutSize:     %u", xa->header.outSize);
    lsx_debug(" wTag:          0x%04x", xa->header.tag);
    lsx_debug(" wChannels:     %u", xa->header.channels);
    lsx_debug(" dwSampleRate:  %u", xa->header.sampleRate);
    lsx_debug(" dwAvgByteRate: %u", xa->header.avgByteRate);
    lsx_debug(" wAlign:        %u", xa->header.align);
    lsx_debug(" wBits:         %u", xa->header.bits);

    /* Populate the sox_soundstream structure */
    ft->encoding.encoding = SOX_ENCODING_SIGN2;

    if (!ft->encoding.bits_per_sample || ft->encoding.bits_per_sample == xa->header.bits) {
        ft->encoding.bits_per_sample = xa->header.bits;
    } else {
        lsx_report("User options overriding size read in .xa header");
    }

    if (ft->signal.channels == 0 || ft->signal.channels == xa->header.channels) {
        ft->signal.channels = xa->header.channels;
    } else {
        lsx_report("User options overriding channels read in .xa header");
    }

    if (ft->signal.rate == 0 || ft->signal.rate == xa->header.sampleRate) {
        ft->signal.rate = xa->header.sampleRate;
    } else {
        lsx_report("User options overriding rate read in .xa header");
    }

    if (ft->signal.channels == 0 || ft->signal.channels > UINT16_MAX) {
        lsx_fail_errno(ft, SOX_EFMT, "invalid channel count %d",
                       ft->signal.channels);
        return SOX_EOF;
    }

    /* Check for supported formats */
    if (ft->encoding.bits_per_sample != 16) {
        lsx_fail_errno(ft, SOX_EFMT, "%d-bit sample resolution not supported.",
            ft->encoding.bits_per_sample);
        return SOX_EOF;
    }

    /* Validate the header */
    if (xa->header.bits != ft->encoding.bits_per_sample) {
        lsx_report("Invalid sample resolution %d bits.  Assuming %d bits.",
            xa->header.bits, ft->encoding.bits_per_sample);
        xa->header.bits = ft->encoding.bits_per_sample;
    }
    if (xa->header.align != (ft->encoding.bits_per_sample >> 3) * xa->header.channels) {
        lsx_report("Invalid sample alignment value %d.  Assuming %d.",
            xa->header.align, (ft->encoding.bits_per_sample >> 3) * xa->header.channels);
        xa->header.align = (ft->encoding.bits_per_sample >> 3) * xa->header.channels;
    }
    if (xa->header.avgByteRate != (xa->header.align * xa->header.sampleRate)) {
        lsx_report("Invalid dwAvgByteRate value %d.  Assuming %d.",
            xa->header.avgByteRate, xa->header.align * xa->header.sampleRate);
        xa->header.avgByteRate = xa->header.align * xa->header.sampleRate;
    }

    /* Set up the block buffer */
    xa->blockSize = ft->signal.channels * 0xf;
    xa->bufPos = xa->blockSize;

    /* Allocate memory for the block buffer */
    xa->buf = lsx_calloc(1, (size_t)xa->blockSize);

    /* Allocate memory for the state */
    xa->state = lsx_calloc(sizeof(xa_state_t), ft->signal.channels);

    /* Final initialization */
    xa->bytesDecoded = 0;

    return SOX_SUCCESS;
}

/*
 * Read up to len samples from a file, converted to signed longs.
 * Return the number of samples read.
 */
static size_t read_samples(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
    priv_t * xa = (priv_t *) ft->priv;
    int32_t sample;
    unsigned char inByte;
    size_t i, done, bytes;

    ft->sox_errno = SOX_SUCCESS;
    done = 0;
    while (done < len) {
        if (xa->bufPos >= xa->blockSize) {
            /* Read the next block */
            bytes = lsx_readbuf(ft, xa->buf, (size_t) xa->blockSize);
            if (bytes < xa->blockSize) {
                if (lsx_eof(ft)) {
                    if (done > 0) {
                        return done;
                    }
                    lsx_fail_errno(ft,SOX_EOF,"Premature EOF on .xa input file");
                    return 0;
                } else {
                    /* error */
                    lsx_fail_errno(ft,SOX_EOF,"read error on input stream");
                    return 0;
                }
            }
            xa->bufPos = 0;

            for (i = 0; i < ft->signal.channels; i++) {
                inByte = xa->buf[i];
                xa->state[i].c1 = EA_ADPCM_Table[HNIBBLE(inByte)];
                xa->state[i].c2 = EA_ADPCM_Table[HNIBBLE(inByte) + 4];
                xa->state[i].shift = LNIBBLE(inByte) + 8;
            }
            xa->bufPos += ft->signal.channels;
        } else {
            /* Process the block */
            for (i = 0; i < ft->signal.channels && done < len; i++) {
                /* high nibble */
                sample = HNIBBLE(xa->buf[xa->bufPos+i]);
                sample = (sample << 28) >> xa->state[i].shift;
                sample = (sample +
                          xa->state[i].curSample * xa->state[i].c1 +
                          xa->state[i].prevSample * xa->state[i].c2 + 0x80) >> 8;
                sample = clip16(sample);
                xa->state[i].prevSample = xa->state[i].curSample;
                xa->state[i].curSample = sample;

                buf[done++] = SOX_SIGNED_16BIT_TO_SAMPLE(sample,);
                xa->bytesDecoded += (ft->encoding.bits_per_sample >> 3);
            }
            for (i = 0; i < ft->signal.channels && done < len; i++) {
                /* low nibble */
                sample = LNIBBLE(xa->buf[xa->bufPos+i]);
                sample = (sample << 28) >> xa->state[i].shift;
                sample = (sample +
                          xa->state[i].curSample * xa->state[i].c1 +
                          xa->state[i].prevSample * xa->state[i].c2 + 0x80) >> 8;
                sample = clip16(sample);
                xa->state[i].prevSample = xa->state[i].curSample;
                xa->state[i].curSample = sample;

                buf[done++] = SOX_SIGNED_16BIT_TO_SAMPLE(sample,);
                xa->bytesDecoded += (ft->encoding.bits_per_sample >> 3);
            }

            xa->bufPos += ft->signal.channels;
        }
    }
    if (done == 0) {
        return 0;
    }
    return done;
}

static int stopread(sox_format_t * ft)
{
    priv_t * xa = (priv_t *) ft->priv;

    ft->sox_errno = SOX_SUCCESS;

    /* Free memory */
    free(xa->buf);
    xa->buf = NULL;
    free(xa->state);
    xa->state = NULL;

    return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(xa)
{
  static char const * const names[] = {"xa", NULL };
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "16-bit ADPCM audio files used by Maxis games",
    names, SOX_FILE_LIT_END,
    startread, read_samples, stopread,
    NULL, NULL, NULL,
    NULL, NULL, NULL, sizeof(priv_t)
  };
  return &handler;
}
