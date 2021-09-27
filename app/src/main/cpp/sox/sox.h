/* libSoX Library Public Interface
 *
 * Copyright 1999-2012 Chris Bagwell and SoX Contributors.
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And SoX Contributors are not responsible for
 * the consequences of using this software.
 */

/** @file
Contains the interface exposed to clients of the libSoX library.
Symbols starting with "sox_" or "SOX_" are part of the public interface for
libSoX clients (applications that consume libSoX). Symbols starting with
"lsx_" or "LSX_" are internal use by libSoX and plugins.
LSX_ and lsx_ symbols should not be used by libSoX-based applications.
*/

#ifndef SOX_H
#define SOX_H /**< Client API: This macro is defined if sox.h has been included. */

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Suppress warnings from use of type long long. */
#if defined __GNUC__
#pragma GCC system_header
#endif

#if defined __GNUC__
#define LSX_GCC(maj, min) \
  ((__GNUC__ > (maj)) || (__GNUC__ == (maj) && __GNUC_MINOR__ >= (min)))
#else
#define LSX_GCC(maj, min) 0
#endif

#if LSX_GCC(4,9)
#define _Ret_ __attribute__ ((returns_nonnull))
#define _Ret_valid_ _Ret_
#define _Ret_z_ _Ret_
#endif

/*****************************************************************************
API decoration macros:
Mostly for documentation purposes. For some compilers, decorations also affect
code generation, influence compiler warnings or activate compiler
optimizations.
*****************************************************************************/

/**
Plugins API:
Attribute required on all functions exported by libSoX and on all function
pointer types used by the libSoX API.
*/
#if defined __GNUC__ && defined __i386__
#define LSX_API  __attribute__ ((cdecl)) /* libSoX function */
#elif _MSC_VER
#define LSX_API  __cdecl /* libSoX function */
#else
#define LSX_API /* libSoX function */
#endif

/**
Plugins API:
Attribute applied to a parameter or local variable to suppress warnings about
the variable being unused (especially in macro-generated code).
*/
#ifdef __GNUC__
#define LSX_UNUSED  __attribute__ ((unused)) /* Parameter or local variable is intentionally unused. */
#else
#define LSX_UNUSED /* Parameter or local variable is intentionally unused. */
#endif

/**
Plugins API:
LSX_PRINTF12: Attribute applied to a function to indicate that it requires
a printf-style format string for arg1 and that printf parameters start at
arg2.
*/
#ifdef __GNUC__
#define LSX_PRINTF12  __attribute__ ((format (printf, 1, 2))) /* Function has printf-style arguments. */
#else
#define LSX_PRINTF12 /* Function has printf-style arguments. */
#endif

/**
Plugins API:
Attribute applied to a function to indicate that it has no side effects and
depends only its input parameters and global memory. If called repeatedly, it
returns the same result each time.
*/
#ifdef __GNUC__
#define LSX_RETURN_PURE __attribute__ ((pure)) /* Function is pure. */
#else
#define LSX_RETURN_PURE /* Function is pure. */
#endif

/**
Plugins API:
Attribute applied to a function to indicate that the
return value is always a pointer to a valid object (never NULL).
*/
#ifdef _Ret_
#define LSX_RETURN_VALID _Ret_ /* Function always returns a valid object (never NULL). */
#else
#define LSX_RETURN_VALID /* Function always returns a valid object (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a function to indicate that the return value is always a
pointer to a valid array (never NULL).
*/
#ifdef _Ret_valid_
#define LSX_RETURN_ARRAY _Ret_valid_ /* Function always returns a valid array (never NULL). */
#else
#define LSX_RETURN_ARRAY /* Function always returns a valid array (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a function to indicate that the return value is always a
pointer to a valid 0-terminated array (never NULL).
*/
#ifdef _Ret_z_
#define LSX_RETURN_VALID_Z _Ret_z_ /* Function always returns a 0-terminated array (never NULL). */
#else
#define LSX_RETURN_VALID_Z /* Function always returns a 0-terminated array (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a function to indicate that the returned pointer may be
null.
*/
#ifdef _Ret_opt_
#define LSX_RETURN_OPT _Ret_opt_ /* Function may return NULL. */
#else
#define LSX_RETURN_OPT /* Function may return NULL. */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to one const element of the pointed-to type (never NULL).
*/
#ifdef _In_
#define LSX_PARAM_IN _In_ /* Required const pointer to a valid object (never NULL). */
#else
#define LSX_PARAM_IN /* Required const pointer to a valid object (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to a const 0-terminated string (never NULL).
*/
#ifdef _In_z_
#define LSX_PARAM_IN_Z _In_z_ /* Required const pointer to 0-terminated string (never NULL). */
#else
#define LSX_PARAM_IN_Z /* Required const pointer to 0-terminated string (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a const
pointer to a 0-terminated printf format string.
*/
#ifdef _Printf_format_string_
#define LSX_PARAM_IN_PRINTF _Printf_format_string_ /* Required const pointer to 0-terminated printf format string (never NULL). */
#else
#define LSX_PARAM_IN_PRINTF /* Required const pointer to 0-terminated printf format string (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to (len) const initialized elements of the pointed-to type, where
(len) is the name of another parameter.
@param len The parameter that contains the number of elements in the array.
*/
#ifdef _In_count_
#define LSX_PARAM_IN_COUNT(len) _In_count_(len) /* Required const pointer to (len) valid objects (never NULL). */
#else
#define LSX_PARAM_IN_COUNT(len) /* Required const pointer to (len) valid objects (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to (len) const bytes of initialized data, where (len) is the name of
another parameter.
@param len The parameter that contains the number of bytes in the array.
*/
#ifdef _In_bytecount_
#define LSX_PARAM_IN_BYTECOUNT(len) _In_bytecount_(len) /* Required const pointer to (len) bytes of data (never NULL). */
#else
#define LSX_PARAM_IN_BYTECOUNT(len) /* Required const pointer to (len) bytes of data (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is either NULL
or a valid pointer to one const element of the pointed-to type.
*/
#ifdef _In_opt_
#define LSX_PARAM_IN_OPT _In_opt_ /* Optional const pointer to a valid object (may be NULL). */
#else
#define LSX_PARAM_IN_OPT /* Optional const pointer to a valid object (may be NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is either NULL
or a valid pointer to a const 0-terminated string.
*/
#ifdef _In_opt_z_
#define LSX_PARAM_IN_OPT_Z _In_opt_z_ /* Optional const pointer to 0-terminated string (may be NULL). */
#else
#define LSX_PARAM_IN_OPT_Z /* Optional const pointer to 0-terminated string (may be NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to one initialized element of the pointed-to type (never NULL). The
function may modify the element.
*/
#ifdef _Inout_
#define LSX_PARAM_INOUT _Inout_ /* Required pointer to a valid object (never NULL). */
#else
#define LSX_PARAM_INOUT /* Required pointer to a valid object (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to (len) initialized elements of the pointed-to type (never NULL). The
function may modify the elements.
@param len The parameter that contains the number of elements in the array.
*/
#ifdef _Inout_count_x_
#define LSX_PARAM_INOUT_COUNT(len) _Inout_count_x_(len) /* Required pointer to (len) valid objects (never NULL). */
#else
#define LSX_PARAM_INOUT_COUNT(len) /* Required pointer to (len) valid objects (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to memory sufficient for one element of the pointed-to type (never
NULL). The function will initialize the element.
*/
#ifdef _Out_
#define LSX_PARAM_OUT _Out_ /* Required pointer to an object to be initialized (never NULL). */
#else
#define LSX_PARAM_OUT /* Required pointer to an object to be initialized (never NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to memory sufficient for (len) bytes of data (never NULL), where (len)
is the name of another parameter. The function may write up to len bytes of
data to this memory.
@param len The parameter that contains the number of bytes in the array.
*/
#ifdef _Out_bytecap_
#define LSX_PARAM_OUT_BYTECAP(len) _Out_bytecap_(len) /* Required pointer to writable buffer with room for len bytes. */
#else
#define LSX_PARAM_OUT_BYTECAP(len) /* Required pointer to writable buffer with room for len bytes. */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to memory sufficient for (len) elements of the pointed-to type (never
NULL), where (len) is the name of another parameter. On return, (filled)
elements will have been initialized, where (filled) is either the dereference
of another pointer parameter (for example "*written") or the "return"
parameter (indicating that the function returns the number of elements
written).
@param len The parameter that contains the number of elements in the array.
@param filled The dereference of the parameter that receives the number of elements written to the array, or "return" if the value is returned.
*/
#ifdef _Out_cap_post_count_
#define LSX_PARAM_OUT_CAP_POST_COUNT(len,filled) _Out_cap_post_count_(len,filled) /* Required pointer to buffer for (len) elements (never NULL); on return, (filled) elements will have been initialized. */
#else
#define LSX_PARAM_OUT_CAP_POST_COUNT(len,filled) /* Required pointer to buffer for (len) elements (never NULL); on return, (filled) elements will have been initialized. */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer to memory sufficient for (len) elements of the pointed-to type (never
NULL), where (len) is the name of another parameter. On return, (filled+1)
elements will have been initialized, with the last element having been
initialized to 0, where (filled) is either the dereference of another pointer
parameter (for example, "*written") or the "return" parameter (indicating that
the function returns the number of elements written).
@param len The parameter that contains the number of elements in the array.
@param filled The dereference of the parameter that receives the number of elements written to the array (not counting the terminating null), or "return" if the value is returned.
*/
#ifdef _Out_z_cap_post_count_
#define LSX_PARAM_OUT_Z_CAP_POST_COUNT(len,filled) _Out_z_cap_post_count_(len,filled) /* Required pointer to buffer for (len) elements (never NULL); on return, (filled+1) elements will have been initialized, and the array will be 0-terminated. */
#else
#define LSX_PARAM_OUT_Z_CAP_POST_COUNT(len,filled) /* Required pointer to buffer for (len) elements (never NULL); on return, (filled+1) elements will have been initialized, and the array will be 0-terminated. */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is either NULL
or a valid pointer to memory sufficient for one element of the pointed-to
type. The function will initialize the element.
*/
#ifdef _Out_opt_
#define LSX_PARAM_OUT_OPT _Out_opt_ /* Optional pointer to an object to be initialized (may be NULL). */
#else
#define LSX_PARAM_OUT_OPT /* Optional pointer to an object to be initialized (may be NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer (never NULL) to another pointer which may be NULL when the function is
invoked.
*/
#ifdef _Deref_pre_maybenull_
#define LSX_PARAM_DEREF_PRE_MAYBENULL _Deref_pre_maybenull_ /* Required pointer (never NULL) to another pointer (may be NULL). */
#else
#define LSX_PARAM_DEREF_PRE_MAYBENULL /* Required pointer (never NULL) to another pointer (may be NULL). */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer (never NULL) to another pointer which will be NULL when the function
returns.
*/
#ifdef _Deref_post_null_
#define LSX_PARAM_DEREF_POST_NULL _Deref_post_null_ /* Required pointer (never NULL) to another pointer, which will be NULL on exit. */
#else
#define LSX_PARAM_DEREF_POST_NULL /* Required pointer (never NULL) to another pointer, which will be NULL on exit. */
#endif

/**
Plugins API:
Attribute applied to a parameter to indicate that the parameter is a valid
pointer (never NULL) to another pointer which will be non-NULL when the
function returns.
*/
#ifdef _Deref_post_notnull_
#define LSX_PARAM_DEREF_POST_NOTNULL _Deref_post_notnull_ /* Required pointer (never NULL) to another pointer, which will be valid (not NULL) on exit. */
#else
#define LSX_PARAM_DEREF_POST_NOTNULL /* Required pointer (never NULL) to another pointer, which will be valid (not NULL) on exit. */
#endif

