/* 
 * sample-store.c --
 *
 *	FastCGI example program using fcgi_stdio library
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *
 * sample-store is a program designed to illustrate one technique
 * for writing a high-performance FastCGI application that maintains
 * permanent state.  It is real enough to demonstrate a range of issues
 * that can arise in FastCGI application programming.
 *
 * sample-store implements per-user shopping carts.  These carts are kept
 * in memory for speed but are backed up on disk for reliability; the
 * program can restart at any time, affecting at most one request.  Unlike
 * CGI applications, the performance impact of sample-store's disk
 * use is negligible: no I/O for query requests, no reads and one write
 * for a typical update request.
 *
 * sample-store's on-disk representation is extremely simple.  The
 * current state of all shopping carts managed by a process is kept
 * in two files, a snapshot and a log.  Both files have the same format,
 * a sequence of ASCII records.  On restart the current state is restored
 * by replaying the snapshot and the log.  When the log grows to a certain
 * length, sample-store writes a new snapshot and empties the log.
 * This prevents the time needed for restart from growing without
 * bound.
 * 
 * Since users "visit" Web sites, but never "leave", sample-store
 * deletes a shopping cart after the cart has been inactive
 * for a certain period of time.  This policy prevents sample-store's
 * memory requirements from growing without bound.
 *
 * sample-store operates both as a FastCGI Responder and as an
 * Authorizer, showing how one program can play two roles.
 *
 * The techniques used in sample-store are not specific to shopping
 * carts; they apply equally well to maintaining all sorts of
 * information.
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: sample-store.c,v 1.3 1999/07/26 05:33:00 roberts Exp $";
#endif /* not lint */

#include "fcgi_stdio.h"  /* FCGI_Accept, FCGI_Finish, stdio */
#include <stdlib.h>      /* malloc/free, getenv, strtol */
#include <string.h>      /* strcmp, strncmp, strlen, strstr, strchr */
#include <tcl.h>         /* Tcl_*Hash* functions */
#include <time.h>        /* time, time_t */
#include <assert.h>      /* assert */
#include <errno.h>       /* errno, ENOENT */
#include <dirent.h>      /* readdir, closedir, DIR, dirent */
#include <unistd.h>      /* fsync */

#if defined __linux__
int fsync(int fd);
#endif

/*
 * sample-store is designed to be configured as follows (for the OM server):
 *
 * SI_Department SampleStoreDept -EnableAnonymousTicketing 1
 * Region /SampleStore/ *  { SI_RequireSI SampleStoreDept 1 }
 *
 * Filemap /SampleStore $fcgi-devel-kit/examples/SampleStore
 * AppClass  SampleStoreAppClass \
 *     $fcgi-devel-kit/examples/sample-store \
 *     -initial-env STATE_DIR=$fcgi-devel-kit/examples/SampleStore.state \
 *     -initial-env CKP_THRESHOLD=100 \
 *     -initial-env CART_HOLD_MINUTES=240 \
 *     -processes 2 -affinity
 * Responder SampleStoreAppClass /SampleStore/App
 * AuthorizeRegion /SampleStore/Protected/ *  SampleStoreAppClass
 *
 * sample-store looks for three initial environment variables:
 *
 *  STATE_DIR
 *    When sample-store is run as a single process without affinity
 *    this is the directory containing the permanent state of the
 *    process.  When sample-store is run as multiple processes
 *    using session affinity, the state directory is
 *    $STATE_DIR.$FCGI_PROCESS_ID, e.g. SampleStore.state.0
 *    and SampleStore.state.1 in the config above.  The process
 *    state directory must exist, but may be empty.
 *
 *  CKP_THRESHOLD
 *    When the log grows to contain this many records the process
 *    writes a new snapshot and truncates the log.  Defaults
 *    to CKP_THRESHOLD_DEFAULT.
 *
 *  CART_HOLD_MINUTES
 *    When a cart has not been accessed for this many minutes it
 *    may be deleted.  Defaults to CART_HOLD_MINUTES_DEFAULT.
 *
 * The program is prepared to run as multiple processes using
 * session affinity (illustrated in config above) or as a single process.
 *
 * The program does not depend upon the specific URL prefix /SampleStore.
 *
 */

/*
 * This code is organized top-down, trying to put the most interesting
 * parts first.  Unfortunately, organizing the program in this way requires
 * lots of extra declarations to take care of forward references.
 *
 * Utility functions for string/list processing and such
 * are left to the very end.  The program uses the Tcl hash table
 * package because it is both adequate and readily available.
 */

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define Strlen(str) (((str) == NULL) ? 0 : strlen(str))

