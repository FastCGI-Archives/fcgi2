# $Id: Makefile.am,v 1.8 2001/12/21 03:12:48 robs Exp $

INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES = $(INCLUDEDIR)/fastcgi.h     \
                $(INCLUDEDIR)/fcgiapp.h     \
                $(INCLUDEDIR)/fcgimisc.h    \
                $(INCLUDEDIR)/fcgi_stdio.h  \
                $(INCLUDEDIR)/fcgios.h

lib_LTLIBRARIES   = libfcgi.la @LIBFCGIXX@
EXTRA_LTLIBRARIES = libfcgi++.la

libfcgi_la_SOURCES = $(INCLUDE_FILES)  \
                     fcgiapp.c         \
                     fcgi_stdio.c      \
                     os_@SYSTEM@.c

libfcgi_la_CC      = @PTHREAD_CC@
libfcgi_la_CFLAGS  = @PTHREAD_CFLAGS@

libfcgi___la_SOURCES = $(INCLUDE_FILES) $(INCLUDEDIR)/fcgio.h fcgio.cpp
libfcgi___la_LDFLAGS = -lfcgi -rpath @libdir@

