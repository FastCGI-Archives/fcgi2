# $Id: FCGI.pm,v 1.10 2000/07/10 09:56:49 skimo Exp $

package FCGI;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	
);

$VERSION = '0.53';

bootstrap FCGI;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

*FAIL_ACCEPT_ON_INTR = sub() { 1 };

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

sub detach() {
    Detach($global_request);
}

sub attach() {
    Attach($global_request);
}

# deprecated
sub set_exit_status {
}

sub start_filter_data() {
    StartFilterData($global_request);
}

$global_request = Request();
$warn_die_handler = sub { print STDERR @_ unless $^S };

package FCGI::Stream;

sub PRINTF {
  shift->PRINT(sprintf(shift, @_));
}

1;

=pod

=head1 NAME

FCGI - Fast CGI module

=head1 SYNOPSIS

    use FCGI;

    my $count = 0;
    my $request = FCGI::Request();

    while($request->accept() >= 0) {
	print("Content-type: text/html\r\n\r\n", ++$count);
    }

=head1 DESCRIPTION

Functions:

=over 4

=item FCGI::Request

Creates a request handle. It has the following optional parameters:

=over 8

=item input perl file handle (default: \*STDIN)

=item output perl file handle (default: \*STDOUT)

=item error perl file handle (default: \*STDERR)

These filehandles will be setup to act as input/output/error
on succesful Accept.

=item environment hash reference (default: \%ENV)

The hash will be populated with the environment.

=item socket (default: 0)

Socket to communicate with the server.
Can be the result of the OpenSocket function.
For the moment, it's the file descriptor of the socket
that should be passed. This may change in the future.

=item flags (default: 0)

Possible values:

=over 12

=item FCGI::FAIL_ACCEPT_ON_INTR

If set, Accept will fail if interrupted.
It not set, it will just keep on waiting.

=back

=back

Example usage:
    my $req = FCGI::Request;

or:
    my %env;
    my $in = new IO::Handle;
    my $out = new IO::Handle;
    my $err = new IO::Handle;
    my $req = FCGI::Request($in, $out, $err, \%env);

=item FCGI::OpenSocket(path, backlog)

=item FCGI::CloseSocket(socket)

Close a socket opened with OpenSocket.

=item $req->Accept()

Accepts a connection on $req, attaching the filehandles and
populating the environment hash.
Returns 0 on success.
If a connection has been accepted before, the old
one will be finished first.

Note that unlike with the old interface, no die and warn
handlers are installed by default.

=item $req->Finish()

Finishes accepted connection.
Also detaches filehandles.

=item $req->Flush()

Flushes accepted connection.

=item $req->Detach()

Temporarily detaches filehandles on an accepted connection.

=item $req->Attach()

Re-attaches filehandles on an accepted connection.

=back

=head1 AUTHOR

Sven Verdoolaege <skimo@kotnet.org>

=cut

__END__
