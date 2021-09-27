/* libSoX SampleVision file format handler.
 * Output is always in little-endian (80x86/VAX) order.
 *
 * Derived from: libSoX skeleton handler file.
 *
 * Add: Loop point verbose info.  It's a start, anyway.
 */

/*
 * June 30, 1992
 * Copyright 1992 Leigh Smith And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Leigh Smith And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

#include "sox_i.h"
#include <string.h>
#include <errno.h>

#define NAMELEN    30           /* Size of Samplevision name */
#define COMMENTLEN 60           /* Size of Samplevision comment, not shared */
#define MIDI_UNITY 60           /* MIDI note number to play sample at unity */
#define MARKERLEN  (size_t)10           /* Size of Marker name */

/* The header preceeding the sample data */
struct smpheader {
        char Id[18];            /* File identifier */
        char version[4];        /* File version */
        char comments[COMMENTLEN];      /* User comments */
        char name[NAMELEN + 1]; /* Sample Name, left justified */
};
#define HEADERSIZE (sizeof(struct smpheader) - 1)       /* -1 for name's \0 */

/* Samplevision loop definition structure */
struct loop {
        uint32_t start; /* Sample count into sample data, not byte count */
        uint32_t end;   /* end point */
        unsigned char type;  /* 0 = loop off, 1 = forward, 2 = forw/back */
        unsigned short count;  /* No of times to loop */
};

/* Samplevision marker definition structure */
struct marker {
        char name[MARKERLEN + 1]; /* Ascii Marker name */
        uint32_t position;        /* Sample Number, not byte number */
};

/* The trailer following the sample data */
struct smptrailer {
        struct loop loops[8];           /* loops */
        struct marker markers[8];       /* markers */
        int8_t MIDInote;                /* for unity pitch playback */
        uint32_t rate;                  /* in hertz */
        uint32_t SMPTEoffset;           /* in subframes - huh? */
        uint32_t CycleSize;             /* sample count in one cycle of the */
                                        /* sampled sound -1 if unknown */
};

/* Private data for SMP file */
typedef struct {
  uint64_t NoOfSamps;           /* Sample data count in words */
  uint64_t dataStart;
  /* comment memory resides in private data because it's small */
  char comment[COMMENTLEN + NAMELEN + 3];
} priv_t;

static char const *SVmagic = "SOUND SAMPLE DATA ", *SVvers = "2.1 ";

/*
 * Read the SampleVision trailer structure.
 * Returns 1 if everything was read ok, 0 if there was an error.
 */
static int readtrailer(sox_format_t * ft, struct smptrailer *trailer)
{
        int i;
        uint16_t trash16;

        lsx_readw(ft, &trash16); /* read reserved word */
        for(i = 0; i < 8; i++) {        /* read the 8 loops */
                lsx_readdw(ft, &(trailer->loops[i].start));
                ft->oob.loops[i].start = trailer->loops[i].start;
                lsx_readdw(ft, &(trailer->loops[i].end));
                ft->oob.loops[i].length =
                        trailer->loops[i].end - trailer->loops[i].start;
                lsx_readb(ft, &(trailer->loops[i].type));
                ft->oob.loops[i].type = trailer->loops[i].type;
                lsx_readw(ft, &(trailer->loops[i].count));
                ft->oob.loops[i].count = trailer->loops[i].count;
        }
        for(i = 0; i < 8; i++) {        /* read the 8 markers */
                if (lsx_readbuf(ft, trailer->markers[i].name, MARKERLEN) != MARKERLEN)
                {
                    lsx_fail_errno(ft,SOX_EHDR,"EOF in SMP");
                    return(SOX_EOF);
                }
                trailer->markers[i].name[MARKERLEN] = 0;
                lsx_readdw(ft, &(trailer->markers[i].position));
        }
        lsx_readsb(ft, &(trailer->MIDInote));
        lsx_readdw(ft, &(trailer->rate));
        lsx_readdw(ft, &(trailer->SMPTEoffset));
        lsx_readdw(ft, &(trailer->CycleSize));
        return(SOX_SUCCESS);
}

