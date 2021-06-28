#ifndef __stdincludes_h_
#define __stdincludes_h_

/*
	This file exists to bring in the system headers and fix up anything
	that's wrong with them. Fun, eh?

	WINDOWS version.
*/

#if !defined( _MSC_VER ) || _MSC_VER < 1400
#error Microsoft Visual C++ 2005 or later required for the Windows build. 
#endif

#include <assert.h>
#include <math.h>

#if _MSC_VER == 1400
/*
	This is a workaround for a known bug in the MS version of <math.h>
	provided with Visual C++ 2005. The standard header has a semicolon
	at the end.
	
	*sigh*
*/
#if !defined (_M_IA64) && !defined (_M_AMD64)
#undef logf
#define logf(x) ((float)log((double)(x)))
#endif    // !defined (_M_IA64) && !defined (_M_AMD64)

#endif

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <setjmp.h>

#endif