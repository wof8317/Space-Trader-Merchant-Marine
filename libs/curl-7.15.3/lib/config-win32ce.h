#ifndef __LIB_CONFIG_WIN32CE_H
#define __LIB_CONFIG_WIN32CE_H

/* ================================================================ */
/*  lib/config-win32ce.h - Hand crafted config file for windows ce  */
/* ================================================================ */

/* ---------------------------------------------------------------- */
/*                          HEADER FILES                            */
/* ---------------------------------------------------------------- */

/* Define if you have the <alloca.h> header file.  */
/* #define HAVE_ALLOCA_H 1 */

/* Define if you have the <arpa/inet.h> header file.  */
/* #define HAVE_ARPA_INET_H 1 */

/* Define if you have the <assert.h> header file.  */
/* #define HAVE_ASSERT_H 1 */

/* Define if you have the <crypto.h> header file.  */
/* #define HAVE_CRYPTO_H 1 */

/* Define if you have the <dlfcn.h> header file.  */
/* #define HAVE_DLFCN_H 1 */

/* Define if you have the <err.h> header file.  */
/* #define HAVE_ERR_H 1 */

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <getopt.h> header file.  */
/* #define HAVE_GETOPT_H 1 */

/* Define if you have the <io.h> header file.  */
#define HAVE_IO_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <netdb.h> header file.  */
/* #define HAVE_NETDB_H 1 */

/* Define if you have the <netinet/in.h> header file.  */
/* #define HAVE_NETINET_IN_H 1 */

/* Define if you have the <sgtty.h> header file.  */
/* #define HAVE_SGTTY_H 1 */

/* Define if you have the <ssl.h> header file.  */
/* #define HAVE_SSL_H 1 */

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <sys/param.h> header file.  */
/* #define HAVE_SYS_PARAM_H 1 */

/* Define if you have the <sys/select.h> header file.  */
/* #define HAVE_SYS_SELECT_H 1 */

/* Define if you have the <sys/socket.h> header file.  */
/* #define HAVE_SYS_SOCKET_H 1 */

/* Define if you have the <sys/sockio.h> header file.  */
/* #define HAVE_SYS_SOCKIO_H 1 */

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/types.h> header file.  */
/* #define HAVE_SYS_TYPES_H 1 */

/* Define if you have the <sys/utime.h> header file */
#define HAVE_SYS_UTIME_H 1

/* Define if you have the <termio.h> header file.  */
/* #define HAVE_TERMIO_H 1 */

/* Define if you have the <termios.h> header file.  */
/* #define HAVE_TERMIOS_H 1 */

/* Define if you have the <time.h> header file.  */
#define HAVE_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#if defined(__MINGW32__) || defined(__WATCOMC__) || defined(__LCC__)
#define HAVE_UNISTD_H 1
#endif

/* Define if you have the <windows.h> header file.  */
#define HAVE_WINDOWS_H 1

/* Define if you have the <winsock.h> header file.  */
#define HAVE_WINSOCK_H 1

/* Define if you have the <winsock2.h> header file.  */
/* #define HAVE_WINSOCK2_H 1 */

/* Define if you have the <ws2tcpip.h> header file.  */
/* #define HAVE_WS2TCPIP_H 1 */

/* ---------------------------------------------------------------- */
/*                        OTHER HEADER INFO                         */
/* ---------------------------------------------------------------- */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
/* #define TIME_WITH_SYS_TIME 1 */

/* ---------------------------------------------------------------- */
/*                             FUNCTIONS                            */
/* ---------------------------------------------------------------- */

/* Define if you have the closesocket function.  */
#define HAVE_CLOSESOCKET 1

/* Define if you don't have vprintf but do have _doprnt.  */
/* #define HAVE_DOPRNT 1 */

/* Define if you have the gethostbyaddr function.  */
#define HAVE_GETHOSTBYADDR 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the getpass function.  */
/* #define HAVE_GETPASS 1 */

/* Define if you have the getservbyname function.  */
#define HAVE_GETSERVBYNAME 1

/* Define if you have the gettimeofday function.  */
/*  #define HAVE_GETTIMEOFDAY 1 */

/* Define if you have the inet_addr function.  */
#define HAVE_INET_ADDR 1

/* Define if you have the inet_ntoa function.  */
#define HAVE_INET_NTOA 1

