// FILE NAME: fcgio.cpp
//
// $Id: fcgio.cpp,v 1.1 1999/08/15 03:37:59 roberts Exp $
//
// REWRITTEN BY: Michael Richards  06/20/1999
//          -added cin support
//          -added ability to replace cin/cout so existing code only
//           needs to include the header file and use the normal cin/cout
//          -added buffered read support which is required by getline
//
// ORIGINAL AUTHOR: George Feinberg
//
// NOTES: encapsulates FCGI function in an ostream and replaces cin,
//        cout and cerr
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

#include <fcgiapp.h>
#include "fcgio.h"


// the definition that replaces cout and cin
#ifndef USE_CIN_COUT_CERR
  fcgio_ostream *FCGIX_cout;
  fcgio_ostream *FCGIX_cerr;
  fcgio_istream *FCGIX_cin;
#endif
   
/*---------------------------------------------------------------------*/
   

// gets a number of characters at once for a more efficient implementation
int fcgio_buf::xsgetn(char *s, int n) {
  return FCGX_GetStr(s, n, fcgi_str);
}
   

// writes a gob of data out, This is called for things like:
// output << "hello world"
// output.write(string, n);
int fcgio_buf::xsputn(const char* s, int n) {
  return FCGX_PutStr(s, n, fcgi_str);
}
   

// overflow is called for "endl" and every time the put buffer
// is full. Since we are using a single char put buffer, we just
// pump the waiting character out and return the number of chars
// we just got rid of (1). Returns EOF on error
int fcgio_buf::overflow(int c) {
  if (c)
    return FCGX_PutChar(c, fcgi_str);
  else
    return EOF;
}


// Underflow is called to get characters to fill up the get buffer
// so something that uses the stream can get the data. If there
// is nothing else to read, this returns EOF
int fcgio_buf::underflow() {
  // if it is unbuffered, then get & return a char
  if (unbuffered())
    return FCGX_GetChar(fcgi_str);

  // read as much data as we can, then adjust the get area so it
  // reflects the data we just read
  int numread=FCGX_GetStr(eback(), blen(), fcgi_str);
  if (numread==0) return EOF;
  setg(eback(),eback(),eback()+numread);

  return *eback();
}


// sets up any buffering used for input or output   
streambuf* fcgio_buf::setbuf(char *p, int n) {
  // if setbuf is offered a buffer, then use it, otherwise, set the stream to unbuffered
  setb(p,p+n,0);
  // output is not buffered
  setp(0,0);
  setg(p,p+n,p+n);

  unbuffered(p==NULL);
  return this;
}


// flush all the output using FCGX_FFlush
int fcgio_buf::sync() {
  return FCGX_FFlush(fcgi_str); 
}


// constructor calls ios::init to assign our streambuf to
// the ostream derived class. Also sets up buffering
fcgio_ostream::fcgio_ostream(FCGX_Stream *str) : buf(str) {
  // init is a protected member of ios
  init(&buf);
  buf.setbuf(NULL,0);
}


// constructor calls ios::init to assign our streambuf to
// the ostream derived class. Also sets up buffering
fcgio_istream::fcgio_istream(FCGX_Stream *str) : buf(str) {
  // init is a protected member of ios
  init(&buf);
  // set up the buffers
  buf.setbuf(buffer,buffersize);
}


