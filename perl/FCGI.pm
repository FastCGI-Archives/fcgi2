# $Id: FCGI.pm,v 1.5 1999/08/10 11:06:37 skimo Exp $

package FCGI;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	
);

$VERSION = '0.48';

bootstrap FCGI;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

sub Request(;***$$$) {
    my @defaults = (\*STDIN, \*STDOUT, \*STDERR, \%ENV, 0, 0);
    splice @defaults,0,@_,@_;
    RequestX(@defaults);
}

sub accept() {
    if (defined %FCGI::ENV) {
	%ENV = %FCGI::ENV;
    } else {
	%FCGI::ENV = %ENV;
    }
    my $rc = Accept($global_request);
    for (keys %FCGI::ENV) {
	$ENV{$_} = $FCGI::ENV{$_} unless exists $ENV{$_};
    }

    # not SFIO
    $SIG{__WARN__} = $SIG{__DIE__} = $warn_die_handler if (tied (*STDIN));

    return $rc;
}

sub finish() {
    %ENV = %FCGI::ENV if (defined %FCGI::ENV);

    # not SFIO
    if (tied (*STDIN)) {
	for (qw(__WARN__ __DIE__)) {
	    delete $SIG{$_} if ($SIG{$_} == $warn_die_handler);
	}
    }

    Finish ($global_request);
}

sub flush() {
    Flush($global_request);
}

# deprecated
sub set_exit_status {
}

sub start_filter_data() {
    StartFilterData($global_request);
}

$global_request = Request();
$warn_die_handler = sub { print STDERR @_ };

package FCGI::Stream;

sub PRINTF {
  shift->PRINT(sprintf(shift, @_));
}

1;

=head1 NAME

FCGI - Fast CGI module

=head1 SYNOPSIS

    use FCGI;

    $count = 0;
    while(FCGI::accept() >= 0) {
	print("Content-type: text/html\r\n\r\n", ++$count);
    }

=head1 DESCRIPTION

Functions:

=over 4

=item FCGI::accept()

Accepts a connection. Returns 0 on success.
If a connection has been accepted before, the old
one will be finished first.

=item FCGI::finish()

Finishes accepted connection.

=item FCGI::flush()

Flushes accepted connection.

=item FCGI::set_exit_status(status)

Sets the exit status that finish returns to the server.

=item FCGI::start_filter_data()

Does anyone use this function ?

=back

=head1 AUTHOR

Sven Verdoolaege <skimo@kotnet.org>

=cut

__END__
