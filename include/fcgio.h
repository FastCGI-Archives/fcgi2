//
// $Id: fcgio.h,v 1.2 2000/07/21 20:56:28 robs Exp $
//
// Allows you communicate with FastCGI streams using C++ iostream
// objects
//
// defines classes fcgi_streambuf, fcgi_ostream, fcgi_istream
// you can redefine cin, cout, cerr or use your own custom
// FCGI stream names.
//
// ORIGINAL AUTHOR: George Feinberg
//
//
// REWRITTEN BY: Michael Richards  06/20/1999
//          -added cin support
//          -added ability to replace cin/cout so existing code only
//           needs to include the header file and use the normal
//           cin/cout. This has been altered as of 2/2000, see below.
//          -added buffered read support which is required for ungets.
//
//
// REWRITTEN AGAIN BY: Michael Shell 02/23/2000
//
//            Previous versions of this code had problems.
//            Two hellish bugs have been fixed and there is now full
//            buffering for both input and output streams.
//
//          - fixed signed char bug in underflow() that would
//            cause a false EOF to be flagged when reading binary
//            data. Uploading short binary files via <input type=file
//            which uses multipart/form-data encoding would reveal
//            this bug. Also could be triggered by hackers
//            sending binary data instead of legitimate form data,
//            in which case the hung network connection, depending on
//            how the FCGI application server handles things, could
//            form the basis for a stupid denial of service attack.
//          - fixed code to properly use the get and put buffers via
//            underflow() and overflow() NOT xsgetn() and xsputn() as
//            was done before. Because of this, the previous
//            version would often drop data, especially if the
//            user did an initial getline() followed by a read().
//          - added the attach() method and a parameterless
//            constructor so that you can declare fcgi_iostream
//            objects and attach() to them later - after accept().
//          - enhanced docs to include examples that actually work.
//          - removed any predefined redefinitions of cin,cout,cerr.
//            The example shows how you can use these names if you
//            want (via properly placed #undefs) or use ones of your
//            choosing (such as fin,fout,ferr). This may be very
//            helpful when testing code. Also, as a result, you
//            no longer have to place fcgio2.h in any special
//            order in your #includes.
//          - added an experimental method drain() to istream which
//            allows the user to drain the get buffer. This is
//            designed to provide users with a way to drain the get
//            buffer prior to using a read function from the FCGI
//            library in applications which mix I/O methods. i.e.
//            it is the input equivalent to flush(). It does not
//            read from the FCGI stream, but gets only characters
//            already in the istream buffer. Mixing I/O methods is
//            not recommended since this iostream implementation
//            is complete and should provide you with everything
//            you need.
//
//
// NOTES: encapsulates the FastCGI protocol in an iostream via a
// nice custom streambuf. Very nice, very robust, and very powerful.
//
// This work is based on routines written by George Feinberg. They
// have been mostly re-written and extensively changed by
// Michael Richards.
//
// Rewritten again with bug fixes and numerous enhancements by
// Michael Shell.
//
// Special Thanks to Dietmar Kuehl for his help and the numerous custom
// streambuf examples on his web site.
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
// If it breaks, you get to keep both halves.


// BEGIN EXAMPLE CODE: test_fcgio2.cpp
/*
// This code uses the fcgiapp interface to show a little bit more
// complexity and to demonstrate the fact that with fcgio2 and just
// a few more lines of code (like FCGX_Init etc.) you can easily
// make FastCGI programs without needing wrapper functions.
// You can use the fcgi_stdio interface if you you want a
// simpler accept(). However note that the fcgio2 interface
// removes the need for the fcgi_stdio wrapper functions.
// i.e. Why override printf when you aren't going to use it anyway?
// Also, be aware that because of iostream buffering, you must take
// care when mixing FCGI iostream I/O with the FCGI library I/O
// commands (such as printf). Be sure to flush() any open
// fcgi_ostreams prior to using commands such as printf. This is true
// even on systems which have the C I/O commands synced with C++
// iostreams, such as Linux, because the FCGI wrapper printf, etc. are
// not the same as the "normal" printf etc. It is recommended that you
// not use any FCGI library input (read) commands if you use
// fcgi_istream (cin) input as there is no easy way to "flush" (drain)
// an istream get buffer. However, an experimental istream method
// drain() has been provided to istream for those of you who need to
// do mixed input. There should be no need to do mixed I/O as the
// fcgio2 iostream implementation is complete.

#include <stdlib.h>
#include "fcgio2.h"  // fcgio2.h includes fcgiapp.h
                     // however you must include fcgi_stdio.h if
                     // you want to use it as fcgio2.h does not
                     // include it for you

#undef cin  // remember you have to undo the stuff predefined
#undef cout // for cin, cout, cerr if you wish to use these
#undef cerr // names for your FastCGI streams

int main(void)
{

 int count = 0;

 // I can create/declare my objects here, but I don't dare use them
 // until I set them up with attach().

 // note that version 1.0 of fcgio used fcgio_istream & fcgio_ostream
 // we made a little change to the names in V2.00 for clarity.
 fcgi_istream cin;  // you do not *HAVE* to use these names, you
 fcgi_ostream cout; // could use other stream names of your choice
 fcgi_ostream cerr; // don't forget that the input is
                    // fcgi_*I*stream class

 FCGX_Request request; // here is our request structure

 // get everything ready for the customers
 FCGX_Init();

 FCGX_InitRequest(&request,0,0);


 // let the games begin
 while(FCGX_Accept_r(&request) >= 0)
    {
    count++;

    cout.attach(request.out); // now I know my pointer values for
    cerr.attach(request.err); // this request. attach to them
    cin.attach(request.in);
    // attach will initialize everything
    // alternatively, you could declare the streams here and the
    // constructor with the (FCGX_Stream *str) parameter would
    // do the same job as attach

    // If you are using fcgi_stdio.h, the equivalent command would
    // be cout.attach(FCGI_stdout->fcgx_stream);
    // and so forth for cin,cerr using FCGI_stdin and FCGI_stderr
    // respectively.


    // now I can fire at will:
    cout << "Content-type: text/html\r\n\r\n"
         << "<title> FastCGI cin, cout, cerr tester </title>\r\n"
         << "<h1><center> FastCGI C++ IOstream test: "
         << "It works! </center></h1>\r\n";
    cout << "<h4><center><i> Total served by this task: "
         << count << "</i></center></h4>\r\n";

// didn't use cin or cerr in this example.

// it is good practice to flush the buffers.
// use cout.flush() if you don't want the line feed.

   cout << endl;

// there is no cxxx.close() and you do not need it with the fcgio
// interface. You would need to call cxxx.close() if cxxx was an
// fstream based object and attached to a file descripter you got
// from a command like fd=open(blah). (GNU (Linux) based fstreams
// support this) Then you need to call cxxx.close() before you
// close the physical file with close(fd). If doing this with
// fstream objects, you should call cxxx.clear() after
// attach(fd) as the file descriptor attach is not as complete in
// initialization as our fcgi_iostream attach()
// If you don't understand any of this, don't worry about it and
// forget I even mentioned it.

   // all done with this request
   FCGX_Finish_r(&request);

   // do the next request
   }
 return (1);
}

*/

