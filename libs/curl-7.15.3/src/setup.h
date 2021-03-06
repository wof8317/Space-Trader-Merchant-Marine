#ifndef __SRC_CURL_SETUP_H
#define __SRC_CURL_SETUP_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2005, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: setup.h,v 1.39 2005/12/17 20:37:54 yangtse Exp $
 ***************************************************************************/

#define CURL_NO_OLDIES

#if !defined(WIN32) && defined(__WIN32__)
/* Borland fix */
#define WIN32
#endif

#if !defined(WIN32) && defined(_WIN32)
/* VS2005 on x64 fix */
#define WIN32
#endif

/*
 * Include configuration script results or hand-crafted
 * configuration file for platforms which lack config tool.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else

#ifdef WIN32
#include "config-win32.h"
#endif

#ifdef macintosh
#include "config-mac.h"
#endif

#ifdef __riscos__
#include "config-riscos.h"
#endif

#ifdef __amigaos__
#include "config-amigaos.h"
#endif

#endif /* HAVE_CONFIG_H */

#if defined(CURLDEBUG) && defined(CURLTOOLDEBUG)
/* This is an ugly hack for CURLDEBUG conditions only. We need to include
   the file here, since it might set the _FILE_OFFSET_BITS define, which must
   be set BEFORE all normal system headers. */
#include "../lib/setup.h"
#endif

/* 
 * Include header files for windows builds before redefining anything.
 * Use this preproessor block only to include or exclude windows.h, 
 * winsock2.h, ws2tcpip.h or winsock.h. Any other windows thing belongs 
 * to any other further and independant block.
 */

#ifdef HAVE_WINDOWS_H
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  ifdef HAVE_WINSOCK2_H
#    include <winsock2.h>
#    ifdef HAVE_WS2TCPIP_H
#       include <ws2tcpip.h>
#    endif
#  else
#    ifdef HAVE_WINSOCK_H
#      include <winsock.h>
#    endif
#  endif
#endif

#include <stdio.h>

#ifdef __TANDEM
#include <floss.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if !defined(__cplusplus) && !defined(__BEOS__) && !defined(typedef_bool)
typedef unsigned char bool;
#define typedef_bool
#endif

#ifndef OS
#define OS "unknown"
#endif

#if !defined(fileno) && !defined(WIN32) /* sunos 4 have this as a macro! */
int fileno( FILE *stream);
#endif

#ifdef WIN32
#define DIR_CHAR      "\\"
#define DOT_CHAR      "_"
#else
#ifdef __EMX__
/* 20000318 mgs
 * OS/2 supports leading dots in filenames if the volume is formatted
 * with JFS or HPFS. */
#define DIR_CHAR      "\\"
#define DOT_CHAR      "."
#else

#ifdef DJGPP
#include <tcp.h>
#ifdef word
#undef word
#endif
#ifndef HAVE_LIMITS_H
#define HAVE_LIMITS_H /* we have limits.h */
#endif
#define DIR_CHAR      "/"
#define DOT_CHAR      "_"
#else

#define DIR_CHAR      "/"
#define DOT_CHAR      "."

#endif /* !DJGPP */
#endif /* !__EMX__ */
#endif /* !WIN32 */

#ifdef __riscos__
#define USE_ENVIRONMENT
#endif

#ifdef __BEOS__
#define typedef_bool
#endif

#ifndef SIZEOF_CURL_OFF_T
#define SIZEOF_CURL_OFF_T sizeof(curl_off_t)
#endif

#endif /* __SRC_CURL_SETUP_H */