/**
Plugins API:
Expression that "uses" a potentially-unused variable to avoid compiler
warnings (especially in macro-generated code).
*/
#ifdef _PREFAST_
#define LSX_USE_VAR(x)  ((void)(x=0)) /* During static analysis, initialize unused variables to 0. */
#else
#define LSX_USE_VAR(x)  ((void)(x)) /* Parameter or variable is intentionally unused. */
#endif

/**
Plugins API:
Compile-time assertion. Causes a compile error if the expression is false.
@param e  The expression to test. If expression is false, compilation will fail.
@param f  A unique identifier for the test, for example foo_must_not_be_zero.
*/
#define lsx_static_assert(e,f) enum {lsx_static_assert_##f = 1/((e) ? 1 : 0)}

/*****************************************************************************
Basic typedefs:
*****************************************************************************/

/**
Client API:
Signed twos-complement 8-bit type. Typically defined as signed char.
*/
typedef int8_t sox_int8_t;

/**
Client API:
Unsigned 8-bit type. Typically defined as unsigned char.
*/
typedef uint8_t sox_uint8_t;

/**
Client API:
Signed twos-complement 16-bit type. Typically defined as short.
*/
typedef int16_t sox_int16_t;

/**
Client API:
Unsigned 16-bit type. Typically defined as unsigned short.
*/
typedef uint16_t sox_uint16_t;

/**
Client API:
Signed twos-complement 32-bit type. Typically defined as int.
*/
typedef int32_t sox_int32_t;

/**
Client API:
Unsigned 32-bit type. Typically defined as unsigned int.
*/
typedef uint32_t sox_uint32_t;

/**
Client API:
Signed twos-complement 64-bit type. Typically defined as long or long long.
*/
typedef int64_t sox_int64_t;

/**
Client API:
Unsigned 64-bit type. Typically defined as unsigned long or unsigned long long.
*/
typedef uint64_t sox_uint64_t;

/**
Client API:
Alias for sox_int32_t (beware of the extra byte).
*/
typedef sox_int32_t sox_int24_t;

/**
Client API:
Alias for sox_uint32_t (beware of the extra byte).
*/
typedef sox_uint32_t sox_uint24_t;

/**
Client API:
Native SoX audio sample type (alias for sox_int32_t).
*/
typedef sox_int32_t sox_sample_t;

/**
Client API:
Samples per second is stored as a double.
*/
typedef double sox_rate_t;

/**
Client API:
File's metadata, access via sox_*_comments functions.
*/
typedef char * * sox_comments_t;

/*****************************************************************************
Enumerations:
*****************************************************************************/

/**
Client API:
Boolean type, assignment (but not necessarily binary) compatible with C++ bool.
*/
typedef enum sox_bool {
    sox_bool_dummy = -1, /* Ensure a signed type */
    sox_false, /**< False = 0. */
    sox_true   /**< True = 1. */
} sox_bool;

/**
Client API:
no, yes, or default (default usually implies some kind of auto-detect logic).
*/
typedef enum sox_option_t {
    sox_option_no,      /**< Option specified as no = 0. */
    sox_option_yes,     /**< Option specified as yes = 1. */
    sox_option_default  /**< Option unspecified = 2. */
} sox_option_t;

/**
Client API:
The libSoX-specific error codes.
libSoX functions may return these codes or others that map from errno codes.
*/
enum sox_error_t {
  SOX_SUCCESS = 0,     /**< Function succeeded = 0 */
  SOX_EOF = -1,        /**< End Of File or other error = -1 */
  SOX_EHDR = 2000,     /**< Invalid Audio Header = 2000 */
  SOX_EFMT,            /**< Unsupported data format = 2001 */
  SOX_ENOMEM,          /**< Can't alloc memory = 2002 */
  SOX_EPERM,           /**< Operation not permitted = 2003 */
  SOX_ENOTSUP,         /**< Operation not supported = 2004 */
  SOX_EINVAL           /**< Invalid argument = 2005 */
};

/**
Client API:
Flags indicating whether optional features are present in this build of libSoX.
*/
typedef enum sox_version_flags_t {
    sox_version_none = 0,         /**< No special features = 0. */
    sox_version_have_popen = 1,   /**< popen = 1. */
    sox_version_have_magic = 2,   /**< magic = 2. */
    sox_version_have_threads = 4, /**< threads = 4. */
    sox_version_have_memopen = 8  /**< memopen = 8. */
} sox_version_flags_t;

/**
Client API:
Format of sample data.
*/
typedef enum sox_encoding_t {
  SOX_ENCODING_UNKNOWN   , /**< encoding has not yet been determined */

  SOX_ENCODING_SIGN2     , /**< signed linear 2's comp: Mac */
  SOX_ENCODING_UNSIGNED  , /**< unsigned linear: Sound Blaster */
  SOX_ENCODING_FLOAT     , /**< floating point (binary format) */
  SOX_ENCODING_FLOAT_TEXT, /**< floating point (text format) */
  SOX_ENCODING_FLAC      , /**< FLAC compression */
  SOX_ENCODING_HCOM      , /**< Mac FSSD files with Huffman compression */
  SOX_ENCODING_WAVPACK   , /**< WavPack with integer samples */
  SOX_ENCODING_WAVPACKF  , /**< WavPack with float samples */
  SOX_ENCODING_ULAW      , /**< u-law signed logs: US telephony, SPARC */
  SOX_ENCODING_ALAW      , /**< A-law signed logs: non-US telephony, Psion */
  SOX_ENCODING_G721      , /**< G.721 4-bit ADPCM */
  SOX_ENCODING_G723      , /**< G.723 3 or 5 bit ADPCM */
  SOX_ENCODING_CL_ADPCM  , /**< Creative Labs 8 --> 2,3,4 bit Compressed PCM */
  SOX_ENCODING_CL_ADPCM16, /**< Creative Labs 16 --> 4 bit Compressed PCM */
  SOX_ENCODING_MS_ADPCM  , /**< Microsoft Compressed PCM */
  SOX_ENCODING_IMA_ADPCM , /**< IMA Compressed PCM */
  SOX_ENCODING_OKI_ADPCM , /**< Dialogic/OKI Compressed PCM */
  SOX_ENCODING_DPCM      , /**< Differential PCM: Fasttracker 2 (xi) */
  SOX_ENCODING_DWVW      , /**< Delta Width Variable Word */
  SOX_ENCODING_DWVWN     , /**< Delta Width Variable Word N-bit */
  SOX_ENCODING_GSM       , /**< GSM 6.10 33byte frame lossy compression */
  SOX_ENCODING_MP3       , /**< MP3 compression */
  SOX_ENCODING_VORBIS    , /**< Vorbis compression */
  SOX_ENCODING_AMR_WB    , /**< AMR-WB compression */
  SOX_ENCODING_AMR_NB    , /**< AMR-NB compression */
  SOX_ENCODING_CVSD      , /**< Continuously Variable Slope Delta modulation */
  SOX_ENCODING_LPC10     , /**< Linear Predictive Coding */
  SOX_ENCODING_OPUS      , /**< Opus compression */

  SOX_ENCODINGS            /**< End of list marker */
} sox_encoding_t;

/**
Client API:
Flags for sox_encodings_info_t: lossless/lossy1/lossy2.
*/
typedef enum sox_encodings_flags_t {
  sox_encodings_none   = 0, /**< no flags specified (implies lossless encoding) = 0. */
  sox_encodings_lossy1 = 1, /**< encode, decode: lossy once = 1. */
  sox_encodings_lossy2 = 2  /**< encode, decode, encode, decode: lossy twice = 2. */
} sox_encodings_flags_t;

/**
Client API:
Type of plot.
*/
typedef enum sox_plot_t {
    sox_plot_off,     /**< No plot = 0. */
    sox_plot_octave,  /**< Octave plot = 1. */
    sox_plot_gnuplot, /**< Gnuplot plot = 2. */
    sox_plot_data     /**< Plot data = 3. */
} sox_plot_t;

/**
Client API:
Loop modes: upper 4 bits mask the loop blass, lower 4 bits describe
the loop behaviour, for example single shot, bidirectional etc.
*/
enum sox_loop_flags_t {
  sox_loop_none = 0,          /**< single-shot = 0 */
  sox_loop_forward = 1,       /**< forward loop = 1 */
  sox_loop_forward_back = 2,  /**< forward/back loop = 2 */
  sox_loop_8 = 32,            /**< 8 loops (??) = 32 */
  sox_loop_sustain_decay = 64 /**< AIFF style, one sustain & one decay loop = 64 */
};

/**
Plugins API:
Is file a real file, a pipe, or a url?
*/
typedef enum lsx_io_type
{
    lsx_io_file, /**< File is a real file = 0. */
    lsx_io_pipe, /**< File is a pipe (no seeking) = 1. */
    lsx_io_url   /**< File is a URL (no seeking) = 2. */
} lsx_io_type;

/*****************************************************************************
Macros:
*****************************************************************************/

/**
Client API:
Compute a 32-bit integer API version from three 8-bit parts.
@param a Major version.
@param b Minor version.
@param c Revision or build number.
@returns 32-bit integer API version 0x000a0b0c.
*/
#define SOX_LIB_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

/**
Client API:
The API version of the sox.h file. It is not meant to follow the version
number of SoX but it has historically. Please do not count on
SOX_LIB_VERSION_CODE staying in sync with the libSoX version.
*/
#define SOX_LIB_VERSION_CODE   SOX_LIB_VERSION(14, 4, 2)

/**
Client API:
Returns the smallest (negative) value storable in a twos-complement signed
integer with the specified number of bits, cast to an unsigned integer;
for example, SOX_INT_MIN(8) = 0x80, SOX_INT_MIN(16) = 0x8000, etc.
@param bits Size of value for which to calculate minimum.
@returns the smallest (negative) value storable in a twos-complement signed
integer with the specified number of bits, cast to an unsigned integer.
*/
#define SOX_INT_MIN(bits) (1 <<((bits)-1))

/**
Client API:
Returns the largest (positive) value storable in a twos-complement signed
integer with the specified number of bits, cast to an unsigned integer;
for example, SOX_INT_MAX(8) = 0x7F, SOX_INT_MAX(16) = 0x7FFF, etc.
@param bits Size of value for which to calculate maximum.
@returns the largest (positive) value storable in a twos-complement signed
integer with the specified number of bits, cast to an unsigned integer.
*/
#define SOX_INT_MAX(bits) (((unsigned)-1)>>(33-(bits)))

/**
Client API:
Returns the largest value storable in an unsigned integer with the specified
number of bits; for example, SOX_UINT_MAX(8) = 0xFF,
SOX_UINT_MAX(16) = 0xFFFF, etc.
@param bits Size of value for which to calculate maximum.
@returns the largest value storable in an unsigned integer with the specified
number of bits.
*/
#define SOX_UINT_MAX(bits) (SOX_INT_MIN(bits)|SOX_INT_MAX(bits))

/**
Client API:
Returns 0x7F.
*/
#define SOX_INT8_MAX  SOX_INT_MAX(8)

/**
Client API:
Returns 0x7FFF.
*/
#define SOX_INT16_MAX SOX_INT_MAX(16)

