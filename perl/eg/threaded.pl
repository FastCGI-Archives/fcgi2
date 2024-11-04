#!/usr/bin/perl
use strict;
use warnings;
use threads;
use threads::shared;

use FCGI       qw[];
use IO::Handle qw[];

use constant THREAD_COUNT => 5;

my @count : shared = (0, (0) x THREAD_COUNT);

sub worker {
    my $k = shift;
    my %env;
    my $in  = IO::Handle->new;
    my $out = IO::Handle->new;
    my $err = IO::Handle->new;

    my $request = FCGI::Request($in, $out, $err, \%env);

    while ($request->Accept >= 0) {
        print $out
               "Content-type: text/html\r\n",
               "\r\n",
               "<title>FastCGI Hello! (multi-threaded perl, fcgiapp library)</title>",
               "<h1>FastCGI Hello! (multi-threaded perl, fcgiapp library)</h1>",
               "Request counts for ", THREAD_COUNT ," threads ",
               "running on host <i>$env{SERVER_NAME}</i>";

        {
            lock(@count);

            ++$count[$k];

            for(my $i = 1; $i <= THREAD_COUNT; $i++) {
                print $out $count[$i];
                print $out " ";
            }
        }
        $request->Flush;
        sleep(1);
    }
}

$_->join for map { threads->create(\&worker, $_) } 1..THREAD_COUNT;

