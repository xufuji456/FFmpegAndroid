/* libSoX microsoft's WAVE sound format handler
 *
 * Copyright 1998-2006 Chris Bagwell and SoX Contributors
 * Copyright 1997 Graeme W. Gill, 93/5/17
 * Copyright 1992 Rick Richardson
 * Copyright 1991 Lance Norskog And Sundry Contributors
 *
 * Info for format tags can be found at:
 *   http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
 *
 */

#include "sox_i.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ima_rw.h"
#include "adpcm.h"

#ifdef HAVE_LIBGSM
#ifdef HAVE_GSM_GSM_H
#include <gsm/gsm.h>
#else
#include <gsm.h>
#endif
#endif

/* Magic length sometimes used to indicate unknown or too large size.
 * When detected on inputs, disable any length logic.
 */
#define MS_UNSPEC 0x7ffff000

#define WAVE_FORMAT_UNKNOWN             0x0000
#define WAVE_FORMAT_PCM                 0x0001
#define WAVE_FORMAT_ADPCM               0x0002
#define WAVE_FORMAT_IEEE_FLOAT          0x0003
#define WAVE_FORMAT_IBM_CVSD            0x0005
#define WAVE_FORMAT_ALAW                0x0006
#define WAVE_FORMAT_MULAW               0x0007
#define WAVE_FORMAT_OKI_ADPCM           0x0010
#define WAVE_FORMAT_IMA_ADPCM           0x0011
#define WAVE_FORMAT_MEDIASPACE_ADPCM    0x0012
#define WAVE_FORMAT_SIERRA_ADPCM        0x0013
#define WAVE_FORMAT_G723_ADPCM          0x0014
#define WAVE_FORMAT_DIGISTD             0x0015
#define WAVE_FORMAT_DIGIFIX             0x0016
#define WAVE_FORMAT_YAMAHA_ADPCM        0x0020
#define WAVE_FORMAT_SONARC              0x0021
#define WAVE_FORMAT_TRUESPEECH          0x0022
#define WAVE_FORMAT_ECHOSC1             0x0023
#define WAVE_FORMAT_AUDIOFILE_AF36      0x0024
#define WAVE_FORMAT_APTX                0x0025
#define WAVE_FORMAT_AUDIOFILE_AF10      0x0026
#define WAVE_FORMAT_DOLBY_AC2           0x0030
#define WAVE_FORMAT_GSM610              0x0031
#define WAVE_FORMAT_ADPCME              0x0033
#define WAVE_FORMAT_CONTROL_RES_VQLPC   0x0034
#define WAVE_FORMAT_DIGIREAL            0x0035
#define WAVE_FORMAT_DIGIADPCM           0x0036
#define WAVE_FORMAT_CONTROL_RES_CR10    0x0037
#define WAVE_FORMAT_ROCKWELL_ADPCM      0x003b
#define WAVE_FORMAT_ROCKWELL_DIGITALK   0x003c
#define WAVE_FORMAT_G721_ADPCM          0x0040
#define WAVE_FORMAT_G728_CELP           0x0041
#define WAVE_FORMAT_MPEG                0x0050
#define WAVE_FORMAT_MPEGLAYER3          0x0055
#define WAVE_FORMAT_G726_ADPCM          0x0064
#define WAVE_FORMAT_G722_ADPCM          0x0065
#define WAVE_FORMAT_CREATIVE_ADPCM      0x0200
#define WAVE_FORMAT_CREATIVE_FSP8       0x0202
#define WAVE_FORMAT_CREATIVE_FSP10      0x0203
#define WAVE_FORMAT_FM_TOWNS_SND        0x0300
#define WAVE_FORMAT_OLIGSM              0x1000
#define WAVE_FORMAT_OLIADPCM            0x1001
#define WAVE_FORMAT_OLISBC              0x1003
#define WAVE_FORMAT_OLIOPR              0x1004
#define WAVE_FORMAT_EXTENSIBLE          0xfffe

/* To allow padding to samplesPerBlock. Works, but currently never true. */
static const size_t pad_nsamps = sox_false;

/* Private data for .wav file */
typedef struct {
    /* samples/channel reading: starts at total count and decremented  */
    /* writing: starts at 0 and counts samples written */
    uint64_t  numSamples;    
    size_t    dataLength;     /* needed for ADPCM writing */
    unsigned short formatTag;       /* What type of encoding file is using */
    unsigned short samplesPerBlock;
    unsigned short blockAlign;
    uint16_t bitsPerSample;     /* bits per sample */
    size_t dataStart;           /* need to for seeking */
    int ignoreSize;                 /* ignoreSize allows us to process 32-bit WAV files that are
                                     * greater then 2 Gb and can't be represented by the
                                     * 32-bit size field. */
  /* FIXME: Have some front-end code which sets this flag. */

    /* following used by *ADPCM wav files */
    unsigned short nCoefs;          /* ADPCM: number of coef sets */
    short         *lsx_ms_adpcm_i_coefs;          /* ADPCM: coef sets           */
    void          *ms_adpcm_data;   /* Private data of adpcm decoder */
    unsigned char *packet;          /* Temporary buffer for packets */
    short         *samples;         /* interleaved samples buffer */
    short         *samplePtr;       /* Pointer to current sample  */
    short         *sampleTop;       /* End of samples-buffer      */
    unsigned short blockSamplesRemaining;/* Samples remaining per channel */
    int            state[16];       /* step-size info for *ADPCM writes */

#ifdef HAVE_LIBGSM
    /* following used by GSM 6.10 wav */
    gsm            gsmhandle;
    gsm_signal     *gsmsample;
    int            gsmindex;
    size_t      gsmbytecount;    /* counts bytes written to data block */
#endif
} priv_t;

struct wave_format {
    uint16_t tag;
    const char *name;
    sox_encoding_t encoding;
    int (*read_fmt)(sox_format_t *ft, uint32_t len);
};

static const char *wav_format_str(unsigned tag);

static int wavwritehdr(sox_format_t *, int);

/****************************************************************************/
/* IMA ADPCM Support Functions Section                                      */
/****************************************************************************/

static int wav_ima_adpcm_fmt(sox_format_t *ft, uint32_t len)
{
    priv_t *wav = ft->priv;
    size_t  bytesPerBlock;
    int err;

    if (wav->bitsPerSample != 4) {
        lsx_fail_errno(ft, SOX_EOF,
                       "Can only handle 4-bit IMA ADPCM in wav files");
        return SOX_EOF;
    }

    err = lsx_read_fields(ft, &len, "h", &wav->samplesPerBlock);
    if (err)
        return SOX_EOF;

    bytesPerBlock = lsx_ima_bytes_per_block(ft->signal.channels,
                                            wav->samplesPerBlock);

    if (bytesPerBlock != wav->blockAlign || wav->samplesPerBlock % 8 != 1) {
        lsx_fail_errno(ft, SOX_EOF,
                       "format[%s]: samplesPerBlock(%d) != blockAlign(%d)",
                       wav_format_str(wav->formatTag),
                       wav->samplesPerBlock, wav->blockAlign);
        return SOX_EOF;
    }

    wav->packet = lsx_malloc(wav->blockAlign);
    wav->samples =
        lsx_malloc(ft->signal.channels * wav->samplesPerBlock * sizeof(short));

    return SOX_SUCCESS;
}

/*
 *
 * ImaAdpcmReadBlock - Grab and decode complete block of samples
 *
 */