/*
 * set the trailer data - loops and markers, to reasonably benign values
 */
static void settrailer(sox_format_t * ft, struct smptrailer *trailer, sox_rate_t rate)
{
        int i;

        for(i = 0; i < 8; i++) {        /* copy the 8 loops */
            if (ft->oob.loops[i].type != 0) {
                trailer->loops[i].start = ft->oob.loops[i].start > UINT_MAX
                    ? UINT_MAX
                    : ft->oob.loops[i].start;
                /* to mark it as not set */
                trailer->loops[i].end = ft->oob.loops[i].start + ft->oob.loops[i].length > UINT_MAX
                    ? UINT_MAX
                    : ft->oob.loops[i].start + ft->oob.loops[i].length;
                trailer->loops[i].type = ft->oob.loops[i].type;
                trailer->loops[i].count = ft->oob.loops[i].count;
            } else {
                /* set first loop start as FFFFFFFF */
                trailer->loops[i].start = ~0u;
                /* to mark it as not set */
                trailer->loops[i].end = 0;
                trailer->loops[i].type = 0;
                trailer->loops[i].count = 0;
            }
        }
        for(i = 0; i < 8; i++) {        /* write the 8 markers */
                strcpy(trailer->markers[i].name, "          ");
                trailer->markers[i].position = ~0u;
        }
        trailer->MIDInote = MIDI_UNITY;         /* Unity play back */
        trailer->rate = rate;
        trailer->SMPTEoffset = 0;
        trailer->CycleSize = ~0u;
}

/*
 * Write the SampleVision trailer structure.
 * Returns 1 if everything was written ok, 0 if there was an error.
 */
static int writetrailer(sox_format_t * ft, struct smptrailer *trailer)
{
        int i;

        lsx_writew(ft, 0);                       /* write the reserved word */
        for(i = 0; i < 8; i++) {        /* write the 8 loops */
                lsx_writedw(ft, trailer->loops[i].start);
                lsx_writedw(ft, trailer->loops[i].end);
                lsx_writeb(ft, trailer->loops[i].type);
                lsx_writew(ft, trailer->loops[i].count);
        }
        for(i = 0; i < 8; i++) {        /* write the 8 markers */
                if (lsx_writes(ft, trailer->markers[i].name) == SOX_EOF)
                {
                    lsx_fail_errno(ft,SOX_EHDR,"EOF in SMP");
                    return(SOX_EOF);
                }
                lsx_writedw(ft, trailer->markers[i].position);
        }
        lsx_writeb(ft, (uint8_t)(trailer->MIDInote));
        lsx_writedw(ft, trailer->rate);
        lsx_writedw(ft, trailer->SMPTEoffset);
        lsx_writedw(ft, trailer->CycleSize);
        return(SOX_SUCCESS);
}

static int sox_smpseek(sox_format_t * ft, uint64_t offset)
{
    uint64_t new_offset;
    size_t channel_block, alignment;
    priv_t * smp = (priv_t *) ft->priv;

    new_offset = offset * (ft->encoding.bits_per_sample >> 3);
    /* Make sure request aligns to a channel block (ie left+right) */
    channel_block = ft->signal.channels * (ft->encoding.bits_per_sample >> 3);
    alignment = new_offset % channel_block;
    /* Most common mistaken is to compute something like
     * "skip everthing upto and including this sample" so
     * advance to next sample block in this case.
     */
    if (alignment != 0)
        new_offset += (channel_block - alignment);
    new_offset += smp->dataStart;

    ft->sox_errno = lsx_seeki(ft, (off_t)new_offset, SEEK_SET);

    if( ft->sox_errno == SOX_SUCCESS )
        smp->NoOfSamps = ft->signal.length - (new_offset / (ft->encoding.bits_per_sample >> 3));

    return(ft->sox_errno);
}
/*
 * Do anything required before you start reading samples.
 * Read file header.
 *      Find out sampling rate,
 *      size and encoding of samples,
 *      mono/stereo/quad.
 */
