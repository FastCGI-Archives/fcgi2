# $Id: Makefile.am,v 1.6 2001/11/28 03:21:39 robs Exp $
noinst_PROGRAMS     = authorizer \
			  echo \
			  echo-x \
			  log-dump \
                          size \
			  @threaded@  \
			  @ECHO_CPP@

EXTRA_PROGRAMS = threaded echo-cpp

INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES   = $(INCLUDEDIR)/fastcgi.h    \
		  $(INCLUDEDIR)/fcgiapp.h    \
		  $(INCLUDEDIR)/fcgimisc.h   \
		  $(INCLUDEDIR)/fcgi_stdio.h

LIBDIR      = ../libfcgi
LIBFCGI     = $(LIBDIR)/libfcgi.la

LDADD = $(LIBFCGI)
echo_SOURCES = $(INCLUDE_FILES) echo.c
echo_x_SOURCES = $(INCLUDE_FILES) echo-x.c
log_dump_SOURCES = $(INCLUDE_FILES) log-dump.c
authorizer_SOURCES = $(INCLUDE_FILES) authorizer.c

threaded_SOURCES = $(INCLUDE_FILES) threaded.c
threaded_LDADD = $(LIBFCGI) -lpthread

echo_cpp_SOURCES = $(INCLUDE_FILES) $(INCLUDEDIR)/fcgio.h echo-cpp.cpp
echo_cpp_LDADD   = $(LIBDIR)/libfcgi++.la