static unsigned short  ImaAdpcmReadBlock(sox_format_t * ft)
{
    priv_t *       wav = (priv_t *) ft->priv;
    size_t bytesRead;
    int samplesThisBlock;

    /* Pull in the packet and check the header */
    bytesRead = lsx_readbuf(ft, wav->packet, (size_t)wav->blockAlign);
    samplesThisBlock = wav->samplesPerBlock;
    if (bytesRead < wav->blockAlign)
    {
        /* If it looks like a valid header is around then try and */
        /* work with partial blocks.  Specs say it should be null */
        /* padded but I guess this is better than trailing quiet. */
        samplesThisBlock = lsx_ima_samples_in((size_t)0, (size_t)ft->signal.channels, bytesRead, (size_t) 0);
        if (samplesThisBlock == 0 || samplesThisBlock > wav->samplesPerBlock)
        {
            lsx_warn("Premature EOF on .wav input file");
            return 0;
        }
    }

    wav->samplePtr = wav->samples;

    /* For a full block, the following should be true: */
    /* wav->samplesPerBlock = blockAlign - 8byte header + 1 sample in header */
    lsx_ima_block_expand_i(ft->signal.channels, wav->packet, wav->samples, samplesThisBlock);
    return samplesThisBlock;

}

/****************************************************************************/
/* MS ADPCM Support Functions Section                                       */
/****************************************************************************/

static int wav_ms_adpcm_fmt(sox_format_t *ft, uint32_t len)
{
    priv_t *wav = ft->priv;
    size_t  bytesPerBlock;
    int i, errct = 0;
    int err;

    if (wav->bitsPerSample != 4) {
        lsx_fail_errno(ft, SOX_EOF,
                       "Can only handle 4-bit MS ADPCM in wav files");
        return SOX_EOF;
    }

    err = lsx_read_fields(ft, &len, "hh", &wav->samplesPerBlock, &wav->nCoefs);
    if (err)
        return SOX_EOF;

    bytesPerBlock = lsx_ms_adpcm_bytes_per_block(ft->signal.channels,
                                                 wav->samplesPerBlock);

    if (bytesPerBlock != wav->blockAlign) {
        lsx_fail_errno(ft, SOX_EOF,
                       "format[%s]: samplesPerBlock(%d) != blockAlign(%d)",
                       wav_format_str(wav->formatTag),
                       wav->samplesPerBlock, wav->blockAlign);
        return SOX_EOF;
    }

    if (wav->nCoefs < 7 || wav->nCoefs > 0x100) {
        lsx_fail_errno(ft, SOX_EOF,
                       "ADPCM file nCoefs (%.4hx) makes no sense",
                       wav->nCoefs);
        return SOX_EOF;
    }

    if (len < 4 * wav->nCoefs) {
        lsx_fail_errno(ft, SOX_EOF, "wave header error: cbSize too small");
        return SOX_EOF;
    }

    wav->packet = lsx_malloc(wav->blockAlign);
    wav->samples =
        lsx_malloc(ft->signal.channels * wav->samplesPerBlock * sizeof(short));

    /* nCoefs, lsx_ms_adpcm_i_coefs used by adpcm.c */
    wav->lsx_ms_adpcm_i_coefs = lsx_malloc(wav->nCoefs * 2 * sizeof(short));
    wav->ms_adpcm_data = lsx_ms_adpcm_alloc(ft->signal.channels);

    err = lsx_read_fields(ft, &len, "*h",
                          2 * wav->nCoefs, wav->lsx_ms_adpcm_i_coefs);
    if (err)
        return SOX_EOF;

    for (i = 0; i < 14; i++)
        errct += wav->lsx_ms_adpcm_i_coefs[i] != lsx_ms_adpcm_i_coef[i/2][i%2];

    if (errct)
        lsx_warn("base lsx_ms_adpcm_i_coefs differ in %d/14 positions", errct);

    return SOX_SUCCESS;
}

/*
 *
 * AdpcmReadBlock - Grab and decode complete block of samples
 *
 */
static unsigned short  AdpcmReadBlock(sox_format_t * ft)
{
    priv_t *       wav = (priv_t *) ft->priv;
    size_t bytesRead;
    int samplesThisBlock;
    const char *errmsg;

    /* Pull in the packet and check the header */
    bytesRead = lsx_readbuf(ft, wav->packet, (size_t) wav->blockAlign);
    samplesThisBlock = wav->samplesPerBlock;
    if (bytesRead < wav->blockAlign)
    {
        /* If it looks like a valid header is around then try and */
        /* work with partial blocks.  Specs say it should be null */
        /* padded but I guess this is better than trailing quiet. */
        samplesThisBlock = lsx_ms_adpcm_samples_in((size_t)0, (size_t)ft->signal.channels, bytesRead, (size_t)0);
        if (samplesThisBlock == 0 || samplesThisBlock > wav->samplesPerBlock)
        {
            lsx_warn("Premature EOF on .wav input file");
            return 0;
        }
    }

    errmsg = lsx_ms_adpcm_block_expand_i(wav->ms_adpcm_data, ft->signal.channels, wav->nCoefs, wav->lsx_ms_adpcm_i_coefs, wav->packet, wav->samples, samplesThisBlock);

    if (errmsg)
        lsx_warn("%s", errmsg);

    return samplesThisBlock;
}

/****************************************************************************/
/* Common ADPCM Write Function                                              */
/****************************************************************************/

static int xxxAdpcmWriteBlock(sox_format_t * ft)
{
    priv_t * wav = (priv_t *) ft->priv;
    size_t chans, ct;
    short *p;

    chans = ft->signal.channels;
    p = wav->samplePtr;
    ct = p - wav->samples;
    if (ct>=chans) {
        /* zero-fill samples if needed to complete block */
        for (p = wav->samplePtr; p < wav->sampleTop; p++) *p=0;
        /* compress the samples to wav->packet */
        if (wav->formatTag == WAVE_FORMAT_ADPCM) {
            lsx_ms_adpcm_block_mash_i((unsigned) chans, wav->samples, wav->samplesPerBlock, wav->state, wav->packet, wav->blockAlign);
        }else{ /* WAVE_FORMAT_IMA_ADPCM */
            lsx_ima_block_mash_i((unsigned) chans, wav->samples, wav->samplesPerBlock, wav->state, wav->packet, 9);
        }
        /* write the compressed packet */
        if (lsx_writebuf(ft, wav->packet, (size_t) wav->blockAlign) != wav->blockAlign)
        {
            lsx_fail_errno(ft,SOX_EOF,"write error");
            return (SOX_EOF);
        }
        /* update lengths and samplePtr */
        wav->dataLength += wav->blockAlign;
        if (pad_nsamps)
          wav->numSamples += wav->samplesPerBlock;
        else
          wav->numSamples += ct/chans;
        wav->samplePtr = wav->samples;
    }
    return (SOX_SUCCESS);
}

#ifdef HAVE_LIBGSM
/****************************************************************************/
/* WAV GSM6.10 support functions                                            */
/****************************************************************************/

static int wav_gsm_fmt(sox_format_t *ft, uint32_t len)
{
    priv_t *wav = ft->priv;
    int err;

    err = lsx_read_fields(ft, &len, "h", &wav->samplesPerBlock);
    if (err)
        return SOX_EOF;

    if (wav->blockAlign != 65) {
        lsx_fail_errno(ft, SOX_EOF, "format[%s]: expects blockAlign(%d) = %d",
                       wav_format_str(wav->formatTag), wav->blockAlign, 65);
        return SOX_EOF;
    }

    if (wav->samplesPerBlock != 320) {
        lsx_fail_errno(ft, SOX_EOF,
                       "format[%s]: expects samplesPerBlock(%d) = %d",
                       wav_format_str(wav->formatTag),
                       wav->samplesPerBlock, 320);
        return SOX_EOF;
    }

    return SOX_SUCCESS;
}

