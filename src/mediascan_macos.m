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
  int link_type = LINK_NONE;
  struct stat fileInfo;
  CFStringRef cfPath = NULL;
  CFURLRef url = NULL;

  //CFStringRef in_path = CFStringCreateWithCString(NULL, incoming_path, kCFStringEncodingMacRoman);

// Use lstat to determine if the file is a directory or symlink
//  if (lstat([[NSFileManager defaultManager]
//fileSystemRepresentationWithPath:(NSString *)in_path], &fileInfo) < 0) {
  if (lstat(incoming_path, &fileInfo) < 0) {
    link_type = LINK_NONE;
    goto exit;
  }

  if (S_ISLNK(fileInfo.st_mode)) {
    link_type = LINK_SYMLINK;
  }
  else {
    // Now check for aliases
    cfPath = CFStringCreateWithCString(kCFAllocatorDefault, incoming_path, kCFStringEncodingUTF8);
    url    = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, NO);

    if (url != NULL) {
      FSRef fsRef;

      if (CFURLGetFSRef(url, &fsRef)) {
        Boolean targetIsFolder, wasAliased;
        OSErr err = FSResolveAliasFile(&fsRef, true, &targetIsFolder, &wasAliased);
        if ((err == noErr) && wasAliased)
    		  link_type = LINK_ALIAS;
        else
    		  link_type = LINK_NONE;
      }

      CFRelease(url);
    }

    CFRelease(cfPath);
  }

exit:
  return link_type;
}                               /* isAlias() */

int CheckMacAlias(const char *incoming_path, char *out_path) {
  NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];
  NSString *NSPath = [[NSString alloc] initWithUTF8String:incoming_path];
  NSString *resolvedPath = [NSPath stringByResolvingSymlinksAndAliases];
  LOG_DEBUG("CheckMacAlias: %s => %s\n", [NSPath UTF8String], [resolvedPath UTF8String]);
  if(resolvedPath == nil) {
    [NSPath autorelease];
    [pool drain];
    return FALSE;
  }

  const char *cString = [resolvedPath UTF8String];
  strncpy(out_path, cString, PATH_MAX);

  [NSPath autorelease];
  [pool drain];
  return TRUE;
}                               /* CheckMacAlias() */
