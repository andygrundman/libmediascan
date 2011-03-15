
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"

void croak(char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	 
	printf(fmt, argptr);
    va_end(argptr);
	exit(-1);
};