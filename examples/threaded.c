/*
 * threaded.c -- A simple multi-threaded FastCGI application.
 */

#ifndef lint
static const char rcsid[] = "$Id: threaded.c,v 1.3 1999/07/30 19:09:46 roberts Exp $";
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

static int counts[THREAD_COUNT];

static void *doit(void *a)
{
    int rc, i, thread_id = (int)a;
    FCGX_Request request;
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    char *server_name;

    FCGX_InitRequest(&request);

    for (;;)
    {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
        static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&in, &out, &err, &envp, &request);
        pthread_mutex_unlock(&accept_mutex);

        if (rc < 0)
            break;

        server_name = FCGX_GetParam("SERVER_NAME", envp);

        FCGX_FPrintF(out,
            "Content-type: text/html\r\n"
            "\r\n"
            "<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>"
            "<h1>FastCGI Hello! (multi-threaded C, fcgiapp library)</h1>"
            "Thread %d<p>"
            "Request counts for %d threads running on host <i>%s</i><p><code>",
            thread_id, THREAD_COUNT, server_name ? server_name : "?");

        pthread_mutex_lock(&counts_mutex);
        ++counts[thread_id];
        for (i = 0; i < THREAD_COUNT; i++)
            FCGX_FPrintF(out, "%5d " , counts[i]);
        pthread_mutex_unlock(&counts_mutex);

        FCGX_Finish_r(&request);
    }

    return NULL;
}

int main(void)
{
    int i;
    pthread_t id[THREAD_COUNT];

    FCGX_Init();

    for (i = 1; i < THREAD_COUNT; i++)
        pthread_create(&id[i], NULL, doit, (void*)i);

    doit(0);

    exit(0);
}

