# $Id: Makefile.am,v 1.5 2001/11/20 03:17:53 robs Exp $
INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES = $(INCLUDEDIR)/fastcgi.h    \
		$(INCLUDEDIR>/fcgio.h	\
                $(INCLUDEDIR)/fcgiapp.h    \
                $(INCLUDEDIR)/fcgimisc.h   \
                $(INCLUDEDIR)/fcgi_stdio.h \
                $(INCLUDEDIR)/fcgios.h

lib_LTLIBRARIES = libfcgi.la
libfcgi_la_SOURCES = $(INCLUDE_FILES)    \
		fcgiapp.c	    \
		fcgi_stdio.c	    \
		os_@SYSTEM@.c
EXTRA_libfcgi_la_SOURCES = \
		fcgio.cpp	\
		os_unix.c   \
		os_win32.c

libfcgi_la_LIBADD = fcgio.lo

# This is only needed if fcgio.cpp is compiled in (CXX is defined)
libfcgi_la_LDFLAGS = -lstdc++

