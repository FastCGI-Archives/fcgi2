# $Id: Makefile.am,v 1.2 2001/09/24 11:28:59 skimo Exp $
bin_PROGRAMS     = cgi-fcgi

INCLUDEDIR  = $(srcdir)/../include
CFLAGS      = @CFLAGS@ -I$(INCLUDEDIR)

INCLUDE_FILES   = $(INCLUDEDIR)/fastcgi.h  \
		  $(INCLUDEDIR)/fcgiapp.h  \
		  $(INCLUDEDIR)/fcgimisc.h

LIBDIR      = $(srcdir)/../libfcgi
LIBFCGI     = $(LIBDIR)/libfcgi.la

LDADD = $(LIBFCGI)
cgi_fcgi_SOURCES = $(INCLUDE_FILES) cgi-fcgi.c