/**
Client API:
Returns 0x7FFFFF.
*/
#define SOX_INT24_MAX SOX_INT_MAX(24)

/**
Client API:
Returns 0x7FFFFFFF.
*/
#define SOX_INT32_MAX SOX_INT_MAX(32)

/**
Client API:
Bits in a sox_sample_t = 32.
*/
#define SOX_SAMPLE_PRECISION 32

/**
Client API:
Max value for sox_sample_t = 0x7FFFFFFF.
*/
#define SOX_SAMPLE_MAX (sox_sample_t)SOX_INT_MAX(32)

/**
Client API:
Min value for sox_sample_t = 0x80000000.
*/
#define SOX_SAMPLE_MIN (sox_sample_t)SOX_INT_MIN(32)


/*                Conversions: Linear PCM <--> sox_sample_t
 *
 *   I/O      Input    sox_sample_t Clips?   Input    sox_sample_t Clips?
 *  Format   Minimum     Minimum     I O    Maximum     Maximum     I O
 *  ------  ---------  ------------ -- --   --------  ------------ -- --
 *  Float     -inf         -1        y n      inf      1 - 5e-10    y n
 *  Int8      -128        -128       n n      127     127.9999999   n y
 *  Int16    -32768      -32768      n n     32767    32767.99998   n y
 *  Int24   -8388608    -8388608     n n    8388607   8388607.996   n y
 *  Int32  -2147483648 -2147483648   n n   2147483647 2147483647    n n
 *
 * Conversions are as accurate as possible (with rounding).
 *
 * Rounding: halves toward +inf, all others to nearest integer.
 *
 * Clips? shows whether on not there is the possibility of a conversion
 * clipping to the minimum or maximum value when inputing from or outputing
 * to a given type.
 *
 * Unsigned integers are converted to and from signed integers by flipping
 * the upper-most bit then treating them as signed integers.
 */

/**
Client API:
Declares the temporary local variables that are required when using SOX
conversion macros.
*/
#define SOX_SAMPLE_LOCALS sox_sample_t sox_macro_temp_sample LSX_UNUSED; \
  double sox_macro_temp_double LSX_UNUSED

/**
Client API:
Sign bit for sox_sample_t = 0x80000000.
*/
#define SOX_SAMPLE_NEG SOX_INT_MIN(32)

/**
Client API:
Converts sox_sample_t to an unsigned integer of width (bits).
@param bits  Width of resulting sample (1 through 32).
@param d     Input sample to be converted.
@param clips Variable that is incremented if the result is too big.
@returns Unsigned integer of width (bits).
*/
#define SOX_SAMPLE_TO_UNSIGNED(bits,d,clips) \
  (sox_uint##bits##_t)(SOX_SAMPLE_TO_SIGNED(bits,d,clips) ^ SOX_INT_MIN(bits))

/**
Client API:
Converts sox_sample_t to a signed integer of width (bits).
@param bits  Width of resulting sample (1 through 32).
@param d     Input sample to be converted.
@param clips Variable that is incremented if the result is too big.
@returns Signed integer of width (bits).
*/
#define SOX_SAMPLE_TO_SIGNED(bits,d,clips)                              \
  (sox_int##bits##_t)(                                                  \
    LSX_USE_VAR(sox_macro_temp_double),                                 \
    sox_macro_temp_sample = (d),                                        \
    sox_macro_temp_sample > SOX_SAMPLE_MAX - (1 << (31-bits)) ?         \
      ++(clips), SOX_INT_MAX(bits) :                                    \
      ((sox_uint32_t)(sox_macro_temp_sample + (1 << (31-bits)))) >> (32-bits))

/**
Client API:
Converts signed integer of width (bits) to sox_sample_t.
@param bits Width of input sample (1 through 32).
@param d    Input sample to be converted.
@returns SoX native sample value.
*/
#define SOX_SIGNED_TO_SAMPLE(bits,d) ((sox_sample_t)(d) << (32-bits))

/**
Client API:
Converts unsigned integer of width (bits) to sox_sample_t.
@param bits Width of input sample (1 through 32).
@param d    Input sample to be converted.
@returns SoX native sample value.
*/
#define SOX_UNSIGNED_TO_SAMPLE(bits,d) \
      (SOX_SIGNED_TO_SAMPLE(bits,d) ^ SOX_SAMPLE_NEG)

/**
Client API:
Converts unsigned 8-bit integer to sox_sample_t.
@param d     Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_UNSIGNED_8BIT_TO_SAMPLE(d,clips) SOX_UNSIGNED_TO_SAMPLE(8,d)

/**
Client API:
Converts signed 8-bit integer to sox_sample_t.
@param d    Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_SIGNED_8BIT_TO_SAMPLE(d,clips) SOX_SIGNED_TO_SAMPLE(8,d)

/**
Client API:
Converts unsigned 16-bit integer to sox_sample_t.
@param d     Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_UNSIGNED_16BIT_TO_SAMPLE(d,clips) SOX_UNSIGNED_TO_SAMPLE(16,d)

/**
Client API:
Converts signed 16-bit integer to sox_sample_t.
@param d    Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_SIGNED_16BIT_TO_SAMPLE(d,clips) SOX_SIGNED_TO_SAMPLE(16,d)

/**
Client API:
Converts unsigned 24-bit integer to sox_sample_t.
@param d     Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_UNSIGNED_24BIT_TO_SAMPLE(d,clips) SOX_UNSIGNED_TO_SAMPLE(24,d)

/**
Client API:
Converts signed 24-bit integer to sox_sample_t.
@param d    Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_SIGNED_24BIT_TO_SAMPLE(d,clips) SOX_SIGNED_TO_SAMPLE(24,d)

/**
Client API:
Converts unsigned 32-bit integer to sox_sample_t.
@param d     Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_UNSIGNED_32BIT_TO_SAMPLE(d,clips) \
  ((sox_sample_t)(d) ^ SOX_SAMPLE_NEG)

/**
Client API:
Converts signed 32-bit integer to sox_sample_t.
@param d    Input sample to be converted.
@param clips The parameter is not used.
@returns SoX native sample value.
*/
#define SOX_SIGNED_32BIT_TO_SAMPLE(d,clips) (sox_sample_t)(d)

/**
Client API:
Converts 32-bit float to sox_sample_t.
@param d     Input sample to be converted, range [-1, 1).
@param clips Variable to increment if the input sample is too large or too small.
@returns SoX native sample value.
*/
#define SOX_FLOAT_32BIT_TO_SAMPLE(d,clips) SOX_FLOAT_64BIT_TO_SAMPLE(d, clips)

/**
Client API:
Converts 64-bit float to sox_sample_t.
@param d     Input sample to be converted, range [-1, 1).
@param clips Variable to increment if the input sample is too large or too small.
@returns SoX native sample value.
*/
#define SOX_FLOAT_64BIT_TO_SAMPLE(d, clips)                     \
  (sox_sample_t)(                                               \
    LSX_USE_VAR(sox_macro_temp_sample),                         \
    sox_macro_temp_double = (d) * (SOX_SAMPLE_MAX + 1.0),       \
    sox_macro_temp_double < 0 ?                                 \
      sox_macro_temp_double <= SOX_SAMPLE_MIN - 0.5 ?           \
        ++(clips), SOX_SAMPLE_MIN :                             \
        sox_macro_temp_double - 0.5 :                           \
      sox_macro_temp_double >= SOX_SAMPLE_MAX + 0.5 ?           \
        sox_macro_temp_double > SOX_SAMPLE_MAX + 1.0 ?          \
          ++(clips), SOX_SAMPLE_MAX :                           \
          SOX_SAMPLE_MAX :                                      \
        sox_macro_temp_double + 0.5                             \
  )

/**
Client API:
Converts SoX native sample to an unsigned 8-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_UNSIGNED_8BIT(d,clips) SOX_SAMPLE_TO_UNSIGNED(8,d,clips)

/**
Client API:
Converts SoX native sample to an signed 8-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_SIGNED_8BIT(d,clips) SOX_SAMPLE_TO_SIGNED(8,d,clips)

/**
Client API:
Converts SoX native sample to an unsigned 16-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_UNSIGNED_16BIT(d,clips) SOX_SAMPLE_TO_UNSIGNED(16,d,clips)

/**
Client API:
Converts SoX native sample to a signed 16-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_SIGNED_16BIT(d,clips) SOX_SAMPLE_TO_SIGNED(16,d,clips)

/**
Client API:
Converts SoX native sample to an unsigned 24-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_UNSIGNED_24BIT(d,clips) SOX_SAMPLE_TO_UNSIGNED(24,d,clips)

/**
Client API:
Converts SoX native sample to a signed 24-bit integer.
@param d Input sample to be converted.
@param clips Variable to increment if input sample is too large.
*/
#define SOX_SAMPLE_TO_SIGNED_24BIT(d,clips) SOX_SAMPLE_TO_SIGNED(24,d,clips)

/**
Client API:
Converts SoX native sample to an unsigned 32-bit integer.
@param d Input sample to be converted.
@param clips The parameter is not used.
*/
#define SOX_SAMPLE_TO_UNSIGNED_32BIT(d,clips) (sox_uint32_t)((d)^SOX_SAMPLE_NEG)

/**
Client API:
Converts SoX native sample to a signed 32-bit integer.
@param d Input sample to be converted.
@param clips The parameter is not used.
*/
#define SOX_SAMPLE_TO_SIGNED_32BIT(d,clips) (sox_int32_t)(d)

/**
Client API:
Converts SoX native sample to a 32-bit float.
@param d Input sample to be converted.
@param clips The parameter is not used.
*/
#define SOX_SAMPLE_TO_FLOAT_32BIT(d,clips) ((d)*(1.0 / (SOX_SAMPLE_MAX + 1.0)))

/**
Client API:
Converts SoX native sample to a 64-bit float.
@param d Input sample to be converted.
@param clips The parameter is not used.
*/
#define SOX_SAMPLE_TO_FLOAT_64BIT(d,clips) ((d)*(1.0 / (SOX_SAMPLE_MAX + 1.0)))

/**
Client API:
Clips a value of a type that is larger then sox_sample_t (for example, int64)
to sox_sample_t's limits and increment a counter if clipping occurs.
@param samp Value (lvalue) to be clipped, updated as necessary.
@param clips Value (lvalue) that is incremented if clipping is needed.
*/
#define SOX_SAMPLE_CLIP_COUNT(samp, clips) \
  do { \
    if (samp > SOX_SAMPLE_MAX) \
      { samp = SOX_SAMPLE_MAX; clips++; } \
    else if (samp < SOX_SAMPLE_MIN) \
      { samp = SOX_SAMPLE_MIN; clips++; } \
  } while (0)

/**
Client API:
Clips a value of a type that is larger then sox_sample_t (for example, int64)
to sox_sample_t's limits and increment a counter if clipping occurs.
@param d Value (rvalue) to be clipped.
@param clips Value (lvalue) that is incremented if clipping is needed.
@returns Clipped value.
*/
#define SOX_ROUND_CLIP_COUNT(d, clips) \
  ((d) < 0? (d) <= SOX_SAMPLE_MIN - 0.5? ++(clips), SOX_SAMPLE_MIN: (d) - 0.5 \
        : (d) >= SOX_SAMPLE_MAX + 0.5? ++(clips), SOX_SAMPLE_MAX: (d) + 0.5)

/**
Client API:
Clips a value to the limits of a signed integer of the specified width
and increment a counter if clipping occurs.
@param bits Width (in bits) of target integer type.
@param i Value (rvalue) to be clipped.
@param clips Value (lvalue) that is incremented if clipping is needed.
@returns Clipped value.
*/
#define SOX_INTEGER_CLIP_COUNT(bits,i,clips) ( \
  (i) >(1 << ((bits)-1))- 1? ++(clips),(1 << ((bits)-1))- 1 : \
  (i) <-1 << ((bits)-1)    ? ++(clips),-1 << ((bits)-1) : (i))

