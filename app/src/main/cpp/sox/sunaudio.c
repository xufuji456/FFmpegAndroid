/* libSoX direct to Sun Audio Driver
 *
 * Added by Chris Bagwell (cbagwell@sprynet.com) on 2/26/96
 * Based on oss handler.
 *
 * Cleaned up changes of format somewhat in sunstartwrite on 03/31/98
 *
 */

/*
 * Copyright 1997 Chris Bagwell And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Rick Richardson, Lance Norskog And Sundry Contributors are not
 * responsible for the consequences of using this software.
 */

#include "sox_i.h"
#include "g711.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef HAVE_SUN_AUDIOIO_H
  #include <sun/audioio.h>
#else
  #include <sys/audioio.h>
#endif
#include <errno.h>
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <stropts.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

typedef struct
{
    char* pOutput;
    unsigned cOutput;
    int device;
    unsigned sample_shift;
} priv_t;

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int sunstartread(sox_format_t * ft)
{
    char const* szDevname;
    priv_t* pPriv = (priv_t*)ft->priv;
 
    size_t samplesize, encoding;
    audio_info_t audio_if;
#ifdef __SVR4
    audio_device_t audio_dev;
#endif
    char simple_hw=0;

    lsx_set_signal_defaults(ft);

    if (ft->filename == 0 || ft->filename[0] == 0 || !strcasecmp("default", ft->filename)) {
        szDevname = "/dev/audio";
    } else {
        szDevname = ft->filename;
    }

    pPriv->device = open(szDevname, O_RDONLY);
    if (pPriv->device < 0) {
        lsx_fail_errno(ft, errno, "open failed for device %s", szDevname);
        return SOX_EOF;
    }

    if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN) ft->encoding.encoding = SOX_ENCODING_ULAW;

#ifdef __SVR4
    /* Read in old values, change to what we need and then send back */
    if (ioctl(pPriv->device, AUDIO_GETDEV, &audio_dev) < 0) {
        lsx_fail_errno(ft,errno,"Unable to get information for device %s", szDevname);
        return(SOX_EOF);
    }
    lsx_report("Hardware detected:  %s",audio_dev.name);
    if (strcmp("SUNW,am79c30",audio_dev.name) == 0)
    {
        simple_hw = 1;
    }
#endif

    /* If simple hardware detected in force data to ulaw. */
    if (simple_hw)
    {
        if (ft->encoding.bits_per_sample == 8)
        {
            if (ft->encoding.encoding != SOX_ENCODING_ULAW &&
                ft->encoding.encoding != SOX_ENCODING_ALAW)
            {
                lsx_report("Warning: Detected simple hardware.  Forcing output to ULAW");
                ft->encoding.encoding = SOX_ENCODING_ULAW;
            }
        }
        else if (ft->encoding.bits_per_sample == 16)
        {
            lsx_report("Warning: Detected simple hardware.  Forcing output to ULAW");
            ft->encoding.bits_per_sample = 8;
            ft->encoding.encoding = SOX_ENCODING_ULAW;
        }
    }

    if (ft->encoding.bits_per_sample == 8) {
        samplesize = 8;
        pPriv->sample_shift = 0;
        if (ft->encoding.encoding != SOX_ENCODING_ULAW &&
            ft->encoding.encoding != SOX_ENCODING_ALAW &&
            ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_fail_errno(ft,SOX_EFMT,"Sun audio driver only supports ULAW, ALAW, and signed linear for bytes.");
                return (SOX_EOF);
        }
        if ((ft->encoding.encoding == SOX_ENCODING_ULAW ||
             ft->encoding.encoding == SOX_ENCODING_ALAW) &&
            ft->signal.channels == 2)
        {
            lsx_report("Warning: only support mono for ULAW and ALAW data.  Forcing to mono.");
            ft->signal.channels = 1;
        }
    }
    else if (ft->encoding.bits_per_sample == 16) {
        samplesize = 16;
        pPriv->sample_shift = 1;
        if (ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_fail_errno(ft,SOX_EFMT,"Sun audio driver only supports signed linear for words.");
            return(SOX_EOF);
        }
    }
    else {
        lsx_fail_errno(ft,SOX_EFMT,"Sun audio driver only supports bytes and words");
        return(SOX_EOF);
    }

    if (ft->signal.channels == 0)
        ft->signal.channels = 1;
    else if (ft->signal.channels > 1) {
        lsx_report("Warning: some Sun audio devices can not play stereo");
        lsx_report("at all or sometimes only with signed words.  If the");
        lsx_report("sound seems sluggish then this is probably the case.");
        lsx_report("Try forcing output to signed words or use the avg");
        lsx_report("filter to reduce the number of channels.");
        ft->signal.channels = 2;
    }

    /* Read in old values, change to what we need and then send back */
    if (ioctl(pPriv->device, AUDIO_GETINFO, &audio_if) < 0) {
        lsx_fail_errno(ft,errno,"Unable to initialize %s", szDevname);
        return(SOX_EOF);
    }
    audio_if.record.precision = samplesize;
    audio_if.record.channels = ft->signal.channels;
    audio_if.record.sample_rate = ft->signal.rate;
    if (ft->encoding.encoding == SOX_ENCODING_ULAW)
        encoding = AUDIO_ENCODING_ULAW;
    else if (ft->encoding.encoding == SOX_ENCODING_ALAW)
        encoding = AUDIO_ENCODING_ALAW;
    else
        encoding = AUDIO_ENCODING_LINEAR;
    audio_if.record.encoding = encoding;

    ioctl(pPriv->device, AUDIO_SETINFO, &audio_if);
    if (audio_if.record.precision != samplesize) {
        lsx_fail_errno(ft,errno,"Unable to initialize sample size for %s", szDevname);
        return(SOX_EOF);
    }
    if (audio_if.record.channels != ft->signal.channels) {
        lsx_fail_errno(ft,errno,"Unable to initialize number of channels for %s", szDevname);
        return(SOX_EOF);
    }
    if (audio_if.record.sample_rate != ft->signal.rate) {
        lsx_fail_errno(ft,errno,"Unable to initialize rate for %s", szDevname);
        return(SOX_EOF);
    }
    if (audio_if.record.encoding != encoding) {
        lsx_fail_errno(ft,errno,"Unable to initialize encoding for %s", szDevname);
        return(SOX_EOF);
    }
    /* Flush any data in the buffers - its probably in the wrong format */
