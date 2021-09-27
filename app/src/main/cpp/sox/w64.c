/* libSoX file format: W64   Copyright (c) 2008 robs@users.sourceforge.net
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

LSX_FORMAT_HANDLER(w64)
{
  static char const * const names[] = {"w64", NULL};
  static unsigned const write_encodings[] = {
    SOX_ENCODING_SIGN2, 16, 24, 32, 0,
    SOX_ENCODING_UNSIGNED, 8, 0,
    SOX_ENCODING_FLOAT, 32, 64, 0,
    SOX_ENCODING_ALAW, 8, 0,
    SOX_ENCODING_ULAW, 8, 0,
    SOX_ENCODING_IMA_ADPCM, 4, 0,
    SOX_ENCODING_MS_ADPCM, 4, 0,
    SOX_ENCODING_GSM, 0,
    0};
  static sox_format_handler_t handler;
  handler = *lsx_sndfile_format_fn();
  handler.description = "Sound Forge Audio Format";
  handler.names = names;
  handler.write_formats = write_encodings;
  return &handler;
}
