/* Copyright 1997 Chris Bagwell And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And Sundry Contributors are not
 * responsible for the consequences of using this software.
 *
 * Direct to Open Sound System (OSS) sound driver
 * OSS is a popular unix sound driver for Intel x86 unices (eg. Linux)
 * and several other unixes (such as SunOS/Solaris).
 * This driver is compatible with OSS original source that was called
 * USS, Voxware and TASD.
 *
 * added by Chris Bagwell (cbagwell@sprynet.com) on 2/19/96
 * based on info grabed from vplay.c in Voxware snd-utils-3.5 package.
 * and on LINUX_PLAYER patches added by Greg Lee
 * which was originally from Directo to Sound Blaster device driver (sbdsp.c).
 * SBLAST patches by John T. Kohl.
 *
 * Changes:
 *
 * Nov. 26, 1999 Stan Brooks <stabro@megsinet.net>
 *   Moved initialization code common to startread and startwrite
 *   into a single function ossdspinit().
 *
 */

#include "sox_i.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_SYS_SOUNDCARD_H
  #include <sys/soundcard.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* these appear in the sys/soundcard.h of OSS 4.x, and in Linux's
 * sound/core/oss/pcm_oss.c (2.6.24 and later), but are typically
 * not included in system header files.
 */
#ifndef AFMT_S32_LE
#define AFMT_S32_LE 0x00001000
#endif
#ifndef AFMT_S32_BE
#define AFMT_S32_BE 0x00002000
#endif

#include <sys/ioctl.h>

typedef struct
{
    char* pOutput;
    unsigned cOutput;
    int device;
    unsigned sample_shift;
} priv_t;

