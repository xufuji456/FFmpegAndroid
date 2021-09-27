/* libao player support for sox
 * (c) Reuben Thomas <rrt@sc3d.org> 2007
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ao/ao.h>

typedef struct {
  int driver_id;
  ao_device *device;
  ao_sample_format format;
  char *buf;
  size_t buf_size;
} priv_t;

static int startwrite(sox_format_t * ft)
{
  priv_t * ao = (priv_t *)ft->priv;

  ao->buf_size = sox_globals.bufsiz - (sox_globals.bufsiz % (ft->encoding.bits_per_sample >> 3));
  ao->buf_size *= (ft->encoding.bits_per_sample >> 3);
  ao->buf = lsx_malloc(ao->buf_size);

  if (!ao->buf)
  {
      lsx_fail_errno(ft, SOX_ENOMEM, "Can not allocate memory for ao driver");
      return SOX_EOF;
  }


  ao_initialize();
  if (strcmp(ft->filename,"default") == 0)
  {
      if ((ao->driver_id = ao_default_driver_id()) < 0) {
          lsx_fail("Could not find a default ao driver");
          return SOX_EOF;
      }
  }
  else
  {
      if ((ao->driver_id = ao_driver_id(ft->filename)) < 0) {
          lsx_fail("Could not find a ao driver %s", ft->filename);
          return SOX_EOF;
      }
  }

  ao->format.bits = ft->encoding.bits_per_sample;
  ao->format.rate = ft->signal.rate;
  ao->format.channels = ft->signal.channels;
  ao->format.byte_format = AO_FMT_NATIVE;
  if ((ao->device = ao_open_live(ao->driver_id, &ao->format, NULL)) == NULL) {
    lsx_fail("Could not open device: error %d", errno);
    return SOX_EOF;
  }

  return SOX_SUCCESS;
}

static void sox_sw_write_buf(char *buf1, sox_sample_t const * buf2, size_t len, sox_bool swap, sox_uint64_t * clips)
{
    while (len--)
    {
        SOX_SAMPLE_LOCALS;
        uint16_t datum = SOX_SAMPLE_TO_SIGNED_16BIT(*buf2++, *clips);
        if (swap)
            datum = lsx_swapw(datum);
        *(uint16_t *)buf1 = datum;
        buf1++; buf1++;
    }
}

static size_t write_samples(sox_format_t *ft, const sox_sample_t *buf, size_t len)
{
  priv_t * ao = (priv_t *)ft->priv;
  uint_32 aobuf_size;

  if (len > ao->buf_size / (ft->encoding.bits_per_sample >> 3))
      len = ao->buf_size / (ft->encoding.bits_per_sample >> 3);

  aobuf_size = (ft->encoding.bits_per_sample >> 3) * len;

  sox_sw_write_buf(ao->buf, buf, len, ft->encoding.reverse_bytes,
                   &(ft->clips));
  if (ao_play(ao->device, (void *)ao->buf, aobuf_size) == 0)
    return 0;

  return len;
}

static int stopwrite(sox_format_t * ft)
{
  priv_t * ao = (priv_t *)ft->priv;

  free(ao->buf);

  if (ao_close(ao->device) == 0) {
    lsx_fail("Error closing libao output");
    return SOX_EOF;
  }
  ao_shutdown();

  return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(ao)
{
  static char const * const names[] = {"ao", NULL};
  static unsigned const encodings[] = {SOX_ENCODING_SIGN2, 16, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "Xiph's libao device driver", names, SOX_FILE_DEVICE | SOX_FILE_NOSTDIO,
    NULL, NULL, NULL,
    startwrite, write_samples, stopwrite,
    NULL, encodings, NULL, sizeof(priv_t)
  };
  return &handler;
}
