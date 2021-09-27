/* Implements a libSoX internal interface for implementing effects.
 * All public functions & data are prefixed with lsx_ .
 *
 * Copyright (c) 2005-2012 Chris Bagwell and SoX contributors
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

#define LSX_EFF_ALIAS
#include "sox_i.h"
#include <string.h>
#include <ctype.h>

int lsx_usage(sox_effect_t * effp)
{
  if (effp->handler.usage)
    lsx_fail("usage: %s", effp->handler.usage);
  else
    lsx_fail("this effect takes no parameters");
  return SOX_EOF;
}

char * lsx_usage_lines(char * * usage, char const * const * lines, size_t n)
{
  if (!*usage) {
    size_t i, len;
    for (len = i = 0; i < n; len += strlen(lines[i++]) + 1);
    *usage = lsx_malloc(len); /* FIXME: this memory will never be freed */
    strcpy(*usage, lines[0]);
    for (i = 1; i < n; ++i) {
      strcat(*usage, "\n");
      strcat(*usage, lines[i]);
    }
  }
  return *usage;
}

static lsx_enum_item const s_lsx_wave_enum[] = {
  LSX_ENUM_ITEM(SOX_WAVE_,SINE)
  LSX_ENUM_ITEM(SOX_WAVE_,TRIANGLE)
  {0, 0}};

lsx_enum_item const * lsx_get_wave_enum(void)
{
  return s_lsx_wave_enum;
}

void lsx_generate_wave_table(
    lsx_wave_t wave_type,
    sox_data_t data_type,
    void *table,
    size_t table_size,
    double min,
    double max,
    double phase)
{
  uint32_t t;
  uint32_t phase_offset = phase / M_PI / 2 * table_size + 0.5;

  for (t = 0; t < table_size; t++)
  {
    uint32_t point = (t + phase_offset) % table_size;
    double d;
    switch (wave_type)
    {
      case SOX_WAVE_SINE:
      d = (sin((double)point / table_size * 2 * M_PI) + 1) / 2;
      break;

      case SOX_WAVE_TRIANGLE:
      d = (double)point * 2 / table_size;
      switch (4 * point / table_size)
      {
        case 0:         d = d + 0.5; break;
        case 1: case 2: d = 1.5 - d; break;
        case 3:         d = d - 1.5; break;
      }
      break;

      default: /* Oops! FIXME */
        d = 0.0; /* Make sure we have a value */
      break;
    }
    d  = d * (max - min) + min;
    switch (data_type)
    {
      case SOX_FLOAT:
        {
          float *fp = (float *)table;
          *fp++ = (float)d;
          table = fp;
          continue;
        }
      case SOX_DOUBLE:
        {
          double *dp = (double *)table;
          *dp++ = d;
          table = dp;
          continue;
        }
      default: break;
    }
    d += d < 0? -0.5 : +0.5;
    switch (data_type)
    {
      case SOX_SHORT:
        {
          short *sp = table;
          *sp++ = (short)d;
          table = sp;
          continue;
        }
      case SOX_INT:
        {
          int *ip = table;
          *ip++ = (int)d;
          table = ip;
          continue;
        }
      default: break;
    }
  }
}

/*
 * lsx_parsesamples
 *
 * Parse a string for # of samples.  The input consists of one or more
 * parts, with '+' or '-' between them indicating if the sample count
 * should be added to or subtracted from the previous value.
 * If a part ends with a 's' then it is interpreted as a
 * user-calculated # of samples.
 * If a part contains ':' or '.' but no 'e' or if it ends with a 't'
 * then it is treated as an amount of time.  This is converted into
 * seconds and fraction of seconds, then the sample rate is used to
 * calculate # of samples.
 * Parameter def specifies which interpretation should be the default
 * for a bare number like "123".  It can either be 't' or 's'.
 * Returns NULL on error, pointer to next char to parse otherwise.
 */
static char const * parsesamples(sox_rate_t rate, const char *str0, uint64_t *samples, int def, int combine);

char const * lsx_parsesamples(sox_rate_t rate, const char *str0, uint64_t *samples, int def)
{
  *samples = 0;
  return parsesamples(rate, str0, samples, def, '+');
}

