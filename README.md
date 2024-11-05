![Github Actions Build](https://github.com/FastCGI-Archives/fcgi2/actions/workflows/build.yml/badge.svg)

FastCGI Developer's Kit
-----------------------

Copyright (c) 1996 Open Market, Inc.
See the file "[LICENSE](LICENSE)" for information on usage and redistribution
of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Documentations
--------------

- [FastCGI Developer's Kit Documentations](http://fastcgi-archives.github.io/fcgi2/doc/overview.html)

Basic Directions
----------------

#### *Build Requires on unix:*

You need to install gcc, gnu make, m4, aclocal, autoconf, automake and libtool packages.
    
*Example on ubuntu :*
```
# apt install gcc make m4 autoconf automake libtool
```

#### *Unix:*

    ./autogen.sh
    ./configure
    make
    make install

#### *Win32:*

    nmake -f Makefile.nt

    (or use the MSVC++ project files in the Win32 directory)


CHANGES
-------

This repository are a fork from the original fastcgi sdk from [FastCGI.com](https://fastcgi-archives.github.io/) that are now down, the new place of FastCGI.com are at https://fastcgi-archives.github.io/.

For more detail regarding changes, please consult the [git log available](https://github.com/FastCGI-Archives/fcgi2/commits/master). 

