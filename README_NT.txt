FastCGI for Windows NT README (V2.0, beta 1)
============================================

    Version 2.0.1, 20 March 1997
    $Id: README_NT.txt,v 1.1 1997/09/16 15:36:24 stanleyg Exp $
    Copyright (c) 1996 Open Market, Inc.
    See the file "LICENSE.TERMS" for information on usage and redistribution
    of this file, and for a DISCLAIMER OF ALL WARRANTIES.

	This is a beta copy of the FastCGI libraries for Windows NT 3.51 
and NT 4.0.  It will also build on (or should build on) all previously 
supported versions on Unix.

        The Unix build instructions are identical to the past.  The 
Windows NT version however is slightly different due to the fact that NT 
doesn't contain a native equivalent to the bourne shell to start with.
NT also doesn't support the same Makefile structure and therefore we have
provided Visual C++ project makefiles for the cgi-fcgi application as well
as the libfcgi DLL.

        The first version of the libraries for Windows NT will require a
little care in installing and building.  However, what do you want for 
free?  Subsequent versions will ideally support a more NT look and feel 
but that's if time permits.

        For those of you that have the MKS toolkit or other bourne shell
equivalent for NT, great.  You're off to a good start and will be able
to use the configure script provided to generate your config header file
at the very least.  However, in order to make life easier, we are 
providing an NT version of the header file that should allow you to build
the sample applications without requiring you to run configure at all.
(NOTE: The NT version has only been tested on Windows NT 4.0 running on
       X86 hardware.  Other CPUs may have slightly different defines.)

        There are two batch files provided which will build Debug versions
of fastcgi.dll and the cgi-fcgi application.  They are:

        build_no_shell.bat - This will copy a canned version of the
                             fcgi_config_x86.h file to fcgi_config.h and
                             remove the need to use the "configure" script
                             to generate this.  (This is the recommended
                             way to build the sample DLL and applications.)

                 build.bat - This version will run the "configure" script
                             and will then build libfcgi.dll and 
                             cgi-fcgi.exe.

	Installation
	============

        Unpack the kit and install it into a directory of your choice.
Try something simple like "C:\FastCGI.beta".

        In order to run under IIS using the cgi-fcgi.exe "shim" program,
we need to create a file extension type that IIS will recognize and will
automatically launch the application and/or connect to the target FastCGI
application.

        1) Make a directory "C:\FastCGI.beta".  The name is not critical
           but is is what is assumed for the remainder of this README.

        2) cd into "C:\FastCGI.beta".

        3) Unpack the kit into this directory.

        4) Run build_no_shell.bat

	5) Add the .fcgi file type to the registry for IIS.  This is done
           using regedit.

\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\W3SVC\Parameters\Script Map

        Edit -> New -> String Value <CR>

        6) Type in ".fcgi" for the "String Value" extension name.

        7) Double click the ".fcgi" entry.

        8) Enter C:\FastCGI.beta\FcgiBin\cgi-fcgi.exe -f %s
           NOTE: This assumes you installed into c:\FastCGI.beta

           Save this value and exit regedit.

        9) Add the pathname of libfcgi.dll to your system path 
           environment variable.
                "C:\FastCGI.beta\libfcgi\Debug"

           NOTE: The build_no_shell.bat command will copy all the sample
                 applications as well as cgi-fci.exe and libfcgi.dll to
                 the directory "FcgiBin" which will be created as a result
                 of the build_no_shell.bat command being run.  This makes
                 it easier to use and removes the need for adding the
                 paths to the system environment as the libfcgi.dll will
                 live in the same directory as the applications which will
                 be using it.  This has been tested and qualified on 
                 IIS 3.0.

                 If your applications live in a directory other than the
                 FcgiBin directory AND there's no path environment 
                 variable registered which contains a pointer to a valid
                 libfcgi.dll the FastCGI application will not work.

        10) Use Internet Service Manager (or the registry editor if you're
            brave) and map in the directory "C:\FastCGI.beta\FcgiBin" 
            as a virtual directory "/fcgi" with execute access and 
            read access.

        You should now be ready to try out the shipped samples.  Try this by
        accessing the following urls:

        The url "http://yourServer/fcgi/tiny-fcgi.exe" reloaded repeatedly
        should produce the following output:

	FastCGI Hello! (C, fcgi_stdio library)

	Request number 1 running on host "yourServer" Process ID: N

        where:

        yourServer is the name of your server.

        N is the process id number of the tiny-fcgi.exe process.  This
        should be changing each time you reload the URL.

	Now try the url "http://yourServer/fcgi/tiny-fcgi_nt.fcgi".  The
        output from this url should produce the same as the preceeding url 
        but you should notice the "Request number" incrementing each time
        you reload and the Process ID should remain constant.  If this is 
        working, you have a persistent FastCGI application running.


	Known Problems/Limitations
	==========================

        1) This port was build for Windows NT 3.51 and above.  It was
           not built with Windows 95 as one of the target platforms.
           The reason is that I/O completion ports are used for 
           asynchronous I/O which are not present on Windows 95.  
           Changing this is not that big a job and involves changing to 
           use overlapped I/O.  Again, the port was towards Windows NT
           which was why the I/O completion ports were chosen.  This
           mechanism was also chosen in anticipation of the multi-threaded
           FastCGI for NT as it will map to the model we currently
           have designed.


NOTE: Use the application "kill.exe" contained in the NT resource kit
      to kill persistent FastCGI applciations!

