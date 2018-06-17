use strict;

use Config;
use ExtUtils::MakeMaker qw(WriteMakefile);
use File::Spec::Functions;
use Getopt::Long;

my (@INC, @LIBPATH, @LIBS);
my ($FFMPEG, $LMS, $EXIF, $JPEG, $PNG, $GIF, $BDB, $MYEXTLIB);

my ($help, $ffmpeg_inc, $ffmpeg_lib, $lms_inc, $lms_lib, $exif_inc, $exif_lib);
my ($jpeg_inc, $jpeg_lib, $png_inc, $png_lib, $gif_inc, $gif_lib);
my ($bdb_inc, $bdb_lib);
my $static;

my $DEFINES = '-O2';
$DEFINES .= ' -Wall' unless $^O =~ /sun|solaris|Win32/i;
$DEFINES .= ' -Wno-unused-value -Wno-format-security' unless $^O =~ /sun|solaris|Win32/i;
$DEFINES .= ' /D "PTW32_STATIC_LIB" /D "HAVE_CONFIG_H" /D "WIN32"' if $^O =~ /Win32/i;

my $result = GetOptions(
    "help|h"                 => \$help,
    "with-ffmpeg-includes=s" => \$ffmpeg_inc,
    "with-ffmpeg-libs=s"     => \$ffmpeg_lib,
    "with-lms-includes=s"    => \$lms_inc,
    "with-lms-libs=s"        => \$lms_lib,
    "with-exif-includes=s"   => \$exif_inc,
    "with-exif-libs=s"       => \$exif_lib,
    "with-jpeg-includes=s"   => \$jpeg_inc,
    "with-jpeg-libs=s"       => \$jpeg_lib,
    "with-png-includes=s"    => \$png_inc,
    "with-png-libs=s"        => \$png_lib,
    "with-gif-includes=s"    => \$gif_inc,
    "with-gif-libs=s"        => \$gif_lib,
    "with-bdb-includes=s"    => \$bdb_inc,
    "with-bdb-libs=s"        => \$bdb_lib,
    "with-static"            => \$static,
);

if ($help || !$result) {
    print STDERR <<END;
Usage: perl Makefile.PL [options]

Configure Media::Scan module.

Options:
    --with-ffmpeg-includes    Path to directory containing libavformat/avformat.h
    --with-ffmpeg-libs        Path to directory containing libavformat/libavcodec/libavutil/libswscale
    --with-lms-includes       Path to directory containing libmediascan.h
    --with-lms-libs           Path to directory containing libmediascan
    --with-exif-includes      Path to directory containing libexif/exif-data.h
    --with-exif-libs          Path to directory containing libexif
    --with-jpeg-includes      Path to directory containing jpeglib.h
    --with-jpeg-libs          Path to directory containing libjpeg
    --with-png-includes       Path to directory containing png.h
    --with-png-libs           Path to directory containing libpng
    --with-gif-includes       Path to directory containing gif_lib.h
    --with-gif-libs           Path to directory containing libgif
    --with-bdb-includes       Path to directory containing BerkeleyDB's db.dh
    --with-bdb-libs           Path to directory containing libdb
    --with-static             Statically link the module with all libraries

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

# Look for libmediascan
for my $incdir ( $lms_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'libmediascan.h') ) {
        $LMS = 1;
        $lms_inc = $incdir;

        unshift @INC, '-I' . $lms_inc;
        if ($lms_lib) {
            unshift @LIBPATH, '-L' . $lms_lib;
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
            unshift @LIBS, '-lmediascan';
        }

        print "Using libmediascan from $lms_inc\n";
        last;
    }
}

if ( !$LMS ) {
    # XXX make a CPAN module version that includes lms as a subdir?
    die "libmediascan not found, please install it and try again. Checked for libmediascan.h in: " . join(", ", $lms_inc, @check) . "\n";
}

if ( $^O =~ /Win32/i ) {
    # Win32 has everything in libmediascan.lib and doesn't need other libraries
    goto MAKEFILE;
}

# Look for ffmpeg
for my $incdir ( $ffmpeg_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'libavformat', 'avformat.h') ) {
        $FFMPEG = 1;
        $ffmpeg_inc = $incdir;

        unshift @INC, '-I' . $ffmpeg_inc;
        if ($ffmpeg_lib) {
            unshift @LIBPATH, '-L' . $ffmpeg_lib;
        }

        if ($static) {
            if ($ffmpeg_lib) {
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavformat$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavcodec$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libswscale$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_lib . '/' . 'libavutil$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavformat$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavcodec$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libswscale$(LIB_EXT) ';
                $MYEXTLIB .= $ffmpeg_inc . '/../lib/libavutil$(LIB_EXT) ';
            }

            push @LIBS, '-lm -lz';
        }
        else {
            push @LIBS, '-lavformat -lavcodec -lm -lz -lswscale -lavutil';
        }

        print "Using ffmpeg from $ffmpeg_inc\n";
        last;
    }
}

if ( !$FFMPEG ) {
    die "FFmpeg not found, please install it and try again. Checked for libavformat/avformat.h in: " . join(", ", $ffmpeg_inc, @check) . "\n";
}

# Look for libexif
for my $incdir ( $exif_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'libexif', 'exif-data.h') ) {
        $EXIF = 1;
        $exif_inc = $incdir;

        unshift @INC, '-I' . $exif_inc;
        if ($exif_lib) {
            unshift @LIBPATH, '-L' . $exif_lib;
        }

        if ($static) {
            if ($exif_lib) {
                $MYEXTLIB .= $exif_lib . '/' . 'libexif$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $exif_inc . '/../lib/libexif$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-lexif';
        }

        push @LIBS, '-lintl'; # XXX only if logging is used for exif

        print "Using libexif from $exif_inc\n";
        last;
    }
}