static char const * parsesamples(sox_rate_t rate, const char *str0, uint64_t *samples, int def, int combine)
{
  char * str = (char *)str0;

  do {
    uint64_t samples_part;
    sox_bool found_samples = sox_false, found_time = sox_false;
    char const * end;
    char const * pos;
    sox_bool found_colon, found_dot, found_e;

    for (;*str == ' '; ++str);
    for (end = str; *end && strchr("0123456789:.ets", *end); ++end);
    if (end == str)
      return NULL; /* error: empty input */

    pos = strchr(str, ':');
    found_colon = pos && pos < end;

    pos = strchr(str, '.');
    found_dot = pos && pos < end;

    pos = strchr(str, 'e');
    found_e = pos && pos < end;

    if (found_colon || (found_dot && !found_e) || *(end-1) == 't')
      found_time = sox_true;
    else if (*(end-1) == 's')
      found_samples = sox_true;

    if (found_time || (def == 't' && !found_samples)) {
      int i;
      if (found_e)
        return NULL; /* error: e notation in time */

      for (samples_part = 0, i = 0; *str != '.' && i < 3; ++i) {
        char * last_str = str;
        long part = strtol(str, &str, 10);
        if (!i && str == last_str)
          return NULL; /* error: empty first component */
        samples_part += rate * part;
        if (i < 2) {
          if (*str != ':')
            break;
          ++str;
          samples_part *= 60;
        }
      }
      if (*str == '.') {
        char * last_str = str;
        double part = strtod(str, &str);
        if (str == last_str)
          return NULL; /* error: empty fractional part */
        samples_part += rate * part + .5;
      }
      if (*str == 't')
        str++;
    } else {
      char * last_str = str;
      double part = strtod(str, &str);
      if (str == last_str)
        return NULL; /* error: no sample count */
      samples_part = part + .5;
      if (*str == 's')
        str++;
    }
    if (str != end)
      return NULL; /* error: trailing characters */

    switch (combine) {
      case '+': *samples += samples_part; break;
      case '-': *samples = samples_part <= *samples ?
                           *samples - samples_part : 0;
        break;
    }
    combine = '\0';
    if (*str && strchr("+-", *str))
      combine = *str++;
  } while (combine);
  return str;
}

#if 0

#include <assert.h>

#define TEST(st, samp, len) \
  str = st; \
  next = lsx_parsesamples(10000, str, &samples, 't'); \
  assert(samples == samp && next == str + len);

int main(int argc, char * * argv)
{
  char const * str, * next;
  uint64_t samples;

  TEST("0"  , 0, 1)
  TEST("1" , 10000, 1)

  TEST("0s" , 0, 2)
  TEST("0s,", 0, 2)
  TEST("0s/", 0, 2)
  TEST("0s@", 0, 2)

  TEST("0t" , 0, 2)
  TEST("0t,", 0, 2)
  TEST("0t/", 0, 2)
  TEST("0t@", 0, 2)

  TEST("1s" , 1, 2)
  TEST("1s,", 1, 2)
  TEST("1s/", 1, 2)
  TEST("1s@", 1, 2)
  TEST(" 01s" , 1, 4)
  TEST("1e6s" , 1000000, 4)

  TEST("1t" , 10000, 2)
  TEST("1t,", 10000, 2)
  TEST("1t/", 10000, 2)
  TEST("1t@", 10000, 2)
  TEST("1.1t" , 11000, 4)
  TEST("1.1t,", 11000, 4)
  TEST("1.1t/", 11000, 4)
  TEST("1.1t@", 11000, 4)
  assert(!lsx_parsesamples(10000, "1e6t", &samples, 't'));

  TEST(".0", 0, 2)
  TEST("0.0", 0, 3)
  TEST("0:0.0", 0, 5)
  TEST("0:0:0.0", 0, 7)

  TEST(".1", 1000, 2)
  TEST(".10", 1000, 3)
  TEST("0.1", 1000, 3)
  TEST("1.1", 11000, 3)
  TEST("1:1.1", 611000, 5)
  TEST("1:1:1.1", 36611000, 7)
  TEST("1:1", 610000, 3)
  TEST("1:01", 610000, 4)
  TEST("1:1:1", 36610000, 5)
  TEST("1:", 600000, 2)
  TEST("1::", 36000000, 3)

  TEST("0.444444", 4444, 8)
  TEST("0.555555", 5556, 8)

  assert(!lsx_parsesamples(10000, "x", &samples, 't'));

  TEST("1:23+37", 1200000, 7)
  TEST("12t+12s",  120012, 7)
  TEST("1e6s-10",  900000, 7)
  TEST("10-2:00",       0, 7)
  TEST("123-45+12s+2:00-3e3s@foo", 1977012, 20)

  TEST("1\0" "2", 10000, 1)

  return 0;
}
#endif

/*
 * lsx_parseposition
 *
 * Parse a string for an audio position.  Similar to lsx_parsesamples
 * above, but an initial '=', '+' or '-' indicates that the specified time
 * is relative to the start of audio, last used position or end of audio,
 * respectively.  Parameter def states which of these is the default.
 * Parameters latest and end are the positions to which '+' and '-' relate;
 * end may be SOX_UNKNOWN_LEN, in which case "-0" is the only valid
 * end-relative input and will result in a position of SOX_UNKNOWN_LEN.
 * Other parameters and return value are the same as for lsx_parsesamples.
 *
 * A test parse that only checks for valid syntax can be done by
 * specifying samples = NULL.  If this passes, a later reparse of the same
 * input will only fail if it is relative to the end ("-"), not "-0", and
 * the end position is unknown.
 */
