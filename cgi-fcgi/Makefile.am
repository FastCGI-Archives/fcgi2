# $Id: Makefile.am,v 1.3 2001/09/24 18:03:03 skimo Exp $
bin_PROGRAMS     = cgi-fcgi

INCLUDEDIR  = $(srcdir)/../include
CPPFLAGS    = @CPPFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES   = $(INCLUDEDIR)/fastcgi.h  \
		  $(INCLUDEDIR)/fcgiapp.h  \
		  $(INCLUDEDIR)/fcgimisc.h

LIBDIR      = ../libfcgi
LIBFCGI     = $(LIBDIR)/libfcgi.la

LDADD = $(LIBFCGI)
cgi_fcgi_SOURCES = $(INCLUDE_FILES) cgi-fcgi.c
