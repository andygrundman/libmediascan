use strict;

use ExtUtils::MakeMaker qw(WriteMakefile);

my (@INC, @LIBPATH, @LIBS);
my $MYEXTLIB;

my $DEFINES = '-O2';
$DEFINES .= ' -Wall' unless $^O =~ /sun|solaris/i;
$DEFINES .= ' -Wno-unused-value -Wno-format-security' unless $^O =~ /Win32|sun|solaris/i;

if ( -e '../../include/libmediascan.h' ) {
    # For development, statically link to parent libmediascan
    if ( !-d '../../src/.libs' ) {
        die "Please build libmediascan first\n";
    }
    
    push @INC, '-I../../include';
    push @LIBPATH, '-L../../src/.libs';
    $MYEXTLIB .= '../../src/.libs/libmediascan.a ';
    push @LIBS, '-lavformat -lavcore -lavcodec -lavutil -lz -lbz2 ';
}
else {
    push @LIBS, '-lmediascan';
}

if ( $^O =~ /Win32/ ) {
    *MY::postamble = sub {};
    $MYEXTLIB .= '../../libmediascan.lib '; # XXX
}

WriteMakefile(
    NAME              => 'Media::Scan',
    VERSION_FROM      => 'lib/Media/Scan.pm',
    PREREQ_PM         => {
        'Test::Warn' => 0,
        'XS::Object::Magic' => 0,
    },
    ABSTRACT_FROM     => 'lib/Media/Scan.pm',
    AUTHOR            => 'Andy Grundman <andy@slimdevices.com>',
    INC               => join(' ', @INC),
    LIBS              => [ join(' ', @LIBPATH, @LIBS) ],
    DEFINE            => $DEFINES,
    MYEXTLIB          => $MYEXTLIB,
);
