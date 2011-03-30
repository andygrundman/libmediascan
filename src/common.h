#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <libmediascan.h>

#ifdef WIN32
#include "win32config.h"
#endif

#if __GNUC__ >= 4
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

extern enum log_level Debug;
extern long PathMax;

#define LOG_OUTPUT(...) fprintf(stdout, __VA_ARGS__)

#define LOG_LEVEL(level, ...) if (unlikely(Debug >= level)) fprintf(stderr, __VA_ARGS__)

#define LOG_ERROR(...) LOG_LEVEL(ERR, __VA_ARGS__)
#define LOG_WARN(...)  LOG_LEVEL(WARN, __VA_ARGS__)
#define LOG_INFO(...)  LOG_LEVEL(INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_LEVEL(DEBUG, __VA_ARGS__)
#define LOG_MEM(...)   LOG_LEVEL(MEMORY, __VA_ARGS__)
#define FATAL(...)     LOG_LEVEL(ERR, __VA_ARGS__);

#endif // _COMMON_H
