//
// $Id: fcgio.cpp,v 1.6 2001/11/20 02:29:39 robs Exp $
//
// Allows you communicate with FastCGI streams using C++ iostreams
//
// ORIGINAL AUTHOR:     George Feinberg
// REWRITTEN BY:        Michael Richards  06/20/1999
// REWRITTEN AGAIN BY:  Michael Shell     02/23/2000
// REWRITTEN AGAIN BY:  Rob Saccoccio     11 Nov 2001
//
// Copyright (c) 2000 Tux the Linux Penguin
//
// You are free to use this software without charge or royalty
// as long as this notice is not removed or altered, and recognition
// is given to the author(s)
//
// This code is offered as-is without any warranty either expressed or
// implied; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.

#define DLLAPI  __declspec(dllexport)
#include "fcgio.h"

fcgi_streambuf::fcgi_streambuf(FCGX_Stream * strm) 
{ 
    this->fcgx = strm;
    this->buf = NULL;
    this->bufsize = 0;
    setbuf(NULL, 0);
}

fcgi_streambuf::~fcgi_streambuf(void)
{
    overflow(EOF);
    // FCGX_Finish()/FCGX_Accept() will flush and close
} 

int fcgi_streambuf::overflow(int c)
{
    if (this->bufsize)
    {
        int plen = pptr() - pbase();

        if (plen) 
        {
            if (FCGX_PutStr(pbase(), plen, this->fcgx) != plen) return EOF;
            pbump(-plen);
        }
    }

    if (c != EOF) 
    {
        if (FCGX_PutChar(c, this->fcgx) != c) return EOF;
    }

    return 0;
}

// default base class behaviour seems to be inconsistent
int fcgi_streambuf::sync()
{
    if (overflow(EOF)) return EOF;
    if (FCGX_FFlush(this->fcgx)) return EOF;
    return 0;
}

int fcgi_streambuf::underflow()
{
    if (this->bufsize)
    {
        if (in_avail() == 0)
        {
            int glen = FCGX_GetStr(eback(), this->bufsize, this->fcgx);
            if (glen <= 0) return EOF;

            setg(eback(), eback(), eback() + glen);
        }

        return (unsigned char) *gptr();       
    }
    else
    {
        return FCGX_GetChar(this->fcgx);
    } 
}

void fcgi_streambuf::reset(void)
{
    // it should be ok to set up both the get and put areas
    char * end = this->buf + this->bufsize;
    this->setg(this->buf, this->buf, end);
    this->setb(this->buf, end);
}

streambuf * fcgi_streambuf::setbuf(char * buf, int len)
{
    // XXX support moving data from an old buffer
    if (this->bufsize) return NULL;

    this->buf = buf;
    this->bufsize = len;

    // the base setbuf() *has* to be called
    streambuf::setbuf(buf, len);

    reset();

    return this;
}

int fcgi_streambuf::attach(FCGX_Stream * strm)
{ 
    this->fcgx = strm;

    if (this->bufsize)
    {
        reset();
    }

    return 0;
}

int fcgi_streambuf::xsgetn(char * s, int n) 
{
    return (this->bufsize) 
        ? streambuf::xsgetn(s, n) 
        : FCGX_GetStr(s, n, this->fcgx);
}
   
int fcgi_streambuf::xsputn(const char * s, int n) 
{
    return (this->bufsize) 
        ? streambuf::xsputn(s, n) 
        : FCGX_PutStr(s, n, this->fcgx);
}

// deprecated
fcgi_istream::fcgi_istream(FCGX_Stream * strm) :
    istream(&fcgi_strmbuf)
{
    fcgi_strmbuf.attach(strm);
}

// deprecated
void fcgi_istream::attach(FCGX_Stream * strm)
{
    fcgi_strmbuf.attach(strm);
}

// deprecated
fcgi_ostream::fcgi_ostream(FCGX_Stream * strm) :
    ostream(&fcgi_strmbuf)
{
    fcgi_strmbuf.attach(strm);
}

// deprecated
void fcgi_ostream::attach(FCGX_Stream * strm)
{
    fcgi_strmbuf.attach(strm);
}
