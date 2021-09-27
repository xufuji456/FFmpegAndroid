/* SoX Memory allocation functions
 *
 * Copyright (c) 2005-2006 Reuben Thomas.  All rights reserved.
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

static void *lsx_checkptr(void *ptr)
{
  if (!ptr) {
    lsx_fail("out of memory");
    exit(2);
  }

  return ptr;
}

/* Resize an allocated memory area; abort if not possible.
 *
 * For malloc, `If the size of the space requested is zero, the behavior is
 * implementation defined: either a null pointer is returned, or the
 * behavior is as if the size were some nonzero value, except that the
 * returned pointer shall not be used to access an object'
 */
void *lsx_realloc(void *ptr, size_t newsize)
{
  if (ptr && newsize == 0) {
    free(ptr);
    return NULL;
  }

  return lsx_checkptr(realloc(ptr, newsize));
}

void *lsx_malloc(size_t size)
{
  return lsx_checkptr(malloc(size + !size));
}

void *lsx_calloc(size_t n, size_t size)
{
  return lsx_checkptr(calloc(n + !n, size + !size));
}

void *lsx_realloc_array(void *p, size_t n, size_t size)
{
  if (n > (size_t)-1 / size) {
    lsx_fail("malloc size overflow");
    exit(2);
  }

  return lsx_realloc(p, n * size);
}

char *lsx_strdup(const char *s)
{
  return lsx_checkptr(strdup(s));
}
