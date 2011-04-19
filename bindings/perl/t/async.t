use strict;

use Test::More tests => 1;

use AnyEvent;
use Data::Dump qw(dump);
use File::Spec::Functions;
use FindBin ();
use Media::Scan;

my $c = 1;
{
    my $cv = AnyEvent->condvar;

    #my $s = Media::Scan->new( [ _f('video') ], {
    my $s = Media::Scan->new( [ '/Users/andy/Music/Slim/DLNATestContent' ], {
        loglevel => 5,
        ignore => [],
        async => 1,
        thumbnails => [
            { width => 200 },
        ],
        on_result => sub {
          my $r = shift;
          #warn "Result: " . dump($r->as_hash) . "\n";
          open my $fh, '>', 'thumb' . $c . '.jpg';
          print $fh $r->thumbnails->[0];
          close $fh;
          warn "Wrote thumb${c}.jpg\n";
          $c++;
        },
        on_error => sub {
          my $e = shift;
          warn "Error: " . dump($e->as_hash);
        },
        on_progress => sub {
          my $p = shift;
          warn "Progress: " . dump($p->as_hash);
        },
        on_finish => sub {
            warn "Finished\n";
            $cv->send;
        },
    } );

    my $w = AnyEvent->io(
        fh   => $s->async_fd,
        poll => 'r',
        cb   => sub {
            $s->async_process;
        },
    );

    $cv->recv;
}

sub _f {
    return catfile( $FindBin::Bin, '..', '..', '..', 'test', 'data', shift );
}
