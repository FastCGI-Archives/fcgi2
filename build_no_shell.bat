rem
rem This build script is intended to be run from the root dir that
rem the FastCGI kit was unpacked in.  It should work on an X86
rem machine but it's only been tested in limited form.
rem
rem $Id: build_no_shell.bat,v 1.1 1997/09/16 15:36:24 stanleyg Exp $
rem
rem

echo off
if not exist "include\fcgi_config_x86.h" goto NO_CONFIG

copy include\fcgi_config_x86.h include\fcgi_config.h

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
nmake -f tiny-fcgi.mak
nmake -f tiny-fcgi2.mak

cd ..

rem
rem Now copy all binaries (including the libfcgi.dll) to a common 
rem directory to make testing and accessing the apps easier.
rem
if not exist "FcgiBin" mkdir FcgiBin
copy libfcgi\Debug\libfcgi.dll FcgiBin
copy examples\*.fcgi FcgiBin
copy examples\Debug\*.exe FcgiBin
copy cgi-fcgi\Debug\cgi-fcgi.exe FcgiBin

goto :DONE

:NO_CONFIG
echo Could not find the file "fcgi_config_x86.h".  Aborting.

:DONE

