// Utility functions for tests

#include <string.h>

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
  