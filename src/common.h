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


#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define LOG_OUTPUT(...) fprintf(stdout, __VA_ARGS__)

#define LOG_LEVEL(level, ...) if (unlikely(Debug >= level)) fprintf(stderr, __VA_ARGS__)

#define LOG_ERROR(...) LOG_LEVEL(ERR, __VA_ARGS__)
#define LOG_WARN(...)  LOG_LEVEL(WARN, __VA_ARGS__)
#define LOG_INFO(...)  LOG_LEVEL(INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_LEVEL(DEBUG, __VA_ARGS__)
#define LOG_MEM(...)   LOG_LEVEL(MEMORY, __VA_ARGS__)
#define FATAL(...)     LOG_LEVEL(ERR, __VA_ARGS__);

#define BUF_SIZE 4096

// Define if running under tcmalloc
#define USING_TCMALLOC

// Define to have ms_dump_result write out thumbnail images
#define DUMP_THUMBNAILS

#endif // _COMMON_H
