rem
rem This build script is intended to be run from the root dir that
rem the FastCGI kit was unpacked in.  It is intended to be used for
rem those that want to run the configure script.
rem
rem $Id: build.bat,v 1.2 2001/08/27 19:52:49 robs Exp $
rem
rem

echo off

if not exist "config.cache" sh -c "./configure"


rem
rem Build the FastCGI DLL and import library.
rem
cd libfcgi
nmake -f libfcgi.mak
%1%

rem
rem Build the cgi-fcgi.exe "shim" application.
rem
cd ..\cgi-fcgi
nmake -f cgi-fcgi.mak
%1%

rem
rem Now build the sample applications that have been qualified.
rem
cd ..\examples
nmake -f echo.mak
nmake -f echo2.mak

cd ..
goto :DONE

:NO_CONFIG
echo Could not find the file "fcgi_config_x86.h".  Aborting.

:DONE
