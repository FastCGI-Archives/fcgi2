# $Id: Makefile.am,v 1.3 2001/09/24 18:03:07 skimo Exp $
INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES = $(INCLUDEDIR)/fastcgi.h    \
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
		os_unix.c   \
		os_win32.c