if ( !$EXIF ) {
    die "libexif not found, please install it and try again. Checked for libexif/exif-data.h in: " . join(", ", $exif_inc, @check) . "\n";
}

# Look for libjpeg
for my $incdir ( $jpeg_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'jpeglib.h') ) {
        $JPEG = 1;
        $jpeg_inc = $incdir;

        unshift @INC, '-I' . $jpeg_inc;
        if ($jpeg_lib) {
            unshift @LIBPATH, '-L' . $jpeg_lib;
        }

        if ($static) {
            if ($jpeg_lib) {
                $MYEXTLIB .= $jpeg_lib . '/' . 'libjpeg$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $jpeg_inc . '/../lib/libjpeg$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-ljpeg';
        }

        print "Using libjpeg from $jpeg_inc\n";
        last;
    }
}

if ( !$JPEG ) {
    die "libjpeg not found, please install it and try again. Checked for jpeglib.h in: " . join(", ", $jpeg_inc, @check) . "\n";
}

# Look for libpng
for my $incdir ( $png_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'png.h') ) {
        $PNG = 1;
        $png_inc = $incdir;

        unshift @INC, '-I' . $png_inc;
        if ($png_lib) {
            unshift @LIBPATH, '-L' . $png_lib;
        }

        if ($static) {
            if ($png_lib) {
                $MYEXTLIB .= $png_lib . '/' . 'libpng$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $png_inc . '/../lib/libpng$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-lpng';
        }

        print "Using libpng from $png_inc\n";
        last;
    }
}

if ( !$PNG ) {
    die "libpng not found, please install it and try again. Checked for png.h in: " . join(", ", $png_inc, @check) . "\n";
}

# Look for giflib
for my $incdir ( $gif_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'gif_lib.h') ) {
        $GIF = 1;
        $gif_inc = $incdir;

        unshift @INC, '-I' . $gif_inc;
        if ($gif_lib) {
            unshift @LIBPATH, '-L' . $gif_lib;
        }

        if ($static) {
            if ($gif_lib) {
                $MYEXTLIB .= $gif_lib . '/' . "libgif\$(LIB_EXT) ";
            }
            else {
                $MYEXTLIB .= $gif_inc . "/../lib/libgif\$(LIB_EXT) ";
            }
        }
        else {
            push @LIBS, "-lgif";
        }

        print "Using libgif from $gif_inc\n";
        last;
    }
}

if ( !$GIF ) {
    die "libgif not found, please install it and try again. Checked for gif_lib.h in: " . join(", ", $gif_inc, @check) . "\n";
}

# Look for bdb
for my $incdir ( $bdb_inc, @check ) {
    if ( $incdir && -e catfile($incdir, 'db.h') ) {
        $BDB = 1;
        $bdb_inc = $incdir;

        unshift @INC, '-I' . $bdb_inc;
        if ($bdb_lib) {
            unshift @LIBPATH, '-L' . $bdb_lib;
        }

        if ($static) {
            if ($bdb_lib) {
                $MYEXTLIB .= $bdb_lib . '/' . 'libdb$(LIB_EXT) ';
            }
            else {
                $MYEXTLIB .= $bdb_inc . '/../lib/libdb$(LIB_EXT) ';
            }
        }
        else {
            push @LIBS, '-ldb';
        }

        print "Using BerkeleyDB libdb from $bdb_inc\n";
        last;
    }
}

if ( !$BDB ) {
    die "BerkeleyDB not found, please install it and try again. Checked for db.h in: " . join(", ", $bdb_inc, @check) . "\n";
}

MAKEFILE:

if ( $^O =~ /Win32/i ) {
    *MY::postamble = sub {};
    $MYEXTLIB .= " ";
}

my $xs_files  = join(' ', glob 'xs/*.xs');

my %opts = (
    NAME              => 'Media::Scan',
    VERSION_FROM      => 'lib/Media/Scan.pm',
    PREREQ_PM         => {
        'Test::Warn' => 0,
    },
    ABSTRACT          => 'Library for scanning audio/video/image file metadata',
    AUTHOR            => 'Andy Grundman <andy@hybridized.org>',
    INC               => join(' ', @INC),
    LIBS              => [ join(' ', @LIBPATH, @LIBS) ],
    DEFINE            => $DEFINES,
    MYEXTLIB          => $MYEXTLIB,
    depend            => { 'Scan.c' => "$xs_files" },
    META_MERGE => {
        'meta-spec' => { version => 2 },
        resources => {
            repository => {
                type => 'git',
                url  => 'https://github.com/andygrundman/libmediascan.git',
                web  => 'https://github.com/andygrundman/libmediascan',
            },
        },
    },
    MIN_PERL_VERSION => 5.008,
    LICENSE => 'gpl_2',
);

if ( $^O =~ /Win32/i ) {
    $opts{LDDLFLAGS} = '-dll -nologo -nodefaultlib -opt:ref,icf  -libpath:"C:\Perl\lib\CORE" -machine:x86 -manifest -ManifestFile:$@.manifest -shared -Wl,-Bsymbolic';
}
elsif ($^O =~ /sun|solaris/i ) {
    $opts{LD} = 'gld';
    $opts{LDDLFLAGS} = '-shared -Bsymbolic';
}
else {
    $opts{LDDLFLAGS} = '-shared -Wl,-Bsymbolic';
}

WriteMakefile(%opts);