/* create the gsm object, malloc buffer for 160*2 samples */
static int wavgsminit(sox_format_t * ft)
{
    int valueP=1;
    priv_t *       wav = (priv_t *) ft->priv;
    wav->gsmbytecount=0;
    wav->gsmhandle=gsm_create();
    if (!wav->gsmhandle)
    {
        lsx_fail_errno(ft,SOX_EOF,"cannot create GSM object");
        return (SOX_EOF);
    }

    if(gsm_option(wav->gsmhandle,GSM_OPT_WAV49,&valueP) == -1){
        lsx_fail_errno(ft,SOX_EOF,"error setting gsm_option for WAV49 format. Recompile gsm library with -DWAV49 option and relink sox");
        return (SOX_EOF);
    }

    wav->gsmsample=lsx_malloc(sizeof(gsm_signal)*160*2);
    wav->gsmindex=0;
    return (SOX_SUCCESS);
}

/*destroy the gsm object and free the buffer */
static void wavgsmdestroy(sox_format_t * ft)
{
    priv_t *       wav = (priv_t *) ft->priv;
    gsm_destroy(wav->gsmhandle);
    free(wav->gsmsample);
}

static size_t wavgsmread(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
    priv_t *       wav = (priv_t *) ft->priv;
    size_t done=0;
    int bytes;
    gsm_byte    frame[65];

    ft->sox_errno = SOX_SUCCESS;

  /* copy out any samples left from the last call */
    while(wav->gsmindex && (wav->gsmindex<160*2) && (done < len))
        buf[done++]=SOX_SIGNED_16BIT_TO_SAMPLE(wav->gsmsample[wav->gsmindex++],);

  /* read and decode loop, possibly leaving some samples in wav->gsmsample */
    while (done < len) {
        wav->gsmindex=0;
        bytes = lsx_readbuf(ft, frame, (size_t)65);
        if (bytes <=0)
            return done;
        if (bytes<65) {
            lsx_warn("invalid wav gsm frame size: %d bytes",bytes);
            return done;
        }
        /* decode the long 33 byte half */
        if(gsm_decode(wav->gsmhandle,frame, wav->gsmsample)<0)
        {
            lsx_fail_errno(ft,SOX_EOF,"error during gsm decode");
            return 0;
        }
        /* decode the short 32 byte half */
        if(gsm_decode(wav->gsmhandle,frame+33, wav->gsmsample+160)<0)
        {
            lsx_fail_errno(ft,SOX_EOF,"error during gsm decode");
            return 0;
        }

        while ((wav->gsmindex <160*2) && (done < len)){
            buf[done++]=SOX_SIGNED_16BIT_TO_SAMPLE(wav->gsmsample[(wav->gsmindex)++],);
        }
    }

    return done;
}

static int wavgsmflush(sox_format_t * ft)
{
    gsm_byte    frame[65];
    priv_t *       wav = (priv_t *) ft->priv;

    /* zero fill as needed */
    while(wav->gsmindex<160*2)
        wav->gsmsample[wav->gsmindex++]=0;

    /*encode the even half short (32 byte) frame */
    gsm_encode(wav->gsmhandle, wav->gsmsample, frame);
    /*encode the odd half long (33 byte) frame */
    gsm_encode(wav->gsmhandle, wav->gsmsample+160, frame+32);
    if (lsx_writebuf(ft, frame, (size_t) 65) != 65)
    {
        lsx_fail_errno(ft,SOX_EOF,"write error");
        return (SOX_EOF);
    }
    wav->gsmbytecount += 65;

    wav->gsmindex = 0;
    return (SOX_SUCCESS);
}

static size_t wavgsmwrite(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
    priv_t * wav = (priv_t *) ft->priv;
    size_t done = 0;
    int rc;

    ft->sox_errno = SOX_SUCCESS;

    while (done < len) {
        SOX_SAMPLE_LOCALS;
        while ((wav->gsmindex < 160*2) && (done < len))
            wav->gsmsample[(wav->gsmindex)++] =
                SOX_SAMPLE_TO_SIGNED_16BIT(buf[done++], ft->clips);

        if (wav->gsmindex < 160*2)
            break;

        rc = wavgsmflush(ft);
        if (rc)
            return 0;
    }
    return done;

}

static void wavgsmstopwrite(sox_format_t * ft)
{
    priv_t *       wav = (priv_t *) ft->priv;

    ft->sox_errno = SOX_SUCCESS;

    if (wav->gsmindex)
        wavgsmflush(ft);

    /* Add a pad byte if amount of written bytes is not even. */
    if (wav->gsmbytecount && wav->gsmbytecount % 2){
        if(lsx_writeb(ft, 0))
            lsx_fail_errno(ft,SOX_EOF,"write error");
        else
            wav->gsmbytecount += 1;
    }

    wavgsmdestroy(ft);
}

#endif  /* HAVE_LIBGSM */

/****************************************************************************/
/* General Sox WAV file code                                                */
/****************************************************************************/

static int wav_pcm_fmt(sox_format_t *ft, uint32_t len)
{
    priv_t *wav = ft->priv;
    int bps = (wav->bitsPerSample + 7) / 8;

    if (bps == 1) {
        ft->encoding.encoding = SOX_ENCODING_UNSIGNED;
    } else if (bps <= 4) {
        ft->encoding.encoding = SOX_ENCODING_SIGN2;
    } else {
        lsx_fail_errno(ft, SOX_EFMT, "%d bytes per sample not suppored", bps);
        return SOX_EOF;
    }

    return SOX_SUCCESS;
}

static const struct wave_format wave_formats[] = {
    { WAVE_FORMAT_UNKNOWN,              "Unknown Wave Type" },
    { WAVE_FORMAT_PCM,                  "PCM",
      SOX_ENCODING_UNKNOWN,
      wav_pcm_fmt,
    },
    { WAVE_FORMAT_ADPCM,                "Microsoft ADPCM",
      SOX_ENCODING_MS_ADPCM,
      wav_ms_adpcm_fmt,
    },
    { WAVE_FORMAT_IEEE_FLOAT,           "IEEE Float",
      SOX_ENCODING_FLOAT },
    { WAVE_FORMAT_IBM_CVSD,             "Digispeech CVSD" },
    { WAVE_FORMAT_ALAW,                 "CCITT A-law",
      SOX_ENCODING_ALAW },
    { WAVE_FORMAT_MULAW,                "CCITT u-law",
      SOX_ENCODING_ULAW },
    { WAVE_FORMAT_OKI_ADPCM,            "OKI ADPCM" },
    { WAVE_FORMAT_IMA_ADPCM,            "IMA ADPCM",
      SOX_ENCODING_IMA_ADPCM,
      wav_ima_adpcm_fmt,
    },
    { WAVE_FORMAT_MEDIASPACE_ADPCM,     "MediaSpace ADPCM" },
    { WAVE_FORMAT_SIERRA_ADPCM,         "Sierra ADPCM" },
    { WAVE_FORMAT_G723_ADPCM,           "G.723 ADPCM" },
    { WAVE_FORMAT_DIGISTD,              "DIGISTD" },
    { WAVE_FORMAT_DIGIFIX,              "DigiFix" },
    { WAVE_FORMAT_YAMAHA_ADPCM,         "Yamaha ADPCM" },
    { WAVE_FORMAT_SONARC,               "Sonarc" },
    { WAVE_FORMAT_TRUESPEECH,           "Truespeech" },
    { WAVE_FORMAT_ECHOSC1,              "ECHO SC-1", },
    { WAVE_FORMAT_AUDIOFILE_AF36,       "Audio File AF36" },
    { WAVE_FORMAT_APTX,                 "aptX" },
    { WAVE_FORMAT_AUDIOFILE_AF10,       "Audio File AF10" },
    { WAVE_FORMAT_DOLBY_AC2,            "Dolby AC-2" },
    { WAVE_FORMAT_GSM610,               "GSM 6.10",
#ifdef HAVE_LIBGSM
      SOX_ENCODING_GSM,
      wav_gsm_fmt,
#endif
    },
    { WAVE_FORMAT_ADPCME,               "Antex ADPCME" },
    { WAVE_FORMAT_CONTROL_RES_VQLPC,    "Control Resources VQLPC" },
    { WAVE_FORMAT_DIGIREAL,             "DSP Solutions REAL" },
    { WAVE_FORMAT_DIGIADPCM,            "DSP Solutions ADPCM" },
    { WAVE_FORMAT_CONTROL_RES_CR10,     "Control Resources CR10" },
    { WAVE_FORMAT_ROCKWELL_ADPCM,       "Rockwell ADPCM" },
    { WAVE_FORMAT_ROCKWELL_DIGITALK,    "Rockwell DIGITALK" },
    { WAVE_FORMAT_G721_ADPCM,           "G.721 ADPCM" },
    { WAVE_FORMAT_G728_CELP,            "G.728 CELP" },
    { WAVE_FORMAT_MPEG,                 "MPEG-1 Audio" },
    { WAVE_FORMAT_MPEGLAYER3,           "MPEG-1 Layer 3" },
    { WAVE_FORMAT_G726_ADPCM,           "G.726 ADPCM" },
    { WAVE_FORMAT_G722_ADPCM,           "G.722 ADPCM" },
    { WAVE_FORMAT_CREATIVE_ADPCM,       "Creative Labs ADPCM" },
    { WAVE_FORMAT_CREATIVE_FSP8,        "Creative Labs FastSpeech 8" },
    { WAVE_FORMAT_CREATIVE_FSP10,       "Creative Labs FastSpeech 10" },
    { WAVE_FORMAT_FM_TOWNS_SND,         "Fujitsu FM Towns SND" },
    { WAVE_FORMAT_OLIGSM,               "Olivetti GSM" },
    { WAVE_FORMAT_OLIADPCM,             "Olivetti ADPCM" },
    { WAVE_FORMAT_OLISBC,               "Olivetti CELP" },
    { WAVE_FORMAT_OLIOPR,               "Olivetti OPR" },
    { }
};

