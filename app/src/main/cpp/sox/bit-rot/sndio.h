#ifndef HAVE_SNDIO
/*
 * SoX bit-rot detection file, obtained from:
 * http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libsndio/sndio.h
 */
#if defined __GNUC__
  #pragma GCC system_header
#endif

/*	$OpenBSD: sndio.h,v 1.7 2009/02/03 19:44:58 ratchov Exp $	*/
/*
 * Copyright (c) 2008 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef SNDIO_H
#define SNDIO_H

#include <sys/param.h>

/*
 * private ``handle'' structure
 */
struct sio_hdl;

/*
 * parameters of a full-duplex stream
 */
struct sio_par {
	unsigned bits;		/* bits per sample */
	unsigned bps;		/* bytes per sample */
	unsigned sig;		/* 1 = signed, 0 = unsigned */
	unsigned le;		/* 1 = LE, 0 = BE byte order */
	unsigned msb;		/* 1 = MSB, 0 = LSB aligned */
	unsigned rchan;		/* number channels for recording direction */
	unsigned pchan;		/* number channels for playback direction */
	unsigned rate;		/* frames per second */
	unsigned bufsz;		/* end-to-end buffer size */
#define SIO_IGNORE	0	/* pause during xrun */
#define SIO_SYNC	1	/* resync after xrun */
#define SIO_ERROR	2	/* terminate on xrun */
	unsigned xrun;		/* what to do on overruns/underruns */
	unsigned round;		/* optimal bufsz divisor */
	unsigned appbufsz;	/* minimum buffer size */
	int __pad[3];		/* for future use */
	int __magic;		/* for internal/debug purposes only */
};

/*
 * capabilities of a stream
 */
struct sio_cap {
#define SIO_NENC	8
#define SIO_NCHAN	8
#define SIO_NRATE	16
#define SIO_NCONF	4
	struct sio_enc {			/* allowed sample encodings */
		unsigned bits;
		unsigned bps;
		unsigned sig;
		unsigned le;
		unsigned msb;
	} enc[SIO_NENC];
	unsigned rchan[SIO_NCHAN];	/* allowed values for rchan */
	unsigned pchan[SIO_NCHAN];	/* allowed values for pchan */
	unsigned rate[SIO_NRATE];	/* allowed rates */
	int __pad[7];			/* for future use */
	unsigned nconf;			/* number of elements in confs[] */
	struct sio_conf {
		unsigned enc;		/* mask of enc[] indexes */
		unsigned rchan;		/* mask of chan[] indexes (rec) */
		unsigned pchan;		/* mask of chan[] indexes (play) */
		unsigned rate;		/* mask of rate[] indexes */
	} confs[SIO_NCONF];
};

#define SIO_XSTRINGS { "ignore", "sync", "error" }

/*
 * mode bitmap
 */
#define SIO_PLAY	1
#define SIO_REC		2

/*
 * maximum size of the encording string (the longest possible
 * encoding is ``s24le3msb'')
 */
#define SIO_ENCMAX	10

/*
 * default bytes per sample for the given bits per sample
 */
#define SIO_BPS(bits) (((bits) <= 8) ? 1 : (((bits) <= 16) ? 2 : 4))

/*
 * default value of "sio_par->le" flag
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#define SIO_LE_NATIVE 1
#else
#define SIO_LE_NATIVE 0
#endif

/*
 * default device for the sun audio(4) back-end
 */
#define SIO_SUN_PATH	"/dev/audio"

/*
 * default socket name for the aucat(1) back-end
 */
#define SIO_AUCAT_PATH	"default"

/*
 * maximum value of volume, eg. for sio_setvol()
 */
#define SIO_MAXVOL 127

#ifdef __cplusplus
extern "C" {
#endif

int sio_strtoenc(struct sio_par *, char *);
int sio_enctostr(struct sio_par *, char *);
void sio_initpar(struct sio_par *);

struct sio_hdl *sio_open(char *, unsigned, int);
void sio_close(struct sio_hdl *);
int sio_setpar(struct sio_hdl *, struct sio_par *);
int sio_getpar(struct sio_hdl *, struct sio_par *);
int sio_getcap(struct sio_hdl *, struct sio_cap *);
void sio_onmove(struct sio_hdl *, void (*)(void *, int), void *);
size_t sio_write(struct sio_hdl *, void *, size_t);
size_t sio_read(struct sio_hdl *, void *, size_t);
int sio_start(struct sio_hdl *);
int sio_stop(struct sio_hdl *);
int sio_nfds(struct sio_hdl *);
int sio_pollfd(struct sio_hdl *, struct pollfd *, int);
int sio_revents(struct sio_hdl *, struct pollfd *);
int sio_eof(struct sio_hdl *);
int sio_setvol(struct sio_hdl *, unsigned);
void sio_onvol(struct sio_hdl *, void (*)(void *, unsigned), void *);

#ifdef __cplusplus
}
#endif

#endif /* !defined(SNDIO_H) */
#endif
