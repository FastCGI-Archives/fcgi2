# $Id: Makefile.PL,v 1.8 2000/12/31 21:46:59 skimo Exp $

use ExtUtils::MakeMaker;
use IO::File;
use Config;
use Cwd 'cwd';

@h1 = qw(fastcgi.h fcgiapp.h fcgiappmisc.h fcgimisc.h fcgios.h);
@h = (@h1, 'fcgi_config.h');
@o = qw(FCGI.o);
@dist1 = qw(LICENSE.TERMS acconfig.h);
@dist2 = qw(fcgiapp.c os_unix.c os_win32.c);
@dist3 = (@h1, qw(fcgi_config.h.in fcgi_config_x86.h));

$pure = 
    (prompt("Do you want to use the pure perl implementation", "no") =~ /^y/) 
    ? "1" : "0";
open(CFG,">FCGI.cfg");
print CFG "\$pure = $pure;1;\n";
close CFG;

$devkit = cwd() . "/..";

if (-d "$devkit/libfcgi/" 
    && -d "$devkit/include" && !grep {!-f "$devkit/include/$_"} (@h)) 
{
    unless (-f "$devkit/libfcgi/libfcgi.a") {
	warn "Please compile the library before attempting " .
	      "to compile the perl module.\n";
	exit -1;
    }
    # devkit
    $prefix = $devkit;
    push @libs, "-L$devkit/libfcgi -lfcgi";
} 
else {
    # CPAN  
    for $dir ("/usr", "/usr/local") {
        if (-d "$dir/lib/" && -f "$dir/lib/libfcgi.a" 
        	&& -d "$dir/include" && !grep {!-f "$dir/include/$_"} (@h)) 
        {
            print "Found fcgi library and include files in $dir\n";
            print "Will be using that instead of included files\n";
            print "Edit Makefile.PL if you don't like it\n";
            
            $prefix = $dir;
            push @libs, "-L$dir/lib -lfcgi"; 
            last;
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
unless ($pure) {
    push @extras,
	'LIBS'	=> [ @libs ],
	'OBJECT'	=> "@o",
	'INC'	=> $inc;
}
        
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
$mm = MM->new({
    'NAME'		=> 'FCGI',
    'VERSION_FROM'	=> 'FCGI.PL',
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
    'clean'		=> { FILES => 'config.cache fcgi_config.h FCGI.pm' . 
				      ' FCGI.xs FCGI.c FCGI.cfg' },
    'PL_FILES'		=> $plfiles,
    PM			=> {'FCGI.pm' => '$(INST_ARCHLIBDIR)/FCGI.pm'},
    @extras,
});
# don't install oldinterface pod
delete $mm->{MAN3PODS}{oldinterface.pod};
$mm->flush;

exit if -f 'fcgi_config.h' or $prefix;

# CPAN and no installed lib found
if ($sys eq "win32") {
    # configure will almost certainly not run on a normal NT install,
    # use the pregenerated configuration file

    use File::Copy qw(copy);
    print "Using prebuilt fcgi_config.h file for Windows\n";
    unlink("fcgi_config.h");
    die $! unless copy("fcgi_config_x86.h","fcgi_config.h");

    # Win build system also can't deal with existence of FCGI.xs or absence of
    # FCGI.c
    unlink("FCGI.xs");
    open(F, ">FCGI.c"); close(F);
    $now = time; $before = $now - 600;
    utime $before, $before, "FCGI.c";
    utime $now, $now, "FCGI.PL";

} else {
    print "Running ./configure for you\n";
    print "Please read configure.readme for information on how to run it yourself\n";

    $ENV{'CC'} = $Config{'cc'};
    system("./configure");
}
