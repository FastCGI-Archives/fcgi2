/* 
 * tiny-cgi.c --
 *
 *	CGI example program
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: tiny-cgi.c,v 1.1 1997/09/16 15:36:28 stanleyg Exp $";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>

void main(void)
{
    int count = 0;
    printf("Content-type: text/html\r\n"
           "\r\n"
           "<title>CGI Hello!</title>"
           "<h1>CGI Hello!</h1>"
           "Request number %d running on host <i>%s</i>\n",
           ++count, getenv("SERVER_NAME"));
}