/**
Client API:
Clips a value to the limits of a 16-bit signed integer and increment a counter
if clipping occurs.
@param i Value (rvalue) to be clipped.
@param clips Value (lvalue) that is incremented if clipping is needed.
@returns Clipped value.
*/
#define SOX_16BIT_CLIP_COUNT(i,clips) SOX_INTEGER_CLIP_COUNT(16,i,clips)

/**
Client API:
Clips a value to the limits of a 24-bit signed integer and increment a counter
if clipping occurs.
@param i Value (rvalue) to be clipped.
@param clips Value (lvalue) that is incremented if clipping is needed.
@returns Clipped value.
*/
#define SOX_24BIT_CLIP_COUNT(i,clips) SOX_INTEGER_CLIP_COUNT(24,i,clips)

#define SOX_SIZE_MAX ((size_t)(-1)) /**< Client API: Maximum value of size_t. */

#define SOX_UNSPEC 0                         /**< Client API: Members of sox_signalinfo_t are set to SOX_UNSPEC (= 0) if the actual value is not yet known. */
#define SOX_UNKNOWN_LEN (sox_uint64_t)(-1) /**< Client API: sox_signalinfo_t.length is set to SOX_UNKNOWN_LEN (= -1) within the effects chain if the actual length is not known. Format handlers currently use SOX_UNSPEC instead. */
#define SOX_IGNORE_LENGTH (sox_uint64_t)(-2) /**< Client API: sox_signalinfo_t.length is set to SOX_IGNORE_LENGTH (= -2) to indicate that a format handler should ignore length information in file headers. */

#define SOX_DEFAULT_CHANNELS  2     /**< Client API: Default channel count is 2 (stereo). */
#define SOX_DEFAULT_RATE      48000 /**< Client API: Default rate is 48000Hz. */
#define SOX_DEFAULT_PRECISION 16    /**< Client API: Default precision is 16 bits per sample. */
#define SOX_DEFAULT_ENCODING  SOX_ENCODING_SIGN2 /**< Client API: Default encoding is SIGN2 (linear 2's complement PCM). */

#define SOX_LOOP_NONE          ((unsigned char)sox_loop_none)          /**< Client API: single-shot = 0 */
#define SOX_LOOP_8             ((unsigned char)sox_loop_8)             /**< Client API: 8 loops = 32 */
#define SOX_LOOP_SUSTAIN_DECAY ((unsigned char)sox_loop_sustain_decay) /**< Client API: AIFF style, one sustain & one decay loop = 64 */

#define SOX_MAX_NLOOPS         8 /**< Client API: Maximum number of loops supported by sox_oob_t = 8. */

#define SOX_FILE_NOSTDIO 0x0001 /**< Client API: Does not use stdio routines */
#define SOX_FILE_DEVICE  0x0002 /**< Client API: File is an audio device */
#define SOX_FILE_PHONY   0x0004 /**< Client API: Phony file/device (for example /dev/null) */
#define SOX_FILE_REWIND  0x0008 /**< Client API: File should be rewound to write header */
#define SOX_FILE_BIT_REV 0x0010 /**< Client API: Is file bit-reversed? */
#define SOX_FILE_NIB_REV 0x0020 /**< Client API: Is file nibble-reversed? */
#define SOX_FILE_ENDIAN  0x0040 /**< Client API: Is file format endian? */
#define SOX_FILE_ENDBIG  0x0080 /**< Client API: For endian file format, is it big endian? */
#define SOX_FILE_MONO    0x0100 /**< Client API: Do channel restrictions allow mono? */
#define SOX_FILE_STEREO  0x0200 /**< Client API: Do channel restrictions allow stereo? */
#define SOX_FILE_QUAD    0x0400 /**< Client API: Do channel restrictions allow quad? */

#define SOX_FILE_CHANS   (SOX_FILE_MONO | SOX_FILE_STEREO | SOX_FILE_QUAD) /**< Client API: No channel restrictions */
#define SOX_FILE_LIT_END (SOX_FILE_ENDIAN | 0)                             /**< Client API: File is little-endian */
#define SOX_FILE_BIG_END (SOX_FILE_ENDIAN | SOX_FILE_ENDBIG)               /**< Client API: File is big-endian */

#define SOX_EFF_CHAN     1           /**< Client API: Effect might alter the number of channels */
#define SOX_EFF_RATE     2           /**< Client API: Effect might alter sample rate */
#define SOX_EFF_PREC     4           /**< Client API: Effect does its own calculation of output sample precision (otherwise a default value is taken, depending on the presence of SOX_EFF_MODIFY) */
#define SOX_EFF_LENGTH   8           /**< Client API: Effect might alter audio length (as measured in time units, not necessarily in samples) */
#define SOX_EFF_MCHAN    16          /**< Client API: Effect handles multiple channels internally */
#define SOX_EFF_NULL     32          /**< Client API: Effect does nothing (can be optimized out of chain) */
#define SOX_EFF_DEPRECATED 64        /**< Client API: Effect will soon be removed from SoX */
#define SOX_EFF_GAIN     128         /**< Client API: Effect does not support gain -r */
#define SOX_EFF_MODIFY   256         /**< Client API: Effect does not modify sample values (but might remove or duplicate samples or insert zeros) */
#define SOX_EFF_ALPHA    512         /**< Client API: Effect is experimental/incomplete */
#define SOX_EFF_INTERNAL 1024        /**< Client API: Effect present in libSoX but not valid for use by SoX command-line tools */

/**
Client API:
When used as the "whence" parameter of sox_seek, indicates that the specified
offset is relative to the beginning of the file.
*/
#define SOX_SEEK_SET 0

/*****************************************************************************
Forward declarations:
*****************************************************************************/

typedef struct sox_format_t sox_format_t;
typedef struct sox_effect_t sox_effect_t;
typedef struct sox_effect_handler_t sox_effect_handler_t;
typedef struct sox_format_handler_t sox_format_handler_t;

/*****************************************************************************
Function pointers:
*****************************************************************************/

/**
Client API:
Callback to write a message to an output device (console or log file),
used by sox_globals_t.output_message_handler.
*/
typedef void (LSX_API * sox_output_message_handler_t)(
    unsigned level,                       /**< 1 = FAIL, 2 = WARN, 3 = INFO, 4 = DEBUG, 5 = DEBUG_MORE, 6 = DEBUG_MOST. */
    LSX_PARAM_IN_Z char const * filename, /**< Source code __FILENAME__ from which message originates. */
    LSX_PARAM_IN_PRINTF char const * fmt, /**< Message format string. */
    LSX_PARAM_IN va_list ap               /**< Message format parameters. */
    );

/**
Client API:
Callback to retrieve information about a format handler,
used by sox_format_tab_t.fn.
@returns format handler information.
*/
typedef sox_format_handler_t const * (LSX_API * sox_format_fn_t)(void);

/**
Client API:
Callback to get information about an effect handler,
used by the table returned from sox_get_effect_fns(void).
@returns Pointer to information about an effect handler.
*/
typedef sox_effect_handler_t const * (LSX_API *sox_effect_fn_t)(void);

/**
Client API:
Callback to initialize reader (decoder), used by
sox_format_handler.startread.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_format_handler_startread)(
    LSX_PARAM_INOUT sox_format_t * ft /**< Format pointer. */
    );

/**
Client API:
Callback to read (decode) a block of samples,
used by sox_format_handler.read.
@returns number of samples read, or 0 if unsuccessful.
*/
typedef size_t (LSX_API * sox_format_handler_read)(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    LSX_PARAM_OUT_CAP_POST_COUNT(len,return) sox_sample_t *buf, /**< Buffer from which to read samples. */
    size_t len /**< Number of samples available in buf. */
    );

/**
Client API:
Callback to close reader (decoder),
used by sox_format_handler.stopread.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_format_handler_stopread)(
    LSX_PARAM_INOUT sox_format_t * ft /**< Format pointer. */
    );

/**
Client API:
Callback to initialize writer (encoder),
used by sox_format_handler.startwrite.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_format_handler_startwrite)(
    LSX_PARAM_INOUT sox_format_t * ft /**< Format pointer. */
    );

/**
Client API:
Callback to write (encode) a block of samples,
used by sox_format_handler.write.
@returns number of samples written, or 0 if unsuccessful.
*/
typedef size_t (LSX_API * sox_format_handler_write)(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    LSX_PARAM_IN_COUNT(len) sox_sample_t const * buf, /**< Buffer to which samples are written. */
    size_t len /**< Capacity of buf, measured in samples. */
    );

/**
Client API:
Callback to close writer (decoder),
used by sox_format_handler.stopwrite.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_format_handler_stopwrite)(
    LSX_PARAM_INOUT sox_format_t * ft /**< Format pointer. */
    );

/**
Client API:
Callback to reposition reader,
used by sox_format_handler.seek.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_format_handler_seek)(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    sox_uint64_t offset /**< Sample offset to which reader should be positioned. */
    );

/**
Client API:
Callback to parse command-line arguments (called once per effect),
used by sox_effect_handler.getopts.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_getopts)(
    LSX_PARAM_INOUT sox_effect_t * effp, /**< Effect pointer. */
    int argc, /**< Number of arguments in argv. */
    LSX_PARAM_IN_COUNT(argc) char *argv[] /**< Array of command-line arguments. */
    );

/**
Client API:
Callback to initialize effect (called once per flow),
used by sox_effect_handler.start.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_start)(
    LSX_PARAM_INOUT sox_effect_t * effp /**< Effect pointer. */
    );

/**
Client API:
Callback to process samples,
used by sox_effect_handler.flow.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_flow)(
    LSX_PARAM_INOUT sox_effect_t * effp, /**< Effect pointer. */
    LSX_PARAM_IN_COUNT(*isamp) sox_sample_t const * ibuf, /**< Buffer from which to read samples. */
    LSX_PARAM_OUT_CAP_POST_COUNT(*osamp,*osamp) sox_sample_t * obuf, /**< Buffer to which samples are written. */
    LSX_PARAM_INOUT size_t *isamp, /**< On entry, contains capacity of ibuf; on exit, contains number of samples consumed. */
    LSX_PARAM_INOUT size_t *osamp /**< On entry, contains capacity of obuf; on exit, contains number of samples written. */
    );

/**
Client API:
Callback to finish getting output after input is complete,
used by sox_effect_handler.drain.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_drain)(
    LSX_PARAM_INOUT sox_effect_t * effp, /**< Effect pointer. */
    LSX_PARAM_OUT_CAP_POST_COUNT(*osamp,*osamp) sox_sample_t *obuf, /**< Buffer to which samples are written. */
    LSX_PARAM_INOUT size_t *osamp /**< On entry, contains capacity of obuf; on exit, contains number of samples written. */
    );

/**
Client API:
Callback to shut down effect (called once per flow),
used by sox_effect_handler.stop.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_stop)(
    LSX_PARAM_INOUT sox_effect_t * effp /**< Effect pointer. */
    );

/**
Client API:
Callback to shut down effect (called once per effect),
used by sox_effect_handler.kill.
@returns SOX_SUCCESS if successful.
*/
typedef int (LSX_API * sox_effect_handler_kill)(
    LSX_PARAM_INOUT sox_effect_t * effp /**< Effect pointer. */
    );

