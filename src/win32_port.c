///-------------------------------------------------------------------------------------------------
// file:	win32_port.c
//
// summary:	window 32 port class
///-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include < time.h >
#include <Windows.h>

#include "win32config.h"

///-------------------------------------------------------------------------------------------------
///  Ends the program while outputting a final string to the console.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in]	 fmt parameter list like printf.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void croak(char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	 
	printf(fmt, argptr);
    va_end(argptr);
	//exit(-1);
};

