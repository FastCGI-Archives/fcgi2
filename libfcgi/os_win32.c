/*
 * os_win32.c --
 *
 *
 *  Copyright (c) 1995 Open Market, Inc.
 *  All rights reserved.
 *
 *  This file contains proprietary and confidential information and
 *  remains the unpublished property of Open Market, Inc. Use,
 *  disclosure, or reproduction is prohibited except as permitted by
 *  express written license agreement with Open Market, Inc.
 *
 *  Bill Snapper
 *  snapper@openmarket.com
 *
 * (Special thanks to Karen and Bill.  They made my job much easier and
 *  significantly more enjoyable.)
 */
#ifndef lint
static const char rcsid[] = "$Id: os_win32.c,v 1.7 2000/08/26 02:43:49 robs Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#define DLLAPI  __declspec(dllexport)

#include <assert.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <windows.h>

#include "fcgios.h"

#define ASSERT assert

#define WIN32_OPEN_MAX 32 /* XXX: Small hack */
#define MUTEX_VARNAME "_FCGI_MUTEX_"


static HANDLE hIoCompPort = INVALID_HANDLE_VALUE;
static HANDLE hStdinCompPort = INVALID_HANDLE_VALUE;
static HANDLE hStdinThread = INVALID_HANDLE_VALUE;

static HANDLE stdioHandles[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE,
				 INVALID_HANDLE_VALUE};

static HANDLE hPipeMutex = INVALID_HANDLE_VALUE;;
static char pipeMutexEnv[80] = "";

/*
 * An enumeration of the file types
 * supported by the FD_TABLE structure.
 *
 * XXX: Not all currently supported.  This allows for future
 *      functionality.
 */
typedef enum {
    FD_UNUSED,
    FD_FILE_SYNC,
    FD_FILE_ASYNC,
    FD_SOCKET_SYNC,
    FD_SOCKET_ASYNC,
    FD_PIPE_SYNC,
    FD_PIPE_ASYNC
} FILE_TYPE;

typedef union {
    HANDLE fileHandle;
    SOCKET sock;
    unsigned int value;
} DESCRIPTOR;

/*
 * Structure used to map file handle and socket handle
 * values into values that can be used to create unix-like
 * select bitmaps, read/write for both sockets/files.
 */
struct FD_TABLE {
    DESCRIPTOR fid;
    FILE_TYPE type;
    char *path;
    DWORD Errno;
    unsigned long instance;
    int status;
    int offset;			/* only valid for async file writes */
    LPDWORD offsetHighPtr;	/* pointers to offset high and low words */
    LPDWORD offsetLowPtr;	/* only valid for async file writes (logs) */
    HANDLE  hMapMutex;		/* mutex handle for multi-proc offset update */
    LPVOID  ovList;		/* List of associated OVERLAPPED_REQUESTs */
};

typedef struct FD_TABLE *PFD_TABLE;

/* 
 * XXX Note there is no dyanmic sizing of this table, so if the
 * number of open file descriptors exceeds WIN32_OPEN_MAX the 
 * app will blow up.
 */
static struct FD_TABLE fdTable[WIN32_OPEN_MAX];

struct OVERLAPPED_REQUEST {
    OVERLAPPED overlapped;
    unsigned long instance;	/* file instance (won't match after a close) */
    OS_AsyncProc procPtr;	/* callback routine */
    ClientData clientData;	/* callback argument */
    ClientData clientData1;	/* additional clientData */
};
typedef struct OVERLAPPED_REQUEST *POVERLAPPED_REQUEST;

static const char *bindPathPrefix = "\\\\.\\pipe\\FastCGI\\";

static int listenType = FD_UNUSED;
static HANDLE hListen = INVALID_HANDLE_VALUE;
static int libInitialized = 0;


/*
 *--------------------------------------------------------------
 *
 * Win32NewDescriptor --
 *
 *	Set up for I/O descriptor masquerading.
 *
 * Results:
 *	Returns "fake id" which masquerades as a UNIX-style "small
 *	non-negative integer" file/socket descriptor.
 *	Win32_* routine below will "do the right thing" based on the
 *	descriptor's actual type. -1 indicates failure.
 *
 * Side effects:
 *	Entry in fdTable is reserved to represent the socket/file.
 *
 *--------------------------------------------------------------
 */
static int Win32NewDescriptor(FILE_TYPE type, int fd, int desiredFd)
{
    int index;

    /*
     * If the "desiredFd" is not -1, try to get this entry for our
     * pseudo file descriptor.  If this is not available, return -1
     * as the caller wanted to get this mapping.  This is typically
     * only used for mapping stdio handles.
     */
    if ((desiredFd >= 0) &&
	(desiredFd < WIN32_OPEN_MAX)) {

        if(fdTable[desiredFd].type == FD_UNUSED) {
	    index = desiredFd;
	    goto found_entry;
        } else {
	    return -1;
	}

    }

    /*
     * Next see if the entry that matches "fd" is available.
     */
    if ((fd > 0) &&
	(fd < WIN32_OPEN_MAX) && (fdTable[fd].type == FD_UNUSED)) {
	index = fd;
	goto found_entry;
    }

    /*
     * Scan entries for one we can use. Start at 1 (0 fake id fails
     * in some cases). -K*
     */
    for (index = 1; index < WIN32_OPEN_MAX; index++)
	if (fdTable[index].type == FD_UNUSED)
	    break;

    /* If no table entries are available, return error. */
    if (index == WIN32_OPEN_MAX) {
	SetLastError(WSAEMFILE);
	DebugBreak();
	return -1;
    }

found_entry:
    fdTable[index].fid.value = fd;
    fdTable[index].type = type;
    fdTable[index].path = NULL;
    fdTable[index].Errno = NO_ERROR;
    fdTable[index].status = 0;
    fdTable[index].offset = -1;
    fdTable[index].offsetHighPtr = fdTable[index].offsetLowPtr = NULL;
    fdTable[index].hMapMutex = NULL;
    fdTable[index].ovList = NULL;

    return index;
}

