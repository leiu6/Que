#ifndef QUE_COMMON_H
#define QUE_COMMON_H

/**
 * This file contains common definitions that are used across the Que
 * interpreter. These definitions may need to be edited to work on your
 * specific platform.
 */

/* Common headers */
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

typedef unsigned char Que_Byte;

#define QUE_BYTE_MIN (0x00)
#define QUE_BYTE_MAX (0xFF)

typedef unsigned short Que_Word;

#define QUE_WORD_MIN (0x0000)
#define QUE_WORD_MAX (0xFFFF)

typedef long int Que_Int;
typedef double Que_Float;

#define QUE_TRUE 1
#define QUE_FALSE 0

#ifdef __GNUC__
#        define QUE_NORETURN __attribute__((noreturn))
#elif __clang__
#        define QUE_NORETURN __attribute__((noreturn))
#elif _MSC_VER
#        define QUE_NORETURN __attribute__((noreturn))
#else
#        define QUE_NORETURN
#endif

#endif /* QUE_COMMON_H */
