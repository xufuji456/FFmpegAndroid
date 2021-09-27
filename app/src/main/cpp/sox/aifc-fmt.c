/* File format: AIFF-C (see aiff.c)           (c) 2007-8 SoX contributors
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
#include "aiff.h"

LSX_FORMAT_HANDLER(aifc)
{
  static char const * const names[] = {"aifc", "aiffc", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 32, 24, 16, 8, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    0};
  static sox_format_handler_t const sox_aifc_format = {SOX_LIB_VERSION_CODE,
    "AIFF-C (not compressed), defined in DAVIC 1.4 Part 9 Annex B",
    names, SOX_FILE_BIG_END,
    lsx_aiffstartread, lsx_rawread, lsx_aiffstopread,
    lsx_aifcstartwrite, lsx_rawwrite, lsx_aifcstopwrite,
    lsx_rawseek, write_encodings, NULL, 0
  };
  return &sox_aifc_format;
}
