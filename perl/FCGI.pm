# $Id: FCGI.pm,v 1.3 1999/07/31 21:54:47 skimo Exp $

package FCGI;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	
);

$VERSION = '0.47';

bootstrap FCGI;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

sub request() {
    Request();
}

sub accept(;$***$) {
    return Accept(@_) if @_ == 5;

    if (defined %FCGI::ENV) {
	%ENV = %FCGI::ENV;
    } else {
	%FCGI::ENV = %ENV;
    }
    my $rc = Accept($global_request, \*STDIN, \*STDOUT, \*STDERR, \%ENV);

    # not SFIO
    $SIG{__WARN__} = $SIG{__DIE__} = $warn_die_handler if (tied (*STDIN));

    return $rc;
}

sub finish(;$) {
    return Finish(@_) if @_ == 1;

    %ENV = %FCGI::ENV if (defined %FCGI::ENV);

    # not SFIO
    if (tied (*STDIN)) {
	for (qw(__WARN__ __DIE__)) {
	    delete $SIG{$_} if ($SIG{$_} == $warn_die_handler);
	}
    }

    Finish ($global_request);
}

sub flush(;$) {
    return Flush(@_) if @_ == 1;

    Flush($global_request);
}

# deprecated
sub set_exit_status {
}

sub start_filter_data(;$) {
    return StartFilterData(@_) if @_ == 1;

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
