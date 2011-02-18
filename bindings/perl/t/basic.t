use strict;

use Test::More tests => 1;

use Data::Dump qw(dump);
use File::Spec::Functions;
use FindBin ();
use Media::Scan;

{
    my $s = Media::Scan->new( '/Users/andy/Music/Slim/DLNATestContent', {
        loglevel => 9,
        ignore => [ qw(wav png) ],
        on_file => sub {
          my $result = shift;
        },
        on_error => sub {
          my $error = shift;
        },
        progress => sub {
          my $p = shift;
        },
    } );
    
    warn dump($s);
}

sub _f {
    return catfile( $FindBin::Bin, 'data', shift );
}
