/*
 * threaded.c -- A simple multi-threaded FastCGI application.
 */

#ifndef lint
static const char rcsid[] = "$Id: threaded.c,v 1.2 1999/07/28 00:34:49 roberts Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#include <pthread.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#endif

#include "fcgiapp.h"


#define THREAD_COUNT 20

int count[THREAD_COUNT];

static void *doit(void *a)
{
    int i, k = (int)a;
    FCGX_Request request;
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    char *server_name;

    FCGX_InitRequest(&request);

    while (FCGX_Accept_r(&in, &out, &err, &envp, &request) >= 0)
    {
        server_name = FCGX_GetParam("SERVER_NAME", envp);

        FCGX_FPrintF(out,
            "Content-type: text/html\r\n"
            "\r\n"
            "<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>"
            "<h1>FastCGI Hello! (multi-threaded C, fcgiapp library)</h1>"
            "Thread %d, Request %d<p>"
            "Request counts for %d threads running on host <i>%s</i><p><code>",
            k, ++count[k], THREAD_COUNT, server_name ? server_name : "?");

        for (i = 0; i < THREAD_COUNT; i++)
            FCGX_FPrintF(out, "%5d " , count[i]);
    }

    return NULL;
}

int main(void)
{
    int i;
    pthread_t id[THREAD_COUNT];

    FCGX_Init();

    for (i = 0; i < THREAD_COUNT; i++)
        count[i] = 0;

    for (i = 1; i < THREAD_COUNT; i++)
        pthread_create(&id[i], NULL, doit, (void*)i);

    doit(0);

    exit(0);
}

