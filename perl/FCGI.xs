/* $Id: FCGI.XL,v 1.10 2003/06/22 00:24:11 robs Exp $ */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "fcgi_config.h"
#include "fcgiapp.h"
#include "fastcgi.h"

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef dTHX
#define dTHX
#endif

#ifndef INT2PTR
#define INT2PTR(a,b) ((a) (b))
#endif

/* Deprecation added 2010-10-05.  The deprecated functionality should not be
 * removed for at least a year after that. */
#define WIDE_CHAR_DEPRECATION_MSG "Use of wide characters in %s is deprecated" \
  " and will stop working in a future version of FCGI"

#if defined(USE_ITHREADS)
static perl_mutex accept_mutex;
#endif

typedef struct FCGP_Request {
    int         accepted;
    int         bound;
    SV*         svin;
    SV*         svout;
    SV*         sverr;
    GV*         gv[3];
    HV*         hvEnv;
    FCGX_Request*   requestPtr;
} FCGP_Request;

static void FCGI_Finish(FCGP_Request* request);

static void 
FCGI_Flush(FCGP_Request* request) {
    dTHX;
    if(!request->bound)
        return;
    FCGX_FFlush(INT2PTR(FCGX_Stream *, SvIV((SV*) SvRV(request->svout))));
    FCGX_FFlush(INT2PTR(FCGX_Stream *, SvIV((SV*) SvRV(request->sverr))));
}

static void
FCGI_UndoBinding(FCGP_Request* request) {
    dTHX;
#ifdef USE_PERLIO
    sv_unmagic((SV *)GvIOp(request->gv[0]), 'q');
    sv_unmagic((SV *)GvIOp(request->gv[1]), 'q');
    sv_unmagic((SV *)GvIOp(request->gv[2]), 'q');
#else
    sv_unmagic((SV *)request->gv[0], 'q');
    sv_unmagic((SV *)request->gv[1], 'q');
    sv_unmagic((SV *)request->gv[2], 'q');
#endif
    request->bound = FALSE;
}

static void
FCGI_Bind(FCGP_Request* request) {
    dTHX;
#ifdef USE_PERLIO
    /* For tied filehandles, we apply tiedscalar magic to the IO
       slot of the GP rather than the GV itself. */

    if (!GvIOp(request->gv[1]))
        GvIOp(request->gv[1]) = newIO();
    if (!GvIOp(request->gv[2]))
        GvIOp(request->gv[2]) = newIO();
    if (!GvIOp(request->gv[0]))
        GvIOp(request->gv[0]) = newIO();

    sv_magic((SV *)GvIOp(request->gv[1]), request->svout, 'q', Nullch, 0);
    sv_magic((SV *)GvIOp(request->gv[2]), request->sverr, 'q', Nullch, 0);
    sv_magic((SV *)GvIOp(request->gv[0]), request->svin, 'q', Nullch, 0);
#else
    sv_magic((SV *)request->gv[1], request->svout, 'q', Nullch, 0);
    sv_magic((SV *)request->gv[2], request->sverr, 'q', Nullch, 0);
    sv_magic((SV *)request->gv[0], request->svin, 'q', Nullch, 0);
#endif
    request->bound = TRUE;
}

static void
populate_env(char **envp, HV *hv) {
    int i;
    char *p, *p1;
    SV *sv;
    dTHX;

    hv_clear(hv);
    for(i = 0; ; i++) {
        if((p = envp[i]) == NULL)
            break;
        p1 = strchr(p, '=');
        assert(p1 != NULL);
        sv = newSVpv(p1 + 1, 0);
        /* call magic for this value ourselves */
        hv_store(hv, p, p1 - p, sv, 0);
        SvSETMAGIC(sv);
    }
}

static int
FCGI_IsFastCGI(FCGP_Request* request) {
    static int isCGI = -1; /* -1: not checked; 0: FCGI; 1: CGI */

    if (request->requestPtr->listen_sock == FCGI_LISTENSOCK_FILENO) {
        if (isCGI == -1)
            isCGI = FCGX_IsCGI();
        return !isCGI;
    }

    /* A explicit socket is being used -> assume FastCGI */
    return 1;
}

static int 
FCGI_Accept(FCGP_Request* request) {
    dTHX;

    if (!FCGI_IsFastCGI(request)) {
        static int been_here = 0;

        /*
        * Not first call to FCGI_Accept and running as CGI means
        * application is done.
        */
        if (been_here)
            return EOF;
        been_here = 1;
    } 
    else {
        FCGX_Request *fcgx_req = request->requestPtr;
        int acceptResult;

        FCGI_Finish(request);
#if defined(USE_ITHREADS)
        MUTEX_LOCK(&accept_mutex);
#endif
        acceptResult = FCGX_Accept_r(fcgx_req);
#if defined(USE_ITHREADS)
        MUTEX_UNLOCK(&accept_mutex);
#endif
        if(acceptResult < 0) {
            return acceptResult;
        }

        populate_env(fcgx_req->envp, request->hvEnv);

        if (!request->svout) {
            newSVrv(request->svout = newSV(0), "FCGI::Stream");
            newSVrv(request->sverr = newSV(0), "FCGI::Stream");
            newSVrv(request->svin = newSV(0), "FCGI::Stream");
        }
        sv_setiv(SvRV(request->svout), INT2PTR(IV, fcgx_req->out));
        sv_setiv(SvRV(request->sverr), INT2PTR(IV, fcgx_req->err));
        sv_setiv(SvRV(request->svin), INT2PTR(IV, fcgx_req->in));
        FCGI_Bind(request);
        request->accepted = TRUE;
    }
    return 0;
}