void panic(char *format,
        char *arg1, char *arg2, char *arg3, char *arg4,
        char *arg5, char *arg6, char *arg7, char *arg8);

static void *Malloc(size_t size);
static void Free(void *ptr);
static char *StringNCopy(char *str, int strLen);
static char *StringCopy(char *str);
static char *StringCat(char *str1, char *str2);
static char *StringCat4(char *str1, char *str2, char *str3, char *str4);
static char *QueryLookup(char *query, char *name);
static char *PathTail(char *path);

typedef struct ListOfString {
    char *head;
    struct ListOfString *tail;
} ListOfString;
static char *ListOfString_Head(ListOfString *list);
static ListOfString *ListOfString_Tail(ListOfString *list);
static int ListOfString_Length(ListOfString *list);
static int ListOfString_IsElement(ListOfString *list, char *element);
static ListOfString *ListOfString_AppendElement(
        ListOfString *list, char *element);
static ListOfString *ListOfString_RemoveElement(
        ListOfString *list, char *element);

static int IntGetEnv(char *varName, int defaultValue);

static void Initialize(void);
static void PerformRequest(void);
static void GarbageCollectStep(void);
static void ConditionalCheckpoint(void);

/*
 * A typical FastCGI main program: Initialize, then loop
 * calling FCGI_Accept and performing the accepted request.
 * Do cleanup operations incrementally between requests.
 */
int main(void)
{
    Initialize();

    while (FCGI_Accept() >= 0) {
        PerformRequest();
        FCGI_Finish();
        GarbageCollectStep();
        ConditionalCheckpoint();
    }

    return 0;
}

/*
 * All the global variables
 */
typedef struct CartObj {
    int inactive;                   /* This cart not accessed since mark      */
    ListOfString *items;            /* Items in cart                          */
} CartObj;
static Tcl_HashTable *cartTablePtr; /* Table of CartObj, indexed by userId    */
static Tcl_HashTable cartTable;
static char *fcgiProcessId;         /* Id of this process in affinity group   */
static char *stateDir;              /* Path to dir with snapshot and log      */
char *snapshotPath, *logPath;       /* Paths to current snapshot and log      */
static int generation;              /* Number embedded in paths, inc on ckp   */
static FILE *logFile = NULL;        /* Open for append to current log file    */
static int numLogRecords;           /* Number of records in current log file  */
static int checkpointThreshold;     /* Do ckp when numLogRecords exceeds this */
static int purge = TRUE;            /* Cart collector is removing inactives   */
static time_t timeCartsMarked;      /* Time all carts marked inactive         */
static int cartHoldSeconds;         /* Begin purge when this interval elapsed */

#define STATE_DIR_VAR             "STATE_DIR"
#define PID_VAR                   "FCGI_PROCESS_ID"
#define CKP_THRESHOLD_VAR         "CKP_THRESHOLD"
#define CKP_THRESHOLD_DEFAULT     200
#define CART_HOLD_MINUTES_VAR     "CART_HOLD_MINUTES"
#define CART_HOLD_MINUTES_DEFAULT 300

#define SNP_PREFIX    "snapshot"
#define LOG_PREFIX    "log"
#define TMP_SNP_NAME  "tmp-snapshot"

#define LR_ADD_ITEM    "Add"
#define LR_REMOVE_ITEM "Rem"
#define LR_EMPTY_CART  "Emp"


static char *MakePath(char *dir, char *prefix, int gen);
static void AnalyzeStateDir(
    char *dir, char *prefix, int *largestP, ListOfString **fileListP);
static int RecoverFile(char *pathname);
static void Checkpoint(void);

/*
 * Initialize the process by reading environment variables and files
 */