static const struct wave_format *wav_find_format(unsigned tag)
{
    const struct wave_format *f;

    for (f = wave_formats; f->name; f++)
        if (f->tag == tag)
            return f;

    return NULL;
}

static int wavfail(sox_format_t *ft, int tag, const char *name)
{
    if (name)
        lsx_fail_errno(ft, SOX_EHDR, "WAVE format '%s' (%04x) not supported",
                       name, tag);
    else
        lsx_fail_errno(ft, SOX_EHDR, "Unknown WAVE format %04x", tag);

    return SOX_EOF;
}

static int wav_read_fmt(sox_format_t *ft, uint32_t len)
{
    priv_t  *wav = ft->priv;
    uint16_t wChannels;          /* number of channels */
    uint32_t dwSamplesPerSecond; /* samples per second per channel */
    uint32_t dwAvgBytesPerSec;   /* estimate of bytes per second needed */
    uint16_t wExtSize = 0;       /* extended field for non-PCM */
    const struct wave_format *fmt;
    sox_encoding_t user_enc = ft->encoding.encoding;
    int err;

    if (len < 16) {
        lsx_fail_errno(ft, SOX_EHDR, "WAVE file fmt chunk is too short");
        return SOX_EOF;
    }

    err = lsx_read_fields(ft, &len, "hhiihh",
                          &wav->formatTag,
                          &wChannels,
                          &dwSamplesPerSecond,
                          &dwAvgBytesPerSec,
                          &wav->blockAlign,
                          &wav->bitsPerSample);
    if (err)
        return SOX_EOF;

    /* non-PCM formats except alaw and mulaw formats have extended fmt chunk.
     * Check for those cases.
     */
    if (wav->formatTag != WAVE_FORMAT_PCM &&
        wav->formatTag != WAVE_FORMAT_ALAW &&
        wav->formatTag != WAVE_FORMAT_MULAW &&
        len < 2)
        lsx_warn("WAVE file missing extended part of fmt chunk");

    if (len >= 2) {
        err = lsx_read_fields(ft, &len, "h", &wExtSize);
        if (err)
            return SOX_EOF;
    }

    if (wExtSize != len) {
        lsx_fail_errno(ft, SOX_EOF,
                       "WAVE header error: cbSize inconsistent with fmt size");
        return SOX_EOF;
    }

    if (wav->formatTag == WAVE_FORMAT_EXTENSIBLE) {
        uint16_t numberOfValidBits;
        uint32_t speakerPositionMask;
        uint16_t subFormatTag;

        if (len < 22) {
            lsx_fail_errno(ft, SOX_EHDR, "WAVE file fmt chunk is too short");
            return SOX_EOF;
        }

        err = lsx_read_fields(ft, &len, "hih14x",
                              &numberOfValidBits,
                              &speakerPositionMask,
                              &subFormatTag);
        if (err)
            return SOX_EOF;

        if (numberOfValidBits > wav->bitsPerSample) {
            lsx_fail_errno(ft, SOX_EHDR,
                           "wValidBitsPerSample > wBitsPerSample");
            return SOX_EOF;
        }

        wav->formatTag = subFormatTag;
        lsx_report("EXTENSIBLE");
    }

    /* User options take precedence */
    if (ft->signal.channels == 0 || ft->signal.channels == wChannels)
        ft->signal.channels = wChannels;
    else
        lsx_report("User options overriding channels read in .wav header");

    if (ft->signal.channels == 0) {
        lsx_fail_errno(ft, SOX_EHDR, "Channel count is zero");
        return SOX_EOF;
    }

    if (ft->signal.rate == 0 || ft->signal.rate == dwSamplesPerSecond)
        ft->signal.rate = dwSamplesPerSecond;
    else
        lsx_report("User options overriding rate read in .wav header");

    fmt = wav_find_format(wav->formatTag);
    if (!fmt)
        return wavfail(ft, wav->formatTag, NULL);

    /* format handler might override */
    ft->encoding.encoding = fmt->encoding;

    if (fmt->read_fmt) {
        if (fmt->read_fmt(ft, len))
            return SOX_EOF;
    } else if (!fmt->encoding) {
        return wavfail(ft, wav->formatTag, fmt->name);
    }

    /* User options take precedence */
    if (!ft->encoding.bits_per_sample ||
        ft->encoding.bits_per_sample == wav->bitsPerSample)
        ft->encoding.bits_per_sample = wav->bitsPerSample;
    else
        lsx_warn("User options overriding size read in .wav header");

    if (user_enc && user_enc != ft->encoding.encoding) {
        lsx_report("User options overriding encoding read in .wav header");
        ft->encoding.encoding = user_enc;
    }

    return 0;
}

static sox_bool valid_chunk_id(const char p[4])
{
    int i;

    for (i = 0; i < 4; i++)
        if (p[i] < 0x20 || p[i] > 0x7f)
            return sox_false;

    return sox_true;
}

