/* Addressable FIFO buffer    Copyright (c) 2007 robs@users.sourceforge.net
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

#ifndef fifo_included
#define fifo_included

#include <string.h>

#ifndef FIFO_SIZE_T
#define FIFO_SIZE_T size_t
#endif

typedef struct {
  char * data;
  size_t allocation;   /* Number of bytes allocated for data. */
  size_t item_size;    /* Size of each item in data */
  size_t begin;        /* Offset of the first byte to read. */
  size_t end;          /* 1 + Offset of the last byte byte to read. */
} fifo_t;

#define FIFO_MIN 0x4000

UNUSED static void fifo_clear(fifo_t * f)
{
  f->end = f->begin = 0;
}

UNUSED static void * fifo_reserve(fifo_t * f, FIFO_SIZE_T n)
{
  n *= f->item_size;

  if (f->begin == f->end)
    fifo_clear(f);

  while (1) {
    if (f->end + n <= f->allocation) {
      void *p = f->data + f->end;

      f->end += n;
      return p;
    }
    if (f->begin > FIFO_MIN) {
      memmove(f->data, f->data + f->begin, f->end - f->begin);
      f->end -= f->begin;
      f->begin = 0;
      continue;
    }
    f->allocation += n;
    f->data = lsx_realloc(f->data, f->allocation);
  }
}

UNUSED static void * fifo_write(fifo_t * f, FIFO_SIZE_T n, void const * data)
{
  void * s = fifo_reserve(f, n);
  if (data)
    memcpy(s, data, n * f->item_size);
  return s;
}

UNUSED static void fifo_trim_to(fifo_t * f, FIFO_SIZE_T n)
{
  n *= f->item_size;
  f->end = f->begin + n;
}

UNUSED static void fifo_trim_by(fifo_t * f, FIFO_SIZE_T n)
{
  n *= f->item_size;
  f->end -= n;
}

UNUSED static FIFO_SIZE_T fifo_occupancy(fifo_t * f)
{
  return (f->end - f->begin) / f->item_size;
}

UNUSED static void * fifo_read(fifo_t * f, FIFO_SIZE_T n, void * data)
{
  char * ret = f->data + f->begin;
  n *= f->item_size;
  if (n > (FIFO_SIZE_T)(f->end - f->begin))
    return NULL;
  if (data)
    memcpy(data, ret, (size_t)n);
  f->begin += n;
  return ret;
}

#define fifo_read_ptr(f) fifo_read(f, (FIFO_SIZE_T)0, NULL)

UNUSED static void fifo_delete(fifo_t * f)
{
  free(f->data);
}

UNUSED static void fifo_create(fifo_t * f, FIFO_SIZE_T item_size)
{
  f->item_size = item_size;
  f->allocation = FIFO_MIN;
  f->data = lsx_malloc(f->allocation);
  fifo_clear(f);
}

#endif
