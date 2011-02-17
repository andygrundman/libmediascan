use strict;

use ExtUtils::MakeMaker qw(WriteMakefile);

my (@INC, @LIBPATH, @LIBS);
my $MYEXTLIB;

if ( $^O !~ /Win32/ ) {
    push @LIBS, '-lmediascan';
}

my $DEFINES = '-O2';
$DEFINES .= ' -Wall' unless $^O =~ /sun|solaris/i;
$DEFINES .= ' -Wno-unused-value -Wno-format-security' unless $^O =~ /Win32|sun|solaris/i;

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