/* common r/w initialization code */
static int ossinit(sox_format_t* ft)
{
    int sampletype, samplesize;
    int tmp, rc;
    char const* szDevname;
    priv_t* pPriv = (priv_t*)ft->priv;
 
    if (ft->filename == 0 || ft->filename[0] == 0 || !strcasecmp("default", ft->filename))
    {
        szDevname = getenv("OSS_AUDIODEV");
        if (szDevname != NULL)
        {
            lsx_report("Using device name from OSS_AUDIODEV environment variable: %s", szDevname);
        }
        else
        {
            szDevname = "/dev/dsp";
            lsx_report("Using default OSS device name: %s", szDevname);
        }
    }
    else
    {
        szDevname = ft->filename;
        lsx_report("Using user-specified device name: %s", szDevname);
    }

    pPriv->device = open(
        szDevname,
        ft->mode == 'r' ? O_RDONLY : O_WRONLY);
    if (pPriv->device < 0) {
        lsx_fail_errno(ft, errno, "open failed for device: %s", szDevname);
        return SOX_EOF;
    }

    if (ft->encoding.bits_per_sample == 8) {
        sampletype = AFMT_U8;
        samplesize = 8;
        pPriv->sample_shift = 0;
        if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
            ft->encoding.encoding = SOX_ENCODING_UNSIGNED;
        if (ft->encoding.encoding != SOX_ENCODING_UNSIGNED) {
            lsx_report("OSS driver only supports unsigned with bytes");
            lsx_report("Forcing to unsigned");
            ft->encoding.encoding = SOX_ENCODING_UNSIGNED;
        }
    }
    else if (ft->encoding.bits_per_sample == 16) {
        /* Attempt to use endian that user specified */
        if (ft->encoding.reverse_bytes)
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S16_LE : AFMT_S16_BE;
        else
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S16_BE : AFMT_S16_LE;
        samplesize = 16;
        pPriv->sample_shift = 1;
        if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        if (ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_report("OSS driver only supports signed with words");
            lsx_report("Forcing to signed linear");
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        }
    }
    else if (ft->encoding.bits_per_sample == 32) {
        /* Attempt to use endian that user specified */
        if (ft->encoding.reverse_bytes)
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S32_LE : AFMT_S32_BE;
        else
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S32_BE : AFMT_S32_LE;
        samplesize = 32;
        pPriv->sample_shift = 2;
        if (ft->encoding.encoding == SOX_ENCODING_UNKNOWN)
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        if (ft->encoding.encoding != SOX_ENCODING_SIGN2) {
            lsx_report("OSS driver only supports signed with words");
            lsx_report("Forcing to signed linear");
            ft->encoding.encoding = SOX_ENCODING_SIGN2;
        }
    }
    else {
        /* Attempt to use endian that user specified */
        if (ft->encoding.reverse_bytes)
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S16_LE : AFMT_S16_BE;
        else
            sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S16_BE : AFMT_S16_LE;
        samplesize = 16;
        pPriv->sample_shift = 1;
        ft->encoding.bits_per_sample = 16;
        ft->encoding.encoding = SOX_ENCODING_SIGN2;
        lsx_report("OSS driver only supports bytes and words");
        lsx_report("Forcing to signed linear word");
    }

    ft->signal.channels = 2;

    if (ioctl(pPriv->device, (size_t) SNDCTL_DSP_RESET, 0) < 0)
    {
        lsx_fail_errno(ft,SOX_EOF,"Unable to reset OSS device %s. Possibly accessing an invalid file/device", szDevname);
        return(SOX_EOF);
    }

    /* Query the supported formats and find the best match
     */
    rc = ioctl(pPriv->device, SNDCTL_DSP_GETFMTS, &tmp);
    if (rc == 0) {
        if ((tmp & sampletype) == 0)
        {
            /* is 16-bit supported? */
            if (samplesize == 16 && (tmp & (AFMT_S16_LE|AFMT_S16_BE)) == 0)
            {
                /* Must not like 16-bits, try 8-bits */
                ft->encoding.bits_per_sample = 8;
                ft->encoding.encoding = SOX_ENCODING_UNSIGNED;
                lsx_report("OSS driver doesn't like signed words");
                lsx_report("Forcing to unsigned bytes");
                tmp = sampletype = AFMT_U8;
                samplesize = 8;
                pPriv->sample_shift = 0;
            }
            /* is 8-bit supported */
            else if (samplesize == 8 && (tmp & AFMT_U8) == 0)
            {
                ft->encoding.bits_per_sample = 16;
                ft->encoding.encoding = SOX_ENCODING_SIGN2;
                lsx_report("OSS driver doesn't like unsigned bytes");
                lsx_report("Forcing to signed words");
                sampletype = (MACHINE_IS_BIGENDIAN) ? AFMT_S16_BE : AFMT_S16_LE;
                samplesize = 16;
                pPriv->sample_shift = 1;
            }
            /* determine which 16-bit format to use */
            if (samplesize == 16 && (tmp & sampletype) == 0)
            {
                /* Either user requested something not supported
                 * or hardware doesn't support machine endian.
                 * Force to opposite as the above test showed
                 * it supports at least one of the two endians.
                 */
                sampletype = (sampletype == AFMT_S16_BE) ? AFMT_S16_LE : AFMT_S16_BE;
                ft->encoding.reverse_bytes = !ft->encoding.reverse_bytes;
            }

        }
        tmp = sampletype;
        rc = ioctl(pPriv->device, SNDCTL_DSP_SETFMT, &tmp);
    }
    /* Give up and exit */
    if (rc < 0 || tmp != sampletype)
    {
        lsx_fail_errno(ft,SOX_EOF,"Unable to set the sample size to %d", samplesize);
        return (SOX_EOF);
    }

    tmp = 1;
    if (ioctl(pPriv->device, SNDCTL_DSP_STEREO, &tmp) < 0 || tmp != 1)
    {
        lsx_warn("Couldn't set to stereo");
        ft->signal.channels = 1;
    }

    tmp = ft->signal.rate;
    if (ioctl(pPriv->device, SNDCTL_DSP_SPEED, &tmp) < 0 ||
        (int)ft->signal.rate != tmp) {
        /* If the rate the sound card is using is not within 1% of what
         * the user specified then override the user setting.
         * The only reason not to always override this is because of
         * clock-rounding problems. Sound cards will sometimes use
         * things like 44101 when you ask for 44100.  No need overriding
         * this and having strange output file rates for something that
         * we can't hear anyways.
         */
        if ((int)ft->signal.rate - tmp > (tmp * .01) ||
            tmp - (int)ft->signal.rate > (tmp * .01))
            ft->signal.rate = tmp;
    }

    if (ioctl(pPriv->device, (size_t) SNDCTL_DSP_SYNC, NULL) < 0) {
        lsx_fail_errno(ft,SOX_EOF,"Unable to sync dsp");
        return (SOX_EOF);
    }

    if (ft->mode == 'r') {
        pPriv->cOutput = 0;
        pPriv->pOutput = NULL;
    } else {
        size_t cbOutput = sox_globals.bufsiz;
        pPriv->cOutput = cbOutput >> pPriv->sample_shift;
        pPriv->pOutput = lsx_malloc(cbOutput);
    }

    return(SOX_SUCCESS);
}

