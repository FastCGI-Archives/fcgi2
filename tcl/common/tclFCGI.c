/* 
 * tclFCGI.c --
 *
 *	TCL functions needed to set up FastCGI commands
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: tclFCGI.c,v 1.1 1997/09/16 15:36:36 stanleyg Exp $";
#endif /* not lint */

#include <tcl.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "fcgiapp.h"

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#endif

extern char **environ;
static char **requestEnviron = NULL;


/*
 * For each variable in the array envp, either set or unset it
 * in the interpreter interp.
 */
static void DoTclEnv(Tcl_Interp *interp, char **envp, int set)
{
    int i;
    char *p, *p1;
    for (i = 0; ; i++) {
	if ((p = envp[i]) == NULL) {
	    break;
	}
        p1 = strchr(p, '=');
	*p1 = '\0';
        if(set) {
	    Tcl_SetVar2(interp, "env", p, p1 + 1, TCL_GLOBAL_ONLY);
        } else {
	    Tcl_UnsetVar2(interp, "env", p, TCL_GLOBAL_ONLY);
	}
	*p1 = '=';
    }
}


/*
 * If running as FastCGI, grab FDs 1 and 2 so the Tcl open
 * command won't see them and think it has discovered stdout/stderr.
 * Grab the FDs using /dev/null so that attempts to write
 * to stdout/stderr before calling FCGI_Accept are a no-op rather
 * than crashing Tcl.
 */
static void GrabFDs(void)
{
    if(FCGX_IsCGI()) {
        return;
    }
    for (;;) {
        int fd = open("/dev/null", O_WRONLY, 0);
        if(fd >= 2) {
            break;
        }
    }
}


static int FcgiAcceptCmd(
        ClientData dummy, Tcl_Interp *interp, int argc, char **argv)
{
    char **savedEnviron;
    int acceptStatus;
    /*
     * Unmake Tcl variable settings for the request just completed.
     */
    if(requestEnviron != NULL) {
        DoTclEnv(interp, requestEnviron, FALSE);
        requestEnviron = NULL;
    }
    /*
     * Call FCGI_Accept but preserve environ.
     */
    savedEnviron = environ;
    acceptStatus = FCGI_Accept();
    requestEnviron = environ;
    environ = savedEnviron;
    /*
     * Make Tcl variable settings for the new request.
     */
    if(acceptStatus >= 0 && !FCGX_IsCGI()) {
        DoTclEnv(interp, requestEnviron, TRUE);
    } else {
        requestEnviron = NULL;
    }
    sprintf(interp->result, "%d", acceptStatus);
    return TCL_OK;
}


static int FcgiFinishCmd(
        ClientData dummy, Tcl_Interp *interp, int argc, char **argv)
{
    /*
     * Unmake Tcl variable settings for the completed request.
     */
    if(requestEnviron != NULL) {
        DoTclEnv(interp, requestEnviron, FALSE);
        requestEnviron = NULL;
    }
    /*
     * Call FCGI_Finish.
     */
    FCGI_Finish();
    sprintf(interp->result, "%d", 0);
    return TCL_OK;
}


static int FcgiSetExitStatusCmd(
        ClientData dummy, Tcl_Interp *interp, int argc, char **argv)
{
    if (argc != 2) {
	sprintf(interp->result, "wrong # args");
	return TCL_ERROR;
    }
    FCGI_SetExitStatus(atoi(argv[1]));
    sprintf(interp->result, "%d", 0);
    return TCL_OK;
}


static int FcgiStartFilterDataCmd(
        ClientData dummy, Tcl_Interp *interp, int argc, char **argv)
{
    sprintf(interp->result, "%d", FCGI_StartFilterData());
    return TCL_OK;
}


int FCGI_Init(Tcl_Interp *interp) {
    GrabFDs();
    Tcl_CreateCommand(
            interp, "FCGI_Accept", FcgiAcceptCmd, 0, NULL);
    Tcl_CreateCommand(
            interp, "FCGI_Finish", FcgiFinishCmd, 0, NULL);
    Tcl_CreateCommand(
            interp, "FCGI_SetExitStatus", FcgiSetExitStatusCmd, 0, NULL);
    Tcl_CreateCommand(
            interp, "FCGI_StartFilterData", FcgiStartFilterDataCmd, 0, NULL);
    return TCL_OK;
}