static int sox_smpstartread(sox_format_t * ft)
{
        priv_t * smp = (priv_t *) ft->priv;
        int namelen, commentlen;
        off_t samplestart;
        size_t i;
        unsigned dw;
        struct smpheader header;
        struct smptrailer trailer;

        /* If you need to seek around the input file. */
        if (! ft->seekable)
        {
                lsx_fail_errno(ft,SOX_EOF,"SMP input file must be a file, not a pipe");
                return(SOX_EOF);
        }

        /* Read SampleVision header */
        if (lsx_readbuf(ft, &header, HEADERSIZE) != HEADERSIZE)
        {
                lsx_fail_errno(ft,SOX_EHDR,"unexpected EOF in SMP header");
                return(SOX_EOF);
        }
        if (strncmp(header.Id, SVmagic, (size_t)17) != 0)
        {
                lsx_fail_errno(ft,SOX_EHDR,"SMP header does not begin with magic word %s", SVmagic);
                return(SOX_EOF);
        }
        if (strncmp(header.version, SVvers, (size_t)4) != 0)
        {
                lsx_fail_errno(ft,SOX_EHDR,"SMP header is not version %s", SVvers);
                return(SOX_EOF);
        }

        /* Format the sample name and comments to a single comment */
        /* string. We decrement the counters till we encounter non */
        /* padding space chars, so the *lengths* are low by one */
        for (namelen = NAMELEN-1;
            namelen >= 0 && header.name[namelen] == ' '; namelen--)
          ;
        for (commentlen = COMMENTLEN-1;
            commentlen >= 0 && header.comments[commentlen] == ' '; commentlen--)
          ;
        sprintf(smp->comment, "%.*s: %.*s", namelen+1, header.name,
                commentlen+1, header.comments);
        sox_append_comments(&ft->oob.comments, smp->comment);

        /* Extract out the sample size (always intel format) */
        lsx_readdw(ft, &dw);
        smp->NoOfSamps = dw;
        /* mark the start of the sample data */
        samplestart = lsx_tell(ft);

        /* seek from the current position (the start of sample data) by */
        /* NoOfSamps * sizeof(int16_t) */
        if (lsx_seeki(ft, (off_t)(smp->NoOfSamps * 2), 1) == -1)
        {
                lsx_fail_errno(ft,errno,"SMP unable to seek to trailer");
                return(SOX_EOF);
        }
        if (readtrailer(ft, &trailer))
        {
                lsx_fail_errno(ft,SOX_EHDR,"unexpected EOF in SMP trailer");
                return(SOX_EOF);
        }

        /* seek back to the beginning of the data */
        if (lsx_seeki(ft, (off_t)samplestart, 0) == -1)
        {
                lsx_fail_errno(ft,errno,"SMP unable to seek back to start of sample data");
                return(SOX_EOF);
        }

        ft->signal.rate = (int) trailer.rate;
        ft->encoding.bits_per_sample = 16;
        ft->encoding.encoding = SOX_ENCODING_SIGN2;
        ft->signal.channels = 1;
        smp->dataStart = samplestart;
        ft->signal.length = smp->NoOfSamps;

        lsx_report("SampleVision trailer:");
        for(i = 0; i < 8; i++) if (1 || trailer.loops[i].count) {
                lsx_report("Loop %lu: start: %6d", (unsigned long)i, trailer.loops[i].start);
                lsx_report(" end:   %6d", trailer.loops[i].end);
                lsx_report(" count: %6d", trailer.loops[i].count);
                switch(trailer.loops[i].type) {
                    case 0: lsx_report("type:  off"); break;
                    case 1: lsx_report("type:  forward"); break;
                    case 2: lsx_report("type:  forward/backward"); break;
                }
        }
        lsx_report("MIDI Note number: %d", trailer.MIDInote);

        ft->oob.instr.nloops = 0;
        for(i = 0; i < 8; i++)
                if (trailer.loops[i].type)
                        ft->oob.instr.nloops++;
        for(i = 0; i < ft->oob.instr.nloops; i++) {
                ft->oob.loops[i].type = trailer.loops[i].type;
                ft->oob.loops[i].count = trailer.loops[i].count;
                ft->oob.loops[i].start = trailer.loops[i].start;
                ft->oob.loops[i].length = trailer.loops[i].end
                        - trailer.loops[i].start;
        }
        ft->oob.instr.MIDIlow = ft->oob.instr.MIDIhi =
                ft->oob.instr.MIDInote = trailer.MIDInote;
        if (ft->oob.instr.nloops > 0)
                ft->oob.instr.loopmode = SOX_LOOP_8;
        else
                ft->oob.instr.loopmode = SOX_LOOP_NONE;

        return(SOX_SUCCESS);
}