static void 
FCGI_Finish(FCGP_Request* request) {
    int was_bound;
    dTHX;

    if(!request->accepted)
        return;

    if (was_bound = request->bound)
        FCGI_UndoBinding(request);
    if (was_bound)
        FCGX_Finish_r(request->requestPtr);
    else
        FCGX_Free(request->requestPtr, 1);
    request->accepted = FALSE;
}

static int 
FCGI_StartFilterData(FCGP_Request* request) {
    return request->requestPtr->in ? 
        FCGX_StartFilterData(request->requestPtr->in) : -1;
}

static FCGP_Request *
FCGI_Request(GV *in, GV *out, GV *err, HV *env, int socket, int flags) {
    FCGX_Request* fcgx_req;
    FCGP_Request* req;

    Newz(551, fcgx_req, 1, FCGX_Request);
    FCGX_InitRequest(fcgx_req, socket, flags);
    Newz(551, req, 1, FCGP_Request);
    req->requestPtr = fcgx_req;
    SvREFCNT_inc(in);
    req->gv[0] = in;
    SvREFCNT_inc(out);
    req->gv[1] = out;
    SvREFCNT_inc(err);
    req->gv[2] = err;
    SvREFCNT_inc(env);
    req->hvEnv = env;

    return req;
}

static void
FCGI_Release_Request(FCGP_Request *req) {
    SvREFCNT_dec(req->gv[0]);
    SvREFCNT_dec(req->gv[1]);
    SvREFCNT_dec(req->gv[2]);
    SvREFCNT_dec(req->hvEnv);
    FCGI_Finish(req);
    Safefree(req->requestPtr);
    Safefree(req);
}

static void
FCGI_Init() {
#if defined(USE_ITHREADS)
    dTHX;
    MUTEX_INIT(&accept_mutex);
#endif
    FCGX_Init();
}

typedef FCGX_Stream* FCGI__Stream;
typedef FCGP_Request* FCGI;
typedef GV* GLOBREF;
typedef HV* HASHREF;

MODULE = FCGI PACKAGE = FCGI PREFIX = FCGI_

BOOT:
    FCGI_Init();

SV *
RequestX(in, out, err, env, socket, flags)
    GLOBREF in;
    GLOBREF out;
    GLOBREF err;
    HASHREF env;
    int     socket;
    int     flags;
  PROTOTYPE: ***$$$
  CODE:
    RETVAL = sv_setref_pv(newSV(0), "FCGI", 
        FCGI_Request(in, out, err, env, socket, flags));
  OUTPUT:
    RETVAL

int
OpenSocket(path, backlog)
    char* path;
    int backlog;
  PROTOTYPE: $$
  CODE:
    RETVAL = FCGX_OpenSocket(path, backlog);
  OUTPUT:
    RETVAL

void
CloseSocket(socket)
    int socket;
  PROTOTYPE: $
  CODE:
    close(socket);

int
FCGI_Accept(request)
    FCGI    request;
  PROTOTYPE: $

void
FCGI_Finish(request)
    FCGI request;
  PROTOTYPE: $

void
FCGI_Flush(request)
    FCGI request;
  PROTOTYPE: $

HV *
GetEnvironment(request)
    FCGI request;
  PROTOTYPE: $
  CODE:
    RETVAL = request->hvEnv;
  OUTPUT: 
    RETVAL

void
GetHandles(request)
    FCGI request;
  PROTOTYPE: $
  PREINIT:
    int i;
  PPCODE:
    EXTEND(sp,3);
    for (i = 0; i < 3; ++i)
        PUSHs(sv_2mortal(newRV((SV *) request->gv[i])));

int
FCGI_IsFastCGI(request)
    FCGI request;
  PROTOTYPE: $

void
Detach(request)
    FCGI request;
  PROTOTYPE: $
  CODE:
    if (request->accepted && request->bound) {
        FCGI_UndoBinding(request);
        FCGX_Detach(request->requestPtr);
    }

void
Attach(request)
    FCGI request;
  PROTOTYPE: $
  CODE:
    if (request->accepted && !request->bound) {
        FCGI_Bind(request);
        FCGX_Attach(request->requestPtr);
    }

