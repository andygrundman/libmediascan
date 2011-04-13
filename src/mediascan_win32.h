///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_win32.h
///
///  Win32 specific file system functionality
///-------------------------------------------------------------------------------------------------

#define inline __inline

///-------------------------------------------------------------------------------------------------
///  Win32 specific MediaScan initalization.
///
/// @author Henry Bennett
/// @date 03/15/2011
///-------------------------------------------------------------------------------------------------

void win32_init(void);

///-------------------------------------------------------------------------------------------------
///  Begin a recursive scan of all paths previously provided to ms_add_path(). If async mode
///   is enabled, this call will return immediately. You must obtain the file descriptor using
///   ms_async_fd and this must be checked using an event loop or select(). When the fd becomes
///   readable you must call ms_async_process to trigger any necessary callbacks.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_scan(MediaScan *s);