/* Define if you have the ioctlsocket function.  */
#define HAVE_IOCTLSOCKET 1

/* Define if you have the perror function.  */
#define HAVE_PERROR 1

/* Define if you have the RAND_screen function when using SSL  */
#define HAVE_RAND_SCREEN 1

/* Define if you have the `RAND_status' function when using SSL. */
#define HAVE_RAND_STATUS 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setvbuf function.  */
#define HAVE_SETVBUF 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strcasecmp function.  */
/* #define HAVE_STRCASECMP 1 */

/* Define if you have the stricmp function.  */
/* #define HAVE_STRICMP 1 */

/* Define if you have the strdup function.  */
/* #define HAVE_STRDUP 1 */

/* Define if you have the strftime function.  */
/* #define HAVE_STRFTIME 1 */

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtoll function.  */
#if defined(__MINGW32__) || defined(__WATCOMC__)
#define HAVE_STRTOLL 1
#endif

/* Define if you have the tcgetattr function.  */
/* #define HAVE_TCGETATTR 1 */

/* Define if you have the tcsetattr function.  */
/* #define HAVE_TCSETATTR 1 */

/* Define if you have the uname function.  */
/* #define HAVE_UNAME 1 */

/* Define if you have the utime function */
#define HAVE_UTIME 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* ---------------------------------------------------------------- */
/*                       TYPEDEF REPLACEMENTS                       */
/* ---------------------------------------------------------------- */

/* Define this if in_addr_t is not an available 'typedefed' type */
#define in_addr_t unsigned long

/* Define as the return type of signal handlers (int or void).  */
/* #define RETSIGTYPE void */

/* Define to `unsigned' if size_t is not an available 'typedefed' type */
/* #define size_t unsigned */

/* Define to 'int' if ssize_t is not an available 'typedefed' type */
#define ssize_t int

/* Define to 'int' if socklen_t is not an available 'typedefed' type */
#ifndef HAVE_WS2TCPIP_H
#define socklen_t int
#endif

/* ---------------------------------------------------------------- */
/*                            TYPE SIZES                            */
/* ---------------------------------------------------------------- */

/* The number of bytes in a long double.  */
#define SIZEOF_LONG_DOUBLE 16

/* The number of bytes in a long long.  */
/* #define SIZEOF_LONG_LONG 8 */

/* Undef SIZEOF_CURL_OFF_T if already defined. */
#ifdef SIZEOF_CURL_OFF_T
#undef SIZEOF_CURL_OFF_T
#endif

/* Define SIZEOF_CURL_OFF_T as computed by sizeof(curl_off_t) */
#define SIZEOF_CURL_OFF_T 4

/* ---------------------------------------------------------------- */
/*                          STRUCT RELATED                          */
/* ---------------------------------------------------------------- */

/* Define this if you have struct sockaddr_storage */
/* #define HAVE_STRUCT_SOCKADDR_STORAGE 1 */

/* ---------------------------------------------------------------- */
/*                        COMPILER SPECIFIC                         */
/* ---------------------------------------------------------------- */

/* Undef keyword 'const' if it does not work.  */
/* #undef const */

/* ---------------------------------------------------------------- */
/*                        LDAP LIBRARY FILES                        */
/* ---------------------------------------------------------------- */

/* lber dynamic library file */
/* #define DL_LBER_FILE */

/* ldap dynamic library file */
/* #define DL_LDAP_FILE "wldap32.dll" */

/* ---------------------------------------------------------------- */
/*                       ADDITIONAL DEFINITIONS                     */
/* ---------------------------------------------------------------- */

/* Defines set for VS2005 to _not_ deprecate a few functions we use. */
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

/* Define cpu-machine-OS */
#undef OS
#define OS "i386-pc-win32ce"

/* Name of package */
#define PACKAGE "curl"

/* ---------------------------------------------------------------- */
/*                               WinCE                              */
/* ---------------------------------------------------------------- */

#define CURL_DISABLE_FILE 1
#define CURL_DISABLE_TELNET 1
#define CURL_DISABLE_LDAP 1
#define WITHOUT_MM_LIB 1

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
#  include <process.h>
#endif

#define ENOSPC 1
#define ENOMEM 2
#define EAGAIN 3

extern int stat(const char *path,struct stat *buffer );

#endif /* __LIB_CONFIG_WIN32CE_H */
