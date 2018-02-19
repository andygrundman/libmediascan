#!/usr/bin/perl

use Config;

if ( $Config{myarchname} =~ /i386/ ) {
    # Read OS version
    my $ver = `sw_vers -productVersion`;
    my ($osx_ver) = $ver =~ /(10\.(?:[5679]|1[0]))/;
    if ($osx_ver eq '10.5' ) {
        $arch = "-arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4";
    }
    elsif ( $osx_ver eq '10.6' ) {
        $arch = '-arch x86_64 -arch i386';
        if ( -d '/Developer/SDKs/MacOSX10.5.sdk' ) {
            $arch .= " -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5";
        }
        else {
            # 10.5 SDK not installed, use 10.6
            $arch .= " -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.6";
        }
    }
    elsif ( $osx_ver eq '10.7' ) {
        $arch = "-arch x86_64 -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.6";
    }
    elsif ( $osx_ver eq '10.9' ) {
        $arch = "-arch x86_64 -mmacosx-version-min=10.9";
    }
    elsif ( $osx_ver eq '10.10' ) {
        $arch = "-arch x86_64 -mmacosx-version-min=10.10";
    }
    else {
        die "Unsupported OSX version $osx_ver\n";
    }

    print "Adding $arch\n";

    my $ccflags   = $Config{ccflags};
    my $ldflags   = $Config{ldflags};
    my $lddlflags = $Config{lddlflags};

    # Remove extra -arch flags from these
    $ccflags  =~ s/-arch\s+\w+//g;
    $ldflags  =~ s/-arch\s+\w+//g;
    $lddlflags =~ s/-arch\s+\w+//g;

    # LMS requires some frameworks
    $ldflags .= " -framework CoreFoundation -framework CoreServices -framework Carbon";
    $lddlflags .= " -framework CoreFoundation -framework CoreServices -framework Carbon";

    $self->{CCFLAGS} = "$arch -I/usr/include $ccflags";
    $self->{LDFLAGS} = "$arch -L/usr/lib $ldflags";
    $self->{LDDLFLAGS} = "$arch -L/usr/lib $lddlflags";
}
