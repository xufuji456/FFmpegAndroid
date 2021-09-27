/* General purpose, i.e. non SoX specific, utility functions.
 * Copyright (c) 2007-8 robs@users.sourceforge.net
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
#include <ctype.h>
#include <stdio.h>

int lsx_strcasecmp(const char * s1, const char * s2)
{
#if defined(HAVE_STRCASECMP)
  return strcasecmp(s1, s2);
#elif defined(_MSC_VER)
  return _stricmp(s1, s2);
#else
  while (*s1 && (toupper(*s1) == toupper(*s2)))
    s1++, s2++;
  return toupper(*s1) - toupper(*s2);
#endif
}

int lsx_strncasecmp(char const * s1, char const * s2, size_t n)
{
#if defined(HAVE_STRCASECMP)
  return strncasecmp(s1, s2, n);
#elif defined(_MSC_VER)
  return _strnicmp(s1, s2, n);
#else
  while (--n && *s1 && (toupper(*s1) == toupper(*s2)))
    s1++, s2++;
  return toupper(*s1) - toupper(*s2);
#endif
}

sox_bool lsx_strends(char const * str, char const * end)
{
  size_t str_len = strlen(str), end_len = strlen(end);
  return str_len >= end_len && !strcmp(str + str_len - end_len, end);
}

char const * lsx_find_file_extension(char const * pathname)
{
  /* First, chop off any path portions of filename.  This
   * prevents the next search from considering that part. */
  char const * result = LAST_SLASH(pathname);
  if (!result)
    result = pathname;

  /* Now look for an filename extension */
  result = strrchr(result, '.');
  if (result)
    ++result;
  return result;
}

lsx_enum_item const * lsx_find_enum_text(char const * text, lsx_enum_item const * enum_items, int flags)
{
  lsx_enum_item const * result = NULL; /* Assume not found */
  sox_bool sensitive = !!(flags & lsx_find_enum_item_case_sensitive);

  while (enum_items->text) {
    if ((!sensitive && !strcasecmp(text, enum_items->text)) ||
        ( sensitive && !    strcmp(text, enum_items->text)))
      return enum_items;    /* Found exact match */
    if ((!sensitive && !strncasecmp(text, enum_items->text, strlen(text))) ||
        ( sensitive && !    strncmp(text, enum_items->text, strlen(text)))) {
      if (result != NULL && result->value != enum_items->value)
        return NULL;        /* Found ambiguity */
      result = enum_items;  /* Found sub-string match */
    }
    ++enum_items;
  }
  return result;
}

lsx_enum_item const * lsx_find_enum_value(unsigned value, lsx_enum_item const * enum_items)
{
  for (;enum_items->text; ++enum_items)
    if (value == enum_items->value)
      return enum_items;
  return NULL;
}

int lsx_enum_option(int c, char const * arg, lsx_enum_item const * items)
{
  lsx_enum_item const * p = lsx_find_enum_text(arg, items, sox_false);
  if (p == NULL) {
    size_t len = 1;
    char * set = lsx_malloc(len);
    *set = 0;
    for (p = items; p->text; ++p) {
      set = lsx_realloc(set, len += 2 + strlen(p->text));
      strcat(set, ", "); strcat(set, p->text);
    }
    lsx_fail("-%c: `%s' is not one of: %s.", c, arg, set + 2);
    free(set);
    return INT_MAX;
  }
  return p->value;
}

char const * lsx_sigfigs3(double number)
{
  static char const symbols[] = "\0kMGTPEZY";
  static char string[16][10];   /* FIXME: not thread-safe */
  static unsigned n;            /* ditto */
  unsigned a, b, c;
  sprintf(string[n = (n+1) & 15], "%#.3g", number);
  switch (sscanf(string[n], "%u.%ue%u", &a, &b, &c)) {
    case 2: if (b) return string[n]; /* Can fall through */
    case 1: c = 2; break;
    case 3: a = 100*a + b; break;
  }
  if (c < array_length(symbols) * 3 - 3) switch (c%3) {
    case 0: sprintf(string[n], "%u.%02u%c", a/100,a%100, symbols[c/3]); break;
    case 1: sprintf(string[n], "%u.%u%c"  , a/10 ,a%10 , symbols[c/3]); break;
    case 2: sprintf(string[n], "%u%c"     , a          , symbols[c/3]); break;
  }
  return string[n];
}