/*
 *--------------------------------------------------------------
 *
 * StdinThread--
 *
 *	This thread performs I/O on stadard input.  It is needed
 *      because you can't guarantee that all applications will
 *      create standard input with sufficient access to perform
 *      asynchronous I/O.  Since we don't want to block the app
 *      reading from stdin we make it look like it's using I/O
 *      completion ports to perform async I/O.
 *
 * Results:
 *	Data is read from stdin and posted to the io completion
 *      port.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
static void StdinThread(LPDWORD startup){

    int doIo = TRUE;
    int fd;
    int bytesRead;
    POVERLAPPED_REQUEST pOv;

    while(doIo) {
        /*
         * Block until a request to read from stdin comes in or a
         * request to terminate the thread arrives (fd = -1).
         */
        if (!GetQueuedCompletionStatus(hStdinCompPort, &bytesRead, &fd,
	    (LPOVERLAPPED *)&pOv, (DWORD)-1) && !pOv) {
            doIo = 0;
            break;
        }

	ASSERT((fd == STDIN_FILENO) || (fd == -1));
        if(fd == -1) {
            doIo = 0;
            break;
        }
        ASSERT(pOv->clientData1 != NULL);

        if(ReadFile(stdioHandles[STDIN_FILENO], pOv->clientData1, bytesRead,
                    &bytesRead, NULL)) {
            PostQueuedCompletionStatus(hIoCompPort, bytesRead,
                                       STDIN_FILENO, (LPOVERLAPPED)pOv);
        } else {
            doIo = 0;
            break;
        }
    }

    ExitThread(0);
}


/*
 *--------------------------------------------------------------
 *
 * OS_LibInit --
 *
 *	Set up the OS library for use.
 *
 * Results:
 *	Returns 0 if success, -1 if not.
 *
 * Side effects:
 *	Sockets initialized, pseudo file descriptors setup, etc.
 *
 *--------------------------------------------------------------
 */
int OS_LibInit(int stdioFds[3])
{
    WORD  wVersion;
    WSADATA wsaData;
    int err;
    int fakeFd;
    DWORD pipeMode;
    DWORD threadId;
    char *cLenPtr = NULL;
    char *mutexPtr = NULL;

    if(libInitialized)
        return 0;

    /*
     * Initialize windows sockets library.
     */
    wVersion = MAKEWORD(1,1);
    err = WSAStartup( wVersion, &wsaData );
    if (err) {
        fprintf(stderr, "Error starting Windows Sockets.  Error: %d",
		WSAGetLastError());
	exit(111);
    }

    /*
     * Create the I/O completion port to be used for our I/O queue.
     */
    if (hIoCompPort == INVALID_HANDLE_VALUE) {
	hIoCompPort = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL,
					      0, 1);
	if(hIoCompPort == INVALID_HANDLE_VALUE) {
	    printf("<H2>OS_LibInit Failed CreateIoCompletionPort!  ERROR: %d</H2>\r\n\r\n",
	       GetLastError());
	    return -1;
	}
    }

    /*
     * Determine if this library is being used to listen for FastCGI
     * connections.  This is communicated by STDIN containing a
     * valid handle to a listener object.  In this case, both the
     * "stdout" and "stderr" handles will be INVALID (ie. closed) by
     * the starting process.
     *
     * The trick is determining if this is a pipe or a socket...
     *
     * XXX: Add the async accept test to determine socket or handle to a
     *      pipe!!!
     */
    if((GetStdHandle(STD_OUTPUT_HANDLE) == INVALID_HANDLE_VALUE) &&
       (GetStdHandle(STD_ERROR_HANDLE)  == INVALID_HANDLE_VALUE) &&
       (GetStdHandle(STD_INPUT_HANDLE)  != INVALID_HANDLE_VALUE) ) {

        hListen = GetStdHandle(STD_INPUT_HANDLE);

	/*
	 * Set the pipe handle state so that it operates in wait mode.
	 *
	 * NOTE: The listenFd is not mapped to a pseudo file descriptor
	 *       as all work done on it is contained to the OS library.
	 *
	 * XXX: Initial assumption is that SetNamedPipeHandleState will
	 *      fail if this is an IP socket...
	 */
        pipeMode = PIPE_READMODE_BYTE | PIPE_WAIT;
        if(SetNamedPipeHandleState(hListen, &pipeMode, NULL, NULL)) {
            listenType = FD_PIPE_SYNC;
            /*
             * Lookup the mutex.  If one is found, save it and
             * remove it from the env table if it's not already
             * been done.
             */
            mutexPtr = getenv(MUTEX_VARNAME);
            if(mutexPtr != NULL) {
                hPipeMutex = (HANDLE)atoi(mutexPtr);
                putenv(MUTEX_VARNAME"=");
            }
        } else {
            listenType = FD_SOCKET_SYNC;
        }
    }

    /*
     * If there are no stdioFds passed in, we're done.
     */
    if(stdioFds == NULL) {
        libInitialized = 1;
        return 0;
    }

    /*
     * Setup standard input asynchronous I/O.  There is actually a separate
     * thread spawned for this purpose.  The reason for this is that some
     * web servers use anonymous pipes for the connection between itself
     * and a CGI application.  Anonymous pipes can't perform asynchronous
     * I/O or use I/O completion ports.  Therefore in order to present a
     * consistent I/O dispatch model to an application we emulate I/O
     * completion port behavior by having the standard input thread posting
     * messages to the hIoCompPort which look like a complete overlapped
     * I/O structure.  This keeps the event dispatching simple from the
     * application perspective.
     */
    stdioHandles[STDIN_FILENO] = GetStdHandle(STD_INPUT_HANDLE);

    if(!SetHandleInformation(stdioHandles[STDIN_FILENO],
			     HANDLE_FLAG_INHERIT, 0)) {
/*
 * XXX: Causes error when run from command line.  Check KB
        err = GetLastError();
        DebugBreak();
	exit(99);
 */
    }

    if ((fakeFd = Win32NewDescriptor(FD_PIPE_SYNC,
				     (int)stdioHandles[STDIN_FILENO],
				     STDIN_FILENO)) == -1) {
        return -1;
    } else {
        /*
	 * Set stdin equal to our pseudo FD and create the I/O completion
	 * port to be used for async I/O.
	 */
	stdioFds[STDIN_FILENO] = fakeFd;
    }

    /*
     * Create the I/O completion port to be used for communicating with
     * the thread doing I/O on standard in.  This port will carry read
     * and possibly thread termination requests to the StdinThread.
     */
    if (hStdinCompPort == INVALID_HANDLE_VALUE) {
	hStdinCompPort = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL,
					      0, 1);
	if(hStdinCompPort == INVALID_HANDLE_VALUE) {
	    printf("<H2>OS_LibInit Failed CreateIoCompletionPort: STDIN!  ERROR: %d</H2>\r\n\r\n",
	       GetLastError());
	    return -1;
	}
    }

    /*
     * Create the thread that will read stdin if the CONTENT_LENGTH
     * is non-zero.
     */
    if((cLenPtr = getenv("CONTENT_LENGTH")) != NULL &&
       atoi(cLenPtr) > 0) {
        hStdinThread = CreateThread(NULL, 8192,
				    (LPTHREAD_START_ROUTINE)&StdinThread,
				    NULL, 0, &threadId);
	if (hStdinThread == NULL) {
	    printf("<H2>OS_LibInit Failed to create STDIN thread!  ERROR: %d</H2>\r\n\r\n",
		   GetLastError());
	    return -1;
        }
    }

    /*
     * STDOUT will be used synchronously.
     *
     * XXX: May want to convert this so that it could be used for OVERLAPPED
     *      I/O later.  If so, model it after the Stdin I/O as stdout is
     *      also incapable of async I/O on some servers.
     */
    stdioHandles[STDOUT_FILENO] = GetStdHandle(STD_OUTPUT_HANDLE);
    if(!SetHandleInformation(stdioHandles[STDOUT_FILENO],
			     HANDLE_FLAG_INHERIT, FALSE)) {
        DebugBreak();
	exit(99);
    }

    if ((fakeFd = Win32NewDescriptor(FD_PIPE_SYNC,
				     (int)stdioHandles[STDOUT_FILENO],
				     STDOUT_FILENO)) == -1) {
        return -1;
    } else {
        /*
	 * Set stdout equal to our pseudo FD
	 */
	stdioFds[STDOUT_FILENO] = fakeFd;
    }

    stdioHandles[STDERR_FILENO] = GetStdHandle(STD_ERROR_HANDLE);
    if(!SetHandleInformation(stdioHandles[STDERR_FILENO],
			     HANDLE_FLAG_INHERIT, FALSE)) {
        DebugBreak();
	exit(99);
    }
    if ((fakeFd = Win32NewDescriptor(FD_PIPE_SYNC,
				     (int)stdioHandles[STDERR_FILENO],
				     STDERR_FILENO)) == -1) {
        return -1;
    } else {
        /*
	 * Set stderr equal to our pseudo FD
	 */
	stdioFds[STDERR_FILENO] = fakeFd;
    }

    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * OS_LibShutdown --
 *
 *	Shutdown the OS library.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed, handles closed.
 *
 *--------------------------------------------------------------
 */
