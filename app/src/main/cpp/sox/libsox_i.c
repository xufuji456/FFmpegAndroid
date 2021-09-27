/* libSoX internal functions that apply to both formats and effects
 * All public functions & data are prefixed with lsx_ .
 *
 * Copyright (c) 2008 robs@users.sourceforge.net
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

#ifdef HAVE_IO_H
  #include <io.h>
#endif

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
  #define MKTEMP_X _O_BINARY|_O_TEMPORARY
#else
  #define MKTEMP_X 0
#endif

#ifndef HAVE_MKSTEMP
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #define mkstemp(t) open(mktemp(t), MKTEMP_X|O_RDWR|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE)
  #define FAKE_MKSTEMP "fake "
#else
  #define FAKE_MKSTEMP
#endif

#ifdef WIN32
static int check_dir(char * buf, size_t buflen, char const * name)
{
  struct stat st;
  if (!name || stat(name, &st) || (st.st_mode & S_IFMT) != S_IFDIR)
  {
    return 0;
  }
  else
  {
    strncpy(buf, name, buflen);
    buf[buflen - 1] = 0;
    return strlen(name) == strlen(buf);
  }
}
#endif

FILE * lsx_tmpfile(void)
{
  char const * path = sox_globals.tmp_path;

  /*
  On Win32, tmpfile() is broken - it creates the file in the root directory of
  the current drive (the user probably doesn't have permission to write there!)
  instead of in a valid temporary directory (like TEMP or TMP). So if tmp_path
  is null, figure out a reasonable default.
  To force use of tmpfile(), set sox_globals.tmp_path = "".
  */
#ifdef WIN32
  if (!path)
  {
    static char default_path[260] = "";
    if (default_path[0] == 0
        && !check_dir(default_path, sizeof(default_path), getenv("TEMP"))
        && !check_dir(default_path, sizeof(default_path), getenv("TMP"))
    #ifdef __CYGWIN__
        && !check_dir(default_path, sizeof(default_path), "/tmp")
    #endif
        )
    {
      strcpy(default_path, ".");
    }

    path = default_path;
  }
#endif

  if (path && path[0]) {
    /* Emulate tmpfile (delete on close); tmp dir is given tmp_path: */
    char const * const end = "/libSoX.tmp.XXXXXX";
    char * name = lsx_malloc(strlen(path) + strlen(end) + 1);
    int fildes;
    strcpy(name, path);
    strcat(name, end);
    fildes = mkstemp(name);
#ifdef HAVE_UNISTD_H
    lsx_debug(FAKE_MKSTEMP "mkstemp, name=%s (unlinked)", name);
    unlink(name);
#else
    lsx_debug(FAKE_MKSTEMP "mkstemp, name=%s (O_TEMPORARY)", name);
#endif
    free(name);
    return fildes == -1? NULL : fdopen(fildes, "w+b");
  }

  /* Use standard tmpfile (delete on close); tmp dir is undefined: */
  lsx_debug("tmpfile()");
  return tmpfile();
}
