use strict;

use Test::More tests => 1;

use Data::Dump qw(dump);
use File::Spec::Functions;
use FindBin ();
use Media::Scan;

{
    my $s = Media::Scan->new( [ _f('video') ], {
        #loglevel => 9,
        ignore => [],
        on_result => sub {
          my $r = shift;
          warn dump($r->hash) . "\n";
        },
        on_error => sub {
          my $e = shift;
          warn dump($e);
        },
        on_progress => sub {
          my $p = shift;
          warn dump($p);
        },
    } );
}

sub _f {
    return catfile( $FindBin::Bin, '..', '..', '..', 'test', 'data', shift );
}
