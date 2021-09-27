/* libSoX Internal header
 *
 *   This file is meant for libSoX internal use only
 *
 * Copyright 2001-2012 Chris Bagwell and SoX Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And SoX Contributors are not responsible for
 * the consequences of using this software.
 */

#ifndef SOX_I_H
#define SOX_I_H

#include "soxomp.h"  /* Note: soxomp.h includes soxconfig.h */
#include "sox.h"

#if defined HAVE_FMEMOPEN
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

#if defined(LSX_EFF_ALIAS)
#undef lsx_debug
#undef lsx_fail
#undef lsx_report
#undef lsx_warn
#define lsx_debug sox_globals.subsystem=effp->handler.name,lsx_debug_impl
#define lsx_fail sox_globals.subsystem=effp->handler.name,lsx_fail_impl
#define lsx_report sox_globals.subsystem=effp->handler.name,lsx_report_impl
#define lsx_warn sox_globals.subsystem=effp->handler.name,lsx_warn_impl
#endif

#define RANQD1 ranqd1(sox_globals.ranqd1)
#define DRANQD1 dranqd1(sox_globals.ranqd1)

typedef enum {SOX_SHORT, SOX_INT, SOX_FLOAT, SOX_DOUBLE} sox_data_t;
typedef enum {SOX_WAVE_SINE, SOX_WAVE_TRIANGLE} lsx_wave_t;
lsx_enum_item const * lsx_get_wave_enum(void);

/* Define fseeko and ftello for platforms lacking them */
#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif

#ifdef _FILE_OFFSET_BITS
assert_static(sizeof(off_t) == _FILE_OFFSET_BITS >> 3, OFF_T_BUILD_PROBLEM);
#endif

FILE * lsx_tmpfile(void);

void lsx_debug_more_impl(char const * fmt, ...) LSX_PRINTF12;
void lsx_debug_most_impl(char const * fmt, ...) LSX_PRINTF12;

#define lsx_debug_more sox_get_globals()->subsystem=__FILE__,lsx_debug_more_impl
#define lsx_debug_most sox_get_globals()->subsystem=__FILE__,lsx_debug_most_impl

/* Digitise one cycle of a wave and store it as
 * a table of samples of a specified data-type.
 */
void lsx_generate_wave_table(
    lsx_wave_t wave_type,
    sox_data_t data_type,
    void * table,       /* Really of type indicated by data_type. */
    size_t table_size,  /* Number of points on the x-axis. */
    double min,         /* Minimum value on the y-axis. (e.g. -1) */
    double max,         /* Maximum value on the y-axis. (e.g. +1) */
    double phase);      /* Phase at 1st point; 0..2pi. (e.g. pi/2 for cosine) */
char const * lsx_parsesamples(sox_rate_t rate, const char *str, uint64_t *samples, int def);
char const * lsx_parseposition(sox_rate_t rate, const char *str, uint64_t *samples, uint64_t latest, uint64_t end, int def);
int lsx_parse_note(char const * text, char * * end_ptr);
double lsx_parse_frequency_k(char const * text, char * * end_ptr, int key);
#define lsx_parse_frequency(a, b) lsx_parse_frequency_k(a, b, INT_MAX)
FILE * lsx_open_input_file(sox_effect_t * effp, char const * filename, sox_bool text_mode);

void lsx_prepare_spline3(double const * x, double const * y, int n,
    double start_1d, double end_1d, double * y_2d);
double lsx_spline3(double const * x, double const * y, double const * y_2d,
    int n, double x1);

double lsx_bessel_I_0(double x);
int lsx_set_dft_length(int num_taps);
void init_fft_cache(void);
void clear_fft_cache(void);
#define lsx_is_power_of_2(x) !(x < 2 || (x & (x - 1)))
void lsx_safe_rdft(int len, int type, double * d);
void lsx_safe_cdft(int len, int type, double * d);
void lsx_power_spectrum(int n, double const * in, double * out);
void lsx_power_spectrum_f(int n, float const * in, float * out);
void lsx_apply_hann_f(float h[], const int num_points);
void lsx_apply_hann(double h[], const int num_points);
void lsx_apply_hamming(double h[], const int num_points);
void lsx_apply_bartlett(double h[], const int num_points);
void lsx_apply_blackman(double h[], const int num_points, double alpha);
void lsx_apply_blackman_nutall(double h[], const int num_points);
double lsx_kaiser_beta(double att, double tr_bw);
void lsx_apply_kaiser(double h[], const int num_points, double beta);
void lsx_apply_dolph(double h[], const int num_points, double att);
double * lsx_make_lpf(int num_taps, double Fc, double beta, double rho,
    double scale, sox_bool dc_norm);