static void Initialize(void)
{
    ListOfString *fileList;
    int stateDirLen;
    /*
     * Process miscellaneous parameters from the initial environment.
     */
    checkpointThreshold =
            IntGetEnv(CKP_THRESHOLD_VAR, CKP_THRESHOLD_DEFAULT);
    cartHoldSeconds =
            IntGetEnv(CART_HOLD_MINUTES_VAR, CART_HOLD_MINUTES_DEFAULT)*60;
    /*
     * Create an empty in-memory shopping cart data structure.
     */
    cartTablePtr = &cartTable;
    Tcl_InitHashTable(cartTablePtr, TCL_STRING_KEYS);
    /*
     * Compute the state directory name from the initial environment
     * variables.
     */
    stateDir = getenv(STATE_DIR_VAR);
    stateDirLen = Strlen(stateDir);
    assert(stateDirLen > 0);
    if(stateDir[stateDirLen - 1] == '/') {
        stateDir[stateDirLen - 1] = '\000';
    }
    fcgiProcessId = getenv(PID_VAR);
    if(fcgiProcessId != NULL) {
        stateDir = StringCat4(stateDir, ".", fcgiProcessId, "/");
    } else {
        stateDir = StringCat(stateDir, "/");
    }
    /*
     * Read the state directory to determine the current
     * generation number and a list of files that may
     * need to be deleted (perhaps left over from an earlier
     * system crash).  Recover the current generation
     * snapshot and log (either or both may be missing),
     * populating the in-memory shopping cart data structure.
     * Take a checkpoint, making the current log empty.
     */
    AnalyzeStateDir(stateDir, SNP_PREFIX, &generation, &fileList);
    snapshotPath = MakePath(stateDir, SNP_PREFIX, generation);
    RecoverFile(snapshotPath);
    logPath = MakePath(stateDir, LOG_PREFIX, generation);
    numLogRecords = RecoverFile(logPath);
    Checkpoint();
    /*
     * Clean up stateDir without removing the current snapshot and log.
     */
    while(fileList != NULL) {
        char *cur = ListOfString_Head(fileList);
        if(strcmp(snapshotPath, cur) && strcmp(logPath, cur)) {
            remove(cur);
        }
        fileList = ListOfString_RemoveElement(fileList, cur);
    }
}

static char *MakePath(char *dir, char *prefix, int gen)
{
    char nameBuffer[24];
    sprintf(nameBuffer, "%s.%d", prefix, gen);
    return  StringCat(dir, nameBuffer);
}

static void ConditionalCheckpoint(void)
{
    if(numLogRecords >= checkpointThreshold) {
        Checkpoint();
    }
}
static void WriteSnapshot(char *snpPath);

static void Checkpoint(void)
{
    char *tempSnapshotPath, *newLogPath, *newSnapshotPath;
    /*
     * Close the current log file.
     */
    if(logFile != NULL) {
        fclose(logFile);
    }
    /*
     * Create a new snapshot with a temporary name.
     */
    tempSnapshotPath = StringCat(stateDir, TMP_SNP_NAME);
    WriteSnapshot(tempSnapshotPath);
    ++generation;
    /*
     * Ensure that the new log file doesn't already exist by removing it.
     */
    newLogPath = MakePath(stateDir, LOG_PREFIX, generation);
    remove(newLogPath);
    /*
     * Commit by renaming the snapshot.  The rename atomically
     * makes the old snapshot and log obsolete.
     */
    newSnapshotPath = MakePath(stateDir, SNP_PREFIX, generation);
    rename(tempSnapshotPath, newSnapshotPath);
    /*
     * Clean up the old snapshot and log.
     */
    Free(tempSnapshotPath);
    remove(snapshotPath);
    Free(snapshotPath);
    snapshotPath = newSnapshotPath;
    remove(logPath);
    Free(logPath);
    logPath = newLogPath;
    /*
     * Open the new, empty log.
     */
    logFile = fopen(logPath, "a");
    numLogRecords = 0;
}

/*
 * Return *largestP     = the largest int N such that the name prefix.N
 *                        is in the directory dir.  0 if no such name
 *        *fileListP    = list of all files in the directory dir,
 *                        excluding '.' and '..'
 */
static void AnalyzeStateDir(
    char *dir, char *prefix, int *largestP, ListOfString **fileListP)
{
    DIR *dp;
    struct dirent *dirp;
    int prefixLen = strlen(prefix);
    int largest = 0;
    int cur;
    char *curName;
    ListOfString *fileList = NULL;
    dp = opendir(dir);
    assert(dp != NULL);
    while((dirp = readdir(dp)) != NULL) {
        if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) {
            continue;
	}
        curName = StringCat(dir, dirp->d_name);
        fileList = ListOfString_AppendElement(fileList, curName);
        if(!strncmp(dirp->d_name, prefix, prefixLen)
                && (dirp->d_name)[prefixLen] == '.') {
            cur = strtol(dirp->d_name + prefixLen + 1, NULL, 10);
            if(cur > largest) {
                largest = cur;
	    }
	}
    }
    assert(closedir(dp) >= 0);
    *largestP = largest;
    *fileListP = fileList;
}

static int DoAddItemToCart(char *userId, char *item, int writeLog);
static int DoRemoveItemFromCart(char *userId, char *item, int writeLog);
static int DoEmptyCart(char *userId, int writeLog);

/*
 * Read either a snapshot or a log and perform the specified
 * actions on the in-memory representation.
 */
