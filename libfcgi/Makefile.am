# $Id: Makefile.am,v 1.7 2001/11/23 14:39:39 robs Exp $
INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES = $(INCLUDEDIR)/fastcgi.h    \
                $(INCLUDEDIR)/fcgiapp.h    \
                $(INCLUDEDIR)/fcgimisc.h   \
                $(INCLUDEDIR)/fcgi_stdio.h \
                $(INCLUDEDIR)/fcgios.h

lib_LTLIBRARIES   = libfcgi.la @LIBFCGIXX@
EXTRA_LTLIBRARIES = libfcgi++.la

libfcgi_la_SOURCES = $(INCLUDE_FILES)    \
		fcgiapp.c	    \
		fcgi_stdio.c	    \
		os_@SYSTEM@.c

EXTRA_libfcgi_la_SOURCES = \
		os_unix.c  \
		os_win32.c

libfcgi___la_SOURCES = $(INCLUDE_FILES) $(INCLUDEDIR)/fcgio.h fcgio.cpp
libfcgi___la_LDFLAGS = -lfcgi -rpath @libdir@

