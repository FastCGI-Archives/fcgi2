noinst_PROGRAMS     = authorizer \
			  echo \
			  echo-x \
			  log-dump \
			  @threaded@

EXTRA_PROGRAMS = threaded

INCLUDEDIR  = $(srcdir)/../include
CFLAGS      = @CCDEFS@ -I$(INCLUDEDIR)

INCLUDE_FILES   = $(INCLUDEDIR)/fastcgi.h    \
		  $(INCLUDEDIR)/fcgiapp.h    \
		  $(INCLUDEDIR)/fcgimisc.h   \
		  $(INCLUDEDIR)/fcgi_stdio.h

LIBDIR      = $(srcdir)/../libfcgi
LIBFCGI     = $(LIBDIR)/libfcgi.la

LDADD = $(LIBFCGI)
echo_SOURCES = $(INCLUDE_FILES) echo.c
echo_x_SOURCES = $(INCLUDE_FILES) echo-x.c
log_dump_SOURCES = $(INCLUDE_FILES) log-dump.c
authorizer_SOURCES = $(INCLUDE_FILES) authorizer.c

threaded_SOURCES = $(INCLUDE_FILES) threaded.c
threaded_LDADD = $(LIBFCGI) -lpthread
