///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_macos.m
///
///  mediascan cocoa methods
///-------------------------------------------------------------------------------------------------

#import <Cocoa/Cocoa.h>
#import <Foundation/NSString.h>
#import <CoreServices/CoreServices.h>

#include <libmediascan.h>

#include "common.h"

int isAlias(const char *incoming_path) {

CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault,
							incoming_path, kCFStringEncodingUTF8);

CFURLRef url = CFURLCreateWithFileSystemPath
                   (kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, NO);
                   
if (url != NULL)
{
    FSRef fsRef;
 	if (CFURLGetFSRef(url, &fsRef))
    {
        Boolean targetIsFolder, wasAliased;
        OSErr err = FSResolveAliasFile (&fsRef, true, &targetIsFolder, &wasAliased);
        if((err == noErr) && wasAliased)
        	return 1;
        else 
        	return 0;
    }
    CFRelease(url);
}
return 0;
} /* isAlias() */

int CheckMacAlias(const char *incoming_path, char *out_path) {

CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault,
							incoming_path, kCFStringEncodingUTF8);
NSString *resolvedPath = nil;

CFURLRef url = CFURLCreateWithFileSystemPath
                   (kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, NO);

if (url != NULL)
{
    FSRef fsRef;
    if (CFURLGetFSRef(url, &fsRef))
    {
        Boolean targetIsFolder, wasAliased;
        OSErr err = FSResolveAliasFile (&fsRef, true, &targetIsFolder, &wasAliased);
        if ((err == noErr) && wasAliased)
        {
            CFURLRef resolvedUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsRef);
            if (resolvedUrl != NULL)
            {
                resolvedPath = (NSString*)
                        CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle);
                CFRelease(resolvedUrl);
            }
        }
    }
    CFRelease(url);
}

if (resolvedPath == nil)
{
    resolvedPath = [[NSString alloc] initWithString:incoming_path];
}

CFStringGetCString(resolvedPath, out_path, MAX_PATH, kCFStringEncodingMacRoman);

} /* CheckMacAlias() */