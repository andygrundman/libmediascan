use strict;

use Test::More tests => 1;

use Data::Dump qw(dump);
use File::Spec::Functions;
use FindBin ();
use Media::Scan;

{
    my $s = Media::Scan->new( [ _f('video') ], {
    #my $s = Media::Scan->new( [ '/Users/andy/Music/Slim/DLNATestContent' ], {
        #loglevel => 5,
        ignore => [],
        on_result => sub {
          my $r = shift;
          warn "Result: " . dump($r->hash) . "\n";
        },
        on_error => sub {
          my $e = shift;
          warn "Error: " . dump($e->hash);
        },
        on_progress => sub {
          my $p = shift;
          warn "Progress: " . dump($p->hash);
        },
    } );
}

sub _f {
    return catfile( $FindBin::Bin, '..', '..', '..', 'test', 'data', shift );
}
