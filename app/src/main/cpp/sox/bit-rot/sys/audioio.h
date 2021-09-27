#ifndef HAVE_SUN_AUDIO
/*
 * SoX bit-rot detection file, obtained from: Solaris 9 /usr/include/sys
 */
#if defined __GNUC__
  #pragma GCC system_header
#endif

/*
 * Copyright (c) 1995-2001 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_SYS_AUDIOIO_H
#define	_SYS_AUDIOIO_H

#pragma ident	"@(#)audioio.h	1.30	01/01/10 SMI"

#include <sys/types.h>
#if 0
#include <sys/types32.h>
#include <sys/time.h>
#include <sys/ioccom.h>
#else
#define ushort_t unsigned short
#define uint_t unsigned int
#define uchar_t unsigned char
struct timeval32
{
    unsigned tv_sec;
    unsigned tv_usec;
};
#endif

/*
 * These are the ioctl calls for all Solaris audio devices, including
 * the x86 and SPARCstation audio devices.
 *
 * You are encouraged to design your code in a modular fashion so that
 * future changes to the interface can be incorporated with little
 * trouble.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This structure contains state information for audio device IO streams.
 */
struct audio_prinfo {
	/*
	 * The following values describe the audio data encoding.
	 */
	uint_t sample_rate;	/* samples per second */
	uint_t channels;	/* number of interleaved channels */
	uint_t precision;	/* bit-width of each sample */
	uint_t encoding;	/* data encoding method */

	/*
	 * The following values control audio device configuration
	 */
	uint_t gain;		/* gain level: 0 - 255 */
	uint_t port;		/* selected I/O port (see below) */
	uint_t avail_ports;	/* available I/O ports (see below) */
	uint_t mod_ports;	/* I/O ports that are modifiable (see below) */
	uint_t _xxx;		/* Reserved for future use */

	uint_t buffer_size;	/* I/O buffer size */

	/*
	 * The following values describe driver state
	 */
	uint_t samples;		/* number of samples converted */
	uint_t eof;		/* End Of File counter (play only) */

	uchar_t	pause;		/* non-zero for pause, zero to resume */
	uchar_t	error;		/* non-zero if overflow/underflow */
	uchar_t	waiting;	/* non-zero if a process wants access */
	uchar_t balance;	/* stereo channel balance */

	ushort_t minordev;

	/*
	 * The following values are read-only state flags
	 */
	uchar_t open;		/* non-zero if open access permitted */
	uchar_t active;		/* non-zero if I/O is active */
};
typedef struct audio_prinfo audio_prinfo_t;


/*
 * This structure describes the current state of the audio device.
 */
struct audio_info {
	/*
	 * Per-stream information
	 */
	audio_prinfo_t play;	/* output status information */
	audio_prinfo_t record;	/* input status information */

	/*
	 * Per-unit/channel information
	 */
	uint_t monitor_gain;	/* input to output mix: 0 - 255 */
	uchar_t output_muted;	/* non-zero if output is muted */
	uchar_t ref_cnt;	/* driver reference count, read only */
	uchar_t _xxx[2];	/* Reserved for future use */
	uint_t hw_features;	/* hardware features this driver supports */
	uint_t sw_features;	/* supported SW features */
	uint_t sw_features_enabled;	/* supported SW feat. enabled */
};
typedef struct audio_info audio_info_t;


/*
 * Audio encoding types
 */
#define	AUDIO_ENCODING_NONE	(0)	/* no encoding assigned	*/
#define	AUDIO_ENCODING_ULAW	(1)	/* u-law encoding	*/
#define	AUDIO_ENCODING_ALAW	(2)	/* A-law encoding	*/
#define	AUDIO_ENCODING_LINEAR	(3)	/* Signed Linear PCM encoding	*/
#define	AUDIO_ENCODING_DVI	(104)	/* DVI ADPCM		*/
#define	AUDIO_ENCODING_LINEAR8	(105)	/* 8 bit UNSIGNED	*/

/*
 * These ranges apply to record, play, and monitor gain values
 */
#define	AUDIO_MIN_GAIN	(0)	/* minimum gain value */
#define	AUDIO_MAX_GAIN	(255)	/* maximum gain value */
#define	AUDIO_MID_GAIN	(AUDIO_MAX_GAIN / 2)

/*
 * These values apply to the balance field to adjust channel gain values
 */
#define	AUDIO_LEFT_BALANCE	(0)	/* left channel only	*/
#define	AUDIO_MID_BALANCE	(32)	/* equal left/right channel */
#define	AUDIO_RIGHT_BALANCE	(64)	/* right channel only	*/
#define	AUDIO_BALANCE_SHIFT	(3)

/*
 * Generic minimum/maximum limits for number of channels, both modes
 */
#define	AUDIO_CHANNELS_MONO	(1)
#define	AUDIO_CHANNELS_STEREO	(2)
#define	AUDIO_MIN_PLAY_CHANNELS	(AUDIO_CHANNELS_MONO)
#define	AUDIO_MAX_PLAY_CHANNELS	(AUDIO_CHANNELS_STEREO)
#define	AUDIO_MIN_REC_CHANNELS	(AUDIO_CHANNELS_MONO)
#define	AUDIO_MAX_REC_CHANNELS	(AUDIO_CHANNELS_STEREO)

/*
 * Generic minimum/maximum limits for sample precision
 */
#define	AUDIO_PRECISION_8		(8)
#define	AUDIO_PRECISION_16		(16)