/**
Client API:
Callback called while flow is running (called once per buffer),
used by sox_flow_effects.callback.
@returns SOX_SUCCESS to continue, other value to abort flow.
*/
typedef int (LSX_API * sox_flow_effects_callback)(
    sox_bool all_done,
    void * client_data
    );

/**
Client API:
Callback for enumerating the contents of a playlist,
used by the sox_parse_playlist function.
@returns SOX_SUCCESS if successful, any other value to abort playlist enumeration.
*/
typedef int (LSX_API * sox_playlist_callback_t)(
    void * callback_data,
    LSX_PARAM_IN_Z char const * filename
    );

/*****************************************************************************
Structures:
*****************************************************************************/

/**
Client API:
Information about a build of libSoX, returned from the sox_version_info
function.
*/
typedef struct sox_version_info_t {
    size_t       size;         /**< structure size = sizeof(sox_version_info_t) */
    sox_version_flags_t flags; /**< feature flags = popen | magic | threads | memopen */
    sox_uint32_t version_code; /**< version number = 0x140400 */
    char const * version;      /**< version string = sox_version(), for example, "14.4.0" */
    char const * version_extra;/**< version extra info or null = "PACKAGE_EXTRA", for example, "beta" */
    char const * time;         /**< build time = "__DATE__ __TIME__", for example, "Jan  7 2010 03:31:50" */
    char const * distro;       /**< distro or null = "DISTRO", for example, "Debian" */
    char const * compiler;     /**< compiler info or null, for example, "msvc 160040219" */
    char const * arch;         /**< arch, for example, "1248 48 44 L OMP" */
    /* new info should be added at the end for version backwards-compatibility. */
} sox_version_info_t;

/**
Client API:
Global parameters (for effects & formats), returned from the sox_get_globals
function.
*/
typedef struct sox_globals_t {
/* public: */
  unsigned     verbosity; /**< messages are only written if globals.verbosity >= message.level */
  sox_output_message_handler_t output_message_handler; /**< client-specified message output callback */
  sox_bool     repeatable; /**< true to use pre-determined timestamps and PRNG seed */

  /**
  Default size (in bytes) used by libSoX for blocks of sample data.
  Plugins should use similarly-sized buffers to get best performance.
  */
  size_t       bufsiz;

  /**
  Default size (in bytes) used by libSoX for blocks of input sample data.
  Plugins should use similarly-sized buffers to get best performance.
  */
  size_t       input_bufsiz;

  sox_int32_t  ranqd1; /**< Can be used to re-seed libSoX's PRNG */

  char const * stdin_in_use_by;  /**< Private: tracks the name of the handler currently using stdin */
  char const * stdout_in_use_by; /**< Private: tracks the name of the handler currently using stdout */
  char const * subsystem;        /**< Private: tracks the name of the handler currently writing an output message */
  char       * tmp_path;         /**< Private: client-configured path to use for temporary files */
  sox_bool     use_magic;        /**< Private: true if client has requested use of 'magic' file-type detection */
  sox_bool     use_threads;      /**< Private: true if client has requested parallel effects processing */

  /**
  Log to base 2 of minimum size (in bytes) used by libSoX for DFT (filtering).
  Plugins should use similarly-sized DFTs to get best performance.
  */
  size_t       log2_dft_min_size;
} sox_globals_t;

/**
Client API:
Signal parameters; members should be set to SOX_UNSPEC (= 0) if unknown.
*/
typedef struct sox_signalinfo_t {
  sox_rate_t       rate;         /**< samples per second, 0 if unknown */
  unsigned         channels;     /**< number of sound channels, 0 if unknown */
  unsigned         precision;    /**< bits per sample, 0 if unknown */
  sox_uint64_t     length;       /**< samples * chans in file, 0 if unknown, -1 if unspecified */
  double           * mult;       /**< Effects headroom multiplier; may be null */
} sox_signalinfo_t;

/**
Client API:
Basic information about an encoding.
*/
typedef struct sox_encodings_info_t {
  sox_encodings_flags_t flags; /**< lossy once (lossy1), lossy twice (lossy2), or lossless (none). */
  char const * name;           /**< encoding name. */
  char const * desc;           /**< encoding description. */
} sox_encodings_info_t;

/**
Client API:
Encoding parameters.
*/
typedef struct sox_encodinginfo_t {
  sox_encoding_t encoding; /**< format of sample numbers */
  unsigned bits_per_sample;/**< 0 if unknown or variable; uncompressed value if lossless; compressed value if lossy */
  double compression;      /**< compression factor (where applicable) */

  /**
  Should bytes be reversed? If this is default during sox_open_read or
  sox_open_write, libSoX will set them to either no or yes according to the
  machine or format default.
  */
  sox_option_t reverse_bytes;

  /**
  Should nibbles be reversed? If this is default during sox_open_read or
  sox_open_write, libSoX will set them to either no or yes according to the
  machine or format default.
  */
  sox_option_t reverse_nibbles;

  /**
  Should bits be reversed? If this is default during sox_open_read or
  sox_open_write, libSoX will set them to either no or yes according to the
  machine or format default.
  */
  sox_option_t reverse_bits;

  /**
  If set to true, the format should reverse its default endianness.
  */
  sox_bool opposite_endian;
} sox_encodinginfo_t;

/**
Client API:
Looping parameters (out-of-band data).
*/
typedef struct sox_loopinfo_t {
  sox_uint64_t  start;  /**< first sample */
  sox_uint64_t  length; /**< length */
  unsigned      count;  /**< number of repeats, 0=forever */
  unsigned char type;   /**< 0=no, 1=forward, 2=forward/back (see sox_loop_* for valid values). */
} sox_loopinfo_t;

/**
Client API:
Instrument information.
*/
typedef struct sox_instrinfo_t{
  signed char MIDInote;   /**< for unity pitch playback */
  signed char MIDIlow;    /**< MIDI pitch-bend low range */
  signed char MIDIhi;     /**< MIDI pitch-bend high range */
  unsigned char loopmode; /**< 0=no, 1=forward, 2=forward/back (see sox_loop_* values) */
  unsigned nloops;  /**< number of active loops (max SOX_MAX_NLOOPS). */
} sox_instrinfo_t;

/**
Client API:
File buffer info.  Holds info so that data can be read in blocks.
*/
typedef struct sox_fileinfo_t {
  char          *buf;                 /**< Pointer to data buffer */
  size_t        size;                 /**< Size of buffer in bytes */
  size_t        count;                /**< Count read into buffer */
  size_t        pos;                  /**< Position in buffer */
} sox_fileinfo_t;

/**
Client API:
Handler structure defined by each format.
*/
struct sox_format_handler_t {
  unsigned     sox_lib_version_code;  /**< Checked on load; must be 1st in struct*/
  char         const * description;   /**< short description of format */
  char         const * const * names; /**< null-terminated array of filename extensions that are handled by this format */
  unsigned int flags;                 /**< File flags (SOX_FILE_* values). */
  sox_format_handler_startread startread; /**< called to initialize reader (decoder) */
  sox_format_handler_read read;       /**< called to read (decode) a block of samples */
  sox_format_handler_stopread stopread; /**< called to close reader (decoder); may be null if no closing necessary */
  sox_format_handler_startwrite startwrite; /**< called to initialize writer (encoder) */
  sox_format_handler_write write;     /**< called to write (encode) a block of samples */
  sox_format_handler_stopwrite stopwrite; /**< called to close writer (decoder); may be null if no closing necessary */
  sox_format_handler_seek seek;       /**< called to reposition reader; may be null if not supported */

  /**
  Array of values indicating the encodings and precisions supported for
  writing (encoding). Precisions specified with default precision first.
  Encoding, precision, precision, ..., 0, repeat. End with one more 0.
  Example:
  unsigned const * formats = {
    SOX_ENCODING_SIGN2, 16, 24, 0, // Support SIGN2 at 16 and 24 bits, default to 16 bits.
    SOX_ENCODING_UNSIGNED, 8, 0,   // Support UNSIGNED at 8 bits, default to 8 bits.
    0 // No more supported encodings.
  };
  */
  unsigned     const * write_formats;

  /**
  Array of sample rates (samples per second) supported for writing (encoding).
  NULL if all (or almost all) rates are supported. End with 0.
  */
  sox_rate_t   const * write_rates;

  /**
  SoX will automatically allocate a buffer in which the handler can store data.
  Specify the size of the buffer needed here. Usually this will be sizeof(your_struct).
  The buffer will be allocated and zeroed before the call to startread/startwrite.
  The buffer will be freed after the call to stopread/stopwrite.
  The buffer will be provided via format.priv in each call to the handler.
  */
  size_t       priv_size;
};

/**
Client API:
Comments, instrument info, loop info (out-of-band data).
*/
typedef struct sox_oob_t{
  /* Decoded: */
  sox_comments_t   comments;              /**< Comment strings in id=value format. */
  sox_instrinfo_t  instr;                 /**< Instrument specification */
  sox_loopinfo_t   loops[SOX_MAX_NLOOPS]; /**< Looping specification */

  /* TBD: Non-decoded chunks, etc: */
} sox_oob_t;

/**
Client API:
Data passed to/from the format handler
*/
struct sox_format_t {
  char             * filename;      /**< File name */

  /**
  Signal specifications for reader (decoder) or writer (encoder):
  sample rate, number of channels, precision, length, headroom multiplier.
  Any info specified by the user is here on entry to startread or
  startwrite. Info will be SOX_UNSPEC if the user provided no info.
  At exit from startread, should be completely filled in, using
  either data from the file's headers (if available) or whatever
  the format is guessing/assuming (if header data is not available).
  At exit from startwrite, should be completely filled in, using
  either the data that was specified, or values chosen by the format
  based on the format's defaults or capabilities.
  */
  sox_signalinfo_t signal;

  /**
  Encoding specifications for reader (decoder) or writer (encoder):
  encoding (sample format), bits per sample, compression rate, endianness.
  Should be filled in by startread. Values specified should be used
  by startwrite when it is configuring the encoding parameters.
  */
  sox_encodinginfo_t encoding;

  char             * filetype;      /**< Type of file, as determined by header inspection or libmagic. */
  sox_oob_t        oob;             /**< comments, instrument info, loop info (out-of-band data) */
  sox_bool         seekable;        /**< Can seek on this file */
  char             mode;            /**< Read or write mode ('r' or 'w') */
  sox_uint64_t     olength;         /**< Samples * chans written to file */
  sox_uint64_t     clips;           /**< Incremented if clipping occurs */
  int              sox_errno;       /**< Failure error code */
  char             sox_errstr[256]; /**< Failure error text */
  void             * fp;            /**< File stream pointer */
  lsx_io_type      io_type;         /**< Stores whether this is a file, pipe or URL */
  sox_uint64_t     tell_off;        /**< Current offset within file */
  sox_uint64_t     data_start;      /**< Offset at which headers end and sound data begins (set by lsx_check_read_params) */
  sox_format_handler_t handler;     /**< Format handler for this file */
  void             * priv;          /**< Format handler's private data area */
};

/**
Client API:
Information about a loaded format handler, including the format name and a
function pointer that can be invoked to get additional information about the
format.
*/
typedef struct sox_format_tab_t {
  char *name;         /**< Name of format handler */
  sox_format_fn_t fn; /**< Function to call to get format handler's information */
} sox_format_tab_t;