static int read_chunk_header(sox_format_t *ft, char tag[4], uint32_t *len)
{
    int r;

    r = lsx_readbuf(ft, tag, 4);
    if (r < 4)
        return SOX_EOF;

    return lsx_readdw(ft, len);
}

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int startread(sox_format_t *ft)
{
    priv_t  *wav = ft->priv;
    char     magic[5] = { 0 };
    uint32_t clen;
    int      err;

    sox_bool isRF64 = sox_false;
    uint64_t ds64_riff_size;
    uint64_t ds64_data_size;
    uint64_t ds64_sample_count;

    /* wave file characteristics */
    uint64_t qwRiffLength;
    uint64_t qwDataLength = 0;
    sox_bool have_fmt = sox_false;

    ft->sox_errno = SOX_SUCCESS;
    wav->ignoreSize = ft->signal.length == SOX_IGNORE_LENGTH;
    ft->encoding.reverse_bytes = MACHINE_IS_BIGENDIAN;

    if (read_chunk_header(ft, magic, &clen))
        return SOX_EOF;

    if (!memcmp(magic, "RIFX", 4)) {
        lsx_debug("Found RIFX header");
        ft->encoding.reverse_bytes = MACHINE_IS_LITTLEENDIAN;
    } else if (!memcmp(magic, "RF64", 4)) {
        lsx_debug("Found RF64 header");
        isRF64 = sox_true;
    } else if (memcmp(magic, "RIFF", 4)) {
        lsx_fail_errno(ft, SOX_EHDR, "WAVE: RIFF header not found");
        return SOX_EOF;
    }

    qwRiffLength = clen;

    if (lsx_readbuf(ft, magic, 4) < 4 || memcmp(magic, "WAVE", 4)) {
        lsx_fail_errno(ft, SOX_EHDR, "WAVE header not found");
        return SOX_EOF;
    }

    while (!read_chunk_header(ft, magic, &clen)) {
        uint32_t len = clen;
        off_t cstart = lsx_tell(ft);
        off_t pos;

        if (!valid_chunk_id(magic)) {
            lsx_fail_errno(ft, SOX_EHDR, "invalid chunk ID found");
            return SOX_EOF;
        }

        lsx_debug("Found chunk '%s', size %u", magic, clen);

        if (!memcmp(magic, "ds64", 4)) {
            if (!isRF64)
                lsx_warn("ds64 chunk in non-RF64 file");

            if (clen < 28) {
                lsx_fail_errno(ft, SOX_EHDR, "ds64 chunk too small");
                return SOX_EOF;
            }

            if (clen == 32) {
                lsx_warn("ds64 chunk size invalid, attempting workaround");
                clen = 28;
            }

            err = lsx_read_fields(ft, &len, "qqq",
                                  &ds64_riff_size,
                                  &ds64_data_size,
                                  &ds64_sample_count);
            if (err)
                return SOX_EOF;

            goto next;
        }

        if (!memcmp(magic, "fmt ", 4)) {
            err = wav_read_fmt(ft, clen);
            if (err)
                return err;

            have_fmt = sox_true;

            goto next;
        }

        if (!memcmp(magic, "fact", 4)) {
            uint32_t val;

            err = lsx_read_fields(ft, &len, "i", &val);
            if (err)
                return SOX_EOF;

            wav->numSamples = val;

            goto next;
        }

        if (!memcmp(magic, "data", 4)) {
            if (isRF64 && clen == UINT32_MAX)
                clen = ds64_data_size;

            qwDataLength = clen;
            wav->dataStart = lsx_tell(ft);

            if (qwDataLength == UINT32_MAX || qwDataLength == MS_UNSPEC)
                break;

            if (!ft->seekable)
                break;

            goto next;
        }

    next:
        pos = lsx_tell(ft);
        clen += clen & 1;

        if (pos > cstart + clen) {
            lsx_fail_errno(ft, SOX_EHDR, "malformed chunk %s", magic);
            return SOX_EOF;
        }

        err = lsx_seeki(ft, cstart + clen - pos, SEEK_CUR);
        if (err)
            return SOX_EOF;
    }

    if (isRF64) {
        if (wav->numSamples == UINT32_MAX)
            wav->numSamples = ds64_sample_count;

        if (qwRiffLength == UINT32_MAX)
            qwRiffLength = ds64_riff_size;
    }

    if (!have_fmt) {
        lsx_fail_errno(ft, SOX_EOF, "fmt chunk not found");
        return SOX_EOF;
    }

    if (!wav->dataStart) {
        lsx_fail_errno(ft, SOX_EOF, "data chunk not found");
        return SOX_EOF;
    }

    if (ft->seekable)
        lsx_seeki(ft, wav->dataStart, SEEK_SET);

    /* some files wrongly report total samples across all channels */
    if (wav->numSamples * wav->blockAlign == qwDataLength * ft->signal.channels)
        wav->numSamples /= ft->signal.channels;

    if ((qwDataLength == UINT32_MAX && !wav->numSamples) ||
        qwDataLength == MS_UNSPEC) {
        lsx_warn("WAV data length is magic value or UINT32_MAX, ignoring");
        wav->ignoreSize = 1;
    }

    switch (wav->formatTag) {
    case WAVE_FORMAT_ADPCM:
        wav->numSamples =
            lsx_ms_adpcm_samples_in(qwDataLength, ft->signal.channels,
                                    wav->blockAlign, wav->samplesPerBlock);
        wav->blockSamplesRemaining = 0;        /* Samples left in buffer */
        break;

    case WAVE_FORMAT_IMA_ADPCM:
        /* Compute easiest part of number of samples.  For every block, there
           are samplesPerBlock samples to read. */
        wav->numSamples =
            lsx_ima_samples_in(qwDataLength, ft->signal.channels,
                               wav->blockAlign, wav->samplesPerBlock);
        wav->blockSamplesRemaining = 0;        /* Samples left in buffer */
        lsx_ima_init_table();
        break;

#ifdef HAVE_LIBGSM
    case WAVE_FORMAT_GSM610:
        wav->numSamples = qwDataLength / wav->blockAlign * wav->samplesPerBlock;
        wavgsminit(ft);
        break;
#endif
    }

    if (!wav->numSamples)
        wav->numSamples = div_bits(qwDataLength, ft->encoding.bits_per_sample)
            / ft->signal.channels;

    if (wav->ignoreSize)
        ft->signal.length = SOX_UNSPEC;
    else
        ft->signal.length = wav->numSamples * ft->signal.channels;

    return lsx_rawstartread(ft);
}


/*
 * Read up to len samples from file.
 * Convert to signed longs.
 * Place in buf[].
 * Return number of samples read.
 */

static size_t read_samples(sox_format_t *ft, sox_sample_t *buf, size_t len)
{
    priv_t *wav = ft->priv;
    size_t done;

    ft->sox_errno = SOX_SUCCESS;

    if (!wav->ignoreSize)
        len = min(len, wav->numSamples * ft->signal.channels);

    /* If file is in ADPCM encoding then read in multiple blocks else */
    /* read as much as possible and return quickly. */
    switch (ft->encoding.encoding) {
    case SOX_ENCODING_IMA_ADPCM:
    case SOX_ENCODING_MS_ADPCM:
        done = 0;
        while (done < len) { /* Still want data? */
            short *p, *top;
            size_t ct;

            /* See if need to read more from disk */
            if (wav->blockSamplesRemaining == 0) {
                if (wav->formatTag == WAVE_FORMAT_IMA_ADPCM)
                    wav->blockSamplesRemaining = ImaAdpcmReadBlock(ft);
                else
                    wav->blockSamplesRemaining = AdpcmReadBlock(ft);

                if (wav->blockSamplesRemaining == 0) {
                    /* Don't try to read any more samples */
                    wav->numSamples = 0;
                    return done;
                }
                wav->samplePtr = wav->samples;
            }

            /* Copy interleaved data into buf, converting to sox_sample_t */
            ct = len - done;
            if (ct > wav->blockSamplesRemaining * ft->signal.channels)
                ct = wav->blockSamplesRemaining * ft->signal.channels;

            done += ct;
            wav->blockSamplesRemaining -= ct / ft->signal.channels;
            p = wav->samplePtr;
            top = p + ct;

            /* Output is already signed */
            while (p < top)
                *buf++ = SOX_SIGNED_16BIT_TO_SAMPLE(*p++,);

            wav->samplePtr = p;
        }

        /* "done" for ADPCM equals total data processed and not
         * total samples procesed.  The only way to take care of that
         * is to return here and not fall thru.
         */
        wav->numSamples -= done / ft->signal.channels;

        return done;

#ifdef HAVE_LIBGSM
    case SOX_ENCODING_GSM:
        done = wavgsmread(ft, buf, len);
        break;
#endif

    default: /* assume PCM or float encoding */
        done = lsx_rawread(ft, buf, len);
        break;
    }

    if (done == 0 && wav->numSamples && !wav->ignoreSize)
        lsx_warn("Premature EOF on .wav input file");

    /* Only return buffers that contain a totally playable
     * amount of audio.
     */
    done -= done % ft->signal.channels;

    if (done / ft->signal.channels > wav->numSamples)
        wav->numSamples = 0;
    else
        wav->numSamples -= done / ft->signal.channels;

    return done;
}