// END EXAMPLE CODE

/*------------------------------------------------------------------*/


#ifndef FCGIO_H
#define FCGIO_H

#include <iostream.h>
#include <fcgiapp.h>


// FastCGI streambuf replacement. Implements low level I/O to the
// FastCGI C functions so our higher level iostreams will talk
// in the FastCGI protocol
class fcgi_streambuf : public streambuf
{
  public:
    // constructor
    fcgi_streambuf(void);
    ~fcgi_streambuf();

    // handles allocation of buffers by doing nothing since buffer
    // allocation isn't needed
    virtual int doallocate();

    // gets data (upto the given maximum) from the get buffer and
    // copies it to the given char array. Returns the number of chars
    // written or -1 on error. A returned value less than the given
    // maximum, assuming the user requested at least one char,
    // indicates that the get buffer is empty. The underflow() method
    // is never called to refill the get buffer, so this method can be
    // used to drain the get buffer. It is used to form an istream
    // drain() method which is the input equivalent to flush().
    virtual int drain_strm(char *,int);

    // let's us know if this strembuf has been initialized
    virtual int isstrmdefined(void);

    // (for writers) empties the put buffer and possibly an
    // overflow char into the FCGI interface
    virtual int overflow(int);

    // bogus routine in case somebody thinks they know
    // better and calls it. We currently are happy with
    // our static buffer.
    virtual streambuf * setbuf(char *, int);

    // initializes the buffering and FCGI interface
    virtual void stream_initialize(FCGX_Stream *,int);

    // (for writers) flushes the put buffer into the FCGI
    // interface and then flushes the FCGI interface
    virtual int sync();

    // (for readers) fills the get buffer with data from the
    // FCGI interface
    virtual int underflow();

  private:
    // pointer to our underlying FCGI interface
    FCGX_Stream * fcgx_strm;

    // our buffer
    // we aren't pulling from the heap, so it is best not
    // to make it too big
    static const int buffersize=200;
    char buffer[buffersize];

    // little flag so that we can tell if the
    // fcgi_str pointer was ever set
    int defined;
};



// Here's the istream class definition.
class fcgi_istream : public istream
{
  public:
    fcgi_istream();
    fcgi_istream(FCGX_Stream *str);
    ~fcgi_istream();

    // connects the fcgi_streambuf of  an existing fcgi_istream
    // object to a given FCGI interface
    virtual void attach(FCGX_Stream *str);

    // allows you to drain down the streambuf buffer. It will not
    // read any chars from the FCGI interface. You can repeatedly
    // call drain() to empty the get buffer prior to using a
    // FCGI library function to ensure syncronization of the
    // reads. i.e. it flushes the input stream
    // This method should be considered both nonstandard and
    // experimental. It's use is entirely optional to the user,
    // but could be very helpful for applications which use
    // both istream and FCGI library based reads on a single
    // FCGI interface and need a way to sync the reads.
    // It copies upto the given number of chars into the given
    // char array. It returns the number of chars written or
    // -1 on error. If the number of chars written is less than
    // the number the user requested, the get buffer is empty.
    // This method does not alter or check any of the input
    // status flags such as EOF or FAIL since it does not interact
    // with the underlying FCGI interface at all - it only reads from
    // the get buffer.
    virtual int drain(char *,int);

    // lets us know if this object has been initialized
    virtual int isdefined(void);

  private:
    // FastCGI streambuf
    fcgi_streambuf fcgi_strmbuf;

};



// Here's the ostream class definition.
class fcgi_ostream : public ostream
{
  public:
    fcgi_ostream(void);
    fcgi_ostream(FCGX_Stream *str);
    ~fcgi_ostream();

  // connects the fcgi_streambuf of an existing fcgi_ostream
  // object to a given FCGI interface
  virtual void attach(FCGX_Stream *str);

  // lets us know if this object has been initialized
  virtual int isdefined(void);

  private:
    // FastCGI streambuf
    fcgi_streambuf fcgi_strmbuf;
};

#endif FCGIO_H
