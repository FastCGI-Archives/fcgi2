//
// $Id: fcgio.cpp,v 1.3 2000/07/27 12:21:54 robs Exp $
//
// Allows you communicate with FastCGI streams using C++ iostream
// objects
//
// ORIGINAL AUTHOR:     George Feinberg
// REWRITTEN BY:        Michael Richards  06/20/1999
// REWRITTEN AGAIN BY:  Michael Shell     02/23/2000
//
// Special Thanks to Dietmar Kuehl for his help and the numerous custom
// streambuf examples on his web site.
//
// see the header file fcgio2.h for lotsa docs and a code example.
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

/*------------------------------------------------------------------*/


#include "fcgio.h"


// **** fcgio_streambuf

// default constructor
fcgi_streambuf::fcgi_streambuf(void)
   {
   // we setup the buffer
   setb(buffer,buffer + (buffersize * sizeof(char)),0);
   // but we do not know yet if we are a reader or
   // a writer
   setp(0,0);
   setg(0,0,0);

   // good idea to declare buffer status
   if (buffersize < 1) unbuffered(1);
   else unbuffered(0);

   // disarm till initialized
   fcgx_strm = NULL;
   defined = 0;
   }


// destructor
fcgi_streambuf::~fcgi_streambuf(void) {}


// do nothing
int fcgi_streambuf::doallocate()
   {
   return (0);
   }


// drain the get area
// this method provides an advanced feature and is
// considered experimental, see the header file for docs
// it is provided as it may save somebody someday
int fcgi_streambuf::drain_strm(char *s, int n)
   {
   int numinget;     // number of chars in get buffer
   int numtoextract; // number of chars to pull out of the
                     // get area

   int i;            // counter

   // no meaning if not initialized
   if (!defined) return (EOF);
   // is already drained if no buffer
   if (unbuffered()) return (0);

   numinget = egptr() - gptr(); // calculate how much stuff is in
                                // the get buffer

   // bogus requests deserve bogus answers
   if (n<1) return (0);

   // an empty get area is an already drained get area
   if (numinget<1) return(0);

   // the number we extract is the lesser of the number of chars
   // in the get area and the number of chars the user's array
   // can hold. Maybe should use a ? operator here.
   if (numinget<n) numtoextract = numinget;
   else numtoextract = n;

   // copy over the data
   // adjust the get pointer
   // probably could use memcpy() to speed things up,
   // however this may not be portable and if you are
   // using drain(), performance is gonna take a back seat
   for (i=0;i<numtoextract;i++)
       {
       s[i] = *gptr();
       gbump(1);
       }
   // all done
   return (numtoextract);
   }


// little routine so that you can tell if the streambuf
// was ever initialized
int fcgi_streambuf::isstrmdefined(void)
   {
   return defined;
   }


// overflow is called for flush and every time the put buffer
// is full. Returns EOF on error
// This is the only method that actually writes data to
// the FCGI interface.
int fcgi_streambuf::overflow(int c)
  {
  int numinbuf; // number of chars in put buffer

  // we will not allow a bogus fcgx_strm (because the user
  // forgot to attach()) to do even more harm.
  if (!defined) return (EOF);

  // get the number of chars in the put buffer
  numinbuf = pptr() - pbase();

  // send out the entire put buffer
  if (numinbuf > 0 && !unbuffered())
     if (FCGX_PutStr(pbase(), numinbuf, fcgx_strm) < 0) return (EOF);

  // reset the put buffer to empty
  setp(pbase(),epptr());

  // if we are given an overflow char, send it out too.
  if (c >= 0)
     if (FCGX_PutChar(c, fcgx_strm) < 0) return (EOF);

  // all done
  return (0);
  }


// we have our own methods to setup buffering
// in case somebody calls this, ignore it
streambuf * fcgi_streambuf::setbuf(char *s, int n)
   {
   // tell them what they want to hear to make them
   // go away
   return this;
   }


// just sets up the pointer and declares things defined.
// used by fcgi_iostream attach()
void fcgi_streambuf::stream_initialize(FCGX_Stream *str, int dir)
   {
   // setup the main buffer
   setb(buffer,buffer + (buffersize * sizeof(char)),0);
   setp(0,0);
   setg(0,0,0);

   if (buffersize < 1) unbuffered(1);
   else // setup the get or put buffers
      {
      unbuffered(0);
      if (dir) // if writer
         {
         setg(0,0,0); // no get buffer
         setp(base(),ebuf()); // buffer is all put
         }
      else // reader
         {
         setg(base(),ebuf(),ebuf()); // buffer is all get
         setp(0,0); // no put buffer
         }
      }

   // set the FCGI interface
   fcgx_strm = str;
   // we are ready for action
   defined = 1;
   }


