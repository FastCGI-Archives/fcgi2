# $Id: Makefile.PL,v 1.25 2001/09/21 16:14:11 skimo Exp $

use ExtUtils::MakeMaker;
use IO::File;
use Config;
use Cwd 'cwd';
use Getopt::Long;

@h1 = qw(fastcgi.h fcgiapp.h fcgimisc.h fcgios.h);
@h = (@h1, 'fcgi_config.h');
@o = qw(FCGI.o);
@dist1 = qw(LICENSE.TERMS configure configure.in install.sh missing
            config.sub config.guess ltmain.sh);
@dist2 = qw(fcgiapp.c os_unix.c os_win32.c);
@dist3 = (@h1, qw(fcgi_config.h.in fcgi_config_x86.h));

GetOptions ("pure-perl!" => \$pure);
$pure = "0" unless defined $pure;
open(CFG,">FCGI.cfg");
print CFG "\$pure = $pure;1;\n";
close CFG;

if (! $pure) {
	my $cwd = cwd();
	my $devkit = "$cwd/..";

	if (-d "$devkit/libfcgi" && -d "$devkit/include") {
		# devkit
		if (grep { ! -f "$devkit/include/$_" } @dist3 
			or grep { ! -f "$devkit/libfcgi/$_" } @dist2)
		{
			warn "This appears to be a FastCGI devkit distribution, " .
				 "but one or more FastCGI library files are missing. \n" .
				 "Please check the integrity of the distribution.\n";
			exit -1;
		}

		# Copy the C lib files down to ensure a compatible build.
		print "Copying C library files from the devkit distribution\n";
	    use File::Copy qw(copy);

		# TODO: stop or save if a local file has been modified
		foreach (@dist1, @dist2, @dist3) { unlink };
		foreach (@dist1) { copy("$devkit/$_", ".") || die $! };
		foreach (@dist2) { copy("$devkit/libfcgi/$_", ".") || die $! };
		foreach (@dist3) { copy("$devkit/include/$_", ".") || die $! };
	}
	else {
	    # CPAN  
	    for $dir ("/usr", "/usr/local") {
	        if (-d "$dir/lib/" && -f "$dir/lib/libfcgi.a" 
	        	&& -d "$dir/include" && !grep {!-f "$dir/include/$_"} (@h)) 
	        {
	            print "Found fcgi library and include files in $dir\n";
	            print "Will be using those instead of the included files\n";
	            print "Edit Makefile.PL if you don't like it\n";
	            
	            $prefix = $dir;
	            push @libs, "-L$dir/lib -lfcgi"; 
	            last;
	        }
	    }
	}
}

$sys = $^O eq 'MSWin32' ? 'win32' : 'unix';
push @o, "fcgiapp.o", "os_$sys.o" unless $prefix;
$inc = $prefix ? "-I$prefix/include" : '-I.';

push(@extras, CAPI => 'TRUE')
     if ($] >= 5.005 and $^O eq 'MSWin32'
        and $Config{archname} =~ /-object\b/i);

push(@extras,
    ABSTRACT => 'Fast CGI module',
    AUTHOR   => 'Sven Verdoolaege (skimo@kotnet.org)')
        if ($ExtUtils::MakeMaker::VERSION >= 5.4301); 

$plfiles = { 'echo.PL' => 'echo.fpl', 
	     'remote.PL' => 'remote.fpl',
	     'threaded.PL' => 'threaded.fpl',
	     'FCGI.PL' => 'FCGI.pm',
	   };
$plfiles->{'FCGI.XL'} = 'FCGI.xs' unless $pure;
if ($pure) {
    push @extras,
	LINKTYPE => ' ';
} else {

	if ("$sys" eq "win32") {
		@libs = (scalar @libs)
			? map { "$_ :nosearch -lws2_32" } @libs 
			: (':nosearch -lws2_32');
		push @extras, 'DEFINE' => '-DDLLAPI=__declspec(dllexport)';
	}

    push @extras,
	'LIBS'	=> [ @libs ],
	'OBJECT'	=> "@o",
	'INC'	=> $inc;
}
        
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

# Work around bug in previous versions of MakeMaker
WriteMakefile(NAME => 'FCGI') 
    if $ExtUtils::MakeMaker::VERSION <= 5.4302;

$mm = MM->new({
    'NAME'		=> 'FCGI',
    'VERSION_FROM'	=> 'version.pm',
    'dist'		=> { 'COMPRESS' => 'gzip -9f', 
		             'SUFFIX' => 'gz',
			     'PREOP' => '$(CP) '.join(' ',
				map {"../$_"} @dist1,
				(map {"libfcgi/$_"} @dist2),
				map {"include/$_"} @dist3).' $(DISTVNAME);'.
				'$(CP) MANIFEST MANIFEST.old;'.
				'echo -e '. join('\\\n',@dist1,@dist2,@dist3) .
				'>> $(DISTVNAME)/MANIFEST',
			      'POSTOP' => 
				'$(MV) MANIFEST.old MANIFEST',
			    },
    'clean'		=> { FILES => 'config.cache fcgi_config.h' . 
				      ' FCGI.xs FCGI.c FCGI.cfg ' .
				      (join ' ', values %$plfiles)},
    'PL_FILES'		=> $plfiles,
    PM			=> {'FCGI.pm' => '$(INST_ARCHLIBDIR)/FCGI.pm'},
    @extras,
});
# don't install oldinterface pod
delete $mm->{MAN3PODS}{oldinterface.pod};
$mm->flush;

exit if -f 'fcgi_config.h' or $prefix or $pure;

# CPAN and no installed lib found
if ($sys eq "win32") {
    # configure will almost certainly not run on a normal NT install,
    # use the pregenerated configuration file

    use File::Copy qw(copy);
    print "Using prebuilt fcgi_config.h file for Windows\n";
    unlink("fcgi_config.h");
    die $! unless copy("fcgi_config_x86.h","fcgi_config.h");

    # Win can't deal with existence of FCGI.xs or absence of FCGI.c
    unlink("FCGI.xs");
    open(F, ">FCGI.c"); close(F);
    $now = time; $before = $now - 600;
    utime $before, $before, "FCGI.c";
    utime $now, $now, "FCGI.PL";
} else {
    print "Running ./configure for you\n";
    print "Please read configure.readme for information on how to run it yourself\n";

    $ENV{'CC'} = $Config{'cc'};
    system("$Config{sh} configure");
}