#if defined(__NetBSD__) || defined(__OpenBSD__)
    ioctl(pPriv->device, AUDIO_FLUSH);
#elif defined __GLIBC__
    ioctl(pPriv->device, (unsigned long int)I_FLUSH, FLUSHR);
#else
    ioctl(pPriv->device, I_FLUSH, FLUSHR);
#endif

    pPriv->cOutput = 0;
    pPriv->pOutput = NULL;

    return (SOX_SUCCESS);
}

static int sunstartwrite(sox_format_t * ft)
{
    size_t samplesize, encoding;
    audio_info_t audio_if;
#ifdef __SVR4
    audio_device_t audio_dev;
#endif
    char simple_hw=0;
    char const* szDevname;
    priv_t* pPriv = (priv_t*)ft->priv;
 
    if (ft->filename == 0 || ft->filename[0] == 0 || !strcasecmp("default", ft->filename)) {
        szDevname = "/dev/audio";
    } else {
        szDevname = ft->filename;
    }

    pPriv->device = open(szDevname, O_WRONLY);
    if (pPriv->device < 0) {
        lsx_fail_errno(ft, errno, "open failed for device: %s", szDevname);
        return SOX_EOF;
    }

#ifdef __SVR4
    /* Read in old values, change to what we need and then send back */
    if (ioctl(pPriv->device, AUDIO_GETDEV, &audio_dev) < 0) {
        lsx_fail_errno(ft,errno,"Unable to get device information.");
        return(SOX_EOF);
    }
    lsx_report("Hardware detected:  %s",audio_dev.name);
    if (strcmp("SUNW,am79c30",audio_dev.name) == 0)
    {
        simple_hw = 1;
    }
#endif

    if (simple_hw)
    {
        if (ft->encoding.bits_per_sample == 8)
        {
            if (ft->encoding.encoding != SOX_ENCODING_ULAW &&
                ft->encoding.encoding != SOX_ENCODING_ALAW)
            {
                lsx_report("Warning: Detected simple hardware.  Forcing output to ULAW");
                ft->encoding.encoding = SOX_ENCODING_ULAW;
            }
        }
        else if (ft->encoding.bits_per_sample == 16)
        {
            lsx_report("Warning: Detected simple hardware.  Forcing output to ULAW");
            ft->encoding.bits_per_sample = 8;
            ft->encoding.encoding = SOX_ENCODING_ULAW;
        }
    }

    if (ft->encoding.bits_per_sample == 8)
    {
        samplesize = 8;
        pPriv->sample_shift = 0;
        if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
            ft->encoding.encoding = SOX_ENCODING_ULAW;
        else if (ft->encoding.encoding != SOX_ENCODING_ULAW &&
            ft->encoding.encoding != SOX_ENCODING_ALAW &&
            ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_report("Sun Audio driver only supports ULAW, ALAW, and Signed Linear for bytes.");
            lsx_report("Forcing to ULAW");
            ft->encoding.encoding = SOX_ENCODING_ULAW;
        }
        if ((ft->encoding.encoding == SOX_ENCODING_ULAW ||
             ft->encoding.encoding == SOX_ENCODING_ALAW) &&
            ft->signal.channels == 2)
        {
            lsx_report("Warning: only support mono for ULAW and ALAW data.  Forcing to mono.");
            ft->signal.channels = 1;
        }

    }
    else if (ft->encoding.bits_per_sample == 16) {
        samplesize = 16;
        pPriv->sample_shift = 1;
        if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        else if (ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_report("Sun Audio driver only supports Signed Linear for words.");
            lsx_report("Forcing to Signed Linear");
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        }
    }
    else {
        lsx_report("Sun Audio driver only supports bytes and words");
        ft->encoding.bits_per_sample = 16;
        ft->encoding.encoding = SOX_ENCODING_SIGN2;
        samplesize = 16;
        pPriv->sample_shift = 1;
    }

    if (ft->signal.channels > 1) ft->signal.channels = 2;

    /* Read in old values, change to what we need and then send back */
    if (ioctl(pPriv->device, AUDIO_GETINFO, &audio_if) < 0) {
        lsx_fail_errno(ft,errno,"Unable to initialize /dev/audio");
        return(SOX_EOF);
    }
    audio_if.play.precision = samplesize;
    audio_if.play.channels = ft->signal.channels;
    audio_if.play.sample_rate = ft->signal.rate;
    if (ft->encoding.encoding == SOX_ENCODING_ULAW)
        encoding = AUDIO_ENCODING_ULAW;
    else if (ft->encoding.encoding == SOX_ENCODING_ALAW)
        encoding = AUDIO_ENCODING_ALAW;
    else
        encoding = AUDIO_ENCODING_LINEAR;
    audio_if.play.encoding = encoding;

    ioctl(pPriv->device, AUDIO_SETINFO, &audio_if);
    if (audio_if.play.precision != samplesize) {
        lsx_fail_errno(ft,errno,"Unable to initialize sample size for /dev/audio");
        return(SOX_EOF);
    }
    if (audio_if.play.channels != ft->signal.channels) {
        lsx_fail_errno(ft,errno,"Unable to initialize number of channels for /dev/audio");
        return(SOX_EOF);
    }
    if (audio_if.play.sample_rate != ft->signal.rate) {
        lsx_fail_errno(ft,errno,"Unable to initialize rate for /dev/audio");
        return(SOX_EOF);
    }
    if (audio_if.play.encoding != encoding) {
        lsx_fail_errno(ft,errno,"Unable to initialize encoding for /dev/audio");
        return(SOX_EOF);
    }

    pPriv->cOutput = sox_globals.bufsiz >> pPriv->sample_shift;
    pPriv->pOutput = lsx_malloc((size_t)pPriv->cOutput << pPriv->sample_shift);

    return (SOX_SUCCESS);
}

