/* libSoX file format: raw Dialogic/OKI ADPCM        (c) 2007-8 SoX contributors
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
#include "adpcms.h"
#include "vox.h"

LSX_FORMAT_HANDLER(vox)
{
  static char const * const names[] = {"vox", NULL};
  static unsigned const write_encodings[] = {SOX_ENCODING_OKI_ADPCM, 4, 0, 0};
  static sox_format_handler_t handler = {SOX_LIB_VERSION_CODE,
    "Raw OKI/Dialogic ADPCM", names, SOX_FILE_MONO,
    lsx_vox_start, lsx_vox_read, lsx_vox_stopread,
    lsx_vox_start, lsx_vox_write, lsx_vox_stopwrite,
    lsx_rawseek, write_encodings, NULL, sizeof(adpcm_io_t)
  };
  return &handler;
}