char const * lsx_sigfigs3p(double percentage)
{
  static char string[16][10];
  static unsigned n;
  sprintf(string[n = (n+1) & 15], "%.1f%%", percentage);
  if (strlen(string[n]) < 5)
    sprintf(string[n], "%.2f%%", percentage);
  else if (strlen(string[n]) > 5)
    sprintf(string[n], "%.0f%%", percentage);
  return string[n];
}

int lsx_open_dllibrary(
  int show_error_on_failure,
  const char* library_description,
  const char* const library_names[] UNUSED,
  const lsx_dlfunction_info func_infos[],
  lsx_dlptr selected_funcs[],
  lsx_dlhandle* pdl)
{
  int failed = 0;
  lsx_dlhandle dl = NULL;

  /* Track enough information to give a good error message about one failure.
   * Let failed symbol load override failed library open, and let failed
   * library open override missing static symbols.
   */
  const char* failed_libname = NULL;
  const char* failed_funcname = NULL;

#ifdef HAVE_LIBLTDL
  if (library_names && library_names[0])
  {
    const char* const* libname;
    if (lt_dlinit())
    {
      lsx_fail(
        "Unable to load %s - failed to initialize ltdl.",
        library_description);
      return 1;
    }

    for (libname = library_names; *libname; libname++)
    {
      lsx_debug("Attempting to open %s (%s).", library_description, *libname);
      dl = lt_dlopenext(*libname);
      if (dl)
      {
        size_t i;
        lsx_debug("Opened %s (%s).", library_description, *libname);
        for (i = 0; func_infos[i].name; i++)
        {
          union {lsx_dlptr fn; lt_ptr ptr;} func;
          func.ptr = lt_dlsym(dl, func_infos[i].name);
          selected_funcs[i] = func.fn ? func.fn : func_infos[i].stub_func;
          if (!selected_funcs[i])
          {
            lt_dlclose(dl);
            dl = NULL;
            failed_libname = *libname;
            failed_funcname = func_infos[i].name;
            lsx_debug("Cannot use %s (%s) - missing function \"%s\".", library_description, failed_libname, failed_funcname);
            break;
          }
        }

        if (dl)
          break;
      }
      else if (!failed_libname)
      {
        failed_libname = *libname;
      }
    }

    if (!dl)
      lt_dlexit();
  }
#endif /* HAVE_LIBLTDL */

  if (!dl)
  {
    size_t i;
    for (i = 0; func_infos[i].name; i++)
    {
      selected_funcs[i] =
          func_infos[i].static_func
          ? func_infos[i].static_func
          : func_infos[i].stub_func;
      if (!selected_funcs[i])
      {
        if (!failed_libname)
        {
          failed_libname = "static";
          failed_funcname = func_infos[i].name;
        }

        failed = 1;
        break;
      }
    }
  }

  if (failed)
  {
    size_t i;
    for (i = 0; func_infos[i].name; i++)
      selected_funcs[i] = NULL;
#ifdef HAVE_LIBLTDL
#define LTDL_MISSING ""
#else
#define LTDL_MISSING " (Dynamic library support not configured.)"
#endif /* HAVE_LIBLTDL */
    if (failed_funcname)
    {
      if (show_error_on_failure)
        lsx_fail(
          "Unable to load %s (%s) function \"%s\"." LTDL_MISSING,
          library_description,
          failed_libname,
          failed_funcname);
      else
        lsx_report(
          "Unable to load %s (%s) function \"%s\"." LTDL_MISSING,
          library_description,
          failed_libname,
          failed_funcname);
    }
    else if (failed_libname)
    {
      if (show_error_on_failure)
        lsx_fail(
          "Unable to load %s (%s)." LTDL_MISSING,
          library_description,
          failed_libname);
      else
        lsx_report(
          "Unable to load %s (%s)." LTDL_MISSING,
          library_description,
          failed_libname);
    }
    else
    {
      if (show_error_on_failure)
        lsx_fail(
          "Unable to load %s - no dynamic library names selected." LTDL_MISSING,
          library_description);
      else
        lsx_report(
          "Unable to load %s - no dynamic library names selected." LTDL_MISSING,
          library_description);
    }
  }

  *pdl = dl;
  return failed;
}

void lsx_close_dllibrary(
  lsx_dlhandle dl UNUSED)
{
#ifdef HAVE_LIBLTDL
  if (dl)
  {
    lt_dlclose(dl);
    lt_dlexit();
  }
#endif /* HAVE_LIBLTDL */
}
