// Utility functions for tests

#include <stdio.h>
#include <string.h>

// If we are on MSVC, disable some stupid MSVC warnings
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif

#undef LOG_WARN
#define LOG_WARN(...)  fprintf(stderr, __VA_ARGS__)

#undef LOG_ERROR
#define LOG_ERROR LOG_WARN

static char *
_findbin(const char *cmd)
{
  char *buf;
  
  if (cmd[0] == '/') {
    buf = strdup(cmd);
  }
  else {
    buf = (char *)malloc(1024);
    getcwd(buf, 1024);
    strcat(buf, "/");
    strcat(buf, cmd);
  }
  
  return buf;
}

static char *
_abspath(const char *bin, const char *path)
{
  char *buf = (char *)malloc(1024);
  char *last_slash = strrchr(bin, '/') + 1;
  
  char *s = (char *)&bin[0];
  char *d = &buf[0];
  while (s != last_slash)
    *d++ = *s++;
  
  strcat(buf, path);
  
  return buf;
}

#if (defined(__APPLE__) && defined(__MACH__))

#define LINK_NONE 		0
#define LINK_ALIAS 		1
#define LINK_SYMLINK 	2


int isAlias(const char *incoming_path);
int CheckMacAlias(const char *incoming_path, char *out_path);

#endif
