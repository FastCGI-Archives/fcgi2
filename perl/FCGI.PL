use Config;

open OUT, ">FCGI.xs";

print "Generating FCGI.xs for Perl version $]\n";
#unless (exists $Config{apiversion} && $Config{apiversion} >= 5.005) 
unless ($] >= 5.005) {
    for (qw(sv_undef diehook warnhook in_eval)) {
	print OUT "#define PL_$_ $_\n" 
    }
}
print OUT while <DATA>;
close OUT;
__END__
/* $Id: FCGI.PL,v 1.6 1999/07/28 23:09:56 skimo Exp $ */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "fcgiapp.h"

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#endif

#ifdef WIN32
#define environ _environ
#endif

extern char **environ;

#ifdef USE_SFIO
typedef struct
{
    Sfdisc_t	disc;
    FCGX_Stream	*stream;
} FCGI_Disc;

static ssize_t
sffcgiread(f, buf, n, disc)
Sfio_t*		f;      /* stream involved */
Void_t*		buf;    /* buffer to read into */
size_t		n;      /* number of bytes to read */
Sfdisc_t*	disc;   /* discipline */
{
    return FCGX_GetStr(buf, n, ((FCGI_Disc *)disc)->stream);
}

static ssize_t
sffcgiwrite(f, buf, n, disc)
Sfio_t*		f;      /* stream involved */
const Void_t*	buf;    /* buffer to read into */
size_t		n;      /* number of bytes to read */
Sfdisc_t*	disc;   /* discipline */
{
    n = FCGX_PutStr(buf, n, ((FCGI_Disc *)disc)->stream);
    FCGX_FFlush(((FCGI_Disc *)disc)->stream);
    return n;
}

Sfdisc_t *
sfdcnewfcgi(stream)
	FCGX_Stream *stream;
{
    FCGI_Disc*	disc;

    New(1000,disc,1,FCGI_Disc);
    if (!disc) return (Sfdisc_t *)disc;

    disc->disc.exceptf = (Sfexcept_f)NULL;
    disc->disc.seekf = (Sfseek_f)NULL;
    disc->disc.readf = sffcgiread;
    disc->disc.writef = sffcgiwrite;
    disc->stream = stream;
    return (Sfdisc_t *)disc;
}

Sfdisc_t *
sfdcdelfcgi(disc)
    Sfdisc_t*	disc;
{
    Safefree(disc);
    return 0;
}
#endif

static int isCGI = -1; /* -1: not checked; 0: FCGI; 1: FCGI */

static FCGX_Request global_fcgx_request;

typedef struct FCGP_Request {
    int		    compat;
    int		    acceptCalled;
    int		    finishCalled;
    SV*		    svin;
    SV*		    svout;
    SV*		    sverr;
    GV*		    gv[3];
    HV*		    hvEnv;
    FCGX_Stream*    in;
    FCGX_ParamArray envp;
    FCGX_Request*   requestPtr;
} FCGP_Request;

static FCGP_Request global_request;
static SV*	    global_sverr;

static int 
FCGI_Flush(FCGP_Request* request)
{
    if(!request->acceptCalled || isCGI) {
	return;
    }
#ifdef USE_SFIO
    sfsync(IoIFP(GvIOp(request->gv[1])));
    sfsync(IoIFP(GvIOp(request->gv[2])));
#else
    FCGX_FFlush((FCGX_Stream *) SvIV((SV*) SvRV(request->svout)));
    FCGX_FFlush((FCGX_Stream *) SvIV((SV*) SvRV(request->sverr)));
#endif
}

static void
FCGI_UndoBinding(FCGP_Request* request)
{
#ifdef USE_SFIO
    sfdcdelfcgi(sfdisc(IoIFP(GvIOp(request->gv[0])), SF_POPDISC));
    sfdcdelfcgi(sfdisc(IoIFP(GvIOp(request->gv[1])), SF_POPDISC));
    sfdcdelfcgi(sfdisc(IoIFP(GvIOp(request->gv[2])), SF_POPDISC));
#else
    FCGI_Flush(request);
    sv_unmagic((SV *)request->gv[0], 'q');
    sv_unmagic((SV *)request->gv[1], 'q');
    sv_unmagic((SV *)request->gv[2], 'q');
#endif
}

