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
    my $s = Media::Scan->new( [ '/Users/andy/QA/DLNATestContent/Additional Content' ], {
        loglevel => MS_LOG_WARN,
        ignore => [],
        async => 1,
        cachedir => '/tmp/libmediascan',
        thumbnails => [
            { width => 200 },
        ],
        on_result => sub {
            my $r = shift;
            #warn "Result: " . dump($r->as_hash) . "\n";
            for my $thumb ( @{ $r->thumbnails } ) {
                my $ext = $thumb->{codec} eq 'JPEG' ? 'jpg' : 'png';
                warn "Wrote " . $thumb->{width} . "x" . $thumb->{height} . " thumb${c}.${ext}\n";
                open my $fh, '>', 'thumb' . $c . ".${ext}";
                print $fh $thumb->{data};
                close $fh;
                $c++;
            }
        },
        on_error => sub {
          my $e = shift;
          warn "Error: " . dump($e->as_hash);
        },
        on_progress => sub {
          my $p = shift;
          warn "Progress: " . $p->done . " / " . $p->total . ' (' . $p->cur_item . ") ETA " . $p->eta . "\n";
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
