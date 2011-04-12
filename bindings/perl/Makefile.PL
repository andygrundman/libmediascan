use strict;

use ExtUtils::MakeMaker qw(WriteMakefile);
use File::Spec::Functions;
use Getopt::Long;

my (@INC, @LIBPATH, @LIBS);
my ($FFMPEG, $LMS, $MYEXTLIB);

my ($help, $ffmpeg_inc, $ffmpeg_lib, $lms_inc, $lms_lib);
my $static;

my $DEFINES = '-O2';
$DEFINES .= ' -Wall' unless $^O =~ /sun|solaris|Win32/i;
$DEFINES .= ' -Wno-unused-value -Wno-format-security' unless $^O =~ /Win32|sun|solaris/i;

my $result = GetOptions(
    "help|h"                 => \$help,
    "with-ffmpeg-includes=s" => \$ffmpeg_inc,
    "with-ffmpeg-libs=s"     => \$ffmpeg_lib,
    "with-lms-includes=s"    => \$lms_inc,
    "with-lms-libs=s"        => \$lms_lib,
    "with-static"            => \$static,
);

if ($help || !$result) {
    print STDERR <<END;
Usage: perl Makefile.PL [options]

Configure Media::Scan module.

Options:
    --with-ffmpeg-includes    Path to directory containing libavformat/avformat.h
    --with-ffmpeg-libs        Path to directory containing libavformat/libavcodec/libavutil subdirectories
    --with-lms-includes       Path to directory containing libmediascan.h
    --with-lms-libs           Path to directory containing libmediascan
    --with-static             Statically link the module

END

    exit 0;
}

my @check = qw(/usr/include /usr/local/include /opt/local/include);

# Check in environment-supplied locations before standard ones
unshift @check, split /:/, $ENV{INCLUDE};

if ( $^O =~ /Win32/i ) {
    if ( !-d 'win32' ) {
        die "Please read README.win32 for instructions on building on Windows\n";
    }
    push @check, 'win32/include';
    $static = 1;
}

# Look for ffmpeg
for my $incdir ( $ffmpeg_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'libavformat', 'avformat.h') ) {
        $FFMPEG = 1;
        $ffmpeg_inc = $incdir;
        
        push @INC, '-I' . $ffmpeg_inc;
        if ($ffmpeg_lib) {
            push @LIBPATH, '-L' . $ffmpeg_lib;
        }
        
        if ($static) {
            if ($ffmpeg_lib) {
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavformat$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavcodec$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavutil$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavformat$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavcodec$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavutil$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-lavformat -lavcodec -lavutil';
        }
        
        print "Using ffmpeg from $ffmpeg_inc\n";
        last;
    }
}

if ( !$FFMPEG ) {
    die "FFmpeg not found, please install it and try again. Checked for libavformat/avformat.h in: " . join(", ", @check) . "\n";
}

# Look for libmediascan
for my $incdir ( $lms_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'libmediascan.h') ) {
        $LMS = 1;
        $lms_inc = $incdir;
        
        push @INC, '-I' . $lms_inc;
        if ($lms_lib) {
            push @LIBPATH, '-L' . $lms_lib;
        }
        
        if ($static) {
            if ($lms_lib) {
                $MYEXTLIB .= $lms_lib . '/' . 'libmediascan$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $lms_inc . '/../lib/libmediascan$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-lmediascan';
        }
        
        print "Using libmediascan from $lms_inc\n";
        last;
    }
}

if ( !$LMS ) {
    # XXX make a CPAN module version that includes lms as a subdir?
    die "libmediascan not found, please install it and try again. Checked for libmediascan.h in: " . join(", ", @check) . "\n";
}

if ( $^O =~ /Win32/i ) {
    $MYEXTLIB .= 'win32/lib/libgcc.a win32/lib/libmingwex.a win32/lib/libz.a win32/lib/libbz2.a ';
    *MY::postamble = sub {};
}

my $xs_files  = join(' ', glob 'xs/*.xs');

WriteMakefile(
    NAME              => 'Media::Scan',
    VERSION_FROM      => 'lib/Media/Scan.pm',
    PREREQ_PM         => {
        'Test::Warn' => 0,
    },
    ABSTRACT_FROM     => 'lib/Media/Scan.pm',
    AUTHOR            => 'Andy Grundman <andy@slimdevices.com>',
    INC               => join(' ', @INC),
    LIBS              => [ join(' ', @LIBPATH, @LIBS) ],
    DEFINE            => $DEFINES,
    MYEXTLIB          => $MYEXTLIB,
    depend            => { 'Scan.c' => "$xs_files" },
);