static int ossstop(sox_format_t* ft)
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

static size_t ossread(sox_format_t* ft, sox_sample_t* pOutput, size_t cOutput)
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
    if (ft->encoding.reverse_bytes) {
        switch (pPriv->sample_shift)
        {
        case 0:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_UNSIGNED_8BIT_TO_SAMPLE(
                ((sox_uint8_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        case 1:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_SIGNED_16BIT_TO_SAMPLE(
                lsx_swapw(((sox_int16_t*)pOutput)[i - 1]),
                dummy);
            }
            break;
        case 2:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_SIGNED_32BIT_TO_SAMPLE(
                lsx_swapdw(((sox_int32_t*)pOutput)[i - 1]),
                dummy);
            }
            break;
        }
    } else {
        switch (pPriv->sample_shift)
        {
        case 0:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_UNSIGNED_8BIT_TO_SAMPLE(
                ((sox_uint8_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        case 1:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_SIGNED_16BIT_TO_SAMPLE(
                ((sox_int16_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        case 2:
            for (i = cRead; i != 0; i--) {
                pOutput[i - 1] = SOX_SIGNED_32BIT_TO_SAMPLE(
                ((sox_int32_t*)pOutput)[i - 1],
                dummy);
            }
            break;
        }
     }

    return cRead;
}

static size_t osswrite(
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

        if (ft->encoding.reverse_bytes)
        {
            switch (pPriv->sample_shift)
            {
            case 0:
                for (i = 0; i != cStride; i++) {
                    ((sox_uint8_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_UNSIGNED_8BIT(pInput[i], cClips);
                }
                break;
            case 1:
                for (i = 0; i != cStride; i++) {
                    sox_int16_t s16 = SOX_SAMPLE_TO_SIGNED_16BIT(pInput[i], cClips);
                    ((sox_int16_t*)pPriv->pOutput)[i] = lsx_swapw(s16);
                }
                break;
            case 2:
                for (i = 0; i != cStride; i++) {
                    ((sox_int32_t*)pPriv->pOutput)[i] =
                        lsx_swapdw(SOX_SAMPLE_TO_SIGNED_32BIT(pInput[i], cClips));
                }
                break;
            }
        } else {
            switch (pPriv->sample_shift)
            {
            case 0:
                for (i = 0; i != cStride; i++) {
                    ((sox_uint8_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_UNSIGNED_8BIT(pInput[i], cClips);
                }
                break;
            case 1:
                for (i = 0; i != cStride; i++) {
                    ((sox_int16_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_SIGNED_16BIT(pInput[i], cClips);
                }
                break;
            case 2:
                for (i = 0; i != cStride; i++) {
                    ((sox_int32_t*)pPriv->pOutput)[i] =
                        SOX_SAMPLE_TO_SIGNED_32BIT(pInput[i], cClips);
                }
                break;
            }
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

LSX_FORMAT_HANDLER(oss)
{
  static char const* const names[] = {"ossdsp", "oss", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 32, 16, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Open Sound System device driver for unix-like systems",
    names, SOX_FILE_DEVICE | SOX_FILE_NOSTDIO,
    ossinit, ossread, ossstop,
    ossinit, osswrite, ossstop,
    NULL, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
