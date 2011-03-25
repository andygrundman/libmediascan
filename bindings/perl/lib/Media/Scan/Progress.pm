package Media::Scan::Progress;

use strict;

# Implementation is in xs/Progress.xs

sub hash {
    my $self = shift;
    
    return {
        phase      => $self->phase,
        cur_item   => $self->cur_item,
        dir_total  => $self->dir_total,
        dir_done   => $self->dir_done,
        file_total => $self->file_total,
        file_done  => $self->file_done,
        eta        => $self->eta,
        rate       => $self->rate,
    };
}

1;