// fcgio.h - defines classes fcgio_buf, fcgio_ostream, fcgio_istream
//           and replaces cin, cout and cerr with FastCGI versions
//           you must include this file in any source file which 
//           uses cin, cout or cerr. It must also appear at the end
//           of your includes
//
// $Id: fcgio.h,v 1.1 1999/08/15 03:37:59 roberts Exp $
//
// This work is based on routines written by George Feinberg. They 
// have been mostly re-written and extensively changed by 
// Michael Richards.
//
// Copyright (c) 1999 by Apollo Software. All rights reserved.
//
// You are free to use this software without charge or royalty
// as long as this notice is not removed or altered, and recognition 
// is given to the author(s)
//
   
#ifndef FCGIO_H
#define FCGIO_H
   
#include <iostream.h>
#include <fcgiapp.h>
   

// FastCGI streambuf replacement. Implements low level io to the 
// FastCGI c functions so our higher level iostreams will talk
// in the fcgi protocol
class fcgio_buf : public streambuf {
  // construction
  public:
    fcgio_buf( FCGX_Stream *str) { fcgi_str = str; }
    ~fcgio_buf() {}
   
    // from streambuf
    // tells the stream to go flush itself
    virtual int sync();
    // handles allocation of buffers by failing since buffer support isn't used
    virtual int doallocate() { return 0; }
   
    virtual int xsgetn(char *s, int n);
    virtual int xsputn(const char* s, int n);
    virtual int overflow(int);
    virtual int underflow();
    virtual streambuf* setbuf(char* s, int n);
   
  private:
    FCGX_Stream * fcgi_str;
};

   
// Here's the ostream class definition. All it has to do is
// call ios::init() passing an instance of fcgio_buf
class fcgio_ostream : public ostream {
  public:
    fcgio_ostream(FCGX_Stream *str);
    ~fcgio_ostream() { buf.sync(); }

  private:
    fcgio_buf buf;
};


// Here's the istream class definition. All it has to do is
// call ios::init() passing an instance of fcgio_buf
class fcgio_istream : public istream {
  public:
    fcgio_istream(FCGX_Stream *str);
    ~fcgio_istream() {}

  private:
    // FastCGI stuff used for the FCGX_ functions
    fcgio_buf buf;
    // buffer for getting data
    const int buffersize=100;
    char buffer[buffersize];
};


// replace cin and cout with our own versions
#ifndef USE_CIN_COUT_CERR
  #undef cout   
  #define cout (*FCGIX_cout)
  extern fcgio_ostream *FCGIX_cout;

  #undef cerr   
  #define cerr (*FCGIX_cerr)
  extern fcgio_ostream *FCGIX_cerr;

  #undef cin
  #define cin (*FCGIX_cin)
  extern fcgio_istream *FCGIX_cin;
#endif

#endif __FCCIO_H__
