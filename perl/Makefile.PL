# $Id: Makefile.PL,v 1.33 2002/12/15 19:40:19 skimo Exp $

use 5.006;
use ExtUtils::MakeMaker;
use IO::File;
use Config;
use Cwd 'cwd';
use Getopt::Long;
use File::Copy qw(copy);

@h1 = qw(fastcgi.h fcgiapp.h fcgimisc.h fcgios.h);
@h = (@h1, 'fcgi_config.h');
@o = qw(FCGI.o);
@dist1 = qw(LICENSE);
@dist2 = qw(fcgiapp.c os_unix.c os_win32.c);
@dist3 = (@h1, qw(fcgi_config_x86.h));

GetOptions ("use-installed:s" => \$useinstalled);

$libfound = 0;
@libs = ();

my $cwd = cwd();
my $devkit = "$cwd/..";

if (defined $useinstalled) {
    require ExtUtils::Liblist;
    my $libspec = $useinstalled ? "-L$useinstalled/lib " : "";
    $libspec .= "-lfcgi";
    my @l = MM->ext($libspec);
    if ($l[0] || $l[1] || $l[2]) {
        $prefix = "$useinstalled/include" if $useinstalled;
        $libfound = 1;
        push @libs, $libspec;
    }
}
if (!$libfound && -d "$devkit/libfcgi" && -d "$devkit/include") {
    # devkit
    if (grep { ! -f "$devkit/include/$_" } @dist3 
        or grep { ! -f "$devkit/libfcgi/$_" } @dist2)
    {
        warn "This appears to be a FastCGI devkit distribution, " .
            "but one or more FastCGI library files are missing. \n" .
            "Please check the integrity of the distribution.\n";
        exit -1;
    }

    my $extrarules = join "\n",
        map { $b = $_; $b =~ s/\.c$//; my $s="$devkit/libfcgi/$b.c";
            "$b\$(OBJ_EXT): $s\n\t".
            '$(CCCMD) $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) '."$s\n"; }
            @dist2;
    eval 'package MY; sub postamble { $extrarules; }';
    $prefix = $devkit;
}


$sys = $^O eq 'MSWin32' ? 'win32' : 'unix';
push @o, "fcgiapp.o", "os_$sys.o" unless $libfound;
$inc = '-I.' unless $libfound;
$inc .= " -I$prefix/include" if $prefix;

push(@extras, CAPI => 'TRUE')
     if ($] >= 5.005 and $^O eq 'MSWin32'
        and $Config{archname} =~ /-object\b/i);

push(@extras,
    ABSTRACT => 'Fast CGI module',
    AUTHOR   => 'Sven Verdoolaege (skimo@kotnet.org)'
) if ($ExtUtils::MakeMaker::VERSION >= 5.4301);

push @extras, META_MERGE => {
    'meta-spec' => { version => 2 },
    dynamic_config => 0,
    resources => {
        repository => {
            # this is the real repository
            # r/w: catagits@git.shadowcat.co.uk:fcgi2.git
            # r/o: git://git.shadowcat.co.uk/catagits/fcgi2.git
            # web: http://git.shadowcat.co.uk/gitweb/gitweb.cgi?p=catagits/fcgi2.git
            # this is a mirror, but can receive pull requests
            url => 'https://github.com/perl-catalyst/FCGI.git',
            web => 'https://github.com/perl-catalyst/FCGI',
            type => 'git',
        },
        bugtracker => {
            mailto => 'bug-FCGI@rt.cpan.org',
            web => 'https://rt.cpan.org/Public/Dist/Display.html?Name=FCGI',
        },
    },
} if $ExtUtils::MakeMaker::VERSION >= 6.46;

push(@extras,
    MIN_PERL_VERSION => '5.006',
) if $ExtUtils::MakeMaker::VERSION >= 6.48;

# not strictly necessary as everything is in core...
#push(@extras,
#    CONFIGURE_REQUIRES => {
#        ...
#    },
#) if $ExtUtils::MakeMaker::VERSION >= 6.51_03;

if ("$sys" eq "win32") {
    push @libs, ":nosearch -lws2_32";
    push @extras, 'DEFINE' => '-DDLLAPI=__declspec(dllexport)';
}

push @extras,
    'LIBS'    => [ "@libs" ],
    'OBJECT'    => "@o",
    'INC'    => $inc;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

# Work around bug in previous versions of MakeMaker
WriteMakefile(
    'NAME'        => 'FCGI',
    'VERSION_FROM'    => 'FCGI.pm',
    'dist' => {
        'COMPRESS' => 'gzip -9f',
        'SUFFIX' => 'gz',
        'PREOP' => '$(CP) '.join(' ',
            map {"../$_"} @dist1,
            (map {"libfcgi/$_"} @dist2),
            map {"include/$_"} @dist3).' $(DISTVNAME);'.
                '$(CP) MANIFEST MANIFEST.old;'.
                '$(ECHO) '. join('\\\n',@dist1,@dist2,@dist3) .
                '>> $(DISTVNAME)/MANIFEST',
                  'POSTOP' => 
                '$(MV) MANIFEST.old MANIFEST',
    },
    'clean'        => { FILES => 'config.cache fcgi_config.h fcgi_config.h.in' .
                      ' FCGI.c aclocal.m4 autom4te.cache config.log config.status' .
                      ' FCGI.cfg' },
    'realclean'    => { FILES => 'configure MANIFEST.SKIP.bak MANIFEST.bak Makefile.old' },
    PM            => {'FCGI.pm' => '$(INST_ARCHLIBDIR)/FCGI.pm'},
    PREREQ_PM     => {'XSLoader' => '0'},
    TEST_REQUIRES => {
        'Config' => 0,
        'FCGI::Client' => 0.09,
        'File::Temp' => 0,
        'IO::Socket' => 0,
        'Test::More' => 0,
    },
    @extras,
);

exit if -f 'fcgi_config.h' or $libfound;

# CPAN and no installed lib found
if ($sys eq "win32") {
    # configure will almost certainly not run on a normal NT install,
    # use the pregenerated configuration file

    print "Using prebuilt fcgi_config.h file for Windows\n";
    unlink("fcgi_config.h");
    my $confdir = $prefix ? "$prefix/include/" : '';
    die $! unless copy("${confdir}fcgi_config_x86.h","fcgi_config.h");
} else {
    print "Running ./configure for you\n";
    print "Please read configure.readme for information on how to run it yourself\n";

    $ENV{'CC'} = $Config{'cc'};
    if ( $^O eq 'android' && !$ENV{'TMPDIR'} ) {
        # See http://stackoverflow.com/a/15417261
        require File::Spec;
        $ENV{'TMPDIR'} = File::Spec->tmpdir();
    }
    system("$Config{sh} configure");
}