#define	AUDIO_MIN_PLAY_PRECISION	(8)
#define	AUDIO_MAX_PLAY_PRECISION	(32)
#define	AUDIO_MIN_REC_PRECISION		(8)
#define	AUDIO_MAX_REC_PRECISION		(32)

/*
 * Define some convenient names for typical audio ports
 */
#define	AUDIO_NONE		0x00	/* all ports off */
/*
 * output ports (several may be enabled simultaneously)
 */
#define	AUDIO_SPEAKER		0x01	/* output to built-in speaker */
#define	AUDIO_HEADPHONE		0x02	/* output to headphone jack */
#define	AUDIO_LINE_OUT		0x04	/* output to line out	*/
#define	AUDIO_SPDIF_OUT		0x08	/* output to SPDIF port	*/
#define	AUDIO_AUX1_OUT		0x10	/* output to aux1 out	*/
#define	AUDIO_AUX2_OUT		0x20	/* output to aux2 out	*/

/*
 * input ports (usually only one at a time)
 */
#define	AUDIO_MICROPHONE	0x01	/* input from microphone */
#define	AUDIO_LINE_IN		0x02	/* input from line in	*/
#define	AUDIO_CD		0x04	/* input from on-board CD inputs */
#define	AUDIO_INTERNAL_CD_IN	AUDIO_CD	/* input from internal CDROM */
#define	AUDIO_SPDIF_IN		0x08	/* input from SPDIF port */
#define	AUDIO_AUX1_IN		0x10	/* input from aux1 in	*/
#define	AUDIO_AUX2_IN		0x20	/* input from aux2 in	*/
#define	AUDIO_CODEC_LOOPB_IN	0x40	/* input from Codec internal loopback */
#define	AUDIO_SUNVTS		0x80	/* SunVTS input setting-internal LB */

/*
 * Define the hw_features
 */
#define	AUDIO_HWFEATURE_DUPLEX	0x00000001u	/* simult. play & rec support */
#define	AUDIO_HWFEATURE_MSCODEC	0x00000002u	/* multi-stream Codec */
#define	AUDIO_HWFEATURE_IN2OUT	0x00000004u	/* input to output loopback */
#define	AUDIO_HWFEATURE_PLAY	0x00000008u	/* device supports play */
#define	AUDIO_HWFEATURE_RECORD	0x00000010u	/* device supports record */

/*
 * Define the sw_features
 */
#define	AUDIO_SWFEATURE_MIXER	0x00000001u	/* audio mixer audio pers mod */

/*
 * This macro initializes an audio_info structure to 'harmless' values.
 * Note that (~0) might not be a harmless value for a flag that was
 * a signed int.
 */
#define	AUDIO_INITINFO(i)	{					\
	uint_t	*__x__;						\
	for (__x__ = (uint_t *)(i);				\
	    (char *)__x__ < (((char *)(i)) + sizeof (audio_info_t));	\
	    *__x__++ = ~0);						\
}


/*
 * Parameter for the AUDIO_GETDEV ioctl to determine current
 * audio devices.
 */
#define	MAX_AUDIO_DEV_LEN	(16)
struct audio_device {
	char name[MAX_AUDIO_DEV_LEN];
	char version[MAX_AUDIO_DEV_LEN];
	char config[MAX_AUDIO_DEV_LEN];
};
typedef struct audio_device audio_device_t;


/*
 * Ioctl calls for the audio device.
 */

/*
 * AUDIO_GETINFO retrieves the current state of the audio device.
 *
 * AUDIO_SETINFO copies all fields of the audio_info structure whose
 * values are not set to the initialized value (-1) to the device state.
 * It performs an implicit AUDIO_GETINFO to return the new state of the
 * device.  Note that the record.samples and play.samples fields are set
 * to the last value before the AUDIO_SETINFO took effect.  This allows
 * an application to reset the counters while atomically retrieving the
 * last value.
 *
 * AUDIO_DRAIN suspends the calling process until the write buffers are
 * empty.
 *
 * AUDIO_GETDEV returns a structure of type audio_device_t which contains
 * three strings.  The string "name" is a short identifying string (for
 * example, the SBus Fcode name string), the string "version" identifies
 * the current version of the device, and the "config" string identifies
 * the specific configuration of the audio stream.  All fields are
 * device-dependent -- see the device specific manual pages for details.
 */
#define	AUDIO_GETINFO	_IOR('A', 1, audio_info_t)
#define	AUDIO_SETINFO	_IOWR('A', 2, audio_info_t)
#define	AUDIO_DRAIN	_IO('A', 3)
#define	AUDIO_GETDEV	_IOR('A', 4, audio_device_t)

/*
 * The following ioctl sets the audio device into an internal loopback mode,
 * if the hardware supports this.  The argument is TRUE to set loopback,
 * FALSE to reset to normal operation.  If the hardware does not support
 * internal loopback, the ioctl should fail with EINVAL.
 */
#define	AUDIO_DIAG_LOOPBACK	_IOW('A', 101, int)


/*
 * Structure sent up as a M_PROTO message on trace streams
 */
struct audtrace_hdr {
	uint_t seq;		/* Sequence number (per-aud_stream) */
	int type;		/* device-dependent */
#if defined(_LP64) || defined(_I32LPx)
	struct timeval32 timestamp;
#else
	struct timeval timestamp;
#endif
	char _f[8];		/* filler */
};
typedef struct audtrace_hdr audtrace_hdr_t;

#ifdef __cplusplus
}
#endif

#endif /* _SYS_AUDIOIO_H */
#endif