static int RecoverFile(char *pathname)
{
    int numRecords;
    FILE *recoveryFile = fopen(pathname, "r");
    if(recoveryFile == NULL) {
        assert(errno == ENOENT);
        return 0;
    }
    for(numRecords = 0; ; numRecords++) {
        char buff[128];
        char op[32], userId[32], item[64];
        int count;
        char *status = fgets(buff, sizeof(buff), recoveryFile);
        if(status == NULL) {
            assert(feof(recoveryFile));
            fclose(recoveryFile);
            return numRecords;
	}
        count = sscanf(buff, "%31s %31s %63s", op, userId, item);
        assert(count == 3);
        if(!strcmp(op, LR_ADD_ITEM)) {
            assert(DoAddItemToCart(userId, item, FALSE) >= 0);
        } else if(!strcmp(op, LR_REMOVE_ITEM)) {
            assert(DoRemoveItemFromCart(userId, item, FALSE) >= 0);
        } else if(!strcmp(op, LR_EMPTY_CART)) {
            assert(DoEmptyCart(userId, FALSE) >= 0);
        } else {
            assert(FALSE);
        }
    }
}

static void WriteLog(char *command, char *userId, char *item, int force);

/*
 * Read the in-memory representation and write a snapshot file
 * that captures it.
 */
static void WriteSnapshot(char *snpPath)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *cartEntry;
    ListOfString *items;
    char *userId;
    logFile = fopen(snpPath, "w");
    assert(logFile != NULL);
    cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
    for(cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
            cartEntry != NULL; cartEntry = Tcl_NextHashEntry(&search)) {
        userId = Tcl_GetHashKey(cartTablePtr, cartEntry);
        for(items = ((CartObj *) Tcl_GetHashValue(cartEntry))->items;
                items != NULL; items = ListOfString_Tail(items)) {
            WriteLog(LR_ADD_ITEM, userId, ListOfString_Head(items), FALSE);
	}
    }
    fflush(logFile);
    fsync(fileno(logFile));
    fclose(logFile);
}

static void WriteLog(char *command, char *userId, char *item, int force)
{
    fprintf(logFile, "%s %s %s\n", command, userId, item);
    ++numLogRecords;
    if(force) {
        fflush(logFile);
        fsync(fileno(logFile));
    }
}

static int RemoveOneInactiveCart(void);
static void MarkAllCartsInactive(void);

/*
 * Incremental garbage collection of inactive shopping carts:
 *
 * Each user access to a shopping cart clears its "inactive" bit via a
 * call to MarkThisCartActive.  When restart creates a cart it
 * also marks the cart active.
 *
 * If purge == TRUE, each call to GarbageCollectStep scans for and removes
 * the first inactive cart found.  If there are no inactive carts,
 * GarbageCollectStep marks *all* carts inactive, records the time in
 * timeCartsMarked, and sets purge = FALSE.
 *
 * If purge == FALSE, each call to GarbageCollectStep checks the
 * elapsed time since timeCartsMarked.  If the elapsed time
 * exceeds a threshold, GarbageCollectStep sets purge = TRUE.
 */

static void GarbageCollectStep(void)
{
    if(purge) {
        if(!RemoveOneInactiveCart()) {
            MarkAllCartsInactive();
            timeCartsMarked = time(NULL);
            purge = FALSE;
	}
    } else {
        int diff = time(NULL)-timeCartsMarked;
        if(diff > cartHoldSeconds) {
            purge = TRUE;
	}
    }
}