void lsx_kaiser_params(double att, double Fc, double tr_bw, double * beta, int * num_taps);
double * lsx_design_lpf(
    double Fp,      /* End of pass-band */
    double Fs,      /* Start of stop-band */
    double Fn,      /* Nyquist freq; e.g. 0.5, 1, PI; < 0: dummy run */
    double att,     /* Stop-band attenuation in dB */
    int * num_taps, /* 0: value will be estimated */
    int k,          /* >0: number of phases; <0: num_taps ≡ 1 (mod -k) */
    double beta);   /* <0: value will be estimated */
void lsx_fir_to_phase(double * * h, int * len,
    int * post_len, double phase0);
void lsx_plot_fir(double * h, int num_points, sox_rate_t rate, sox_plot_t type, char const * title, double y1, double y2);
void lsx_save_samples(sox_sample_t * const dest, double const * const src,
    size_t const n, sox_uint64_t * const clips);
void lsx_load_samples(double * const dest, sox_sample_t const * const src,
    size_t const n);

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#define lsx_swapw(x) bswap_16(x)
#define lsx_swapdw(x) bswap_32(x)
#elif defined(_MSC_VER)
#define lsx_swapw(x) _byteswap_ushort(x)
#define lsx_swapdw(x) _byteswap_ulong(x)
#else
#define lsx_swapw(uw) (((uw >> 8) | (uw << 8)) & 0xffff)
#define lsx_swapdw(udw) ((udw >> 24) | ((udw >> 8) & 0xff00) | ((udw << 8) & 0xff0000) | (udw << 24))
#endif



/*------------------------ Implemented in libsoxio.c -------------------------*/

/* Read and write basic data types from "ft" stream. */
size_t lsx_readbuf(sox_format_t * ft, void *buf, size_t len);
int lsx_skipbytes(sox_format_t * ft, size_t n);
int lsx_padbytes(sox_format_t * ft, size_t n);
size_t lsx_writebuf(sox_format_t * ft, void const *buf, size_t len);
int lsx_reads(sox_format_t * ft, char *c, size_t len);
int lsx_writes(sox_format_t * ft, char const * c);
void lsx_set_signal_defaults(sox_format_t * ft);
#define lsx_writechars(ft, chars, len) (lsx_writebuf(ft, chars, len) == len? SOX_SUCCESS : SOX_EOF)

size_t lsx_read_3_buf(sox_format_t * ft, sox_uint24_t *buf, size_t len);
size_t lsx_read_b_buf(sox_format_t * ft, uint8_t *buf, size_t len);
size_t lsx_read_df_buf(sox_format_t * ft, double *buf, size_t len);
size_t lsx_read_dw_buf(sox_format_t * ft, uint32_t *buf, size_t len);
size_t lsx_read_qw_buf(sox_format_t * ft, uint64_t *buf, size_t len);
size_t lsx_read_f_buf(sox_format_t * ft, float *buf, size_t len);
size_t lsx_read_w_buf(sox_format_t * ft, uint16_t *buf, size_t len);

size_t lsx_write_3_buf(sox_format_t * ft, sox_uint24_t *buf, size_t len);
size_t lsx_write_b_buf(sox_format_t * ft, uint8_t *buf, size_t len);
size_t lsx_write_df_buf(sox_format_t * ft, double *buf, size_t len);
size_t lsx_write_dw_buf(sox_format_t * ft, uint32_t *buf, size_t len);
size_t lsx_write_qw_buf(sox_format_t * ft, uint64_t *buf, size_t len);
size_t lsx_write_f_buf(sox_format_t * ft, float *buf, size_t len);
size_t lsx_write_w_buf(sox_format_t * ft, uint16_t *buf, size_t len);