void OS_LibShutdown()
{

    if(hIoCompPort != INVALID_HANDLE_VALUE) {
        CloseHandle(hIoCompPort);
	hIoCompPort = INVALID_HANDLE_VALUE;
    }

    if(hStdinCompPort != INVALID_HANDLE_VALUE) {
        CloseHandle(hStdinCompPort);
	hStdinCompPort = INVALID_HANDLE_VALUE;
    }

    /*
     * Shutdown the socket library.
     */
    WSACleanup();
    return;
}


/*
 *--------------------------------------------------------------
 *
 * Win32FreeDescriptor --
 *
 *	Free I/O descriptor entry in fdTable.
 *
 * Results:
 *	Frees I/O descriptor entry in fdTable.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
static void Win32FreeDescriptor(int fd)
{
    /* Catch it if fd is a bogus value */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    ASSERT(fdTable[fd].type != FD_UNUSED);

    switch (fdTable[fd].type) {
	case FD_FILE_SYNC:
	case FD_FILE_ASYNC:
	    /* Free file path string */
	    ASSERT(fdTable[fd].path != NULL);
	    free(fdTable[fd].path);
	    fdTable[fd].path = NULL;
	    break;
	default:
	    /*
	     * Break through to generic fdTable free-descriptor code
	     */
	    break;

    }
    ASSERT(fdTable[fd].path == NULL);
    fdTable[fd].type = FD_UNUSED;
    fdTable[fd].path = NULL;
    fdTable[fd].Errno = NO_ERROR;
    fdTable[fd].offsetHighPtr = fdTable[fd].offsetLowPtr = NULL;
    if (fdTable[fd].hMapMutex != NULL) {
        CloseHandle(fdTable[fd].hMapMutex);
        fdTable[fd].hMapMutex = NULL;
    }
    return;
}


/*
 * OS_CreateLocalIpcFd --
 *
 *   This procedure is responsible for creating the listener pipe
 *   on Windows NT for local process communication.  It will create a
 *   named pipe and return a file descriptor to it to the caller.
 *
 * Results:
 *      Listener pipe created.  This call returns either a valid
 *      pseudo file descriptor or -1 on error.
 *
 * Side effects:
 *      Listener pipe and IPC address are stored in the FCGI info
 *         structure.
 *      'errno' will set on errors (-1 is returned).
 *
 *----------------------------------------------------------------------
 */