static int RemoveOneInactiveCart(void)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *cartEntry;
    CartObj *cart;
    char *userId;
    cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
    for(cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
            cartEntry != NULL; cartEntry = Tcl_NextHashEntry(&search)) {
        cart = Tcl_GetHashValue(cartEntry);
        if(cart->inactive) {
            userId = Tcl_GetHashKey(cartTablePtr, cartEntry);
            DoEmptyCart(userId, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

static Tcl_HashEntry *GetCartEntry(char *userId);

static void MarkAllCartsInactive(void)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *cartEntry;
    CartObj *cart;
    cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
    for(cartEntry = Tcl_FirstHashEntry(cartTablePtr, &search);
            cartEntry != NULL; cartEntry = Tcl_NextHashEntry(&search)) {
        cart = Tcl_GetHashValue(cartEntry);
        cart->inactive = TRUE;
    }
}

static void MarkThisCartActive(char *userId)
{
    Tcl_HashEntry *cartEntry = GetCartEntry(userId);
    CartObj *cart = Tcl_GetHashValue(cartEntry);
    cart->inactive = FALSE;
}

#define OP_DISPLAY_STORE "DisplayStore"
#define OP_ADD_ITEM      "AddItemToCart"
#define OP_DISPLAY_CART  "DisplayCart"
#define OP_REMOVE_ITEM   "RemoveItemFromCart"
#define OP_PURCHASE      "Purchase"

static void DisplayStore(
        char *scriptName, char *parent, char *userId, char *processId);
static void AddItemToCart(
        char *scriptName, char *parent, char *userId, char *processId,
        char *item);
static void DisplayCart(
        char *scriptName, char *parent, char *userId, char *processId);
static void RemoveItemFromCart(
        char *scriptName, char *parent, char *userId, char *processId,
        char *item);
static void Purchase(
        char *scriptName, char *parent, char *userId, char *processId);
static void InvalidRequest(char *code, char *message);
static void Authorize(char *userId);

/*
 * As a Responder, this application expects to be called with the
 * GET method and a URL of the form
 *
 *     http://<host-port>/<script-name>?op=<op>&item=<item>
 *
 * The application expects the SI_UID variable to provide
 * a user ID, either authenticated or anonymous.
 *
 * The application expects the directory *containing* <script-name>
 * to contain various static HTML files related to the application.
 *
 * As an Authorizer, the application expects to be called with
 * SID_UID and URL_PATH set.
 */

static void PerformRequest(void)
{
    char *method = getenv("REQUEST_METHOD");
    char *role = getenv("FCGI_ROLE");
    char *scriptName = PathTail(getenv("SCRIPT_NAME"));
    char *parent = "";
    char *op = QueryLookup(getenv("QUERY_STRING"), "op");
    char *item = QueryLookup(getenv("QUERY_STRING"), "item");
    char *userId =  getenv("SI_UID");
    if(userId == NULL) {
        InvalidRequest("405", "Incorrect configuration, no user id");
        goto done;
    } else {
        MarkThisCartActive(userId);
    }
    if(!strcmp(role, "RESPONDER")) {
        if(strcmp(method, "GET")) {
            InvalidRequest("405", "Only GET Method Allowed");
        } else if(op == NULL || !strcmp(op, OP_DISPLAY_STORE)) {
            DisplayStore(scriptName, parent, userId, fcgiProcessId);
	} else if(!strcmp(op, OP_ADD_ITEM)) {
            AddItemToCart(scriptName, parent, userId, fcgiProcessId, item);
	} else if(!strcmp(op, OP_DISPLAY_CART)) {
            DisplayCart(scriptName, parent, userId, fcgiProcessId);
	} else if(!strcmp(op, OP_REMOVE_ITEM)) {
            RemoveItemFromCart(scriptName, parent, userId, fcgiProcessId, item);
	} else if(!strcmp(op, OP_PURCHASE)) {
            Purchase(scriptName, parent, userId, fcgiProcessId);
	} else {
            InvalidRequest("404", "Invalid 'op' argument");
 	}
    } else if(!strcmp(role, "AUTHORIZER")) {
        Authorize(userId);
    } else {
        InvalidRequest("404", "Invalid FastCGI Role");
    }
  done:
    Free(scriptName);
    Free(op);
    Free(item);
}

/*
 * Tiny database of shop inventory.  The first form is the
 * item identifier used in a request, the second form is used
 * for HTML display.  REQUIRED_ITEM is the item required
 * the the Authorizer.  SPECIAL_ITEM is the item on the protected
 * page (must follow unprotected items in table).
 */

char *ItemNames[] = {
        "BrooklynBridge",
        "RMSTitanic",
        "CometKohoutec",
        "YellowSubmarine",
        NULL
        };
char *ItemDisplayNames[] = {
        "<i>Brooklyn Bridge</i>",
        "<i>RMS Titanic</i>",
        "<i>Comet Kohoutec</i>",
        "<i>Yellow Submarine</i>",
        NULL
        };
#define REQUIRED_ITEM 1
#define SPECIAL_ITEM 3


static char *ItemDisplayName(char *item)
{
    int i;
    if(item == NULL) {
        return NULL;
    }
    for(i = 0; ItemNames[i] != NULL; i++) {
        if(!strcmp(item, ItemNames[i])) {
            return ItemDisplayNames[i];
	}
    }
    return NULL;
}

static void DisplayNumberOfItems(int numberOfItems, char *processId);

static void DisplayHead(char *title, char *parent, char *gif)
{
    printf("Content-type: text/html\r\n"
           "\r\n"
           "<html>\n<head>\n<title>%s</title>\n</head>\n\n"
           "<body bgcolor=\"ffffff\" text=\"000000\" link=\"39848c\"\n"
           "      vlink=\"808080\" alink=\"000000\">\n", title);
    if(parent != NULL && gif != NULL) {
        printf("<center>\n<img src=\"%s%s\" alt=\"[%s]\">\n</center>\n\n",
               parent, gif, title);
    } else {
        printf("<h2>%s</h2>\n<hr>\n\n", title);
    }
}

static void DisplayFoot(void)
{
    printf("<hr>\n</body>\n</html>\n");
}

static void DisplayStore(
        char *scriptName, char *parent, char *userId, char *processId)
{
    Tcl_HashEntry *cartEntry = GetCartEntry(userId);
    ListOfString *items = ((CartObj *) Tcl_GetHashValue(cartEntry))->items;
    int numberOfItems = ListOfString_Length(items);
    int i;

    DisplayHead("FastCGI Shop!", parent, "Images/main-hd.gif");
    DisplayNumberOfItems(numberOfItems, processId);
    printf("<h3>Goods for sale:</h3>\n<ul>\n");
    for(i = 0; i < SPECIAL_ITEM; i++) {
        printf("  <li>Add the <a href=\"%s?op=AddItemToCart&item=%s\">%s</a>\n"
               "      to your shopping cart.\n",
               scriptName, ItemNames[i], ItemDisplayNames[i]);
    }
    printf("</ul><p>\n\n");
    printf("If the %s is in your shopping cart,\n"
           "<a href=\"%sProtected/%s.html\">go see a special offer</a>\n"
           "available only to %s purchasers.<p>\n\n",
           ItemDisplayNames[REQUIRED_ITEM], parent,
           ItemNames[REQUIRED_ITEM], ItemDisplayNames[REQUIRED_ITEM]);
    printf("<a href=\"%sUnprotected/Purchase.html\">Purchase\n"
           "the contents of your shopping cart.</a><p>\n\n", parent);
    printf("<a href=\"%s?op=DisplayCart\">View the contents\n"
           "of your shopping cart.</a><p>\n\n", scriptName);
    DisplayFoot();
}

static Tcl_HashEntry *GetCartEntry(char *userId)
{
    Tcl_HashEntry *cartEntry = Tcl_FindHashEntry(cartTablePtr, userId);
    int new;
    if(cartEntry == NULL) {
        CartObj *cart = Malloc(sizeof(CartObj));
        cart->inactive = FALSE;
        cart->items = NULL;
        cartEntry = Tcl_CreateHashEntry(cartTablePtr, userId, &new);
        assert(new);
        Tcl_SetHashValue(cartEntry, cart);
    }
    return cartEntry;
}

static void AddItemToCart(
        char *scriptName, char *parent, char *userId, char *processId,
        char *item)
{
    if(DoAddItemToCart(userId, item, TRUE) < 0) {
        InvalidRequest("404", "Invalid 'item' argument");
    } else {
        /*
         * Would call
         *   DisplayStore(scriptName, parent, userId, processId);
         * except for browser reload issue.  Redirect instead.
         */
        printf("Location: %s?op=%s\r\n"
               "\r\n", scriptName, OP_DISPLAY_STORE);
    }
}  

static int DoAddItemToCart(char *userId, char *item, int writeLog)
{
    if(ItemDisplayName(item) == NULL) {
        return -1;
    } else {
        Tcl_HashEntry *cartEntry = GetCartEntry(userId);
        CartObj *cart = Tcl_GetHashValue(cartEntry);
        cart->items = ListOfString_AppendElement(
                              cart->items, StringCopy(item));
        if(writeLog) {
            WriteLog(LR_ADD_ITEM, userId, item, TRUE);
	}
    }
    return 0;
}

static void DisplayCart(
        char *scriptName, char *parent, char *userId, char *processId)
{
    Tcl_HashEntry *cartEntry = GetCartEntry(userId);
    CartObj *cart = Tcl_GetHashValue(cartEntry);
    ListOfString *items = cart->items;
    int numberOfItems = ListOfString_Length(items);

    DisplayHead("Your shopping cart", parent, "Images/cart-hd.gif");
    DisplayNumberOfItems(numberOfItems, processId);
    printf("<ul>\n");
    for(; items != NULL; items = ListOfString_Tail(items)) {
        char *item = ListOfString_Head(items);
        printf("  <li>%s . . . . . \n"
               "    <a href=\"%s?op=RemoveItemFromCart&item=%s\">Click\n"
               "    to remove</a> from your shopping cart.\n",
               ItemDisplayName(item), scriptName, item);
    }
    printf("</ul><p>\n\n");
    printf("<a href=\"%sUnprotected/Purchase.html\">Purchase\n"
           "the contents of your shopping cart.</a><p>\n\n", parent);
    printf("<a href=\"%s?op=DisplayStore\">Return to shop.</a><p>\n\n",
           scriptName);
    DisplayFoot();
}

static void RemoveItemFromCart(
        char *scriptName, char *parent, char *userId, char *processId,
        char *item)
{
    if(DoRemoveItemFromCart(userId, item, TRUE) < 0) {
        InvalidRequest("404", "Invalid 'item' argument");
    } else {
        /*
         * Would call
         *   DisplayCart(scriptName, parent, userId, processId);
         * except for browser reload issue.  Redirect instead.
         */
        printf("Location: %s?op=%s\r\n"
               "\r\n", scriptName, OP_DISPLAY_CART);
    }
}

static int DoRemoveItemFromCart(char *userId, char *item, int writeLog)
{
    if(ItemDisplayName(item) == NULL) {
        return -1;
    } else {
        Tcl_HashEntry *cartEntry = GetCartEntry(userId);
        CartObj *cart = Tcl_GetHashValue(cartEntry);
        if(ListOfString_IsElement(cart->items, item)) {
            cart->items = ListOfString_RemoveElement(cart->items, item);
            if (writeLog) {
                WriteLog(LR_REMOVE_ITEM, userId, item, TRUE);
	    }
        }
    }
    return 0;
}

static void Purchase(
        char *scriptName, char *parent, char *userId, char *processId)
{
    DoEmptyCart(userId, TRUE);
    printf("Location: %sUnprotected/ThankYou.html\r\n"
           "\r\n", parent);
}

static int DoEmptyCart(char *userId, int writeLog)
{
    Tcl_HashEntry *cartEntry = GetCartEntry(userId);
    CartObj *cart = Tcl_GetHashValue(cartEntry);
    ListOfString *items = cart->items;
    /*
     * Write log *before* tearing down cart structure because userId
     * is part of the structure.  (Thanks, Purify.)
     */
    if (writeLog) {
        WriteLog(LR_EMPTY_CART, userId, "NullItem", TRUE);
    }
    while(items != NULL) {
        items = ListOfString_RemoveElement(
                items, ListOfString_Head(items));
    }
    Free(cart);
    Tcl_DeleteHashEntry(cartEntry);
    return 0;
}

static void NotAuthorized(void);

static void Authorize(char *userId)
{
    Tcl_HashEntry *cartEntry = GetCartEntry(userId);
    ListOfString *items = ((CartObj *) Tcl_GetHashValue(cartEntry))->items;
    for( ; items != NULL; items = ListOfString_Tail(items)) {
        if(!strcmp(ListOfString_Head(items), ItemNames[REQUIRED_ITEM])) {
            printf("Status: 200 OK\r\n"
                   "Variable-Foo: Bar\r\n"
                   "\r\n");
           return;
	}
    }
    NotAuthorized();
}

static void DisplayNumberOfItems(int numberOfItems, char *processId)
{
    if(processId != NULL) {
        printf("FastCGI process %s is serving you today.<br>\n", processId);
    }
    if(numberOfItems == 0) {
        printf("Your shopping cart is empty.<p>\n\n");
    } else if(numberOfItems == 1) {
        printf("Your shopping cart contains 1 item.<p>\n\n");
    } else {
        printf("Your shopping cart contains %d items.<p>\n\n", numberOfItems);
    };
}

static void InvalidRequest(char *code, char *message)
{
    printf("Status: %s %s\r\n", code, message);
    DisplayHead("Invalid request", NULL, NULL);
    printf("%s.\n\n", message);
    DisplayFoot();
}

static void NotAuthorized(void)
{
    printf("Status: 403 Forbidden\r\n");
    DisplayHead("Access Denied", NULL, NULL);
    printf("Put the %s in your cart to access this page.\n\n",
           ItemDisplayNames[REQUIRED_ITEM]);
    DisplayFoot();
}

/*
 * Mundane utility functions, not specific to this application:
 */


/*
 * Fail-fast version of 'malloc'
 */
static void *Malloc(size_t size)
{
    void *result = malloc(size);
    assert(size == 0 || result != NULL);
    return result;
}

/*
 * Protect against old, broken implementations of 'free'
 */
static void Free(void *ptr)
{
    if(ptr != NULL) {
        free(ptr);
      }
}

/*
 * Return a new string created by calling Malloc, copying strLen
 * characters from str to the new string, then appending a null.
 */
static char *StringNCopy(char *str, int strLen)
{
    char *newString = Malloc(strLen + 1);
    memcpy(newString, str, strLen);
    newString[strLen] = '\000';
    return newString;
}

/*
 * Return a new string that's a copy of str, including the null
 */
static char *StringCopy(char *str)
{
    return StringNCopy(str, strlen(str));
}

/*
 * Return a new string that's a copy of str1 followed by str2,
 * including the null
 */
static char *StringCat(char *str1, char *str2)
{
    return StringCat4(str1, str2, NULL, NULL);
}

static char *StringCat4(char *str1, char *str2, char *str3, char *str4)
{
    int str1Len = Strlen(str1);
    int str2Len = Strlen(str2);
    int str3Len = Strlen(str3);
    int str4Len = Strlen(str4);
    char *newString = Malloc(str1Len + str2Len + str3Len + str4Len + 1);
    memcpy(newString, str1, str1Len);
    memcpy(newString + str1Len, str2, str2Len);
    memcpy(newString + str1Len + str2Len, str3, str3Len);
    memcpy(newString + str1Len + str2Len + str3Len, str4, str4Len);
    newString[str1Len + str2Len + str3Len + str4Len] = '\000';
    return newString;
}

/*
 * Return a copy of the value associated with 'name' in 'query'.
 * XXX: does not perform URL-decoding of query.
 */
static char *QueryLookup(char *query, char *name)
{
    int nameLen = strlen(name);
    char *queryTail, *nameFirst, *valueFirst, *valueLast;

    if(query == NULL) {
        return NULL;
    }
    queryTail = query;
    for(;;) {
        nameFirst = strstr(queryTail, name);
        if(nameFirst == NULL) {
            return NULL;
        }
        if(((nameFirst == query) || (nameFirst[-1] == '&')) &&
                (nameFirst[nameLen] == '=')) {
            valueFirst = nameFirst + nameLen + 1;
            valueLast = strchr(valueFirst, '&');
            if(valueLast == NULL) {
                valueLast = strchr(valueFirst, '\000');
	    };
            return StringNCopy(valueFirst, valueLast - valueFirst);
        }
        queryTail = nameFirst + 1;
    }
}

/*
 * Return a copy of the characters following the final '/' character
 * of path.
 */
static char *PathTail(char *path)
{
    char *afterSlash, *slash;
    if(path == NULL) {
        return NULL;
    }
    afterSlash = path;
    while((slash = strchr(afterSlash, '/')) != NULL) {
        afterSlash = slash + 1;
    }
    return StringCopy(afterSlash);
}

/*
 * Return the integer value of the specified environment variable,
 * or a specified default value if the variable is unbound.
 */
static int IntGetEnv(char *varName, int defaultValue)
{
    char *strValue = getenv(varName);
    int value = 0;
    if(strValue != NULL) {
        value = strtol(strValue, NULL, 10);
    }
    if(value <= 0) {
        value = defaultValue;
    }
    return value;
}

/*
 * Should the Tcl hash package detect an unrecoverable error(!), halt.
 */
void panic(char *format,
        char *arg1, char *arg2, char *arg3, char *arg4,
        char *arg5, char *arg6, char *arg7, char *arg8)
{
    assert(FALSE);
}
   

/*
 * ListOfString abstraction
 */

static char *ListOfString_Head(ListOfString *list)
{
    return list->head;
}

static ListOfString *ListOfString_Tail(ListOfString *list)
{
    return list->tail;
}

static int ListOfString_Length(ListOfString *list)
{
    int length = 0;
    for(; list != NULL; list = list->tail) {
        length++;
    }
    return length;
}

static int ListOfString_IsElement(ListOfString *list, char *element)
{
    for(; list != NULL; list = list->tail) {
        if(!strcmp(list->head, element)) {
            return TRUE;
	}
    }
    return FALSE;
}

static ListOfString *ListOfString_AppendElement(
        ListOfString *list, char *element)
{
    ListOfString *cur;
    ListOfString *newCell = Malloc(sizeof(ListOfString));
    newCell->head = element;
    newCell->tail = NULL;
    if(list == NULL) {
        return newCell;
    } else {
        for(cur = list; cur->tail != NULL; cur = cur->tail) {
	}
        cur->tail = newCell;
        return list;
    }
}

static ListOfString *ListOfString_RemoveElement(
        ListOfString *list, char *element)
{
    ListOfString *cur;
    ListOfString *prevCell = NULL;
    for(cur = list; cur != NULL; cur = cur->tail) {
        if(!strcmp(cur->head, element)) {
            if(prevCell == NULL) {
                list = cur->tail;
	    } else {
                prevCell->tail = cur->tail;
	    }
            free(cur->head);
            free(cur);
            return list;
	}
        prevCell = cur;
    }
    return list;
}


/*
 * End
 */