int lsx_read3(sox_format_t * ft, sox_uint24_t * u3);
int lsx_readb(sox_format_t * ft, uint8_t * ub);
int lsx_readchars(sox_format_t * ft, char * chars, size_t len);
int lsx_readdf(sox_format_t * ft, double * d);
int lsx_readdw(sox_format_t * ft, uint32_t * udw);
int lsx_readqw(sox_format_t * ft, uint64_t * udw);
int lsx_readf(sox_format_t * ft, float * f);
int lsx_readw(sox_format_t * ft, uint16_t * uw);

#if 1 /* FIXME: use defines */
UNUSED static int lsx_readsb(sox_format_t * ft, int8_t * sb)
{return lsx_readb(ft, (uint8_t *)sb);}
UNUSED static int lsx_readsw(sox_format_t * ft, int16_t * sw)
{return lsx_readw(ft, (uint16_t *)sw);}
#else
#define lsx_readsb(ft, sb) lsx_readb(ft, (uint8_t *)sb)
#define lsx_readsw(ft, sw) lsx_readb(ft, (uint16_t *)sw)
#endif

int lsx_read_fields(sox_format_t *ft, uint32_t *len,  const char *spec, ...);

int lsx_write3(sox_format_t * ft, unsigned u3);
int lsx_writeb(sox_format_t * ft, unsigned ub);
int lsx_writedf(sox_format_t * ft, double d);
int lsx_writedw(sox_format_t * ft, unsigned udw);
int lsx_writeqw(sox_format_t * ft, uint64_t uqw);
int lsx_writef(sox_format_t * ft, double f);
int lsx_writew(sox_format_t * ft, unsigned uw);

int lsx_writesb(sox_format_t * ft, signed);
int lsx_writesw(sox_format_t * ft, signed);

int lsx_eof(sox_format_t * ft);
int lsx_error(sox_format_t * ft);
int lsx_flush(sox_format_t * ft);
int lsx_seeki(sox_format_t * ft, off_t offset, int whence);
int lsx_unreadb(sox_format_t * ft, unsigned ub);
/* uint64_t lsx_filelength(sox_format_t * ft); Temporarily Moved to sox.h. */
off_t lsx_tell(sox_format_t * ft);
void lsx_clearerr(sox_format_t * ft);
void lsx_rewind(sox_format_t * ft);

int lsx_offset_seek(sox_format_t * ft, off_t byte_offset, off_t to_sample);

void lsx_fail_errno(sox_format_t *, int, const char *, ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 3, 4)));
#else
;
#endif

typedef struct sox_formats_globals { /* Global parameters (for formats) */
  sox_globals_t * global_info;
} sox_formats_globals;



/*------------------------------ File Handlers -------------------------------*/

int lsx_check_read_params(sox_format_t * ft, unsigned channels,
    sox_rate_t rate, sox_encoding_t encoding, unsigned bits_per_sample,
    uint64_t num_samples, sox_bool check_length);
#define LSX_FORMAT_HANDLER(name) \
sox_format_handler_t const * lsx_##name##_format_fn(void); \
sox_format_handler_t const * lsx_##name##_format_fn(void)
#define div_bits(size, bits) ((uint64_t)(size) * 8 / bits)

/* Raw I/O */
int lsx_rawstartread(sox_format_t * ft);
size_t lsx_rawread(sox_format_t * ft, sox_sample_t *buf, size_t nsamp);
int lsx_rawstopread(sox_format_t * ft);
int lsx_rawstartwrite(sox_format_t * ft);
size_t lsx_rawwrite(sox_format_t * ft, const sox_sample_t *buf, size_t nsamp);
int lsx_rawseek(sox_format_t * ft, uint64_t offset);
int lsx_rawstart(sox_format_t * ft, sox_bool default_rate, sox_bool default_channels, sox_bool default_length, sox_encoding_t encoding, unsigned bits_per_sample);
#define lsx_rawstartread(ft) lsx_rawstart(ft, sox_false, sox_false, sox_false, SOX_ENCODING_UNKNOWN, 0)
#define lsx_rawstartwrite lsx_rawstartread
#define lsx_rawstopread NULL
#define lsx_rawstopwrite NULL

extern sox_format_handler_t const * lsx_sndfile_format_fn(void);

char * lsx_cat_comments(sox_comments_t comments);

/*--------------------------------- Effects ----------------------------------*/

int lsx_flow_copy(sox_effect_t * effp, const sox_sample_t * ibuf,
    sox_sample_t * obuf, size_t * isamp, size_t * osamp);
