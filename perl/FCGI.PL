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
/* $Id: FCGI.PL,v 1.3 1999/03/08 16:26:04 skimo Exp $ */

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
static char **requestEnviron = NULL;

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

static int acceptCalled = FALSE;
static int finishCalled = FALSE;
static int isCGI = FALSE;
static FCGX_Stream *in = NULL;
static SV *svout = NULL, *svin, *sverr;

static int 
FCGI_Flush(void)
{
    if(!acceptCalled || isCGI) {
	return;
    }
#ifdef USE_SFIO
    sfsync(PerlIO_stdout());
    sfsync(PerlIO_stderr());
#else
    FCGX_FFlush((FCGX_Stream *) SvIV((SV*) SvRV(svout)));
    FCGX_FFlush((FCGX_Stream *) SvIV((SV*) SvRV(sverr)));
#endif
}

static int 
FCGI_Accept(void)
{
    if(!acceptCalled) {
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
    } else {
	if(!finishCalled) {
#ifdef USE_SFIO
            sfdcdelfcgi(sfdisc(PerlIO_stdin(), SF_POPDISC));
            sfdcdelfcgi(sfdisc(PerlIO_stdout(), SF_POPDISC));
            sfdcdelfcgi(sfdisc(PerlIO_stderr(), SF_POPDISC));
#else
	    FCGI_Flush();
#endif
	}
    }
    if(!isCGI) {
        FCGX_ParamArray envp;
	FCGX_Stream *out, *error;
        int acceptResult = FCGX_Accept(&in, &out, &error, &envp);
        if(acceptResult < 0) {
            return acceptResult;
        }
#ifdef USE_SFIO
        sfdisc(PerlIO_stdin(), sfdcnewfcgi(in));
        sfdisc(PerlIO_stdout(), sfdcnewfcgi(out));
        sfdisc(PerlIO_stderr(), sfdcnewfcgi(error));
#else
	if (!svout) {
	    newSVrv(svout = newSV(0), "FCGI");
	    sv_magic((SV *)gv_fetchpv("STDOUT",TRUE, SVt_PVIO), 
			svout, 'q', Nullch, 0);
	    newSVrv(sverr = newSV(0), "FCGI");
	    sv_magic((SV *)gv_fetchpv("STDERR",TRUE, SVt_PVIO), 
			sverr, 'q', Nullch, 0);
	    newSVrv(svin = newSV(0), "FCGI");
	    sv_magic((SV *)gv_fetchpv("STDIN",TRUE, SVt_PVIO), 
			svin, 'q', Nullch, 0);
	}
	sv_setiv(SvRV(svout), (IV) out);
	sv_setiv(SvRV(sverr), (IV) error);
	sv_setiv(SvRV(svin), (IV) in);
	if (!SvTRUEx(perl_get_sv("FCGI::no_warn_redirection", FALSE)))
	{
	    if (PL_warnhook) SvREFCNT_dec(PL_warnhook);
	    PL_warnhook = SvREFCNT_inc(GvCV(gv_fetchmethod(Nullhv, "FCGI::WARN")));
	}
	if (!SvTRUEx(perl_get_sv("FCGI::no_die_redirection", FALSE)))
	{
	    if (PL_diehook) SvREFCNT_dec(PL_diehook);
	    PL_diehook = SvREFCNT_inc(GvCV(gv_fetchmethod(Nullhv, "FCGI::DIE")));
	}
#endif
	finishCalled = FALSE;
        environ = envp;
    }
    acceptCalled = TRUE;
    return 0;
}

static void 
FCGI_Finish(void)
{
    if(!acceptCalled || isCGI) {
	return;
    }
#ifdef USE_SFIO
    sfdcdelfcgi(sfdisc(PerlIO_stdin(), SF_POPDISC));
    sfdcdelfcgi(sfdisc(PerlIO_stdout(), SF_POPDISC));
    sfdcdelfcgi(sfdisc(PerlIO_stderr(), SF_POPDISC));
#else
    FCGI_Flush();
#endif
    in = NULL;
    FCGX_Finish();
    /*
    environ = NULL;
    */
    finishCalled = TRUE;
#ifndef USE_SFIO
    if (!SvTRUEx(perl_get_sv("FCGI::no_warn_redirection", FALSE)) &&
	    PL_warnhook == (SV*)GvCV(gv_fetchmethod(Nullhv, "FCGI::WARN"))) {
	SvREFCNT_dec(PL_warnhook);
	PL_warnhook = Nullsv;
    }
    if (!SvTRUEx(perl_get_sv("FCGI::no_die_redirection", FALSE)) &&
	    PL_diehook == (SV*)GvCV(gv_fetchmethod(Nullhv, "FCGI::DIE"))) {
	SvREFCNT_dec(PL_diehook);
	PL_diehook = Nullsv;
    }
#endif
}