int OS_CreateLocalIpcFd(const char *bindPath, int backlog)
{
    int retFd = -1;
    SECURITY_ATTRIBUTES     sa;
    HANDLE hListenPipe = INVALID_HANDLE_VALUE;
    char *localPath;
    SOCKET listenSock;
    int bpLen;
    int servLen;
    struct  sockaddr_in	sockAddr;
    char    host[1024];
    short   port;
    int	    tcp = FALSE;
    int flag = 1;
    char    *tp;

    strcpy(host, bindPath);
    if((tp = strchr(host, ':')) != 0) {
	*tp++ = 0;
	if((port = atoi(tp)) == 0) {
	    *--tp = ':';
	 } else {
	    tcp = TRUE;
	 }
    }
    if(tcp && (*host && strcmp(host, "localhost") != 0)) {
	fprintf(stderr, "To start a service on a TCP port can not "
			"specify a host name.\n"
			"You should either use \"localhost:<port>\" or "
			" just use \":<port>.\"\n");
	exit(1);
    }

    if(tcp) {
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
        if(listenSock == SOCKET_ERROR) {
	    return -1;
	}
	/*
	 * Bind the listening socket.
	 */
	memset((char *) &sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockAddr.sin_port = htons(port);
	servLen = sizeof(sockAddr);

	if(bind(listenSock, (struct sockaddr *) &sockAddr, servLen) < 0
	   || listen(listenSock, backlog) < 0) {
	    perror("bind/listen");
	    exit(errno);
	}

	retFd = Win32NewDescriptor(FD_SOCKET_SYNC, (int)listenSock, -1);
	return retFd;
    }


    /*
     * Initialize the SECURITY_ATTRIUBTES structure.
     */
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;       /* This will be inherited by the
                                     * FastCGI process
                                     */

    /*
     * Create a mutex to be used to synchronize access to accepting a
     * connection on a named pipe.  We don't want to own this at creation
     * time but would rather let the first process that goes for it
     * be able to acquire it.
     */
    hPipeMutex = CreateMutex(NULL, FALSE, NULL);
    if(hPipeMutex == NULL) {
        return -1;
    }
    if(!SetHandleInformation(hPipeMutex, HANDLE_FLAG_INHERIT,
                                 TRUE)) {
        return -1;
    }
    sprintf(pipeMutexEnv, "%s=%d", MUTEX_VARNAME, (int)hPipeMutex);
    putenv(pipeMutexEnv);

    /*
     * Create a unique name to be used for the socket bind path.
     * Make sure that this name is unique and that there's no process
     * bound to it.
     *
     * Named Pipe Pathname: \\.\pipe\FastCGI\OM_WS.pid.N
     * Where: N is the pipe instance on the machine.
     *
     */
    bpLen = (int)strlen(bindPathPrefix);
    bpLen += strlen(bindPath);
    localPath = malloc(bpLen+2);
    strcpy(localPath, bindPathPrefix);
    strcat(localPath, bindPath);

    /*
     * Create and setup the named pipe to be used by the fcgi server.
     */
    hListenPipe = CreateNamedPipe(localPath, /* name of pipe */
		    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		    PIPE_TYPE_BYTE | PIPE_WAIT |
		    PIPE_READMODE_BYTE,		/* pipe IO type */
		    PIPE_UNLIMITED_INSTANCES,	/* number of instances */
		    4096,			/* size of outbuf (0 == allocate as necessary) */
		    4096,				/* size of inbuf */
		    0, /*1000,*/		/* default time-out value */
		    &sa);			/* security attributes */
    free(localPath);
    /*
     * Can't create an instance of the pipe, fail...
     */
    if (hListenPipe == INVALID_HANDLE_VALUE) {
	return -1;
    }

    retFd = Win32NewDescriptor(FD_PIPE_SYNC, (int)hListenPipe, -1);
    return retFd;
}


/*
 *----------------------------------------------------------------------
 *
 * OS_FcgiConnect --
 *
 *	Create the pipe pathname connect to the remote application if
 *      possible.
 *
 * Results:
 *      -1 if fail or a valid handle if connection succeeds.
 *
 * Side effects:
 *      Remote connection established.
 *
 *----------------------------------------------------------------------
 */
int OS_FcgiConnect(char *bindPath)
{
    char *pipePath = NULL;
    HANDLE hPipe;
    int pseudoFd, err;

    struct  sockaddr_in	sockAddr;
    int servLen, resultSock;
    int connectStatus;
    char    *tp;
    char    host[1024];
    short   port;
    int	    tcp = FALSE;

    strcpy(host, bindPath);
    if((tp = strchr(host, ':')) != 0) {
	*tp++ = 0;
	if((port = atoi(tp)) == 0) {
	    *--tp = ':';
	 } else {
	    tcp = TRUE;
	 }
    }
    if(tcp == TRUE) {
	struct	hostent	*hp;
	if((hp = gethostbyname((*host ? host : "localhost"))) == NULL) {
	    fprintf(stderr, "Unknown host: %s\n", bindPath);
	    exit(1000);
	}
	sockAddr.sin_family = AF_INET;
	memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
	sockAddr.sin_port = htons(port);
	servLen = sizeof(sockAddr);
	resultSock = socket(AF_INET, SOCK_STREAM, 0);

	ASSERT(resultSock >= 0);
	connectStatus = connect(resultSock, (struct sockaddr *)
				&sockAddr, servLen);
	if(connectStatus < 0) {
	    /*
	     * Most likely (errno == ENOENT || errno == ECONNREFUSED)
	     * and no FCGI application server is running.
	     */
	    closesocket(resultSock);
	    return -1;
	}
	pseudoFd = Win32NewDescriptor(FD_SOCKET_SYNC, resultSock, -1);
	if(pseudoFd == -1) {
	    closesocket(resultSock);
	}
	return pseudoFd;
    }

    /*
     * Not a TCP connection, create and connect to a named pipe.
     */
    pipePath = malloc((size_t)(strlen(bindPathPrefix) + strlen(bindPath) + 2));
    if(pipePath == NULL) {
        return -1;
    }
    strcpy(pipePath, bindPathPrefix);
    strcat(pipePath, bindPath);

    hPipe = CreateFile (pipePath,
			/* Generic access, read/write. */
			GENERIC_WRITE | GENERIC_READ,
			/* Share both read and write. */
			FILE_SHARE_READ | FILE_SHARE_WRITE ,
			NULL,                  /* No security.*/
			OPEN_EXISTING,         /* Fail if not existing. */
			FILE_FLAG_OVERLAPPED,  /* Use overlap. */
			NULL);                 /* No template. */

    free(pipePath);
    if(hPipe == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if ((pseudoFd = Win32NewDescriptor(FD_PIPE_ASYNC, (int)hPipe, -1)) == -1) {
        CloseHandle(hPipe);
        return -1;
    } else {
        /*
	 * Set stdin equal to our pseudo FD and create the I/O completion
	 * port to be used for async I/O.
	 */
        if (!CreateIoCompletionPort(hPipe, hIoCompPort, pseudoFd, 1)) {
	    err = GetLastError();
	    Win32FreeDescriptor(pseudoFd);
	    CloseHandle(hPipe);
	    return -1;
	}
    }
    return pseudoFd;
}


/*
 *--------------------------------------------------------------
 *
 * OS_Read --
 *
 *	Pass through to the appropriate NT read function.
 *
 * Results:
 *	Returns number of byes read. Mimics unix read:.
 *		n bytes read, 0 or -1 failure: errno contains actual error
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
int OS_Read(int fd, char * buf, size_t len)
{
    DWORD bytesRead;
    int ret;

    /*
     * Catch any bogus fd values
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));

    switch (fdTable[fd].type) {
	case FD_FILE_SYNC:
	case FD_FILE_ASYNC:
	case FD_PIPE_SYNC:
	case FD_PIPE_ASYNC:
	    bytesRead = fd;
	    /*
	     * ReadFile returns: TRUE success, FALSE failure
	     */
	    if (!ReadFile(fdTable[fd].fid.fileHandle, buf, len, &bytesRead,
		NULL)) {
		fdTable[fd].Errno = GetLastError();
		return -1;
	    }
	    return bytesRead;

	case FD_SOCKET_SYNC:
	case FD_SOCKET_ASYNC:
	    /* winsock recv returns n bytes recv'ed, SOCKET_ERROR failure */
	    /*
	     * XXX: Test this with ReadFile.  If it works, remove this code
	     *      to simplify the routine.
	     */
	    if ((ret = recv(fdTable[fd].fid.sock, buf, len, 0)) ==
		SOCKET_ERROR) {
		fdTable[fd].Errno = WSAGetLastError();
		return -1;
	    }
	    return ret;
	default:
		return -1;
    }
}