int lsx_usage(sox_effect_t * effp);
char * lsx_usage_lines(char * * usage, char const * const * lines, size_t n);
#define EFFECT(f) extern sox_effect_handler_t const * lsx_##f##_effect_fn(void);
#include "effects.h"
#undef EFFECT

#define NUMERIC_PARAMETER(name, min, max) { \
  char * end_ptr; \
  double d; \
  if (argc == 0) break; \
  d = strtod(*argv, &end_ptr); \
  if (end_ptr != *argv) { \
    if (d < min || d > max || *end_ptr != '\0') {\
      lsx_fail("parameter `%s' must be between %g and %g", #name, (double)min, (double)max); \
      return lsx_usage(effp); \
    } \
    p->name = d; \
    --argc, ++argv; \
  } \
}

#define TEXTUAL_PARAMETER(name, enum_table) { \
  lsx_enum_item const * e; \
  if (argc == 0) break; \
  e = lsx_find_enum_text(*argv, enum_table, 0); \
  if (e != NULL) { \
    p->name = e->value; \
    --argc, ++argv; \
  } \
}

#define GETOPT_LOCAL_NUMERIC(state, ch, name, min, max) case ch:{ \
  char * end_ptr; \
  double d = strtod(state.arg, &end_ptr); \
  if (end_ptr == state.arg || d < min || d > max || *end_ptr != '\0') {\
    lsx_fail("parameter `%s' must be between %g and %g", #name, (double)min, (double)max); \
    return lsx_usage(effp); \
  } \
  name = d; \
  break; \
}
#define GETOPT_NUMERIC(state, ch, name, min, max) GETOPT_LOCAL_NUMERIC(state, ch, p->name, min, max)

int lsx_effect_set_imin(sox_effect_t * effp, size_t imin);

int lsx_effects_init(void);
int lsx_effects_quit(void);

/*--------------------------------- Dynamic Library ----------------------------------*/

#if defined(HAVE_LIBLTDL)
    #include <ltdl.h>
    typedef lt_dlhandle lsx_dlhandle;
#else
    struct lsx_dlhandle_tag;
    typedef struct lsx_dlhandle_tag *lsx_dlhandle;
#endif

typedef void (*lsx_dlptr)(void);

typedef struct lsx_dlfunction_info
{
    const char* name;
    lsx_dlptr static_func;
    lsx_dlptr stub_func;
} lsx_dlfunction_info;

int lsx_open_dllibrary(
    int show_error_on_failure,
    const char* library_description,
    const char * const library_names[],
    const lsx_dlfunction_info func_infos[],
    lsx_dlptr selected_funcs[],
    lsx_dlhandle* pdl);

void lsx_close_dllibrary(
    lsx_dlhandle dl);

#define LSX_DLENTRIES_APPLY__(entries, f, x) entries(f, x)

#define LSX_DLENTRY_TO_PTR__(unused, func_return, func_name, func_args, static_func, stub_func, func_ptr) \
    func_return (*func_ptr) func_args;

#define LSX_DLENTRIES_TO_FUNCTIONS__(unused, func_return, func_name, func_args, static_func, stub_func, func_ptr) \
    func_return func_name func_args;

/* LSX_DLENTRIES_TO_PTRS: Given an ENTRIES macro and the name of the dlhandle
   variable, declares the corresponding function pointer variables and the
   dlhandle variable. */
#define LSX_DLENTRIES_TO_PTRS(entries, dlhandle) \
    LSX_DLENTRIES_APPLY__(entries, LSX_DLENTRY_TO_PTR__, 0) \
    lsx_dlhandle dlhandle

/* LSX_DLENTRIES_TO_FUNCTIONS: Given an ENTRIES macro, declares the corresponding
   functions. */
#define LSX_DLENTRIES_TO_FUNCTIONS(entries) \
    LSX_DLENTRIES_APPLY__(entries, LSX_DLENTRIES_TO_FUNCTIONS__, 0)

