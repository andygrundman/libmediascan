///-------------------------------------------------------------------------------------------------
// file:	win32_port.c
//
// summary:	window 32 port class
///-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"

///-------------------------------------------------------------------------------------------------
/// <summary>	Ends the program while outputting a final string to the console. </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="fmt">	[in] parameter list like printf. </param>
///-------------------------------------------------------------------------------------------------

void croak(char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	 
	printf(fmt, argptr);
    va_end(argptr);
	exit(-1);
};