/*
 * Do anything required when you stop reading samples.
 * Don't close input file!
 */
static int stopread(sox_format_t * ft)
{
    priv_t *       wav = (priv_t *) ft->priv;

    ft->sox_errno = SOX_SUCCESS;

    free(wav->packet);
    free(wav->samples);
    free(wav->lsx_ms_adpcm_i_coefs);
    free(wav->ms_adpcm_data);

    switch (ft->encoding.encoding)
    {
#ifdef HAVE_LIBGSM
    case SOX_ENCODING_GSM:
        wavgsmdestroy(ft);
        break;
#endif
    case SOX_ENCODING_IMA_ADPCM:
    case SOX_ENCODING_MS_ADPCM:
        break;
    default:
        break;
    }
    return SOX_SUCCESS;
}

static int startwrite(sox_format_t * ft)
{
    priv_t * wav = (priv_t *) ft->priv;
    int rc;

    ft->sox_errno = SOX_SUCCESS;

    if (ft->encoding.encoding != SOX_ENCODING_MS_ADPCM &&
        ft->encoding.encoding != SOX_ENCODING_IMA_ADPCM &&
        ft->encoding.encoding != SOX_ENCODING_GSM)
    {
        rc = lsx_rawstartwrite(ft);
        if (rc)
            return rc;
    }

    wav->numSamples = 0;
    wav->dataLength = 0;
    if (!ft->signal.length && !ft->seekable)
        lsx_warn("Length in output .wav header will be wrong since can't seek to fix it");

    rc = wavwritehdr(ft, 0);  /* also calculates various wav->* info */
    if (rc != 0)
        return rc;

    wav->packet = NULL;
    wav->samples = NULL;
    wav->lsx_ms_adpcm_i_coefs = NULL;
    switch (wav->formatTag)
    {
        size_t ch, sbsize;

        case WAVE_FORMAT_IMA_ADPCM:
            lsx_ima_init_table();
        /* intentional case fallthru! */
        case WAVE_FORMAT_ADPCM:
            /* #channels already range-checked for overflow in wavwritehdr() */
            for (ch=0; ch<ft->signal.channels; ch++)
                wav->state[ch] = 0;
            sbsize = ft->signal.channels * wav->samplesPerBlock;
            wav->packet = lsx_malloc((size_t)wav->blockAlign);
            wav->samples = lsx_malloc(sbsize*sizeof(short));
            wav->sampleTop = wav->samples + sbsize;
            wav->samplePtr = wav->samples;
            break;

#ifdef HAVE_LIBGSM
        case WAVE_FORMAT_GSM610:
            return wavgsminit(ft);
#endif

        default:
            break;
    }
    return SOX_SUCCESS;
}

/* wavwritehdr:  write .wav headers as follows:

bytes      variable      description
0  - 3     'RIFF'/'RIFX' Little/Big-endian
4  - 7     wRiffLength   length of file minus the 8 byte riff header
8  - 11    'WAVE'
12 - 15    'fmt '
16 - 19    wFmtSize       length of format chunk minus 8 byte header
20 - 21    wFormatTag     identifies PCM, ULAW etc
22 - 23    wChannels
24 - 27    dwSamplesPerSecond  samples per second per channel
28 - 31    dwAvgBytesPerSec    non-trivial for compressed formats
32 - 33    wBlockAlign         basic block size
34 - 35    wBitsPerSample      non-trivial for compressed formats

PCM formats then go straight to the data chunk:
36 - 39    'data'
40 - 43     dwDataLength   length of data chunk minus 8 byte header
44 - (dwDataLength + 43)   the data
(+ a padding byte if dwDataLength is odd)

non-PCM formats must write an extended format chunk and a fact chunk:

ULAW, ALAW formats:
36 - 37    wExtSize = 0  the length of the format extension
38 - 41    'fact'
42 - 45    dwFactSize = 4  length of the fact chunk minus 8 byte header
46 - 49    dwSamplesWritten   actual number of samples written out
50 - 53    'data'
54 - 57     dwDataLength  length of data chunk minus 8 byte header
58 - (dwDataLength + 57)  the data
(+ a padding byte if dwDataLength is odd)


GSM6.10  format:
36 - 37    wExtSize = 2 the length in bytes of the format-dependent extension
38 - 39    320           number of samples per  block
40 - 43    'fact'
44 - 47    dwFactSize = 4  length of the fact chunk minus 8 byte header
48 - 51    dwSamplesWritten   actual number of samples written out
52 - 55    'data'
56 - 59     dwDataLength  length of data chunk minus 8 byte header
60 - (dwDataLength + 59)  the data (including a padding byte, if necessary,
                            so dwDataLength is always even)


note that header contains (up to) 3 separate ways of describing the
length of the file, all derived here from the number of (input)
samples wav->numSamples in a way that is non-trivial for the blocked
and padded compressed formats:

wRiffLength -      (riff header) the length of the file, minus 8
dwSamplesWritten - (fact header) the number of samples written (after padding
                   to a complete block eg for GSM)
dwDataLength     - (data chunk header) the number of (valid) data bytes written

*/