/*
 *--------------------------------------------------------------
 *
 * OS_Write --
 *
 *	Perform a synchronous OS write.
 *
 * Results:
 *	Returns number of bytes written. Mimics unix write:
 *		n bytes written, 0 or -1 failure (??? couldn't find man page).
 *
 * Side effects:
 *	none.
 *
 *--------------------------------------------------------------
 */
int OS_Write(int fd, char * buf, size_t len)
{
    DWORD bytesWritten;
    int ret;

    /*
     * Catch any bogus fd values
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    ASSERT((fdTable[fd].type > FD_UNUSED) &&
	   (fdTable[fd].type <= FD_PIPE_ASYNC));

    switch (fdTable[fd].type) {
	case FD_FILE_SYNC:
	case FD_FILE_ASYNC:
	case FD_PIPE_SYNC:
	case FD_PIPE_ASYNC:
	    bytesWritten = fd;
	    /*
	     * WriteFile returns: TRUE success, FALSE failure
	     */
	    if (!WriteFile(fdTable[fd].fid.fileHandle, buf, len,
		&bytesWritten, NULL)) {
		fdTable[fd].Errno = GetLastError();
		return -1;
	    }
	    return bytesWritten;
	case FD_SOCKET_SYNC:
	case FD_SOCKET_ASYNC:
	    /* winsock send returns n bytes written, SOCKET_ERROR failure */
	    /*
	     * XXX: Test this with WriteFile.  If it works, remove this code
	     *      to simplify the routine.
	     */
	    if ((ret = send(fdTable[fd].fid.sock, buf, len, 0)) ==
		SOCKET_ERROR) {
		fdTable[fd].Errno = WSAGetLastError();
		return -1;
	    }
	    return ret;
	default:
	    return -1;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * OS_SpawnChild --
 *
 *	Spawns a new server listener process, and stores the information
 *      relating to the child in the supplied record.  A wait handler is
 *	registered on the child's completion.  This involves creating
 *        a process on NT and preparing a command line with the required
 *        state (currently a -childproc flag and the server socket to use
 *        for accepting connections).
 *
 * Results:
 *      0 if success, -1 if error.
 *
 * Side effects:
 *      Child process spawned.
 *
 *----------------------------------------------------------------------
 */
int OS_SpawnChild(char *execPath, int listenFd)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION pInfo;
    BOOL success;

    memset((void *)&StartupInfo, 0, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof (STARTUPINFO);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpDesktop = NULL;

    /*
     * FastCGI on NT will set the listener pipe HANDLE in the stdin of
     * the new process.  The fact that there is a stdin and NULL handles
     * for stdout and stderr tells the FastCGI process that this is a
     * FastCGI process and not a CGI process.
     */
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    /*
     * XXX: Do I have to dup the handle before spawning the process or is
     *      it sufficient to use the handle as it's reference counted
     *      by NT anyway?
     */
    StartupInfo.hStdInput  = fdTable[listenFd].fid.fileHandle;
    StartupInfo.hStdOutput = INVALID_HANDLE_VALUE;
    StartupInfo.hStdError  = INVALID_HANDLE_VALUE;

    /*
     * Make the listener socket inheritable.
     */
    success = SetHandleInformation(StartupInfo.hStdInput, HANDLE_FLAG_INHERIT,
				   TRUE);
    if(!success) {
        exit(99);
    }

    /*
     * XXX: Might want to apply some specific security attributes to the
     *      processes.
     */
    success = CreateProcess(execPath,	/* LPCSTR address of module name */
			NULL,           /* LPCSTR address of command line */
		        NULL,		/* Process security attributes */
			NULL,		/* Thread security attributes */
			TRUE,		/* Inheritable Handes inherited. */
			0,		/* DWORD creation flags  */
		        NULL,           /* Use parent environment block */
			NULL,		/* Address of current directory name */
			&StartupInfo,   /* Address of STARTUPINFO  */
			&pInfo);	/* Address of PROCESS_INFORMATION   */
    if(success) {
        return 0;
    } else {
        return -1;
    }
}


/*
 *--------------------------------------------------------------
 *
 * OS_AsyncReadStdin --
 *
 *	This initiates an asynchronous read on the standard
 *	input handle.  This handle is not guaranteed to be
 *      capable of performing asynchronous I/O so we send a
 *      message to the StdinThread to do the synchronous read.
 *
 * Results:
 *	-1 if error, 0 otherwise.
 *
 * Side effects:
 *	Asynchronous message is queued to the StdinThread and an
 *      overlapped structure is allocated/initialized.
 *
 *--------------------------------------------------------------
 */
int OS_AsyncReadStdin(void *buf, int len, OS_AsyncProc procPtr,
                      ClientData clientData)
{
    POVERLAPPED_REQUEST pOv;

    ASSERT(fdTable[STDIN_FILENO].type != FD_UNUSED);

    pOv = (POVERLAPPED_REQUEST)malloc(sizeof(struct OVERLAPPED_REQUEST));
    ASSERT(pOv);
    memset((void *)pOv, 0, sizeof(struct OVERLAPPED_REQUEST));
    pOv->clientData1 = (ClientData)buf;
    pOv->instance = fdTable[STDIN_FILENO].instance;
    pOv->procPtr = procPtr;
    pOv->clientData = clientData;

    PostQueuedCompletionStatus(hStdinCompPort, len, STDIN_FILENO,
                               (LPOVERLAPPED)pOv);
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * OS_AsyncRead --
 *
 *	This initiates an asynchronous read on the file
 *	handle which may be a socket or named pipe.
 *
 *	We also must save the ProcPtr and ClientData, so later
 *	when the io completes, we know who to call.
 *
 *	We don't look at any results here (the ReadFile may
 *	return data if it is cached) but do all completion
 *	processing in OS_Select when we get the io completion
 *	port done notifications.  Then we call the callback.
 *
 * Results:
 *	-1 if error, 0 otherwise.
 *
 * Side effects:
 *	Asynchronous I/O operation is queued for completion.
 *
 *--------------------------------------------------------------
 */
int OS_AsyncRead(int fd, int offset, void *buf, int len,
		 OS_AsyncProc procPtr, ClientData clientData)
{
    DWORD bytesRead;
    POVERLAPPED_REQUEST pOv;

    /*
     * Catch any bogus fd values
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    /*
     * Confirm that this is an async fd
     */
    ASSERT(fdTable[fd].type != FD_UNUSED);
    ASSERT(fdTable[fd].type != FD_FILE_SYNC);
    ASSERT(fdTable[fd].type != FD_PIPE_SYNC);
    ASSERT(fdTable[fd].type != FD_SOCKET_SYNC);

    pOv = (POVERLAPPED_REQUEST)malloc(sizeof(struct OVERLAPPED_REQUEST));
    ASSERT(pOv);
    memset((void *)pOv, 0, sizeof(struct OVERLAPPED_REQUEST));
    /*
     * Only file offsets should be non-zero, but make sure.
     */
    if (fdTable[fd].type == FD_FILE_ASYNC)
	if (fdTable[fd].offset >= 0)
	    pOv->overlapped.Offset = fdTable[fd].offset;
	else
	    pOv->overlapped.Offset = offset;
    pOv->instance = fdTable[fd].instance;
    pOv->procPtr = procPtr;
    pOv->clientData = clientData;
    bytesRead = fd;
    /*
     * ReadFile returns: TRUE success, FALSE failure
     */
    if (!ReadFile(fdTable[fd].fid.fileHandle, buf, len, &bytesRead,
	(LPOVERLAPPED)pOv)) {
	fdTable[fd].Errno = GetLastError();
	if(fdTable[fd].Errno == ERROR_NO_DATA ||
	   fdTable[fd].Errno == ERROR_PIPE_NOT_CONNECTED) {
	    PostQueuedCompletionStatus(hIoCompPort, 0, fd, (LPOVERLAPPED)pOv);
	    return 0;
	}
	if(fdTable[fd].Errno != ERROR_IO_PENDING) {
	    PostQueuedCompletionStatus(hIoCompPort, 0, fd, (LPOVERLAPPED)pOv);
	    return -1;
	}
	fdTable[fd].Errno = 0;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * OS_AsyncWrite --
 *
 *	This initiates an asynchronous write on the "fake" file
 *	descriptor (which may be a file, socket, or named pipe).
 *	We also must save the ProcPtr and ClientData, so later
 *	when the io completes, we know who to call.
 *
 *	We don't look at any results here (the WriteFile generally
 *	completes immediately) but do all completion processing
 *	in OS_DoIo when we get the io completion port done
 *	notifications.  Then we call the callback.
 *
 * Results:
 *	-1 if error, 0 otherwise.
 *
 * Side effects:
 *	Asynchronous I/O operation is queued for completion.
 *
 *--------------------------------------------------------------
 */
int OS_AsyncWrite(int fd, int offset, void *buf, int len,
		  OS_AsyncProc procPtr, ClientData clientData)
{
    DWORD bytesWritten;
    POVERLAPPED_REQUEST pOv;

    /*
     * Catch any bogus fd values
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    /*
     * Confirm that this is an async fd
     */
    ASSERT(fdTable[fd].type != FD_UNUSED);
    ASSERT(fdTable[fd].type != FD_FILE_SYNC);
    ASSERT(fdTable[fd].type != FD_PIPE_SYNC);
    ASSERT(fdTable[fd].type != FD_SOCKET_SYNC);

    pOv = (POVERLAPPED_REQUEST)malloc(sizeof(struct OVERLAPPED_REQUEST));
    ASSERT(pOv);
    memset((void *)pOv, 0, sizeof(struct OVERLAPPED_REQUEST));
    /*
     * Only file offsets should be non-zero, but make sure.
     */
    if (fdTable[fd].type == FD_FILE_ASYNC)
	/*
	 * Only file opened via OS_AsyncWrite with
	 * O_APPEND will have an offset != -1.
	 */
	if (fdTable[fd].offset >= 0)
	    /*
	     * If the descriptor has a memory mapped file
	     * handle, take the offsets from there.
	     */
	    if (fdTable[fd].hMapMutex != NULL) {
		/*
		 * Wait infinitely; this *should* not cause problems.
		 */
		WaitForSingleObject(fdTable[fd].hMapMutex, INFINITE);

		/*
		 * Retrieve the shared offset values.
		 */
		pOv->overlapped.OffsetHigh = *(fdTable[fd].offsetHighPtr);
		pOv->overlapped.Offset = *(fdTable[fd].offsetLowPtr);

		/*
		 * Update the shared offset values for the next write
		 */
		*(fdTable[fd].offsetHighPtr) += 0;	/* XXX How do I handle overflow */
		*(fdTable[fd].offsetLowPtr) += len;

		ReleaseMutex(fdTable[fd].hMapMutex);
	    } else
	        pOv->overlapped.Offset = fdTable[fd].offset;
	else
	    pOv->overlapped.Offset = offset;
    pOv->instance = fdTable[fd].instance;
    pOv->procPtr = procPtr;
    pOv->clientData = clientData;
    bytesWritten = fd;
    /*
     * WriteFile returns: TRUE success, FALSE failure
     */
    if (!WriteFile(fdTable[fd].fid.fileHandle, buf, len, &bytesWritten,
	(LPOVERLAPPED)pOv)) {
	fdTable[fd].Errno = GetLastError();
	if(fdTable[fd].Errno != ERROR_IO_PENDING) {
	    PostQueuedCompletionStatus(hIoCompPort, 0, fd, (LPOVERLAPPED)pOv);
	    return -1;
	}
	fdTable[fd].Errno = 0;
    }
    if (fdTable[fd].offset >= 0)
	fdTable[fd].offset += len;
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * OS_Close --
 *
 *	Closes the descriptor with routine appropriate for
 *      descriptor's type.
 *
 * Results:
 *	Socket or file is closed. Return values mimic Unix close:
 *		0 success, -1 failure
 *
 * Side effects:
 *	Entry in fdTable is marked as free.
 *
 *--------------------------------------------------------------
 */
int OS_Close(int fd)
{
    int ret = 0;

    /*
     * Catch it if fd is a bogus value
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    ASSERT(fdTable[fd].type != FD_UNUSED);

    switch (fdTable[fd].type) {
	case FD_PIPE_SYNC:
	case FD_PIPE_ASYNC:
	case FD_FILE_SYNC:
	case FD_FILE_ASYNC:
	    /*
	     * CloseHandle returns: TRUE success, 0 failure
	     */
	    if (CloseHandle(fdTable[fd].fid.fileHandle) == FALSE)
		ret = -1;
	    break;
	case FD_SOCKET_SYNC:
	case FD_SOCKET_ASYNC:
	    /*
	     * Closing a socket that has an async read outstanding causes a
	     * tcp reset and possible data loss.  The shutdown call seems to
	     * prevent this.
	     */
	    shutdown(fdTable[fd].fid.sock, 2);
	    /*
	     * closesocket returns: 0 success, SOCKET_ERROR failure
	     */
	    if (closesocket(fdTable[fd].fid.sock) == SOCKET_ERROR)
		ret = -1;
	    break;
	default:
	    return -1;		/* fake failure */
    }

    Win32FreeDescriptor(fd);
    return ret;
}

/*
 *--------------------------------------------------------------
 *
 * OS_CloseRead --
 *
 *	Cancel outstanding asynchronous reads and prevent subsequent
 *      reads from completing.
 *
 * Results:
 *	Socket or file is shutdown. Return values mimic Unix shutdown:
 *		0 success, -1 failure
 *
 *--------------------------------------------------------------
 */
int OS_CloseRead(int fd)
{
    int ret = 0;

    /*
     * Catch it if fd is a bogus value
     */
    ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
    ASSERT(fdTable[fd].type == FD_SOCKET_ASYNC
	|| fdTable[fd].type == FD_SOCKET_SYNC);

    if (shutdown(fdTable[fd].fid.sock,0) == SOCKET_ERROR)
	ret = -1;
    return ret;
}

/*
 *--------------------------------------------------------------
 *
 * OS_DoIo --
 *
 *	This function was formerly OS_Select.  It's purpose is
 *      to pull I/O completion events off the queue and dispatch
 *      them to the appropriate place.
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	Handlers are called.
 *
 *--------------------------------------------------------------
 */
int OS_DoIo(struct timeval *tmo)
{
    int fd;
    int bytes;
    POVERLAPPED_REQUEST pOv;
    struct timeb tb;
    int ms;
    int ms_last;
    int err;

    /* XXX
     * We can loop in here, but not too long, as wait handlers
     * must run.
     * For cgi stdin, apparently select returns when io completion
     * ports don't, so don't wait the full timeout.
     */
    if(tmo)
	ms = (tmo->tv_sec*1000 + tmo->tv_usec/1000) / 2;
    else
	ms = 1000;
    ftime(&tb);
    ms_last = tb.time*1000 + tb.millitm;
    while (ms >= 0) {
	if(tmo && (ms = tmo->tv_sec*1000 + tmo->tv_usec/1000)> 100)
	    ms = 100;
	if (!GetQueuedCompletionStatus(hIoCompPort, &bytes, &fd,
	    (LPOVERLAPPED *)&pOv, ms) && !pOv) {
	    err = WSAGetLastError();
	    return 0; /* timeout */
        }

	ASSERT((fd >= 0) && (fd < WIN32_OPEN_MAX));
	/* call callback if descriptor still valid */
	ASSERT(pOv);
	if(pOv->instance == fdTable[fd].instance)
	  (*pOv->procPtr)(pOv->clientData, bytes);
	free(pOv);

	ftime(&tb);
	ms -= (tb.time*1000 + tb.millitm - ms_last);
	ms_last = tb.time*1000 + tb.millitm;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * OS_Accept --
 *
 *	Accepts a new FastCGI connection.  This routine knows whether
 *      we're dealing with TCP based sockets or NT Named Pipes for IPC.
 *
 * Results:
 *      -1 if the operation fails, otherwise this is a valid IPC fd.
 *
 * Side effects:
 *      New IPC connection is accepted.
 *
 *----------------------------------------------------------------------
 */
int OS_Accept(int listen_sock, int fail_on_intr, const char *webServerAddrs)
{
    /* XXX This is broken for listen_sock & fail_on_intr */
    struct sockaddr_in sa;
    int isNewConnection;
    int ipcFd = -1;
    BOOL pConnected;
    HANDLE hDup;
    SOCKET hSock;
    int clilen = sizeof(sa);
    DWORD waitForStatus;

    switch(listenType) {

    case FD_PIPE_SYNC:
        waitForStatus = WaitForSingleObject(hPipeMutex,INFINITE);
        switch(waitForStatus) {
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED:
            break;

        case WAIT_FAILED:
        default:
            return -1;
        }

        /*
         * We have the mutex, go for the connection.
         */
    	pConnected = ConnectNamedPipe(hListen, NULL) ?
	                      TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        ReleaseMutex(hPipeMutex);
	    if(pConnected) {
	        /*
	         * Success...
	         */
	        if (!DuplicateHandle(GetCurrentProcess(), hListen,
				 GetCurrentProcess(), &hDup, 0,
				 TRUE,		/* allow inheritance */
				 DUPLICATE_SAME_ACCESS)) {
	            return -1;
	        }
	        ipcFd = Win32NewDescriptor(FD_PIPE_SYNC, (int)hDup, -1);
	        if(ipcFd == -1) {
                DisconnectNamedPipe(hListen);
                CloseHandle(hDup);
            }
            return ipcFd;
        } else {
            return -1;
    }
	break;

    case FD_SOCKET_SYNC:
	hSock = accept((int)hListen, (struct sockaddr *) &sa, &clilen);
	if(hSock == -1) {
	    return -1;
	} else if (sa.sin_family != AF_INET) { /* What are we? */
	    closesocket(hSock);
	    hSock = (SOCKET)-1;
	    return -1;
	} else {
	    char	*tp1, *tp2;
	    int	match = 0;
	    if (webServerAddrs == NULL)
	        isNewConnection = TRUE;
	    else {
	        tp1 = (char *) malloc(strlen(webServerAddrs)+1);
            ASSERT(tp1 != NULL);
		strcpy(tp1, webServerAddrs);
		while(tp1) {
		    if ((tp2 = strchr(tp1, ',')) != NULL)
		        *tp2++ = 0;

		    if (inet_addr(tp1) == sa.sin_addr.s_addr) {
		        match = 1;
			break;
		    }
		    tp1 = tp2;
		}
		free(tp1);
		if (match)
		    isNewConnection = TRUE;
		else {
		    closesocket(hSock);
		    hSock = (SOCKET)-1;
		    return -1;
		}
	    }
	}

	ipcFd = Win32NewDescriptor(FD_SOCKET_SYNC, hSock, -1);
	if(ipcFd == -1) {
	    closesocket(hSock);
	}
	return ipcFd;
	break;

    case FD_UNUSED:
      default:
        exit(101);
	break;

    }
}

/*
 *----------------------------------------------------------------------
 *
 * OS_IpcClose
 *
 *	OS IPC routine to close an IPC connection.
 *
 * Results:
 *
 *
 * Side effects:
 *      IPC connection is closed.
 *
 *----------------------------------------------------------------------
 */
int OS_IpcClose(int ipcFd)
{
    /*
     * Catch it if fd is a bogus value
     */
    ASSERT((ipcFd >= 0) && (ipcFd < WIN32_OPEN_MAX));
    ASSERT(fdTable[ipcFd].type != FD_UNUSED);

    switch(listenType) {

    case FD_PIPE_SYNC:
	/*
	 * Make sure that the client (ie. a Web Server in this case) has
	 * read all data from the pipe before we disconnect.
	 */
	if(!FlushFileBuffers(fdTable[ipcFd].fid.fileHandle))
	    return -1;
	if(DisconnectNamedPipe(fdTable[ipcFd].fid.fileHandle)) {
	    OS_Close(ipcFd);
	    return 0;
	} else {
	    return -1;
	}
	break;

    case FD_SOCKET_SYNC:
	OS_Close(ipcFd);
        return 0;
	break;

    case FD_UNUSED:
    default:
	exit(106);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * OS_IsFcgi --
 *
 *	Determines whether this process is a FastCGI process or not.
 *
 * Results:
 *      Returns 1 if FastCGI, 0 if not.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int OS_IsFcgi(int sock)
{
    /* XXX This is broken for sock */
    if(listenType == FD_UNUSED) {
        return FALSE;
    } else {
        return TRUE;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * OS_SetFlags --
 *
 *      Sets selected flag bits in an open file descriptor.  Currently
 *      this is only to put a SOCKET into non-blocking mode.
 *
 *----------------------------------------------------------------------
 */
void OS_SetFlags(int fd, int flags)
{
    unsigned long pLong = 1L;
    int err;

    if (fdTable[fd].type == FD_SOCKET_SYNC && flags == O_NONBLOCK) {
        if (ioctlsocket(fdTable[fd].fid.sock, FIONBIO, &pLong) ==
	    SOCKET_ERROR) {
	    exit(WSAGetLastError());
        }
        if (!CreateIoCompletionPort((HANDLE)fdTable[fd].fid.sock,
				    hIoCompPort, fd, 1)) {
	    err = GetLastError();
	    exit(err);
	}

        fdTable[fd].type = FD_SOCKET_ASYNC;
    }
    return;
}

