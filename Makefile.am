#
#  Makefile for FastCGI development kit
#
#  $Id: Makefile.am,v 1.8 2003/11/02 21:42:47 robs Exp $
#

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES= compile config.guess config.sub configure configure~ depcomp fcgi_config.h.in fcgi_config.h.in~ \
		aclocal.m4 m4/libtool.m4 m4/lt*.m4 Makefile.in install-sh missing ltmain.sh \
		COPYING INSTALL fcgi-*.tar.gz

SUBDIRS = libfcgi cgi-fcgi include

if HAVE_EXAMPLES
SUBDIRS += examples
endif

include_HEADERS = fcgi_config.h

man_MANS=	doc/cgi-fcgi.1             \
		doc/FCGI_Accept.3          \
		doc/FCGI_Finish.3          \
		doc/FCGI_SetExitStatus.3   \
		doc/FCGI_StartFilterData.3

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = fcgi.pc \
                 fcgi++.pc

EXTRA_DIST = LICENSE              \
             Makefile.nt                \
             cgi-fcgi/cgi-fcgi.mak      \
             examples/authorizer.mak    \
             examples/echo.mak          \
             examples/echox.mak         \
             examples/size.mak          \
             examples/echo-cpp.mak      \
             libfcgi/libfcgi.mak        \
             images/aplib-hd.gif        \
             images/divider.gif         \
             images/fcgi-hd.gif         \
             images/mail-hd.gif         \
             images/navbar.gif          \
             images/serv-hd.gif         \
             images/words-hd.gif        \
             include/fcgi_config_x86.h  \
             java/FCGIGlobalDefs.java   \
             java/FCGIInputStream.java  \
             java/FCGIInterface.java    \
             java/FCGIMessage.java      \
             java/FCGIOutputStream.java \
             java/FCGIRequest.java      \
             libfcgi/os_unix.c          \
             libfcgi/os_win32.c         \
	     perl/eg/echo.pl \
	     perl/eg/remote.pl \
	     perl/eg/threaded.pl \
	     perl/README \
	     perl/ChangeLog \
	     perl/t/02-unix_domain_socket.t \
	     perl/t/01-load.t \
	     perl/FCGI.pm \
	     perl/Makefile.PL \
	     perl/MANIFEST \
	     perl/MANIFEST.SKIP \
	     perl/configure.in \
	     perl/distrib \
	     perl/configure.readme \
	     perl/typemap \
	     perl/FCGI.xs \
             Win32/FastCGI.dsw          \
             Win32/authorizer.dsp       \
             Win32/cgifcgi.dsp          \
             Win32/config_h.dsp         \
             Win32/echo-cpp.dsp         \
             Win32/echo.dsp             \
             Win32/echox.dsp            \
             Win32/libfcgi.dsp          \
             Win32/logdump.dsp          \
             Win32/size.dsp             \
             Win32/threaded.dsp         \
             doc/FCGI_Accept.3          \
             doc/FCGI_Finish.3          \
             doc/FCGI_SetExitStatus.3   \
             doc/FCGI_StartFilterData.3 \
             doc/cgi-fcgi.1             \
             doc/fcgi-devel-kit.htm     \
             doc/fcgi-java.htm          \
             doc/fcgi-perf.htm          \
             doc/fcgi-perl.htm          \
             doc/fcgi-spec.html         \
             doc/fcgi-tcl.htm           \
             doc/omi-logo.gif           \
             doc/overview.html          \
             doc/www5-api-workshop.html \
             doc/fastcgi-prog-guide/ap_guida.htm        \
             doc/fastcgi-prog-guide/ap_guide.htm        \
             doc/fastcgi-prog-guide/apaman.htm          \
             doc/fastcgi-prog-guide/ch1inta1.gif        \
             doc/fastcgi-prog-guide/ch1intra.gif        \
             doc/fastcgi-prog-guide/ch1intro.htm        \
             doc/fastcgi-prog-guide/ch2c.htm            \
             doc/fastcgi-prog-guide/ch3perl.htm         \
             doc/fastcgi-prog-guide/ch4tcl.htm          \
             doc/fastcgi-prog-guide/cover.htm           \
             doc/fastcgi-prog-guide/covera.gif          \
             doc/fastcgi-whitepaper/fastcgi.htm         \
             doc/fastcgi-whitepaper/img00001.gif        \
             doc/fastcgi-whitepaper/img00002.gif        \
             doc/fastcgi-whitepaper/img00003.gif

distclean-local:
	-rm -rf autom4te.cache
