#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#define DEBUG 1

#if DEBUG
# define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
# define LOG_DEBUG(...)
#endif