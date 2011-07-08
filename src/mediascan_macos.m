///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_macos.m
///
///  mediascan cocoa methods
///-------------------------------------------------------------------------------------------------

#import <Cocoa/Cocoa.h>
#import <Foundation/NSString.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/NSFileManager.h>
#import "NSString+SymlinksAndAliases.h"

#include <sys/stat.h>

#include <libmediascan.h>

#define LINK_NONE 		0
#define LINK_ALIAS 		1
#define LINK_SYMLINK 	2

#include "common.h"

int isAlias(const char *incoming_path) {

  struct stat fileInfo;
  NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];

  CFStringRef in_path = CFStringCreateWithCString(NULL, incoming_path, kCFStringEncodingMacRoman);

// Use lstat to determine if the file is a directory or symlink
  if (lstat([[NSFileManager defaultManager]
fileSystemRepresentationWithPath:(NSString *)in_path], &fileInfo) < 0) {
    return LINK_NONE;
  }


  if (S_ISLNK(fileInfo.st_mode)) {
    return LINK_SYMLINK;
  }
  else {
    // Now check for aliases
    CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault,
                                                   incoming_path, kCFStringEncodingUTF8);

    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, NO);

    if (url != NULL) {
      FSRef fsRef;
      if (CFURLGetFSRef(url, &fsRef)) {
        Boolean targetIsFolder, wasAliased;
        OSErr err = FSResolveAliasFile(&fsRef, true, &targetIsFolder, &wasAliased);
        if ((err == noErr) && wasAliased)
          return LINK_ALIAS;
        else
          return LINK_NONE;
      }
      CFRelease(url);
    }
  }

// free pool
  [pool release];

  return LINK_NONE;
}                               /* isAlias() */

int CheckMacAlias(const char *incoming_path, char *out_path) {

  CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 incoming_path, kCFStringEncodingUTF8);

  NSString *resolvedPath =[cfPath stringByResolvingSymlinksAndAliases];

  CFStringGetCString(resolvedPath, out_path, MAX_PATH, kCFStringEncodingMacRoman);
}                               /* CheckMacAlias() */
