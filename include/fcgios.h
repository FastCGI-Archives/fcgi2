/* 
 * fcgios.h --
 *
 *      Description of file.
 *
 *
 *  Copyright (c) 1996 Open Market, Inc.
 *  All rights reserved.
 *
 *  This file contains proprietary and confidential information and
 *  remains the unpublished property of Open Market, Inc. Use, 
 *  disclosure, or reproduction is prohibited except as permitted by 
 *  express written license agreement with Open Market, Inc. 
 *
 *  Bill Snapper
 *  snapper@openmarket.com
 */
#ifndef _FCGIOS_H
#define _FCGIOS_H

#include <sys/time.h>

#ifdef _WIN32
#define OS_Errno GetLastError()
#define OS_SetErrno(err) SetLastError(err)
#ifndef DLLAPI
#define DLLAPI __declspec(dllimport)
#endif
#else
#define DLLAPI
#define OS_Errno errno
#define OS_SetErrno(err) errno = (err)
#endif

#ifdef _WIN32
#include <io.h>
#endif

/* This is the initializer for a "struct timeval" used in a select() call
 * right after a new request is accept()ed to determine readablity.  Its
 * a drop-dead timer.  Its only used for AF_UNIX sockets (not TCP sockets).
 * Its a workaround for a kernel bug in Linux 2.0.x and SCO Unixware.
 * Making this as small as possible, yet remain reliable would be best.
 * 2 seconds is very conservative.  0,0 is not reliable.  The shorter the 
 * timeout, the faster request processing will recover.  The longer the
 * timeout, the more likely this application being "busy" will cause other
 * requests to abort and cause more dead sockets that need this timeout. */
#define READABLE_UNIX_FD_DROP_DEAD_TIMEVAL 2,0

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif


#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef X_OK
#define X_OK       0x01
#endif

#ifdef _WIN32
#ifndef O_NONBLOCK
#define O_NONBLOCK     0x0004  /* no delay */
#endif
#endif

#if defined (c_plusplus) || defined (__cplusplus)
extern "C" {
#endif

#ifndef _CLIENTDATA
#   if defined(__STDC__) || defined(__cplusplus)
    typedef void *ClientData;
#   else
    typedef int *ClientData;
#   endif /* __STDC__ */
#define _CLIENTDATA
#endif

typedef void (*OS_AsyncProc) (ClientData clientData, int len);

DLLAPI int OS_LibInit(int stdioFds[3]);
DLLAPI void OS_LibShutdown(void);
DLLAPI int OS_CreateLocalIpcFd(char *bindPath);
DLLAPI int OS_FcgiConnect(char *bindPath);
DLLAPI int OS_Read(int fd, char * buf, size_t len);
DLLAPI int OS_Write(int fd, char * buf, size_t len);
DLLAPI int OS_SpawnChild(char *execPath, int listenFd);
DLLAPI int OS_AsyncReadStdin(void *buf, int len, OS_AsyncProc procPtr, 
                      ClientData clientData);
DLLAPI int OS_AsyncRead(int fd, int offset, void *buf, int len,
		 OS_AsyncProc procPtr, ClientData clientData);
DLLAPI int OS_AsyncWrite(int fd, int offset, void *buf, int len, 
		  OS_AsyncProc procPtr, ClientData clientData);
DLLAPI int OS_Close(int fd);
DLLAPI int OS_CloseRead(int fd);
DLLAPI int OS_DoIo(struct timeval *tmo);
DLLAPI int OS_FcgiIpcAccept(char *clientAddrList);
DLLAPI int OS_IpcClose(int ipcFd);
DLLAPI int OS_IsFcgi(void);
DLLAPI void OS_SetFlags(int fd, int flags);

#if defined (__cplusplus) || defined (c_plusplus)
} /* terminate extern "C" { */
#endif

#endif /* _FCGIOS_H */
