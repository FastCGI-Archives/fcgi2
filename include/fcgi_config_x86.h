/* include/fcgi_config.h.  Generated automatically by configure.  */
/* include/fcgi_config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define to type of ssize_t, if not on this platform.  */
#define ssize_t int

/* Define if you have the <sys/select.h> include file */
/* #undef HAVE_SYS_SELECT_H */

/* Define if you don't have fd_set */
#define NO_FD_SET 1

/* Define if want to compile with ASSERT() statements */
#define WITH_ASSERT 1

/* Define if want to compile with additional debugging code */
#define WITH_DEBUG 1

/* Define if want to compile with hooks for testing */
#define WITH_TEST 1

/* Define if sockaddr_un in <sys/un.h> contains the sun_len component */
/* #undef HAVE_SOCKADDR_UN_SUN_LEN */

/* Define if we have f{set,get}pos functions */
#define HAVE_FPOS 1

/* Define if we need cross-process locking */
/* #undef USE_LOCKING */

/* Define if va_arg(arg, long double) crashes the compiler. */
/* #undef HAVE_VA_ARG_LONG_DOUBLE_BUG */

/* Don't know what this stuff is for */
#define HAVE_MATHLIB 1
/* #undef WITH_DOMESTIC */
/* #undef WITH_EXPORT */
/* #undef WITH_GLOBAL */

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a off_t.  */
#define SIZEOF_OFF_T 0

/* The number of bytes in a size_t.  */
#define SIZEOF_SIZE_T 4

/* The number of bytes in a unsigned int.  */
#define SIZEOF_UNSIGNED_INT 4

/* The number of bytes in a unsigned long.  */
#define SIZEOF_UNSIGNED_LONG 4

/* The number of bytes in a unsigned short.  */
#define SIZEOF_UNSIGNED_SHORT 2

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <unistd.h> header file.  */
/* #undef HAVE_UNISTD_H */

/* Define if you have the <netdb.h> header file.  */
/* #undef HAVE_NETDB_H */

/* Define if you have the <netinet/in.h> header file.  */
/* #undef HAVE_NETINET_IN_H */

/* Define if you have the <windows.h> header file.  */
#define HAVE_WINDOWS_H 1

/* Define if you have the <winsock.h> header file.  */
#define HAVE_WINSOCK_H 1

/* Define if you have the <sys/socket.h> header file.  */
/* #undef HAVE_SYS_SOCKET_H */

/* Define if you have the <strings.h> header file.  */
/* #undef HAVE_STRINGS_H */

/* Define if you have the <sys/time.h> header file.  */
/* #undef HAVE_SYS_TIME_H */