// flush all the output
int fcgi_streambuf::sync()
   {
   // flush the put area
   if (overflow(-1) < 0) return (EOF);

   // now flush FCGX
   return FCGX_FFlush(fcgx_strm);
   }


// Underflow is called to get characters to fill up the get buffer
// so something that uses the stream can get the data. If there
// is nothing else to read, this returns EOF. This is the only method
// which reads from the FCGI interface.
int fcgi_streambuf::underflow()
   {
   int numread;

   // we will not allow a bogus fcgx_strm (because the user
   // forgot to attach()) to do even more harm.
   if (!defined) return (EOF);

   // if it is unbuffered, then get & return a char
   if (unbuffered()) return FCGX_GetChar(fcgx_strm);

   // read as much data as we can, then adjust the get area so it
   // reflects the data we just read
   numread=FCGX_GetStr(base(), blen(), fcgx_strm);
   if (numread<=0) return EOF;
   setg(base(),base(),base()+numread);

   // We avoid a common bug. You NEED the unsigned char cast or
   // else this routine will return a false EOF when reading
   // control chars outside the normal ascii range!
   // i.e. "negative" chars map to positive ints and we
   // reserve negative return values for EOF and error
   // conditions. Now, even binary data will come through
   // perfectly.
   // The reason the get buffer uses chars rather
   // than unsigned chars is a mystery from AT&T history.
   // It probably has to due with the old 7bit text limits.
   return ((unsigned char)(*eback()));
   }



// here comes the higher level iostream stuff

// **** Istream

// parameterless constructor allows us to create first, then
// initialize later via attach()
fcgi_istream::fcgi_istream() {}


// or we can create and initialize in one step
// constructor calls ios::init to assign our streambuf to
// the istream derived class. Also sets up buffering.
fcgi_istream::fcgi_istream(FCGX_Stream *str)
   {
   // initialize as a reader, use all buffering for the
   // get area
   fcgi_strmbuf.stream_initialize(str,0);
   // init is a protected member of ios
   init(&fcgi_strmbuf);
   }


// destructor
fcgi_istream::~fcgi_istream() {}


// does everything the constructor with parameter does at the
// request of the programmer
void fcgi_istream::attach(FCGX_Stream *str)
   {
   // initialize as a reader, use all buffering for the
   // get area
   fcgi_strmbuf.stream_initialize(str,0);
   // init is a protected member of ios
   init(&fcgi_strmbuf);
   }


// experimental method to drain the get buffer. Allows you
// to sync reads with FCGI library (non-istream reads).
// reads upto n chars into the s array from the get buffer, does
// not call underflow(). Returned is the number of chars extracted
// or EOF on error. If n>0 (normal use) and returns a nonnegative
// value less than n, the get buffer is empty and it is safe to
// use an FCGI library read.
// see the header file for more info
int fcgi_istream::drain(char *s, int n)
   {
   return (fcgi_strmbuf.drain_strm(s,n));
   }


// little routine so that you can tell if the streambuf
// was ever defined
int fcgi_istream::isdefined(void)
   {
   return (fcgi_strmbuf.isstrmdefined());
   }



// **** Ostream

// parameterless constructor allows us to create first, then
// initialize later via attach()
fcgi_ostream::fcgi_ostream() {}


// or we can create and initialize in one step
// constructor calls ios::init to assign our streambuf to
// the ostream derived class. Also sets up buffering
fcgi_ostream::fcgi_ostream(FCGX_Stream *str)
  {
  // initialize as a writer, use all buffering for the
  // put area
  fcgi_strmbuf.stream_initialize(str,1);
  // init is a protected member of ios
  init(&fcgi_strmbuf);
  }


// destructor
fcgi_ostream::~fcgi_ostream()
   {
   // don't blowup if the user never defined the
   // stream
   if (fcgi_strmbuf.isstrmdefined())
      {
      // this may protect a few poor souls who forgot to flush
      // before deleting. Always flush when you are done.
      fcgi_strmbuf.sync();
      }
   }


// does everything the constructor with parameter does at the
// request of the programmer
void fcgi_ostream::attach(FCGX_Stream *str)
   {
   // initialize as a writer, use all buffering for the
   // put area
   fcgi_strmbuf.stream_initialize(str,1);
   // init is a protected member of ios
   init(&fcgi_strmbuf);
   }


// helper function to find out if this
// stream was ever initialized.
int fcgi_ostream::isdefined(void)
   {
   return (fcgi_strmbuf.isstrmdefined());
   }

