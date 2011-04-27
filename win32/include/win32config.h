#ifndef CONFIG_H
#define CONFIG_H

#undef inline
#define inline _inline

#define strcasecmp stricmp


void croak(char *fmt, ...);

#endif