#define LSX_DLLIBRARY_OPEN1__(unused, func_return, func_name, func_args, static_func, stub_func, func_ptr) \
    { #func_name, (lsx_dlptr)(static_func), (lsx_dlptr)(stub_func) },

#define LSX_DLLIBRARY_OPEN2__(ptr_container, func_return, func_name, func_args, static_func, stub_func, func_ptr) \
    (ptr_container)->func_ptr = (func_return (*)func_args)lsx_dlfunction_open_library_funcs[lsx_dlfunction_open_library_index++];

/* LSX_DLLIBRARY_OPEN: Input an ENTRIES macro, the library's description,
   a null-terminated list of library names (i.e. { "libmp3-0", "libmp3", NULL }),
   the name of the dlhandle variable, the name of the structure that contains
   the function pointer and dlhandle variables, and the name of the variable in
   which the result of the lsx_open_dllibrary call should be stored. This will
   call lsx_open_dllibrary and copy the resulting function pointers into the
   structure members. If the library cannot be opened, show a failure message. */
#define LSX_DLLIBRARY_OPEN(ptr_container, dlhandle, entries, library_description, library_names, return_var) \
    LSX_DLLIBRARY_TRYOPEN(1, ptr_container, dlhandle, entries, library_description, library_names, return_var)

/* LSX_DLLIBRARY_TRYOPEN: Input an ENTRIES macro, the library's description,
   a null-terminated list of library names (i.e. { "libmp3-0", "libmp3", NULL }),
   the name of the dlhandle variable, the name of the structure that contains
   the function pointer and dlhandle variables, and the name of the variable in
   which the result of the lsx_open_dllibrary call should be stored. This will
   call lsx_open_dllibrary and copy the resulting function pointers into the
   structure members. If the library cannot be opened, show a report or a failure
   message, depending on whether error_on_failure is non-zero. */
#define LSX_DLLIBRARY_TRYOPEN(error_on_failure, ptr_container, dlhandle, entries, library_description, library_names, return_var) \
    do { \
      lsx_dlfunction_info lsx_dlfunction_open_library_infos[] = { \
        LSX_DLENTRIES_APPLY__(entries, LSX_DLLIBRARY_OPEN1__, 0) \
        {NULL,NULL,NULL} }; \
      int lsx_dlfunction_open_library_index = 0; \
      lsx_dlptr lsx_dlfunction_open_library_funcs[sizeof(lsx_dlfunction_open_library_infos)/sizeof(lsx_dlfunction_open_library_infos[0])]; \
      (return_var) = lsx_open_dllibrary((error_on_failure), (library_description), (library_names), lsx_dlfunction_open_library_infos, lsx_dlfunction_open_library_funcs, &(ptr_container)->dlhandle); \
      LSX_DLENTRIES_APPLY__(entries, LSX_DLLIBRARY_OPEN2__, ptr_container) \
    } while(0)

#define LSX_DLLIBRARY_CLOSE(ptr_container, dlhandle) \
    lsx_close_dllibrary((ptr_container)->dlhandle)

  /* LSX_DLENTRY_STATIC: For use in creating an ENTRIES macro. func is
     expected to be available at link time. If not present, link will fail. */
#define LSX_DLENTRY_STATIC(f,x, ret, func, args)  f(x, ret, func, args, func, NULL, func)

  /* LSX_DLENTRY_DYNAMIC: For use in creating an ENTRIES macro. func need
     not be available at link time (and if present, the link time version will
     not be used). func will be loaded via dlsym. If this function is not
     found in the shared library, the shared library will not be used. */
#define LSX_DLENTRY_DYNAMIC(f,x, ret, func, args) f(x, ret, func, args, NULL, NULL, func)

  /* LSX_DLENTRY_STUB: For use in creating an ENTRIES macro. func need not
     be available at link time (and if present, the link time version will not
     be used). If using DL_LAME, the func may be loaded via dlopen/dlsym, but
     if not found, the shared library will still be used if all of the
     non-stub functions are found. If the function is not found via dlsym (or
     if we are not loading any shared libraries), the stub will be used. This
     assumes that the name of the stub function is the name of the function +
     "_stub". */
#define LSX_DLENTRY_STUB(f,x, ret, func, args)    f(x, ret, func, args, NULL, func##_stub, func)

  /* LSX_DLFUNC_IS_STUB: returns true if the named function is a do-nothing
     stub. Assumes that the name of the stub function is the name of the
     function + "_stub". */
#define LSX_DLFUNC_IS_STUB(ptr_container, func) ((ptr_container)->func == func##_stub)

#endif