/**
Client API:
Global parameters for effects.
*/
typedef struct sox_effects_globals_t {
  sox_plot_t plot;         /**< To help the user choose effect & options */
  sox_globals_t * global_info; /**< Pointer to associated SoX globals */
} sox_effects_globals_t;

/**
Client API:
Effect handler information.
*/
struct sox_effect_handler_t {
  char const * name;  /**< Effect name */
  char const * usage; /**< Short explanation of parameters accepted by effect */
  unsigned int flags; /**< Combination of SOX_EFF_* flags */
  sox_effect_handler_getopts getopts; /**< Called to parse command-line arguments (called once per effect). */
  sox_effect_handler_start start;     /**< Called to initialize effect (called once per flow). */
  sox_effect_handler_flow flow;       /**< Called to process samples. */
  sox_effect_handler_drain drain;     /**< Called to finish getting output after input is complete. */
  sox_effect_handler_stop stop;       /**< Called to shut down effect (called once per flow). */
  sox_effect_handler_kill kill;       /**< Called to shut down effect (called once per effect). */
  size_t       priv_size;             /**< Size of private data SoX should pre-allocate for effect */
};

/**
Client API:
Effect information.
*/
struct sox_effect_t {
  sox_effects_globals_t    * global_info; /**< global effect parameters */
  sox_signalinfo_t         in_signal;     /**< Information about the incoming data stream */
  sox_signalinfo_t         out_signal;    /**< Information about the outgoing data stream */
  sox_encodinginfo_t       const * in_encoding;  /**< Information about the incoming data encoding */
  sox_encodinginfo_t       const * out_encoding; /**< Information about the outgoing data encoding */
  sox_effect_handler_t     handler;   /**< The handler for this effect */
  sox_uint64_t         clips;         /**< increment if clipping occurs */
  size_t               flows;         /**< 1 if MCHAN, number of chans otherwise */
  size_t               flow;          /**< flow number */
  void                 * priv;        /**< Effect's private data area (each flow has a separate copy) */
  /* The following items are private to the libSoX effects chain functions. */
  sox_sample_t             * obuf;    /**< output buffer */
  size_t                   obeg;      /**< output buffer: start of valid data section */
  size_t                   oend;      /**< output buffer: one past valid data section (oend-obeg is length of current content) */
  size_t               imin;          /**< minimum input buffer content required for calling this effect's flow function; set via lsx_effect_set_imin() */
};

/**
Client API:
Chain of effects to be applied to a stream.
*/
typedef struct sox_effects_chain_t {
  sox_effect_t **effects;                  /**< Table of effects to be applied to a stream */
  size_t length;                           /**< Number of effects to be applied */
  sox_effects_globals_t global_info;       /**< Copy of global effects settings */
  sox_encodinginfo_t const * in_enc;       /**< Input encoding */
  sox_encodinginfo_t const * out_enc;      /**< Output encoding */
  /* The following items are private to the libSoX effects chain functions. */
  size_t table_size;                       /**< Size of effects table (including unused entries) */
  sox_sample_t *il_buf;                    /**< Channel interleave buffer */
} sox_effects_chain_t;

/*****************************************************************************
Functions:
*****************************************************************************/

/**
Client API:
Returns version number string of libSoX, for example, "14.4.0".
@returns The version number string of libSoX, for example, "14.4.0".
*/
LSX_RETURN_VALID_Z
char const *
LSX_API
sox_version(void);

/**
Client API:
Returns information about this build of libsox.
@returns Pointer to a version information structure.
*/
LSX_RETURN_VALID LSX_RETURN_PURE
sox_version_info_t const *
LSX_API
sox_version_info(void);

/**
Client API:
Returns a pointer to the structure with libSoX's global settings.
@returns a pointer to the structure with libSoX's global settings.
*/
LSX_RETURN_VALID LSX_RETURN_PURE
sox_globals_t *
LSX_API
sox_get_globals(void);

/**
Client API:
Deprecated macro that returns the structure with libSoX's global settings
as an lvalue.
*/
#define sox_globals (*sox_get_globals())

/**
Client API:
Returns a pointer to the list of available encodings.
End of list indicated by name == NULL.
@returns pointer to the list of available encodings.
*/
LSX_RETURN_ARRAY LSX_RETURN_PURE
sox_encodings_info_t const *
LSX_API
sox_get_encodings_info(void);

/**
Client API:
Deprecated macro that returns the list of available encodings.
End of list indicated by name == NULL.
*/
#define sox_encodings_info (sox_get_encodings_info())

/**
Client API:
Fills in an encodinginfo with default values.
*/
void
LSX_API
sox_init_encodinginfo(
    LSX_PARAM_OUT sox_encodinginfo_t * e /**< Pointer to uninitialized encoding info structure to be initialized. */
    );

/**
Client API:
Given an encoding (for example, SIGN2) and the encoded bits_per_sample (for
example, 16), returns the number of useful bits per sample in the decoded data
(for example, 16), or returns 0 to indicate that the value returned by the
format handler should be used instead of a pre-determined precision.
@returns the number of useful bits per sample in the decoded data (for example
16), or returns 0 to indicate that the value returned by the format handler
should be used instead of a pre-determined precision.
*/
LSX_RETURN_PURE
unsigned
LSX_API
sox_precision(
    sox_encoding_t encoding,   /**< Encoding for which to lookup precision information. */
    unsigned bits_per_sample   /**< The number of encoded bits per sample. */
    );

/**
Client API:
Returns the number of items in the metadata block.
@returns the number of items in the metadata block.
*/
size_t
LSX_API
sox_num_comments(
    LSX_PARAM_IN_OPT sox_comments_t comments /**< Metadata block. */
    );

/**
Client API:
Adds an "id=value" item to the metadata block.
*/
void
LSX_API
sox_append_comment(
    LSX_PARAM_DEREF_PRE_MAYBENULL LSX_PARAM_DEREF_POST_NOTNULL sox_comments_t * comments, /**< Metadata block. */
    LSX_PARAM_IN_Z char const * item /**< Item to be added in "id=value" format. */
    );

/**
Client API:
Adds a newline-delimited list of "id=value" items to the metadata block.
*/
void
LSX_API
sox_append_comments(
    LSX_PARAM_DEREF_PRE_MAYBENULL LSX_PARAM_DEREF_POST_NOTNULL sox_comments_t * comments, /**< Metadata block. */
    LSX_PARAM_IN_Z char const * items /**< Newline-separated list of items to be added, for example "id1=value1\\nid2=value2". */
    );

/**
Client API:
Duplicates the metadata block.
@returns the copied metadata block.
*/
LSX_RETURN_OPT
sox_comments_t
LSX_API
sox_copy_comments(
    LSX_PARAM_IN_OPT sox_comments_t comments /**< Metadata block to copy. */
    );

/**
Client API:
Frees the metadata block.
*/
void
LSX_API
sox_delete_comments(
    LSX_PARAM_DEREF_PRE_MAYBENULL LSX_PARAM_DEREF_POST_NULL sox_comments_t * comments /**< Metadata block. */
    );

/**
Client API:
If "id=value" is found, return value, else return null.
@returns value, or null if value not found.
*/
LSX_RETURN_OPT
char const *
LSX_API
sox_find_comment(
    LSX_PARAM_IN_OPT sox_comments_t comments, /**< Metadata block in which to search. */
    LSX_PARAM_IN_Z char const * id /**< Id for which to search */
    );

/**
Client API:
Find and load format handler plugins.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_format_init(void);

/**
Client API:
Unload format handler plugins.
*/
void
LSX_API
sox_format_quit(void);

/**
Client API:
Initialize effects library.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_init(void);

/**
Client API:
Close effects library and unload format handler plugins.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_quit(void);

/**
Client API:
Returns the table of format handler names and functions.
@returns the table of format handler names and functions.
*/
LSX_RETURN_ARRAY LSX_RETURN_PURE
sox_format_tab_t const *
LSX_API
sox_get_format_fns(void);

/**
Client API:
Deprecated macro that returns the table of format handler names and functions.
*/
#define sox_format_fns (sox_get_format_fns())

/**
Client API:
Opens a decoding session for a file. Returned handle must be closed with sox_close().
@returns The handle for the new session, or null on failure.
*/
LSX_RETURN_OPT
sox_format_t *
LSX_API
sox_open_read(
    LSX_PARAM_IN_Z   char               const * path,      /**< Path to file to be opened (required). */
    LSX_PARAM_IN_OPT sox_signalinfo_t   const * signal,    /**< Information already known about audio stream, or NULL if none. */
    LSX_PARAM_IN_OPT sox_encodinginfo_t const * encoding,  /**< Information already known about sample encoding, or NULL if none. */
    LSX_PARAM_IN_OPT_Z char             const * filetype   /**< Previously-determined file type, or NULL to auto-detect. */
    );

/**
Client API:
Opens a decoding session for a memory buffer. Returned handle must be closed with sox_close().
@returns The handle for the new session, or null on failure.
*/
LSX_RETURN_OPT
sox_format_t *
LSX_API
sox_open_mem_read(
    LSX_PARAM_IN_BYTECOUNT(buffer_size) void  * buffer,     /**< Pointer to audio data buffer (required). */
    size_t                                      buffer_size,/**< Number of bytes to read from audio data buffer. */
    LSX_PARAM_IN_OPT sox_signalinfo_t   const * signal,     /**< Information already known about audio stream, or NULL if none. */
    LSX_PARAM_IN_OPT sox_encodinginfo_t const * encoding,   /**< Information already known about sample encoding, or NULL if none. */
    LSX_PARAM_IN_OPT_Z char             const * filetype    /**< Previously-determined file type, or NULL to auto-detect. */
    );

/**
Client API:
Returns true if the format handler for the specified file type supports the specified encoding.
@returns true if the format handler for the specified file type supports the specified encoding.
*/
sox_bool
LSX_API
sox_format_supports_encoding(
    LSX_PARAM_IN_OPT_Z char               const * path,       /**< Path to file to be examined (required if filetype is NULL). */
    LSX_PARAM_IN_OPT_Z char               const * filetype,   /**< Previously-determined file type, or NULL to use extension from path. */
    LSX_PARAM_IN       sox_encodinginfo_t const * encoding    /**< Encoding for which format handler should be queried. */
    );

/**
Client API:
Gets the format handler for a specified file type.
@returns The found format handler, or null if not found.
*/
LSX_RETURN_OPT
sox_format_handler_t const *
LSX_API
sox_write_handler(
    LSX_PARAM_IN_OPT_Z char               const * path,         /**< Path to file (required if filetype is NULL). */
    LSX_PARAM_IN_OPT_Z char               const * filetype,     /**< Filetype for which handler is needed, or NULL to use extension from path. */
    LSX_PARAM_OUT_OPT  char               const * * filetype1   /**< Receives the filetype that was detected. Pass NULL if not needed. */
    );

/**
Client API:
Opens an encoding session for a file. Returned handle must be closed with sox_close().
@returns The new session handle, or null on failure.
*/
LSX_RETURN_OPT
sox_format_t *
LSX_API
sox_open_write(
    LSX_PARAM_IN_Z     char               const * path,     /**< Path to file to be written (required). */
    LSX_PARAM_IN       sox_signalinfo_t   const * signal,   /**< Information about desired audio stream (required). */
    LSX_PARAM_IN_OPT   sox_encodinginfo_t const * encoding, /**< Information about desired sample encoding, or NULL to use defaults. */
    LSX_PARAM_IN_OPT_Z char               const * filetype, /**< Previously-determined file type, or NULL to auto-detect. */
    LSX_PARAM_IN_OPT   sox_oob_t          const * oob,      /**< Out-of-band data to add to file, or NULL if none. */
    LSX_PARAM_IN_OPT   sox_bool           (LSX_API * overwrite_permitted)(LSX_PARAM_IN_Z char const * filename) /**< Called if file exists to determine whether overwrite is ok. */
    );