char const * lsx_parseposition(sox_rate_t rate, const char *str0, uint64_t *samples, uint64_t latest, uint64_t end, int def)
{
  char *str = (char *)str0;
  char anchor, combine;

  if (!strchr("+-=", def))
    return NULL; /* error: invalid default anchor */
  anchor = def;
  if (*str && strchr("+-=", *str))
    anchor = *str++;

  combine = '+';
  if (strchr("+-", anchor)) {
    combine = anchor;
    if (*str && strchr("+-", *str))
      combine = *str++;
  }

  if (!samples) {
    /* dummy parse, syntax checking only */
    uint64_t dummy = 0;
    return parsesamples(0., str, &dummy, 't', '+');
  }

  switch (anchor) {
    case '=': *samples = 0; break;
    case '+': *samples = latest; break;
    case '-': *samples = end; break;
  }

  if (anchor == '-' && end == SOX_UNKNOWN_LEN) {
    /* "-0" only valid input here */
    char const *l;
    for (l = str; *l && strchr("0123456789:.ets+-", *l); ++l);
    if (l == str+1 && *str == '0') {
      /* *samples already set to SOX_UNKNOWN_LEN */
      return l;
    }
    return NULL; /* error: end-relative position, but end unknown */
  }

  return parsesamples(rate, str, samples, 't', combine);
}

/* a note is given as an int,
 * 0   => 440 Hz = A
 * >0  => number of half notes 'up',
 * <0  => number of half notes down,
 * example 12 => A of next octave, 880Hz
 *
 * calculated by freq = 440Hz * 2**(note/12)
 */
static double calc_note_freq(double note, int key)
{
  if (key != INT_MAX) {                         /* Just intonation. */
    static const int n[] = {16, 9, 6, 5, 4, 7}; /* Numerator. */
    static const int d[] = {15, 8, 5, 4, 3, 5}; /* Denominator. */
    static double j[13];                        /* Just semitones */
    int i, m = floor(note);

    if (!j[1]) for (i = 1; i <= 12; ++i)
      j[i] = i <= 6? log((double)n[i - 1] / d[i - 1]) / log(2.) : 1 - j[12 - i];
    note -= m;
    m -= key = m - ((INT_MAX / 2 - ((INT_MAX / 2) % 12) + m - key) % 12);
    return 440 * pow(2., key / 12. + j[m] + (j[m + 1] - j[m]) * note);
  }
  return 440 * pow(2., note / 12);
}

int lsx_parse_note(char const * text, char * * end_ptr)
{
  int result = INT_MAX;

  if (*text >= 'A' && *text <= 'G') {
    result = (int)(5/3. * (*text++ - 'A') + 9.5) % 12 - 9;
    if (*text == 'b') {--result; ++text;}
    else if (*text == '#') {++result; ++text;}
    if (isdigit((unsigned char)*text))
      result += 12 * (*text++ - '4'); 
  }
  *end_ptr = (char *)text;
  return result;
}

/* Read string 'text' and convert to frequency.
 * 'text' can be a positive number which is the frequency in Hz.
 * If 'text' starts with a '%' and a following number the corresponding
 * note is calculated.
 * Return -1 on error.
 */
double lsx_parse_frequency_k(char const * text, char * * end_ptr, int key)
{
  double result;

  if (*text == '%') {
    result = strtod(text + 1, end_ptr);
    if (*end_ptr == text + 1)
      return -1;
    return calc_note_freq(result, key);
  }
  if (*text >= 'A' && *text <= 'G') {
    int result2 = lsx_parse_note(text, end_ptr);
    return result2 == INT_MAX? - 1 : calc_note_freq((double)result2, key);
  }
  result = strtod(text, end_ptr);
  if (end_ptr) {
    if (*end_ptr == text)
      return -1;
    if (**end_ptr == 'k') {
      result *= 1000;
      ++*end_ptr;
    }
  }
  return result < 0 ? -1 : result;
}

FILE * lsx_open_input_file(sox_effect_t * effp, char const * filename, sox_bool text_mode)
{
  FILE * file;

  if (!filename || !strcmp(filename, "-")) {
    if (effp->global_info->global_info->stdin_in_use_by) {
      lsx_fail("stdin already in use by `%s'", effp->global_info->global_info->stdin_in_use_by);
      return NULL;
    }
    effp->global_info->global_info->stdin_in_use_by = effp->handler.name;
    file = stdin;
  }
  else if (!(file = fopen(filename, text_mode ? "r" : "rb"))) {
    lsx_fail("couldn't open file %s: %s", filename, strerror(errno));
    return NULL;
  }
  return file;
}

int lsx_effects_init(void)
{
  init_fft_cache();
  return SOX_SUCCESS;
}

int lsx_effects_quit(void)
{
  clear_fft_cache();
  return SOX_SUCCESS;
}
