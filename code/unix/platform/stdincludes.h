#ifndef __stdincludes_h_
#define __stdincludes_h_

/*
	This file exists to bring in the system headers and fix up anything
	that's wrong with them. Fun, eh?

	LINUX version.
*/

#ifndef __GNUC__
#error GCC specific stuff here.
#endif

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
//min, and max are non ansi-c
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#endif