static int wavwritehdr(sox_format_t * ft, int second_header)
{
    priv_t *       wav = (priv_t *) ft->priv;

    /* variables written to wav file header */
    /* RIFF header */
    uint64_t wRiffLength ;  /* length of file after 8 byte riff header */
    /* fmt chunk */
    uint16_t wFmtSize = 16;       /* size field of the fmt chunk */
    uint16_t wFormatTag = 0;      /* data format */
    uint16_t wChannels;           /* number of channels */
    uint32_t dwSamplesPerSecond;  /* samples per second per channel*/
    uint32_t dwAvgBytesPerSec=0;  /* estimate of bytes per second needed */
    uint32_t wBlockAlign=0;       /* byte alignment of a basic sample block */
    uint16_t wBitsPerSample=0;    /* bits per sample */
    /* fmt chunk extension (not PCM) */
    uint16_t wExtSize=0;          /* extra bytes in the format extension */
    uint16_t wSamplesPerBlock;    /* samples per channel per block */
    /* wSamplesPerBlock and other things may go into format extension */

    /* fact chunk (not PCM) */
    uint32_t dwFactSize=4;        /* length of the fact chunk */
    uint64_t dwSamplesWritten=0;  /* windows doesnt seem to use this*/

    /* data chunk */
    uint64_t dwDataLength;        /* length of sound data in bytes */
    /* end of variables written to header */

    /* internal variables, intermediate values etc */
    int bytespersample; /* (uncompressed) bytes per sample (per channel) */
    uint64_t blocksWritten = 0;
    sox_bool isExtensible = sox_false;    /* WAVE_FORMAT_EXTENSIBLE? */

    if (ft->signal.channels > UINT16_MAX) {
        lsx_fail_errno(ft, SOX_EOF, "Too many channels (%u)",
                       ft->signal.channels);
        return SOX_EOF;
    }

    dwSamplesPerSecond = ft->signal.rate;
    wChannels = ft->signal.channels;
    wBitsPerSample = ft->encoding.bits_per_sample;
    wSamplesPerBlock = 1;       /* common default for PCM data */

    switch (ft->encoding.encoding)
    {
        case SOX_ENCODING_UNSIGNED:
        case SOX_ENCODING_SIGN2:
            wFormatTag = WAVE_FORMAT_PCM;
            bytespersample = (wBitsPerSample + 7)/8;
            wBlockAlign = wChannels * bytespersample;
            break;
        case SOX_ENCODING_FLOAT:
            wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            bytespersample = (wBitsPerSample + 7)/8;
            wBlockAlign = wChannels * bytespersample;
            break;
        case SOX_ENCODING_ALAW:
            wFormatTag = WAVE_FORMAT_ALAW;
            wBlockAlign = wChannels;
            break;
        case SOX_ENCODING_ULAW:
            wFormatTag = WAVE_FORMAT_MULAW;
            wBlockAlign = wChannels;
            break;
        case SOX_ENCODING_IMA_ADPCM:
            if (wChannels>16)
            {
                lsx_fail_errno(ft,SOX_EOF,"Channels(%d) must be <= 16",wChannels);
                return SOX_EOF;
            }
            wFormatTag = WAVE_FORMAT_IMA_ADPCM;
            wBlockAlign = wChannels * 256; /* reasonable default */
            wBitsPerSample = 4;
            wExtSize = 2;
            wSamplesPerBlock = lsx_ima_samples_in((size_t) 0, (size_t) wChannels, (size_t) wBlockAlign, (size_t) 0);
            break;
        case SOX_ENCODING_MS_ADPCM:
            if (wChannels>16)
            {
                lsx_fail_errno(ft,SOX_EOF,"Channels(%d) must be <= 16",wChannels);
                return SOX_EOF;
            }
            wFormatTag = WAVE_FORMAT_ADPCM;
            wBlockAlign = ft->signal.rate / 11008;
            wBlockAlign = max(wBlockAlign, 1) * wChannels * 256;
            wBitsPerSample = 4;
            wExtSize = 4+4*7;      /* Ext fmt data length */
            wSamplesPerBlock = lsx_ms_adpcm_samples_in((size_t) 0, (size_t) wChannels, (size_t) wBlockAlign, (size_t) 0);
            break;
#ifdef HAVE_LIBGSM
        case SOX_ENCODING_GSM:
            if (wChannels!=1)
            {
                lsx_report("Overriding GSM audio from %d channel to 1",wChannels);
                if (!second_header)
                  ft->signal.length /= max(1, ft->signal.channels);
                wChannels = ft->signal.channels = 1;
            }
            wFormatTag = WAVE_FORMAT_GSM610;
            /* dwAvgBytesPerSec = 1625*(dwSamplesPerSecond/8000.)+0.5; */
            wBlockAlign=65;
            wBitsPerSample=0;  /* not representable as int   */
            wExtSize=2;        /* length of format extension */
            wSamplesPerBlock = 320;
            break;
#endif
        default:
                break;
    }

    if (wBlockAlign > UINT16_MAX) {
        lsx_fail_errno(ft, SOX_EOF, "Too many channels (%u)",
                       ft->signal.channels);
        return SOX_EOF;
    }

    wav->formatTag = wFormatTag;
    wav->blockAlign = wBlockAlign;
    wav->samplesPerBlock = wSamplesPerBlock;

    /* When creating header, use length hint given by input file.  If no
     * hint then write default value.  Also, use default value even
     * on header update if more then 32-bit length needs to be written.
     */

    dwSamplesWritten =
        second_header ? wav->numSamples : ft->signal.length / wChannels;
    blocksWritten =
        (dwSamplesWritten + wSamplesPerBlock - 1) / wSamplesPerBlock;
    dwDataLength = blocksWritten * wBlockAlign;

    if (wFormatTag == WAVE_FORMAT_GSM610)
        dwDataLength = (dwDataLength+1) & ~1u; /* round up to even */

    if (wFormatTag == WAVE_FORMAT_PCM && (wBitsPerSample > 16 || wChannels > 2)
        && strcmp(ft->filetype, "wavpcm")) {
      isExtensible = sox_true;
      wFmtSize += 2 + 22;
    }
    else if (wFormatTag != WAVE_FORMAT_PCM)
        wFmtSize += 2+wExtSize; /* plus ExtData */

    wRiffLength = 4 + (8+wFmtSize) + (8+dwDataLength+dwDataLength%2);
    if (isExtensible || wFormatTag != WAVE_FORMAT_PCM) /* PCM omits the "fact" chunk */
        wRiffLength += (8+dwFactSize);

    if (dwSamplesWritten > UINT32_MAX)
        dwSamplesWritten = UINT32_MAX;

    if (dwDataLength > UINT32_MAX)
        dwDataLength = UINT32_MAX;

    if (!second_header && !ft->signal.length)
        dwDataLength = UINT32_MAX;

    if (wRiffLength > UINT32_MAX)
        wRiffLength = UINT32_MAX;

    /* dwAvgBytesPerSec <-- this is BEFORE compression, isn't it? guess not. */
    dwAvgBytesPerSec = (double)wBlockAlign*ft->signal.rate / (double)wSamplesPerBlock + 0.5;

    /* figured out header info, so write it */

    /* If user specified opposite swap than we think, assume they are
     * asking to write a RIFX file.
     */
    if (ft->encoding.reverse_bytes == MACHINE_IS_LITTLEENDIAN)
    {
        if (!second_header)
            lsx_report("Requested to swap bytes so writing RIFX header");
        lsx_writes(ft, "RIFX");
    }
    else
        lsx_writes(ft, "RIFF");
    lsx_writedw(ft, wRiffLength);
    lsx_writes(ft, "WAVE");
    lsx_writes(ft, "fmt ");
    lsx_writedw(ft, wFmtSize);
    lsx_writew(ft, isExtensible ? WAVE_FORMAT_EXTENSIBLE : wFormatTag);
    lsx_writew(ft, wChannels);
    lsx_writedw(ft, dwSamplesPerSecond);
    lsx_writedw(ft, dwAvgBytesPerSec);
    lsx_writew(ft, wBlockAlign);
    lsx_writew(ft, wBitsPerSample); /* end info common to all fmts */

    if (isExtensible) {
      uint32_t dwChannelMask=0;  /* unassigned speaker mapping by default */
      static unsigned char const guids[][14] = {
        "\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71",  /* wav */
        "\x00\x00\x21\x07\xd3\x11\x86\x44\xc8\xc1\xca\x00\x00\x00"}; /* amb */

      /* if not amb, assume most likely channel masks from number of channels; not
       * ideal solution, but will make files playable in many/most situations
       */
      if (strcmp(ft->filetype, "amb")) {
        if      (wChannels == 1) dwChannelMask = 0x4;     /* 1 channel (mono) = FC */
        else if (wChannels == 2) dwChannelMask = 0x3;     /* 2 channels (stereo) = FL, FR */
        else if (wChannels == 4) dwChannelMask = 0x33;    /* 4 channels (quad) = FL, FR, BL, BR */
        else if (wChannels == 6) dwChannelMask = 0x3F;    /* 6 channels (5.1) = FL, FR, FC, LF, BL, BR */
        else if (wChannels == 8) dwChannelMask = 0x63F;   /* 8 channels (7.1) = FL, FR, FC, LF, BL, BR, SL, SR */
      }
 
      lsx_writew(ft, 22);
      lsx_writew(ft, wBitsPerSample); /* No padding in container */
      lsx_writedw(ft, dwChannelMask); /* Speaker mapping is something reasonable */
      lsx_writew(ft, wFormatTag);
      lsx_writebuf(ft, guids[!strcmp(ft->filetype, "amb")], (size_t)14);
    }
    else
    /* if not PCM, we need to write out wExtSize even if wExtSize=0 */
    if (wFormatTag != WAVE_FORMAT_PCM)
        lsx_writew(ft,wExtSize);

    switch (wFormatTag)
    {
        int i;
        case WAVE_FORMAT_IMA_ADPCM:
        lsx_writew(ft, wSamplesPerBlock);
        break;
        case WAVE_FORMAT_ADPCM:
        lsx_writew(ft, wSamplesPerBlock);
        lsx_writew(ft, 7); /* nCoefs */
        for (i=0; i<7; i++) {
            lsx_writew(ft, (uint16_t)(lsx_ms_adpcm_i_coef[i][0]));
            lsx_writew(ft, (uint16_t)(lsx_ms_adpcm_i_coef[i][1]));
        }
        break;
        case WAVE_FORMAT_GSM610:
        lsx_writew(ft, wSamplesPerBlock);
        break;
        default:
        break;
    }

    /* if not PCM, write the 'fact' chunk */
    if (isExtensible || wFormatTag != WAVE_FORMAT_PCM){
        lsx_writes(ft, "fact");
        lsx_writedw(ft,dwFactSize);
        lsx_writedw(ft,dwSamplesWritten);
    }

    lsx_writes(ft, "data");
    lsx_writedw(ft, dwDataLength);               /* data chunk size */

    if (!second_header) {
        lsx_debug("Writing Wave file: %s format, %d channel%s, %d samp/sec",
                wav_format_str(wFormatTag), wChannels,
                wChannels == 1 ? "" : "s", dwSamplesPerSecond);
        lsx_debug("        %d byte/sec, %d block align, %d bits/samp",
                dwAvgBytesPerSec, wBlockAlign, wBitsPerSample);
    } else {
        if (wRiffLength == UINT32_MAX || dwDataLength == UINT32_MAX ||
            dwSamplesWritten == UINT32_MAX)
            lsx_warn("File too large, writing truncated values in header");

        lsx_debug("Finished writing Wave file, %"PRIu64" data bytes %"PRIu64" samples",
                  dwDataLength, wav->numSamples);
#ifdef HAVE_LIBGSM
        if (wFormatTag == WAVE_FORMAT_GSM610){
            lsx_debug("GSM6.10 format: %"PRIu64" blocks %"PRIu64" padded samples %"PRIu64" padded data bytes",
                    blocksWritten, dwSamplesWritten, dwDataLength);
            if (wav->gsmbytecount != dwDataLength)
                lsx_warn("help ! internal inconsistency - data_written %"PRIu64" gsmbytecount %zu",
                         dwDataLength, wav->gsmbytecount);

        }
#endif
    }
    return SOX_SUCCESS;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
        priv_t *   wav = (priv_t *) ft->priv;
        ptrdiff_t total_len = len;

        ft->sox_errno = SOX_SUCCESS;

        switch (wav->formatTag)
        {
        case WAVE_FORMAT_IMA_ADPCM:
        case WAVE_FORMAT_ADPCM:
            while (len>0) {
                short *p = wav->samplePtr;
                short *top = wav->sampleTop;

                if (top>p+len) top = p+len;
                len -= top-p; /* update residual len */
                while (p < top)
                   *p++ = (*buf++) >> 16;

                wav->samplePtr = p;
                if (p == wav->sampleTop)
                    xxxAdpcmWriteBlock(ft);

            }
            return total_len - len;
            break;

#ifdef HAVE_LIBGSM
        case WAVE_FORMAT_GSM610:
            len = wavgsmwrite(ft, buf, len);
            wav->numSamples += (len/ft->signal.channels);
            return len;
            break;
#endif

        default:
            len = lsx_rawwrite(ft, buf, len);
            wav->numSamples += (len/ft->signal.channels);
            return len;
        }
}

