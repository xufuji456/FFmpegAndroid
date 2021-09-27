/* libSoX file format: DVMS (see cvsd.c)        (c) 2007-8 SoX contributors
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

#include "cvsd.h"

LSX_FORMAT_HANDLER(dvms)
{
  static char const * const names[] = {"dvms", "vms", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_CVSD, 1, 0, 0};
  static sox_format_handler_t const handler = {SOX_LIB_VERSION_CODE,
    "MIL Std 188 113 Continuously Variable Slope Delta modulation with header",
    names, SOX_FILE_MONO,
    lsx_dvmsstartread, lsx_cvsdread, lsx_cvsdstopread,
    lsx_dvmsstartwrite, lsx_cvsdwrite, lsx_dvmsstopwrite,
    NULL, write_encodings, NULL, sizeof(cvsd_priv_t)
  };
  return &handler;
}