/**
Client API:
Opens an encoding session for a memory buffer. Returned handle must be closed with sox_close().
@returns The new session handle, or null on failure.
*/
LSX_RETURN_OPT
sox_format_t *
LSX_API
sox_open_mem_write(
    LSX_PARAM_OUT_BYTECAP(buffer_size) void                     * buffer,      /**< Pointer to audio data buffer that receives data (required). */
    LSX_PARAM_IN                       size_t                     buffer_size, /**< Maximum number of bytes to write to audio data buffer. */
    LSX_PARAM_IN                       sox_signalinfo_t   const * signal,      /**< Information about desired audio stream (required). */
    LSX_PARAM_IN_OPT                   sox_encodinginfo_t const * encoding,    /**< Information about desired sample encoding, or NULL to use defaults. */
    LSX_PARAM_IN_OPT_Z                 char               const * filetype,    /**< Previously-determined file type, or NULL to auto-detect. */
    LSX_PARAM_IN_OPT                   sox_oob_t          const * oob          /**< Out-of-band data to add to file, or NULL if none. */
    );

/**
Client API:
Opens an encoding session for a memstream buffer. Returned handle must be closed with sox_close().
@returns The new session handle, or null on failure.
*/
LSX_RETURN_OPT
sox_format_t *
LSX_API
sox_open_memstream_write(
    LSX_PARAM_OUT      char                     * * buffer_ptr,    /**< Receives pointer to audio data buffer that receives data (required). */
    LSX_PARAM_OUT      size_t                   * buffer_size_ptr, /**< Receives size of data written to audio data buffer (required). */
    LSX_PARAM_IN       sox_signalinfo_t   const * signal,          /**< Information about desired audio stream (required). */
    LSX_PARAM_IN_OPT   sox_encodinginfo_t const * encoding,        /**< Information about desired sample encoding, or NULL to use defaults. */
    LSX_PARAM_IN_OPT_Z char               const * filetype,        /**< Previously-determined file type, or NULL to auto-detect. */
    LSX_PARAM_IN_OPT   sox_oob_t          const * oob              /**< Out-of-band data to add to file, or NULL if none. */
    );

/**
Client API:
Reads samples from a decoding session into a sample buffer.
@returns Number of samples decoded, or 0 for EOF.
*/
size_t
LSX_API
sox_read(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    LSX_PARAM_OUT_CAP_POST_COUNT(len,return) sox_sample_t *buf, /**< Buffer from which to read samples. */
    size_t len /**< Number of samples available in buf. */
    );

/**
Client API:
Writes samples to an encoding session from a sample buffer.
@returns Number of samples encoded.
*/
size_t
LSX_API
sox_write(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    LSX_PARAM_IN_COUNT(len) sox_sample_t const * buf, /**< Buffer from which to read samples. */
    size_t len /**< Number of samples available in buf. */
    );

/**
Client API:
Closes an encoding or decoding session.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_close(
    LSX_PARAM_INOUT sox_format_t * ft /**< Format pointer. */
    );

/**
Client API:
Sets the location at which next samples will be decoded. Returns SOX_SUCCESS if successful.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_seek(
    LSX_PARAM_INOUT sox_format_t * ft, /**< Format pointer. */
    sox_uint64_t offset, /**< Sample offset at which to position reader. */
    int whence /**< Set to SOX_SEEK_SET. */
    );

/**
Client API:
Finds a format handler by name.
@returns Format handler data, or null if not found.
*/
LSX_RETURN_OPT
sox_format_handler_t const *
LSX_API
sox_find_format(
    LSX_PARAM_IN_Z char const * name, /**< Name of format handler to find. */
    sox_bool ignore_devices /**< Set to true to ignore device names. */
    );

/**
Client API:
Returns global parameters for effects
@returns global parameters for effects.
*/
LSX_RETURN_VALID LSX_RETURN_PURE
sox_effects_globals_t *
LSX_API
sox_get_effects_globals(void);

/**
Client API:
Deprecated macro that returns global parameters for effects.
*/
#define sox_effects_globals (*sox_get_effects_globals())

/**
Client API:
Finds the effect handler with the given name.
@returns Effect pointer, or null if not found.
*/
LSX_RETURN_OPT LSX_RETURN_PURE
sox_effect_handler_t const *
LSX_API
sox_find_effect(
    LSX_PARAM_IN_Z char const * name /**< Name of effect to find. */
    );

/**
Client API:
Creates an effect using the given handler.
@returns The new effect, or null if not found.
*/
LSX_RETURN_OPT
sox_effect_t *
LSX_API
sox_create_effect(
    LSX_PARAM_IN sox_effect_handler_t const * eh /**< Handler to use for effect. */
    );

/**
Client API:
Applies the command-line options to the effect.
@returns the number of arguments consumed.
*/
int
LSX_API
sox_effect_options(
    LSX_PARAM_IN sox_effect_t *effp, /**< Effect pointer on which to set options. */
    int argc, /**< Number of arguments in argv. */
    LSX_PARAM_IN_COUNT(argc) char * const argv[] /**< Array of command-line options. */
    );

/**
Client API:
Returns an array containing the known effect handlers.
@returns An array containing the known effect handlers.
*/
LSX_RETURN_VALID_Z LSX_RETURN_PURE
sox_effect_fn_t const *
LSX_API
sox_get_effect_fns(void);

/**
Client API:
Deprecated macro that returns an array containing the known effect handlers.
*/
#define sox_effect_fns (sox_get_effect_fns())

/**
Client API:
Initializes an effects chain. Returned handle must be closed with sox_delete_effects_chain().
@returns Handle, or null on failure.
*/
LSX_RETURN_OPT
sox_effects_chain_t *
LSX_API
sox_create_effects_chain(
    LSX_PARAM_IN sox_encodinginfo_t const * in_enc, /**< Input encoding. */
    LSX_PARAM_IN sox_encodinginfo_t const * out_enc /**< Output encoding. */
    );

/**
Client API:
Closes an effects chain.
*/
void
LSX_API
sox_delete_effects_chain(
    LSX_PARAM_INOUT sox_effects_chain_t *ecp /**< Effects chain pointer. */
    );

/**
Client API:
Adds an effect to the effects chain, returns SOX_SUCCESS if successful.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_add_effect(
    LSX_PARAM_INOUT sox_effects_chain_t * chain, /**< Effects chain to which effect should be added . */
    LSX_PARAM_INOUT sox_effect_t * effp, /**< Effect to be added. */
    LSX_PARAM_INOUT sox_signalinfo_t * in, /**< Input format. */
    LSX_PARAM_IN    sox_signalinfo_t const * out /**< Output format. */
    );

/**
Client API:
Runs the effects chain, returns SOX_SUCCESS if successful.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_flow_effects(
    LSX_PARAM_INOUT  sox_effects_chain_t * chain, /**< Effects chain to run. */
    LSX_PARAM_IN_OPT sox_flow_effects_callback callback, /**< Callback for monitoring flow progress. */
    LSX_PARAM_IN_OPT void * client_data /**< Data to pass into callback. */
    );

/**
Client API:
Gets the number of clips that occurred while running an effects chain.
@returns the number of clips that occurred while running an effects chain.
*/
sox_uint64_t
LSX_API
sox_effects_clips(
    LSX_PARAM_IN sox_effects_chain_t * chain /**< Effects chain from which to read clip information. */
    );

/**
Client API:
Shuts down an effect (calls stop on each of its flows).
@returns the number of clips from all flows.
*/
sox_uint64_t
LSX_API
sox_stop_effect(
    LSX_PARAM_INOUT_COUNT(effp->flows) sox_effect_t * effp /**< Effect to stop. */
    );

/**
Client API:
Adds an already-initialized effect to the end of the chain.
*/
void
LSX_API
sox_push_effect_last(
    LSX_PARAM_INOUT sox_effects_chain_t * chain, /**< Effects chain to which effect should be added. */
    LSX_PARAM_INOUT sox_effect_t * effp /**< Effect to be added. */
    );

/**
Client API:
Removes and returns an effect from the end of the chain.
@returns the removed effect, or null if no effects.
*/
LSX_RETURN_OPT
sox_effect_t *
LSX_API
sox_pop_effect_last(
    LSX_PARAM_INOUT sox_effects_chain_t *chain /**< Effects chain from which to remove an effect. */
    );

/**
Client API:
Shut down and delete an effect.
*/
void
LSX_API
sox_delete_effect(
    LSX_PARAM_INOUT_COUNT(effp->flows) sox_effect_t *effp /**< Effect to be deleted. */
    );

/**
Client API:
Shut down and delete the last effect in the chain.
*/
void
LSX_API
sox_delete_effect_last(
    LSX_PARAM_INOUT sox_effects_chain_t *chain /**< Effects chain from which to remove the last effect. */
    );

/**
Client API:
Shut down and delete all effects in the chain.
*/
void
LSX_API
sox_delete_effects(
    LSX_PARAM_INOUT sox_effects_chain_t *chain /**< Effects chain from which to delete effects. */
    );

/**
Client API:
Gets the sample offset of the start of the trim, useful for efficiently
skipping the part that will be trimmed anyway (get trim start, seek, then
clear trim start).
@returns the sample offset of the start of the trim.
*/
sox_uint64_t
LSX_API
sox_trim_get_start(
    LSX_PARAM_IN sox_effect_t * effp /**< Trim effect. */
    );

/**
Client API:
Clears the start of the trim to 0.
*/
void
LSX_API
sox_trim_clear_start(
    LSX_PARAM_INOUT sox_effect_t * effp /**< Trim effect. */
    );

/**
Client API:
Returns true if the specified file is a known playlist file type.
@returns true if the specified file is a known playlist file type.
*/
sox_bool
LSX_API
sox_is_playlist(
    LSX_PARAM_IN_Z char const * filename /**< Name of file to examine. */
    );

/**
Client API:
Parses the specified playlist file.
@returns SOX_SUCCESS if successful.
*/
int
LSX_API
sox_parse_playlist(
    LSX_PARAM_IN sox_playlist_callback_t callback, /**< Callback to call for each item in the playlist. */
    void * p, /**< Data to pass to callback. */
    LSX_PARAM_IN char const * const listname /**< Filename of playlist file. */
    );

/**
Client API:
Converts a SoX error code into an error string.
@returns error string corresponding to the specified error code,
or a generic message if the error code is not recognized.
*/
LSX_RETURN_VALID_Z LSX_RETURN_PURE
char const *
LSX_API
sox_strerror(
    int sox_errno /**< Error code to look up. */
    );

/**
Client API:
Gets the basename of the specified file; for example, the basename of
"/a/b/c.d" would be "c".
@returns the number of characters written to base_buffer, excluding the null,
or 0 on failure.
*/
size_t
LSX_API
sox_basename(
    LSX_PARAM_OUT_Z_CAP_POST_COUNT(base_buffer_len,return) char * base_buffer, /**< Buffer into which basename should be written. */
    size_t base_buffer_len, /**< Size of base_buffer, in bytes. */
    LSX_PARAM_IN_Z char const * filename /**< Filename from which to extract basename. */
    );

