use Config;

open OUT, ">threaded.fpl";
print OUT "#!$Config{perlpath}\n";
print OUT while <DATA>;
close OUT;
chmod 0755, "threaded.fpl";
__END__

use FCGI;
use Thread;

use constant THREAD_COUNT => 5;

sub doit {
    my $k = shift;
    my %env;
    my $in = do { local *FH };
    my $out = do { local *FH };
    my $err = do { local *FH };

    my $request = FCGI::request();

    while ($request->accept(\$in, \$out, \$err, \%env) >= 0) {
	print $out
           "Content-type: text/html\r\n",
           "\r\n",
           "<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>",
           "<h1>FastCGI Hello! (multi-threaded C, fcgiapp library)</h1>",
           "Request counts for ", THREAD_COUNT ," threads ",
	   "running on host <i>$env{SERVER_NAME}</i><P><CODE>";

	++$count[$k];

	{
	    lock(@count);
	    for(my $i = 0; $i < THREAD_COUNT; ++$i) {
		print $out $count[$i];
		print $out " ";
	    }
	}
	$request->flush();
	sleep(1);
    }
}

for ($t = 1; $t < THREAD_COUNT; ++$t) {
    new Thread \&doit, $t;
}
doit(0);