static int 
FCGI_Accept(FCGP_Request* request, GV **gvp)
{
    if(isCGI == -1) {
        /*
         * First call to FCGI_Accept.  Is application running
         * as FastCGI or as CGI?
         */
        isCGI = FCGX_IsCGI();
    } else if(isCGI) {
        /*
         * Not first call to FCGI_Accept and running as CGI means
         * application is done.
         */
        return(EOF);
    } 
    if(request->acceptCalled && !request->finishCalled) {
	FCGI_UndoBinding(request);
    }
    if(!isCGI) {
	FCGX_Stream *out, *error;
        int acceptResult = FCGX_Accept_r(&request->in, &out, &error, 
					 &request->envp,
				         request->requestPtr);
        if(acceptResult < 0) {
            return acceptResult;
        }
#ifdef USE_SFIO
        sfdisc(IoIFP(GvIOp(request->gv[0])), sfdcnewfcgi(request->in));
        sfdisc(IoIFP(GvIOp(request->gv[1])), sfdcnewfcgi(out));
        sfdisc(IoIFP(GvIOp(request->gv[2])), sfdcnewfcgi(error));
#else
	if (!request->svout) {
	    newSVrv(request->svout = newSV(0), "FCGI::Stream");
	    newSVrv(request->sverr = newSV(0), "FCGI::Stream");
	    newSVrv(request->svin = newSV(0), "FCGI::Stream");
	}
	sv_magic((SV *)request->gv[1] = gvp[1], request->svout, 'q', Nullch, 0);
	sv_magic((SV *)request->gv[2] = gvp[2], request->sverr, 'q', Nullch, 0);
	sv_magic((SV *)request->gv[0] = gvp[0], request->svin, 'q', Nullch, 0);
	sv_setiv(SvRV(request->svout), (IV) out);
	sv_setiv(SvRV(request->sverr), (IV) error);
	sv_setiv(SvRV(request->svin), (IV) request->in);

	if (request->compat) {
	    global_sverr = request->sverr;
	    if (PL_warnhook) SvREFCNT_dec(PL_warnhook);
	    PL_warnhook = SvREFCNT_inc(GvCV(gv_fetchmethod(Nullhv, "FCGI::WARN")));
	    if (PL_diehook) SvREFCNT_dec(PL_diehook);
	    PL_diehook = SvREFCNT_inc(GvCV(gv_fetchmethod(Nullhv, "FCGI::DIE")));
	}
#endif
	request->finishCalled = FALSE;
    }
    request->acceptCalled = TRUE;
    return 0;
}

static void 
FCGI_Finish(FCGP_Request* request)
{
    if(!request->acceptCalled || isCGI) {
	return;
    }
    FCGI_UndoBinding(request);
    request->in = NULL;
    FCGX_Finish_r(request->requestPtr);
    request->finishCalled = TRUE;
#ifndef USE_SFIO
    if (request->compat) {
	if (PL_warnhook == (SV*)GvCV(gv_fetchmethod(Nullhv, "FCGI::WARN"))) {
	    SvREFCNT_dec(PL_warnhook);
	    PL_warnhook = Nullsv;
	}
	if (PL_diehook == (SV*)GvCV(gv_fetchmethod(Nullhv, "FCGI::DIE"))) {
	    SvREFCNT_dec(PL_diehook);
	    PL_diehook = Nullsv;
	}
    }
#endif
}

static int 
FCGI_StartFilterData(FCGP_Request* request)
{
    return request->in ? FCGX_StartFilterData(request->in) : -1;
}

static void
FCGI_SetExitStatus(FCGP_Request* request, int status)
{
    if (request->in) FCGX_SetExitStatus(status, request->in);
}

static FCGP_Request *
FCGI_Request()
{
    FCGX_Request* fcgx_req;
    FCGP_Request* req;

    Newz(551, fcgx_req, 1, FCGX_Request);
    Newz(551, req, 1, FCGP_Request);
    req->requestPtr = fcgx_req;

    return req;
}

static void
FCGI_Release_Request(FCGP_Request *req)
{
    Safefree(req->requestPtr);
    Safefree(req);
}