/*
 * Read up to len samples from file.
 * Convert to signed longs.
 * Place in buf[].
 * Return number of samples read.
 */
static size_t sox_smpread(sox_format_t * ft, sox_sample_t *buf, size_t len)
{
        priv_t * smp = (priv_t *) ft->priv;
        unsigned short datum;
        size_t done = 0;

        for(; done < len && smp->NoOfSamps; done++, smp->NoOfSamps--) {
                lsx_readw(ft, &datum);
                /* scale signed up to long's range */
                *buf++ = SOX_SIGNED_16BIT_TO_SAMPLE(datum,);
        }
        return done;
}

static int sox_smpstartwrite(sox_format_t * ft)
{
        priv_t * smp = (priv_t *) ft->priv;
        struct smpheader header;
        char * comment = lsx_cat_comments(ft->oob.comments);

        /* If you have to seek around the output file */
        if (! ft->seekable)
        {
                lsx_fail_errno(ft,SOX_EOF,"Output .smp file must be a file, not a pipe");
                return(SOX_EOF);
        }

        memcpy(header.Id, SVmagic, sizeof(header.Id));
        memcpy(header.version, SVvers, sizeof(header.version));
        sprintf(header.comments, "%-*s", COMMENTLEN - 1, "Converted using Sox.");
        sprintf(header.name, "%-*.*s", NAMELEN, NAMELEN, comment);
        free(comment);

        /* Write file header */
        if(lsx_writebuf(ft, &header, HEADERSIZE) != HEADERSIZE)
        {
            lsx_fail_errno(ft,errno,"SMP: Can't write header completely");
            return(SOX_EOF);
        }
        lsx_writedw(ft, 0);      /* write as zero length for now, update later */
        smp->NoOfSamps = 0;

        return(SOX_SUCCESS);
}

static size_t sox_smpwrite(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
        priv_t * smp = (priv_t *) ft->priv;
        int datum;
        size_t done = 0;

        while(done < len) {
                SOX_SAMPLE_LOCALS;
                datum = (int) SOX_SAMPLE_TO_SIGNED_16BIT(*buf++, ft->clips);
                lsx_writew(ft, (uint16_t)datum);
                smp->NoOfSamps++;
                done++;
        }

        return(done);
}

static int sox_smpstopwrite(sox_format_t * ft)
{
        priv_t * smp = (priv_t *) ft->priv;
        struct smptrailer trailer;

        /* Assign the trailer data */
        settrailer(ft, &trailer, ft->signal.rate);
        writetrailer(ft, &trailer);
        if (lsx_seeki(ft, (off_t)112, 0) == -1)
        {
                lsx_fail_errno(ft,errno,"SMP unable to seek back to save size");
                return(SOX_EOF);
        }
        lsx_writedw(ft, smp->NoOfSamps > UINT_MAX ? UINT_MAX : (unsigned)smp->NoOfSamps);

        return(SOX_SUCCESS);
}

LSX_FORMAT_HANDLER(smp)
{
  static char const * const names[] = {"smp", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_SIGN2, 16, 0, 0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Turtle Beach SampleVision", names, SOX_FILE_LIT_END | SOX_FILE_MONO,
    sox_smpstartread, sox_smpread, NULL,
    sox_smpstartwrite, sox_smpwrite, sox_smpstopwrite,
    sox_smpseek, write_encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