static int stopwrite(sox_format_t * ft)
{
        priv_t *   wav = (priv_t *) ft->priv;

        ft->sox_errno = SOX_SUCCESS;


        /* Call this to flush out any remaining data. */
        switch (wav->formatTag)
        {
        case WAVE_FORMAT_IMA_ADPCM:
        case WAVE_FORMAT_ADPCM:
            xxxAdpcmWriteBlock(ft);
            break;
#ifdef HAVE_LIBGSM
        case WAVE_FORMAT_GSM610:
            wavgsmstopwrite(ft);
            break;
#endif
        }

        /* Add a pad byte if the number of data bytes is odd.
           See wavwritehdr() above for the calculation. */
        if (wav->formatTag != WAVE_FORMAT_GSM610)
          lsx_padbytes(ft, (size_t)((wav->numSamples + wav->samplesPerBlock - 1)/wav->samplesPerBlock*wav->blockAlign) % 2);

        free(wav->packet);
        free(wav->samples);
        free(wav->lsx_ms_adpcm_i_coefs);

        /* All samples are already written out. */
        /* If file header needs fixing up, for example it needs the */
        /* the number of samples in a field, seek back and write them here. */
        if (ft->signal.length && wav->numSamples <= 0xffffffff && 
            wav->numSamples == ft->signal.length)
          return SOX_SUCCESS;
        if (!ft->seekable)
          return SOX_EOF;

        if (lsx_seeki(ft, (off_t)0, SEEK_SET) != 0)
        {
                lsx_fail_errno(ft,SOX_EOF,"Can't rewind output file to rewrite .wav header.");
                return SOX_EOF;
        }

        return (wavwritehdr(ft, 1));
}

/*
 * Return a string corresponding to the wave format type.
 */
static const char *wav_format_str(unsigned tag)
{
    const struct wave_format *f = wav_find_format(tag);
    return f ? f->name : "unknown";
}

static int seek(sox_format_t * ft, uint64_t offset)
{
  priv_t *   wav = (priv_t *) ft->priv;

  if (ft->encoding.bits_per_sample & 7)
    lsx_fail_errno(ft, SOX_ENOTSUP, "seeking not supported with this encoding");
  else if (wav->formatTag == WAVE_FORMAT_GSM610) {
    int alignment;
    size_t gsmoff;

    /* rounding bytes to blockAlign so that we
     * don't have to decode partial block. */
    gsmoff = offset * wav->blockAlign / wav->samplesPerBlock +
             wav->blockAlign * ft->signal.channels / 2;
    gsmoff -= gsmoff % (wav->blockAlign * ft->signal.channels);

    ft->sox_errno = lsx_seeki(ft, (off_t)(gsmoff + wav->dataStart), SEEK_SET);
    if (ft->sox_errno == SOX_SUCCESS) {
      /* offset is in samples */
      uint64_t new_offset = offset;
      alignment = offset % wav->samplesPerBlock;
      if (alignment != 0)
          new_offset += (wav->samplesPerBlock - alignment);
      wav->numSamples = ft->signal.length - (new_offset / ft->signal.channels);
    }
  } else {
    double wide_sample = offset - (offset % ft->signal.channels);
    double to_d = wide_sample * ft->encoding.bits_per_sample / 8;
    off_t to = to_d;
    ft->sox_errno = (to != to_d)? SOX_EOF : lsx_seeki(ft, (off_t)wav->dataStart + (off_t)to, SEEK_SET);
    if (ft->sox_errno == SOX_SUCCESS)
      wav->numSamples -= (size_t)wide_sample / ft->signal.channels;
  }

  return ft->sox_errno;
}

LSX_FORMAT_HANDLER(wav)
{
  static char const * const names[] = {"wav", "wavpcm", "amb", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 16, 24, 32, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_ALAW, 8, 0,
#ifdef HAVE_LIBGSM
    SOX_ENCODING_GSM, 0,
#endif
    SOX_ENCODING_MS_ADPCM, 4, 0,
    SOX_ENCODING_IMA_ADPCM, 4, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Microsoft audio format", names, SOX_FILE_LIT_END,
    startread, read_samples, stopread,
    startwrite, write_samples, stopwrite,
    seek, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
