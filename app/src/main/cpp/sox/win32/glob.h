/* libSoX minimal glob for MS-Windows: (c) 2009 SoX contributors
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

#ifndef GLOB_H
#define GLOB_H 1

#define GLOB_NOCHECK (16)
#define GLOB_FLAGS (GLOB_NOCHECK)

typedef struct glob_t
{
    unsigned gl_pathc;
    char **gl_pathv;
} glob_t;

#ifdef __cplusplus
extern "C" {
#endif

int
glob(
    const char *pattern,
    int flags,
    void *unused,
    glob_t *pglob);

void
globfree(
    glob_t* pglob);

#ifdef __cplusplus
}
#endif

#endif /* ifndef GLOB_H */