static void
populate_env(envp, hv)
char **envp;
HV *hv;
{
    int i;
    char *p, *p1;
    SV   *sv;

    for(i = 0; ; i++) {
	if((p = envp[i]) == NULL) {
	    break;
	}
	p1 = strchr(p, '=');
	assert(p1 != NULL);
	sv = newSVpv(p1 + 1, 0);
	/* call magic for this value ourselves */
	hv_store(hv, p, p1 - p, sv, 0);
	SvSETMAGIC(sv);
    }
}

/*
 * For each variable in the array envp, either set or unset it
 * in the global hash %ENV.
 */
static void
DoPerlEnv(envp, set)
char **envp;
int set;
{
    if (!set)
	perl_eval_pv("%ENV = %FCGI::ENV", 0);
    else {
	perl_eval_pv("%FCGI::ENV = %ENV", 0);
	populate_env(envp, perl_get_hv("ENV", TRUE));
    }
}

#define REQUEST_ARG(arg,request) \
	if (sv_isa(ST(arg), "FCGI")) { \
	    request = (FCGP_Request*) SvIV((SV*)SvRV(ST(arg))); \
	} else \
	    croak("request is not of type FCGI")

typedef FCGX_Stream *	FCGI__Stream;
typedef FCGP_Request *	FCGI;

MODULE = FCGI		PACKAGE = FCGI

BOOT:
    FCGX_Init();
    FCGX_InitRequest(&global_fcgx_request);
    memset(&global_request, 0, sizeof(global_request));
    global_request.compat = 1;
    global_request.requestPtr = &global_fcgx_request;
    global_request.gv[0] = gv_fetchpv("STDIN",TRUE, SVt_PVIO);
    global_request.gv[1] = gv_fetchpv("STDOUT",TRUE, SVt_PVIO);
    global_request.gv[2] = gv_fetchpv("STDERR",TRUE, SVt_PVIO);


SV *
request()

    PROTOTYPE:
    CODE:
    RETVAL = Perl_sv_setref_pv(Perl_newSV(0), "FCGI", FCGI_Request());

    OUTPUT:
    RETVAL


int
accept(...)

    PROTOTYPE: ;$***$

    PREINIT:
    FCGP_Request* request = &global_request;
    GV *gv[3];

    CODE:
    if (items != 0 && items != 5)
	croak("Usage: FCGI::accept() or "
	      "FCGI::accept(request, IN, OUT, ERR, env)");
    if (items) {
	static const char* names[] = {"", "IN", "OUT", "ERR"};
	int i;

	REQUEST_ARG(0,request);
	for(i = 1; i <= 3; ++i) {
	    if (SvROK(ST(i)) && isGV(SvRV(ST(i)))) {
		gv[i-1] = (GV*)SvRV(ST(i));
	    } else
		croak("%s is not a GLOB reference", names[i]);
	}
	if (SvROK(ST(4)) && SvTYPE(SvRV(ST(4))) == SVt_PVHV) {
	    request->hvEnv = (HV*)SvRV(ST(4));
	} else
	    croak("env is not a reference to a hash");
    }
    {
        int acceptStatus;
        /*
         * Unmake Perl variable settings for the request just completed.
         */
        if(request->envp != NULL) {
            DoPerlEnv(request->envp, FALSE);
            request->envp = NULL;
        }
        /*
         * Call FCGI_Accept but preserve environ.
         */
        acceptStatus = FCGI_Accept(request, gv);
        /*
         * Make Perl variable settings for the new request.
         */
        if(acceptStatus >= 0 && !isCGI) {
	    if (request->compat)
		DoPerlEnv(request->envp, TRUE);
	    else {
		populate_env(request->envp, request->hvEnv);
		request->envp = NULL;
	    }
        } else {
            request->envp = NULL;
        }
        RETVAL = acceptStatus;
    }
    OUTPUT:
    RETVAL


void
finish(...)

    PROTOTYPE: ;$

    PREINIT:
    FCGP_Request* request = &global_request;

    CODE:
    if (items != 0 && items != 1)
	croak("Usage: FCGI::finish() or "
	      "FCGI::finish(request)");
    if (items) {
	REQUEST_ARG(0,request);
    }
    {
        /*
         * Unmake Perl variable settings for the completed request.
         */
        if(request->envp != NULL) {
            DoPerlEnv(request->envp, FALSE);
            request->envp = NULL;
        }
        /*
         * Finish the request.
         */
        FCGI_Finish(request);
    }


