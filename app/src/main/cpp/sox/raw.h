/* libSoX file formats: raw         (c) 2007-8 SoX contributors
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

#define RAW_FORMAT0(id, size, flags, encoding) \
static int id ## _start(sox_format_t * ft) { \
  return lsx_rawstart(ft, sox_true, sox_true, sox_true, SOX_ENCODING_ ## encoding, size); \
} \
const sox_format_handler_t *lsx_ ## id ## _format_fn(void); \
const sox_format_handler_t *lsx_ ## id ## _format_fn(void) { \
  static unsigned const write_encodings[] = { \
    SOX_ENCODING_ ## encoding, size, 0, 0}; \
  static sox_format_handler_t handler = { \
    SOX_LIB_VERSION_CODE, "Raw audio", \
    names, flags, \
    id ## _start, lsx_rawread , NULL, \
    id ## _start, lsx_rawwrite, NULL, \
    NULL, write_encodings, NULL, 0 \
  }; \
  return &handler; \
}

#define RAW_FORMAT(id, size, flags, encoding) \
  static char const *names[] = {#id, NULL}; \
  RAW_FORMAT0(id, size, flags, encoding)

#define RAW_FORMAT1(id, alt, size, flags, encoding) \
  static char const *names[] = {#id, alt, NULL}; \
  RAW_FORMAT0(id, size, flags, encoding)

#define RAW_FORMAT2(id, alt1, alt2, size, flags, encoding) \
  static char const *names[] = {#id, alt1, alt2, NULL}; \
  RAW_FORMAT0(id, size, flags, encoding)

#define RAW_FORMAT3(id, alt1, alt2, alt3, size, flags, encoding) \
  static char const *names[] = {#id, alt1, alt2, alt3, NULL}; \
  RAW_FORMAT0(id, size, flags, encoding)

#define RAW_FORMAT4(id, alt1, alt2, alt3, alt4, size, flags, encoding) \
  static char const *names[] = {#id, alt1, alt2, alt3, alt4, NULL}; \
  RAW_FORMAT0(id, size, flags, encoding)
