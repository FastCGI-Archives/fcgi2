# $Id: Makefile.am,v 1.9 2001/12/22 03:16:20 robs Exp $

INCLUDEDIR  = ../include
AM_CPPFLAGS = -I$(top_srcdir)/include -W -Wall -pedantic -Wno-unused-parameter

INCLUDE_FILES = $(INCLUDEDIR)/fastcgi.h     \
                $(INCLUDEDIR)/fcgiapp.h     \
                $(INCLUDEDIR)/fcgimisc.h    \
                $(INCLUDEDIR)/fcgi_stdio.h  \
                $(INCLUDEDIR)/fcgios.h

lib_LTLIBRARIES   = libfcgi.la @LIBFCGIXX@
DISTCLEANFILES= Makefile.in 
EXTRA_LTLIBRARIES = libfcgi++.la

libfcgi_la_SOURCES = $(INCLUDE_FILES)  \
                     fcgiapp.c         \
                     fcgi_stdio.c      \
                     os_@SYSTEM@.c
libfcgi_la_CC      = @PTHREAD_CC@
libfcgi_la_CFLAGS  = @PTHREAD_CFLAGS@
libfcgi_la_LIBADD  = -lm
libfcgi_la_LDFLAGS = @EXTRA_LIBS@ -no-undefined

libfcgi___la_SOURCES = $(INCLUDE_FILES)       \
                       $(INCLUDEDIR)/fcgio.h  \
                       fcgio.cpp
libfcgi___la_CFLAGS  = @PTHREAD_CFLAGS@
libfcgi___la_LIBADD  = libfcgi.la
libfcgi___la_LDFLAGS = -rpath @libdir@ -no-undefined