static int sunstop(sox_format_t* ft)
{
    priv_t* pPriv = (priv_t*)ft->priv;
    if (pPriv->device >= 0) {
        close(pPriv->device);
    }
    if (pPriv->pOutput) {
        free(pPriv->pOutput);
    }
    return SOX_SUCCESS;
}

typedef sox_uint16_t sox_uint14_t;
typedef sox_uint16_t sox_uint13_t;
typedef sox_int16_t sox_int14_t;
typedef sox_int16_t sox_int13_t;
#define SOX_ULAW_BYTE_TO_SAMPLE(d,clips)   SOX_SIGNED_16BIT_TO_SAMPLE(sox_ulaw2linear16(d),clips)
#define SOX_ALAW_BYTE_TO_SAMPLE(d,clips)   SOX_SIGNED_16BIT_TO_SAMPLE(sox_alaw2linear16(d),clips)
#define SOX_SAMPLE_TO_ULAW_BYTE(d,c) sox_14linear2ulaw(SOX_SAMPLE_TO_UNSIGNED(14,d,c) - 0x2000)
#define SOX_SAMPLE_TO_ALAW_BYTE(d,c) sox_13linear2alaw(SOX_SAMPLE_TO_UNSIGNED(13,d,c) - 0x1000)

static size_t sunread(sox_format_t* ft, sox_sample_t* pOutput, size_t cOutput)
{
    priv_t* pPriv = (priv_t*)ft->priv;
    char* pbOutput = (char*)pOutput;
    size_t cbOutputLeft = cOutput << pPriv->sample_shift;
    size_t i, cRead;
    int cbRead;
    SOX_SAMPLE_LOCALS;
    LSX_USE_VAR(sox_macro_temp_double);

    while (cbOutputLeft) {
        cbRead = read(pPriv->device, pbOutput, cbOutputLeft);
        if (cbRead <= 0) {
            if (cbRead < 0) {
                lsx_fail_errno(ft, errno, "Error reading from device");
                return 0;
            }
            break;
        }
        cbOutputLeft -= cbRead;
        pbOutput += cbRead;
    }

    /* Convert in-place (backwards) */
    cRead = cOutput - (cbOutputLeft >> pPriv->sample_shift);
    switch (pPriv->sample_shift)
    {
    case 0:
        switch (ft->encoding.encoding)
        {
        case SOX_ENCODING_SIGN2:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_UNSIGNED_8BIT_TO_SAMPLE(
                ((sox_uint8_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        case SOX_ENCODING_ULAW:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_ULAW_BYTE_TO_SAMPLE(
                ((sox_uint8_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        case SOX_ENCODING_ALAW:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_ALAW_BYTE_TO_SAMPLE(
                ((sox_uint8_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        default:
            return 0;
        }
        break;
    case 1:
        for (i = cRead; i != 0; i--) {
            pOutput[i - 1] = SOX_SIGNED_16BIT_TO_SAMPLE(
            ((sox_int16_t*)pOutput)[i - 1],
            dummy);
        }
        break;
    }

    return cRead;
}

static size_t sunwrite(
    sox_format_t* ft,
    const sox_sample_t* pInput,
    size_t cInput)
{
    priv_t* pPriv = (priv_t*)ft->priv;
    size_t cInputRemaining = cInput;
    unsigned cClips = 0;
    SOX_SAMPLE_LOCALS;

    while (cInputRemaining) {
        size_t cStride;
        size_t i;
        size_t cbStride;
        int cbWritten;

        cStride = cInputRemaining;
        if (cStride > pPriv->cOutput) {
            cStride = pPriv->cOutput;
        }

        switch (pPriv->sample_shift)
        {
        case 0:
            switch (ft->encoding.encoding)
            {
            case SOX_ENCODING_SIGN2:
                for (i = 0; i != cStride; i++) {
                    ((sox_uint8_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_UNSIGNED_8BIT(pInput[i], cClips);
                }
                break;
            case SOX_ENCODING_ULAW:
                for (i = 0; i != cStride; i++) {
                    ((sox_uint8_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_ULAW_BYTE(pInput[i], cClips);
                }
                break;
            case SOX_ENCODING_ALAW:
                for (i = 0; i != cStride; i++) {
                    ((sox_uint8_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_ALAW_BYTE(pInput[i], cClips);
                }
                break;
            default:
                return 0;
            }
            break;
        case 1:
            for (i = 0; i != cStride; i++) {
                ((sox_int16_t*)pPriv->pOutput)[i] =
                    SOX_SAMPLE_TO_SIGNED_16BIT(pInput[i], cClips);
            }
            break;
        }

        cbStride = cStride << pPriv->sample_shift;
        i = 0;
        do {
            cbWritten = write(pPriv->device, &pPriv->pOutput[i], cbStride - i);
            i += cbWritten;
            if (cbWritten <= 0) {
                lsx_fail_errno(ft, errno, "Error writing to device");
                return 0;
            }
        } while (i != cbStride);

        cInputRemaining -= cStride;
        pInput += cStride;
    }

    return cInput;
}

LSX_FORMAT_HANDLER(sunau)
{
  static char const * const names[] = {"sunau", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_SIGN2, 8, 16, 0,
    0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Sun audio device driver",
    names, SOX_FILE_DEVICE | SOX_FILE_NOSTDIO,
    sunstartread, sunread, sunstop,
    sunstartwrite, sunwrite, sunstop,
    NULL, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
