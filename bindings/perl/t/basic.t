use strict;

use Test::More tests => 1;

use Data::Dump qw(dump);
use File::Spec::Functions;
use FindBin ();
use Media::Scan;

{
    my $s = Media::Scan->new( [ '/Users/andy/dev' ], {
        #loglevel => 9,
        ignore => [ qw(wav png) ],
        on_file => sub {
          my $result = shift;
          warn dump($result);
        },
        on_error => sub {
          my $error = shift;
          warn dump($error);
        },
        progress => sub {
          my $p = shift;
          warn dump($p);
        },
    } );
    
    warn dump($s);
}

sub _f {
    return catfile( $FindBin::Bin, 'data', shift );
}
