#include <stdio.h>
#include <stddef.h>
#include <sys/param.h>
#define HAVE_SIZE_T

#define MOP_HAVE_INT64

#define INT8 int8_t
#define UINT8 uint8_t
#define INT16 int16_t
#define UINT16 uint16_t
#define INT32 int32_t
#define UINT32 uint32_t
#define INT64 int64_t
#define UINT64 uint64_t

typedef intptr_t INTPTR;

typedef unsigned short WCHAR;

#define SCHAR char
#define WCHAR WCHAR

#undef BOOL
#undef TRUE
#undef FALSE

/* Using proper bool breaks code like BOOL x:8; (Reksio has that...) */
#if 0
#include <stdbool.h>
#define BOOL bool
#define TRUE true
#define FALSE false
#else
#define BOOL unsigned
#define TRUE 1
#define FALSE 0
#endif

#include <stdlib.h>
#define mop_malloc malloc
#define mop_calloc calloc
#define mop_free free
#define mop_realloc realloc

#define bream_malloc malloc
#define bream_calloc calloc
#define bream_free free
#define bream_realloc realloc

// FIXME Android endian detection. Mips BE?
#define MOP_LITTLEENDIAN
#define MOP_IMAGE_BGRA_BYTEORDER

#define MOP_C99_VARIADIC_MACROS

#ifndef NDEBUG
# include <android/log.h>
# define MOP_ASSERT(expr) (((expr) ? (void)0 : __android_log_assert(#expr, "ASSERT", "Assertion failed at %s:%d: %s", __FILE__, __LINE__, #expr)))
# define MOP_STATIC_ASSERT(expr) { int static_assert_error[ (expr)? 1: -1]; static_assert_error; }
#else
# define MOP_ASSERT(expr) ((void)sizeof(expr))
# define MOP_STATIC_ASSERT(expr)
#endif
# define BREAM_ASSERT MOP_ASSERT

#include "core/encoding/encodings.h"

#define SYSTEM_ENCODING ENCODING_UTF8

#define REKSIO_ANDROID_STORAGE_PREFIX "mopstorage"
#define MOP_STORAGE_DIR REKSIO_ANDROID_STORAGE_PREFIX":"

#define REKSIO_ANDROID_PRESTORAGE_PREFIX "mopprestorage"
#define MOP_PRESTORAGE_DIR REKSIO_ANDROID_PRESTORAGE_PREFIX":"