void
LastCall(request)
    FCGI request;
  PROTOTYPE: $
  CODE:
    FCGX_ShutdownPending();

int
FCGI_StartFilterData(request)
    FCGI request;
  PROTOTYPE: $

void
DESTROY(request)
    FCGI request;
  CODE:
    FCGI_Release_Request(request);

MODULE = FCGI PACKAGE = FCGI::Stream

SV *
PRINT(stream, ...)
    FCGI::Stream stream;
  PREINIT:
    int n;
    STRLEN len;
    register char *str;
    bool ok = TRUE;
  CODE:
    for (n = 1; ok && n < items; ++n) {
#ifdef DO_UTF8
        if (DO_UTF8(ST(n)) && !sv_utf8_downgrade(ST(n), 1) && ckWARN_d(WARN_UTF8))
            Perl_warner(aTHX_ WARN_UTF8, WIDE_CHAR_DEPRECATION_MSG,
                        "FCGI::Stream::PRINT");
#endif
        str = (char *)SvPV(ST(n),len);
        if (FCGX_PutStr(str, len, stream) < 0)
            ok = FALSE;
    }
    if (ok && SvTRUEx(perl_get_sv("|", FALSE)) && FCGX_FFlush(stream) < 0)
        ok = FALSE;
    RETVAL = ok ? &PL_sv_yes : &PL_sv_undef;
  OUTPUT:
    RETVAL

int
WRITE(stream, bufsv, len, ...)
    FCGI::Stream stream;
    SV *bufsv;
    int len;
  PREINIT:
    int offset;
    char *buf;
    STRLEN blen;
    int n;
  CODE:
    offset = (items == 4) ? (int)SvIV(ST(3)) : 0;
#ifdef DO_UTF8
    if (DO_UTF8(bufsv) && !sv_utf8_downgrade(bufsv, 1) && ckWARN_d(WARN_UTF8))
         Perl_warner(aTHX_ WARN_UTF8, WIDE_CHAR_DEPRECATION_MSG,
                     "FCGI::Stream::WRITE");
#endif
    buf = SvPV(bufsv, blen);
    if (offset < 0) offset += blen;
    if (len > blen - offset)
        len = blen - offset;
    if (offset < 0 || offset >= blen ||
        (n = FCGX_PutStr(buf+offset, len, stream)) < 0) 
        ST(0) = &PL_sv_undef;
    else {
        ST(0) = sv_newmortal();
        sv_setiv(ST(0), n);
    }

void
READ(stream, bufsv, len, ...)
    FCGI::Stream stream;
    SV *bufsv;
    int len;
  PREINIT:
    int offset = 0;
    char *buf;
    STRLEN blen;
  CODE:
    if (items < 3 || items > 4)
        croak("Usage: FCGI::Stream::READ(STREAM, SCALAR, LENGTH [, OFFSET ])");
    if (len < 0)
        croak("Negative length");
    if (!SvOK(bufsv))
        sv_setpvn(bufsv, "", 0);
#ifdef DO_UTF8
    if (DO_UTF8(bufsv) && !sv_utf8_downgrade(bufsv, 1) && ckWARN_d(WARN_UTF8))
         Perl_warner(aTHX_ WARN_UTF8, WIDE_CHAR_DEPRECATION_MSG,
                     "FCGI::Stream::READ");
#endif
    buf = SvPV_force(bufsv, blen);
    if (items == 4) {
        offset = SvIV(ST(3));
        if (offset < 0) {
            if (-offset > (int)blen)
                croak("Offset outside string");
            offset += blen;
        }
    }
    buf = SvGROW(bufsv, len + offset + 1);
    if (offset > blen)
        Zero(buf + blen, offset - blen, char);
    len = FCGX_GetStr(buf + offset, len, stream);
    SvCUR_set(bufsv, len + offset);
    *SvEND(bufsv) = '\0';
    (void)SvPOK_only(bufsv);
    SvSETMAGIC(bufsv);
    XSRETURN_IV(len);

SV *
GETC(stream)
    FCGI::Stream stream;
  PREINIT:
    int retval;
  CODE:
    if ((retval = FCGX_GetChar(stream)) != -1) {
        ST(0) = sv_newmortal();
        sv_setpvf(ST(0), "%c", retval);
    }
    else
        ST(0) = &PL_sv_undef;

SV *
EOF(stream, called=0)
    FCGI::Stream stream;
    IV called;
  CODE:
    RETVAL = boolSV(FCGX_HasSeenEOF(stream));
  OUTPUT:
    RETVAL

void
FILENO(stream)
    FCGI::Stream stream;
  CODE:
    if (FCGX_HasSeenEOF(stream) != 0)
        XSRETURN_UNDEF;
    else
        XSRETURN_IV(-1);

bool
CLOSE(stream)
    FCGI::Stream stream;
  CODE:
    RETVAL = FCGX_FClose(stream) != -1;
  OUTPUT:
    RETVAL
