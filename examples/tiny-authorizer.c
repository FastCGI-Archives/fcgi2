/* 
 * tiny-authorizer.c --
 *
 *	FastCGI example Authorizer program using fcgi_stdio library
 *
 *
 * Be sure to run this program in a region with CGIPassword
 * in order to get REMOTE_USER and REMOTE_PASSWD in place
 * of HTTP_AUTHORIZATION.
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: tiny-authorizer.c,v 1.1 1997/09/16 15:36:28 stanleyg Exp $";
#endif /* not lint */

#include "fcgi_stdio.h"
#include <stdlib.h>
#include <string.h>

void main(void)
{
    char *user, *password;
    user = getenv("USER");
    if(user == NULL) {
        user = "doe";
    }
    password = getenv("PASSWORD");
    if(password == NULL) {
        password = "xxxx";
    }
    
    while(FCGI_Accept() >= 0) {
        char *remoteUser, *remotePassword;
        remoteUser = getenv("REMOTE_USER");
        remotePassword = getenv("REMOTE_PASSWD");
        if((remoteUser == NULL) || (remotePassword == NULL) ||
           strcmp(remoteUser, user) || strcmp(remotePassword, password)) {
           printf("Status: 401 Unauthorized\r\n"
                  "WWW-Authenticate: Basic realm=\"Test\"\r\n"
                  "\r\n");
        
	} else {
            char *processId = getenv("QUERY_STRING");
            if(processId == NULL || strlen(processId) == 0) {
                processId = "0";
	    }
            printf("Status: 200 OK\r\n"
                   "Variable-AUTH_TYPE: Basic\r\n"
                   "Variable-REMOTE_PASSWD:\r\n"
                   "Variable-PROCESS_ID: %s\r\n"
                   "\r\n", processId);
        }
    }
}
