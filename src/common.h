#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>

#include "config.h"

#ifdef __GNUC__
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

static int Debug = 0;

#define LOG_LEVEL(level, ...) if (unlikely(Debug >= level)) fprintf(stderr, __VA_ARGS__)

#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_WARN(...)  fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
# define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
# define LOG_DEBUG(...)
#endif

#endif