void
flush(...)

    PROTOTYPE: ;$

    PREINIT:
    FCGP_Request* request = &global_request;

    CODE:
    if (items != 0 && items != 1)
	croak("Usage: FCGI::flush([request])");
    if (items) {
	REQUEST_ARG(0,request);
    }
    FCGI_Flush(request);

void
set_exit_status(status,...)

    int status;

    PROTOTYPE: $;$

    PREINIT:
    FCGP_Request* request = &global_request;

    CODE:
    if (items != 1 && items != 2)
	croak("Usage: FCGI::set_exit_status(status[,request])");
    if (items == 2) {
	REQUEST_ARG(1,request);
    }
    FCGI_SetExitStatus(request, status);

int
start_filter_data(...)

    PROTOTYPE: ;$

    PREINIT:
    FCGP_Request* request = &global_request;

    CODE:
    if (items != 0 && items != 1)
	croak("Usage: FCGI::flush([request])");
    if (items) {
	REQUEST_ARG(0,request);
    }
    RETVAL = FCGI_StartFilterData(request);

    OUTPUT:
    RETVAL


void
DESTROY(request)
    FCGI    request;

    CODE:
    FCGI_Release_Request(request);



MODULE = FCGI		PACKAGE = FCGI::Stream

#ifndef USE_SFIO
void
DIE(msg)
	char *	msg;

	CODE:
	if (!PL_in_eval)
	    FCGX_PutS(msg, (FCGX_Stream *) SvIV((SV*) SvRV(global_sverr)));

void
WARN(msg)
	char *	msg;

	CODE:
	FCGX_PutS(msg, (FCGX_Stream *) SvIV((SV*) SvRV(global_sverr)));

void
PRINT(stream, ...)
	FCGI::Stream	stream;

	PREINIT:
	int	n;

	CODE:
	for (n = 1; n < items; ++n) {
            STRLEN len;
            register char *tmps = (char *)SvPV(ST(n),len);
            FCGX_PutStr(tmps, len, stream);
	}
	if (SvTRUEx(perl_get_sv("|", FALSE))) 
	    FCGX_FFlush(stream);

int
WRITE(stream, bufsv, len, ...)
	FCGI::Stream	stream;
	SV *	bufsv;
	int	len;

	PREINIT:
	int	offset;
	char *	buf;
	STRLEN	blen;
	int	n;

	CODE:
	offset = (items == 4) ? (int)SvIV(ST(3)) : 0;
	buf = SvPV(bufsv, blen);
	if (offset < 0) offset += blen;
	if (len > blen - offset)
	    len = blen - offset;
	if (offset < 0 || offset >= blen ||
		(n = FCGX_PutStr(buf+offset, len, stream)) < 0) 
	    ST(0) = &PL_sv_undef;
	else {
	    ST(0) = sv_newmortal();
	    sv_setpvf(ST(0), "%c", n);
	}

int
READ(stream, bufsv, len, ...)
	FCGI::Stream	stream;
	SV *	bufsv;
	int	len;

	PREINIT:
	int	offset;
	char *	buf;

	CODE:
	offset = (items == 4) ? (int)SvIV(ST(3)) : 0;
	if (! SvOK(bufsv))
	    sv_setpvn(bufsv, "", 0);
	buf = SvGROW(bufsv, len+offset+1);
	len = FCGX_GetStr(buf+offset, len, stream);
	SvCUR_set(bufsv, len+offset);
	*SvEND(bufsv) = '\0';
	(void)SvPOK_only(bufsv);
	SvSETMAGIC(bufsv);
	RETVAL = len;

	OUTPUT:
	RETVAL

SV *
GETC(stream)
	FCGI::Stream	stream;

	PREINIT:
	int	retval;

	CODE:
	if ((retval = FCGX_GetChar(stream)) != -1) {
	    ST(0) = sv_newmortal();
	    sv_setpvf(ST(0), "%c", retval);
	} else ST(0) = &PL_sv_undef;

bool
CLOSE(stream)
	FCGI::Stream	stream;

	ALIAS:
	DESTROY = 1

	CODE:
	RETVAL = FCGX_FClose(stream) != -1;

	OUTPUT:
	RETVAL

#endif
