#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>

#include "config.h"

#if __GNUC__ >= 4
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

extern int Debug;

#define LOG_LEVEL(level, ...) if (unlikely(Debug >= level)) fprintf(stderr, __VA_ARGS__)

#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_WARN(...)  fprintf(stderr, __VA_ARGS__)
#define FATAL(...)     fprintf(stderr, __VA_ARGS__); exit(-1);

#ifdef DEBUG
# define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
# define LOG_DEBUG(...)
#endif

#endif // _COMMON_H
