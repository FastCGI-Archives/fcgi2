/*
 *  A simple FastCGI application example in C++.
 *  
 *  $Id: echo-cpp.cpp,v 1.6 2001/11/26 18:08:25 robs Exp $
 *  
 *  Copyright (c) 2001  Rob Saccoccio and Chelsea Networks
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif
#include "fcgio.h"

// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;

static void penv(const char * const * envp)
{
    cout << "<PRE>\n";
    for ( ; *envp; ++envp) 
    {
        cout << *envp << "\n";
    }
    cout << "</PRE>\n";
}

static long gstdin(FCGX_Request * request, char ** content)
{
    char * clenstr = FCGX_GetParam("CONTENT_LENGTH", request->envp);
    unsigned long clen = STDIN_MAX;

    if (clenstr)
    {
        clen = strtol(clenstr, &clenstr, 10);
        if (*clenstr)
        {
            cerr << "can't parse \"CONTENT_LENGTH=" 
                 << FCGX_GetParam("CONTENT_LENGTH", request->envp) 
                 << "\"\n";
            clen = STDIN_MAX;
        }
    }

    // Note that *you* should not read stdin when CONTENT_LENGTH
    // is missing or unparsable (this is a demo/test program).

    // *always* put a cap on the amount of data that will be read
    if (clen > STDIN_MAX) clen = STDIN_MAX;

    *content = new char[clen];

    cin.read(*content, clen);
    clen = cin.gcount();

    // Chew up any remaining stdin - this shouldn't be necessary
    // but is because mod_fastcgi doesn't handle it correctly.
    //
    // ignore() doesn't set the eof bit in some versions of glibc++
    // so use gcount() instead of eof()...
    do cin.ignore(1024); while (cin.gcount() == 1024);

    return clen;
}

int main (void)
{
    int count = 0;
    long pid = getpid();

    FCGX_Request request;
       
    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);

    while (FCGX_Accept_r(&request) == 0) 
    {
        // Note that the default bufsize (0) will cause the use of iostream
        // methods that require positioning (such as peek(), seek(), 
        // unget() and putback()) to fail (in favour of more efficient IO).
        fcgi_streambuf fin(request.in);
        fcgi_streambuf fout(request.out);
        fcgi_streambuf ferr(request.err);

#ifdef _WIN32
        cin = &fin;
        cout = &fout;
        cerr = &ferr;
#else
        cin.rdbuf(&fin);
        cout.rdbuf(&fout);
        cerr.rdbuf(&ferr);
#endif

        // Although FastCGI supports writing before reading,
        // many http clients (browsers) don't support it (so  
        // the connection deadlocks until a timeout expires!).
        char * content;
        unsigned long clen = gstdin(&request, &content);

        cout << "Content-type: text/html\r\n"
                "\r\n"
                "<TITLE>echo-cpp</TITLE>\n"
                "<H1>echo-cpp</H1>\n"
                "<H4>PID: " << pid << "</H4>\n"
                "<H4>Request Number: " << ++count << "</H4>\n";

        cout << "<H4>Request Environment</H4>\n";
        penv(request.envp);

        cout << "<H4>Process/Initial Environment</H4>\n";
        penv(environ);

        cout << "<H4>Standard Input - " << clen;
        if (clen == STDIN_MAX) cout << " (STDIN_MAX)";
        cout << " bytes</H4>\n";
        if (clen) cout.write(content, clen);

        if (content) delete []content;

        // If the output streambufs had non-zero bufsizes and
        // were constructed outside of the accept loop (i.e.
        // their destructor won't be called here), they would
        // have to be flushed here.
    }

    return 0;
}
