/* 
 * tiny-fcgi.c --
 *
 *	FastCGI example program using fcgi_stdio library
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: tiny-fcgi.c,v 1.1 1997/09/16 15:36:28 stanleyg Exp $";
#endif /* not lint */

#include "fcgi_stdio.h"
#include <stdlib.h>

#ifdef _WIN32
#include <process.h>
#endif

void main(void)
{
    int count = 0;
    while(FCGI_Accept() >= 0) {
        printf("Content-type: text/html\r\n"
               "\r\n"
               "<title>FastCGI Hello! (C, fcgi_stdio library)</title>"
               "<h1>FastCGI Hello! (C, fcgi_stdio library)</h1>"
               "Request number %d running on host <i>%s</i>  "
               "Process ID: %d\n",
               ++count, getenv("SERVER_NAME"), getpid());
    }
}