static int 
FCGI_StartFilterData(void)
{
    return in ? FCGX_StartFilterData(in) : -1;
}

static void
FCGI_SetExitStatus(int status)
{
    if (in) FCGX_SetExitStatus(status, in);
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
    int i;
    char *p, *p1;
    HV   *hv;
    SV   *sv;
    hv = perl_get_hv("ENV", TRUE);

    if (!set)
	perl_eval_pv("%ENV = %FCGI::ENV", 0);
    else {
	perl_eval_pv("%FCGI::ENV = %ENV", 0);
	for(i = 0; ; i++) {
	    if((p = envp[i]) == NULL) {
		break;
	    }
	    p1 = strchr(p, '=');
	    assert(p1 != NULL);
	    *p1 = '\0';
	    sv = newSVpv(p1 + 1, 0);
	    /* call magic for this value ourselves */
	    hv_store(hv, p, p1 - p, sv, 0);
	    SvSETMAGIC(sv);
	    *p1 = '=';
	}
    }
}


typedef FCGX_Stream *	FCGI;

MODULE = FCGI		PACKAGE = FCGI

#ifndef USE_SFIO
void
DIE(msg)
	char *	msg;

	CODE:
	if (!PL_in_eval)
	    FCGX_PutS(msg, (FCGX_Stream *) SvIV((SV*) SvRV(sverr)));

void
WARN(msg)
	char *	msg;

	CODE:
	FCGX_PutS(msg, (FCGX_Stream *) SvIV((SV*) SvRV(sverr)));

void
PRINT(stream, ...)
	FCGI	stream;

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
	FCGI	stream;
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
	FCGI	stream;
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
	FCGI	stream;

	PREINIT:
	int	retval;

	CODE:
	if ((retval = FCGX_GetChar(stream)) != -1) {
	    ST(0) = sv_newmortal();
	    sv_setpvf(ST(0), "%c", retval);
	} else ST(0) = &PL_sv_undef;

bool
CLOSE(stream)
	FCGI	stream;

	ALIAS:
	DESTROY = 1

	CODE:
	RETVAL = FCGX_FClose(stream) != -1;

	OUTPUT:
	RETVAL

#endif

int
accept()

    PROTOTYPE:
    CODE:
    {
        char **savedEnviron;
        int acceptStatus;
        /*
         * Unmake Perl variable settings for the request just completed.
         */
        if(requestEnviron != NULL) {
            DoPerlEnv(requestEnviron, FALSE);
            requestEnviron = NULL;
        }
        /*
         * Call FCGI_Accept but preserve environ.
         */
        savedEnviron = environ;
        acceptStatus = FCGI_Accept();
        requestEnviron = environ;
        environ = savedEnviron;
        /*
         * Make Perl variable settings for the new request.
         */
        if(acceptStatus >= 0 && !FCGX_IsCGI()) {
            DoPerlEnv(requestEnviron, TRUE);
        } else {
            requestEnviron = NULL;
        }
        RETVAL = acceptStatus;
    }
    OUTPUT:
    RETVAL


void
finish()

    PROTOTYPE:
    CODE:
    {
        /*
         * Unmake Perl variable settings for the completed request.
         */
        if(requestEnviron != NULL) {
            DoPerlEnv(requestEnviron, FALSE);
            requestEnviron = NULL;
        }
        /*
         * Finish the request.
         */
        FCGI_Finish();
    }


void
flush()

    PROTOTYPE:
    CODE:
    FCGI_Flush();

void
set_exit_status(status)

    int status;

    PROTOTYPE: $
    CODE:
    FCGI_SetExitStatus(status);

int
start_filter_data()

    PROTOTYPE:
    CODE:
    RETVAL = FCGI_StartFilterData();

    OUTPUT:
    RETVAL