/*****************************************************************************
Internal API:
WARNING - The items in this section are subject to instability. They only
exist in the public header because sox (the application) currently uses them.
These may be changed or removed in future versions of libSoX.
*****************************************************************************/

/**
Plugins API:
Print a fatal error in libSoX.
*/
void
LSX_API
lsx_fail_impl(
    LSX_PARAM_IN_PRINTF char const * fmt, /**< printf-style format string. */
    ...)
    LSX_PRINTF12;

/**
Plugins API:
Print a warning in libSoX.
*/
void
LSX_API
lsx_warn_impl(
    LSX_PARAM_IN_PRINTF char const * fmt, /**< printf-style format string. */
    ...)
    LSX_PRINTF12;

/**
Plugins API:
Print an informational message in libSoX.
*/
void
LSX_API
lsx_report_impl(
    LSX_PARAM_IN_PRINTF char const * fmt, /**< printf-style format string. */
    ...)
    LSX_PRINTF12;

/**
Plugins API:
Print a debug message in libSoX.
*/
void
LSX_API
lsx_debug_impl(
    LSX_PARAM_IN_PRINTF char const * fmt, /**< printf-style format string. */
    ...)
    LSX_PRINTF12;

/**
Plugins API:
Report a fatal error in libSoX; printf-style arguments must follow.
*/
#define lsx_fail       sox_get_globals()->subsystem=__FILE__,lsx_fail_impl

/**
Plugins API:
Report a warning in libSoX; printf-style arguments must follow.
*/
#define lsx_warn       sox_get_globals()->subsystem=__FILE__,lsx_warn_impl

/**
Plugins API:
Report an informational message in libSoX; printf-style arguments must follow.
*/
#define lsx_report     sox_get_globals()->subsystem=__FILE__,lsx_report_impl

/**
Plugins API:
Report a debug message in libSoX; printf-style arguments must follow.
*/
#define lsx_debug      sox_get_globals()->subsystem=__FILE__,lsx_debug_impl

/**
Plugins API:
String name and integer values for enumerated types (type metadata), for use
with LSX_ENUM_ITEM, lsx_find_enum_text, and lsx_find_enum_value.
*/
typedef struct lsx_enum_item {
    char const *text; /**< String name of enumeration. */
    unsigned value;   /**< Integer value of enumeration. */
} lsx_enum_item;

/**
Plugins API:
Declares a static instance of an lsx_enum_item structure in format
{ "item", prefixitem }, for use in declaring lsx_enum_item[] arrays.
@param prefix The prefix to prepend to the item in the enumeration symbolic name.
@param item   The user-visible text name of the item (must also be a valid C symbol name).
*/
#define LSX_ENUM_ITEM(prefix, item) {#item, prefix##item},

/**
Plugins API:
Flags for use with lsx_find_enum_item.
*/
enum
{
    lsx_find_enum_item_none = 0, /**< Default parameters (case-insensitive). */
    lsx_find_enum_item_case_sensitive = 1 /**< Enable case-sensitive search. */
};

/**
Plugins API:
Looks up an enumeration by name in an array of lsx_enum_items.
@returns the corresponding item, or null if not found.
*/
LSX_RETURN_OPT LSX_RETURN_PURE
lsx_enum_item const *
LSX_API
lsx_find_enum_text(
    LSX_PARAM_IN_Z char const * text, /**< Name of enumeration to find. */
    LSX_PARAM_IN lsx_enum_item const * lsx_enum_items, /**< Array of items to search, with text == NULL for last item. */
    int flags /**< Search flags: 0 (case-insensitive) or lsx_find_enum_item_case_sensitive (case-sensitive). */
    );

/**
Plugins API:
Looks up an enumeration by value in an array of lsx_enum_items.
@returns the corresponding item, or null if not found.
*/
LSX_RETURN_OPT LSX_RETURN_PURE
lsx_enum_item const *
LSX_API
lsx_find_enum_value(
    unsigned value, /**< Enumeration value to find. */
    LSX_PARAM_IN lsx_enum_item const * lsx_enum_items /**< Array of items to search, with text == NULL for last item. */
    );

/**
Plugins API:
Looks up a command-line argument in a set of enumeration names, showing an
error message if the argument is not found in the set of names.
@returns The enumeration value corresponding to the matching enumeration, or
INT_MAX if the argument does not match any enumeration name.
*/
LSX_RETURN_PURE
int
LSX_API
lsx_enum_option(
    int c, /**< Option character to which arg is associated, for example with -a, c would be 'a'. */
    LSX_PARAM_IN_Z char const * arg, /**< Argument to find in enumeration list. */
    LSX_PARAM_IN lsx_enum_item const * items /**< Array of items to search, with text == NULL for last item. */
    );

/**
Plugins API:
Determines whether the specified string ends with the specified suffix (case-sensitive).
@returns true if the specified string ends with the specified suffix.
*/
LSX_RETURN_PURE
sox_bool
LSX_API
lsx_strends(
    LSX_PARAM_IN_Z char const * str, /**< String to search. */
    LSX_PARAM_IN_Z char const * end  /**< Suffix to search for. */
    );

/**
Plugins API:
Finds the file extension for a filename.
@returns the file extension, not including the '.', or null if filename does
not have an extension.
*/
LSX_RETURN_OPT LSX_RETURN_PURE
char const *
LSX_API
lsx_find_file_extension(
    LSX_PARAM_IN_Z char const * pathname /**< Filename to search for extension. */
    );

/**
Plugins API:
Formats the specified number with up to three significant figures and adds a
metric suffix in place of the exponent, such as 1.23G.
@returns A static buffer with the formatted number, valid until the next time
this function is called (note: not thread safe).
*/
LSX_RETURN_VALID_Z
char const *
LSX_API
lsx_sigfigs3(
    double number /**< Number to be formatted. */
    );

/**
Plugins API:
Formats the specified number as a percentage, showing up to three significant
figures.
@returns A static buffer with the formatted number, valid until the next time
this function is called (note: not thread safe).
*/
LSX_RETURN_VALID_Z
char const *
LSX_API
lsx_sigfigs3p(
    double percentage /**< Number to be formatted. */
    );

/**
Plugins API:
Allocates, deallocates, or resizes; like C's realloc, except that this version
terminates the running application if unable to allocate the requested memory.
@returns New buffer, or null if buffer was freed.
*/
LSX_RETURN_OPT
void *
LSX_API
lsx_realloc(
    LSX_PARAM_IN_OPT void *ptr, /**< Pointer to be freed or resized, or null if allocating a new buffer. */
    size_t newsize /**< New size for buffer, or 0 to free the buffer. */
    );

/**
Plugins API:
Like strcmp, except that the characters are compared without regard to case.
@returns 0 (s1 == s2), negative (s1 < s2), or positive (s1 > s2).
*/
LSX_RETURN_PURE
int
LSX_API
lsx_strcasecmp(
    LSX_PARAM_IN_Z char const * s1, /**< First string. */
    LSX_PARAM_IN_Z char const * s2  /**< Second string. */
    );


/**
Plugins API:
Like strncmp, except that the characters are compared without regard to case.
@returns 0 (s1 == s2), negative (s1 < s2), or positive (s1 > s2).
*/
LSX_RETURN_PURE
int
LSX_API
lsx_strncasecmp(
    LSX_PARAM_IN_Z char const * s1, /**< First string. */
    LSX_PARAM_IN_Z char const * s2, /**< Second string. */
    size_t n /**< Maximum number of characters to examine. */
    );

/**
Plugins API:
Is option argument unsupported, required, or optional.
*/
typedef enum lsx_option_arg_t {
    lsx_option_arg_none, /**< Option does not have an argument. */
    lsx_option_arg_required, /**< Option requires an argument. */
    lsx_option_arg_optional /**< Option can optionally be followed by an argument. */
} lsx_option_arg_t;

/**
Plugins API:
lsx_getopt_init options.
*/
typedef enum lsx_getopt_flags_t {
    lsx_getopt_flag_none = 0,      /**< no flags (no output, not long-only) */
    lsx_getopt_flag_opterr = 1,    /**< if set, invalid options trigger lsx_warn output */
    lsx_getopt_flag_longonly = 2   /**< if set, recognize -option as a long option */
} lsx_getopt_flags_t;

/**
Plugins API:
lsx_getopt long option descriptor.
*/
typedef struct lsx_option_t {
    char const *     name;    /**< Name of the long option. */
    lsx_option_arg_t has_arg; /**< Whether the long option supports an argument and, if so, whether the argument is required or optional. */
    int *            flag;    /**< Flag to set if argument is present. */
    int              val;     /**< Value to put in flag if argument is present. */
} lsx_option_t;

/**
Plugins API:
lsx_getopt session information (initialization data and state).
*/
typedef struct lsx_getopt_t {
    int                  argc;     /**< IN    argc:      Number of arguments in argv */
    char * const *       argv;     /**< IN    argv:      Array of arguments */
    char const *         shortopts;/**< IN    shortopts: Short option characters */
    lsx_option_t const * longopts; /**< IN    longopts:  Array of long option descriptors */
    lsx_getopt_flags_t   flags;    /**< IN    flags:     Flags for longonly and opterr */
    char const *         curpos;   /**< INOUT curpos:    Maintains state between calls to lsx_getopt */
    int                  ind;      /**< INOUT optind:    Maintains the index of next element to be processed */
    int                  opt;      /**< OUT   optopt:    Receives the option character that caused error */
    char const *         arg;      /**< OUT   optarg:    Receives the value of the option's argument */
    int                  lngind;   /**< OUT   lngind:    Receives the index of the matched long option or -1 if not a long option */
} lsx_getopt_t;

/**
Plugins API:
Initializes an lsx_getopt_t structure for use with lsx_getopt.
*/
void
LSX_API
lsx_getopt_init(
    LSX_PARAM_IN             int argc,                      /**< Number of arguments in argv */
    LSX_PARAM_IN_COUNT(argc) char * const * argv,           /**< Array of arguments */
    LSX_PARAM_IN_Z           char const * shortopts,        /**< Short options, for example ":abc:def::ghi" (+/- not supported) */
    LSX_PARAM_IN_OPT         lsx_option_t const * longopts, /**< Array of long option descriptors */
    LSX_PARAM_IN             lsx_getopt_flags_t flags,      /**< Flags for longonly and opterr */
    LSX_PARAM_IN             int first,                     /**< First argv to check (usually 1) */
    LSX_PARAM_OUT            lsx_getopt_t * state           /**< State object to be initialized */
    );

/**
Plugins API:
Gets the next option. Options are parameters that start with "-" or "--".
If no more options, returns -1. If unrecognized short option, returns '?'.
If a recognized short option is missing a required argument,
return (shortopts[0]==':' ? ':' : '?'). If successfully recognized short
option, return the recognized character. If successfully recognized long
option, returns (option.flag ? 0 : option.val).
Note: lsx_getopt does not permute the non-option arguments.
@returns option character (short), val or 0 (long), or -1 (no more).
*/
int
LSX_API
lsx_getopt(
    LSX_PARAM_INOUT lsx_getopt_t * state /**< The getopt state pointer. */
    );

/**
Plugins API:
Gets the file length, or 0 if the file is not seekable/normal.
@returns The file length, or 0 if the file is not seekable/normal.
*/
sox_uint64_t
LSX_API
lsx_filelength(
    LSX_PARAM_IN sox_format_t * ft
    );

/* WARNING END */

#if defined(__cplusplus)
}
#endif

#endif /* SOX_H */
