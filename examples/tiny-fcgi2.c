/*
 * tiny-fcgi2.c --
 *
 *	FastCGI example program using fcgiapp library
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: tiny-fcgi2.c,v 1.4 1999/07/28 00:36:11 roberts Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#endif

#include "fcgiapp.h"

int main(void)
{
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    int count = 0;

    while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
        FCGX_FPrintF(out,
           "Content-type: text/html\r\n"
           "\r\n"
           "<title>FastCGI Hello! (C, fcgiapp library)</title>"
           "<h1>FastCGI Hello! (C, fcgiapp library)</h1>"
           "Request number %d running on host <i>%s</i>  "
           "Process ID: %d\n",
           ++count, FCGX_GetParam("SERVER_NAME", envp), getpid());
    }

    return 0;
}
