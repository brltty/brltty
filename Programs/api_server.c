/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* api_server.c : Main file for BrlApi server */

#include "prologue.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <wchar.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

#ifdef WINDOWS
#include "sys_windows.h"

#ifdef __MINGW32__
#include "win_pthread.h"
#endif /* __MINGW32__ */
#else /* WINDOWS */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* WINDOWS */

#include "api.h"
#include "api_protocol.h"
#include "keyrangelist.h"
#include "cmd.h"
#include "brl.h"
#include "brltty.h"
#include "misc.h"
#include "auth.h"
#include "io_misc.h"
#include "scr.h"
#include "tunes.h"
#include "charset.h"

#ifdef WINDOWS
#define LogSocketError(msg) LogWindowsSocketError(msg)
#else /* WINDOWS */
#define LogSocketError(msg) LogError(msg)
#endif /* WINDOWS */

#define UNAUTH_MAX 5
#define UNAUTH_DELAY 30

#define OUR_STACK_MIN 0X10000
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN OUR_STACK_MIN
#endif /* PTHREAD_STACK_MIN */

typedef enum {
  PARM_AUTH,
  PARM_HOST,
  PARM_STACKSIZE
} Parameters;

const char *const api_parameters[] = { "auth", "host", "stacksize", NULL };

static size_t stackSize;

#define RELEASE "BRLTTY API Library: release " BRLAPI_RELEASE
#define COPYRIGHT "   Copyright (C) 2002-2006 by Sebastien Hinderer <Sebastien.Hinderer@ens-lyon.org>, \
Samuel Thibault <samuel.thibault@ens-lyon.org>"

#define WERR(x, y, ...) do { \
  LogPrint(LOG_DEBUG, "writing error %d", y); \
  LogPrint(LOG_DEBUG, __VA_ARGS__); \
  writeError(x, y); \
} while(0)
#define WEXC(x, y, type, packet, size, ...) do { \
  LogPrint(LOG_DEBUG, "writing exception %d", y); \
  LogPrint(LOG_DEBUG, __VA_ARGS__); \
  writeException(x, y, type, packet, size); \
} while(0)

/* These CHECK* macros check whether a condition is true, and, if not, */
/* send back either a non-fatal error, or an exception */
#define CHECKERR(condition, error, msg) \
if (!( condition )) { \
  WERR(c->fd, error, "%s not met: " msg, #condition); \
  return 0; \
} else { }
#define CHECKEXC(condition, error, msg) \
if (!( condition )) { \
  WEXC(c->fd, error, type, packet, size, "%s not met: " msg, #condition); \
  return 0; \
} else { }

#ifdef brlapi_error
#undef brlapi_error
#endif

static brlapi_error_t brlapiserver_error;
#define brlapi_error brlapiserver_error

#define BRLAPI(fun) brlapiserver_ ## fun
#include "api_common.h"
 
/** ask for \e brltty commands */
#define BRL_COMMANDS 0
/** ask for raw driver keycodes */
#define BRL_KEYCODES 1

/****************************************************************************/
/** GLOBAL TYPES AND VARIABLES                                              */
/****************************************************************************/

extern char *opt_brailleParameters;
extern char *cfg_brailleParameters;

typedef struct {
  unsigned int cursor;
  wchar_t *text;
  unsigned char *andAttr;
  unsigned char *orAttr;
} BrailleWindow;

typedef enum { TODISPLAY, FULL, EMPTY } BrlBufState;

typedef enum {
#ifdef WINDOWS
  READY, /* but no pending ReadFile */
#endif /* WINDOWS */
  READING_HEADER,
  READING_CONTENT,
  DISCARDING
} PacketState;

typedef struct {
  header_t header;
  unsigned char content[BRLAPI_MAXPACKETSIZE+1]; /* +1 for additional \0 */
  PacketState state;
  int readBytes; /* Already read bytes */
  unsigned char *p; /* Where read() should load datas */
  int n; /* Value to give so read() */ 
#ifdef WINDOWS
  OVERLAPPED overl;
#endif /* WINDOWS */
} Packet;

typedef struct Connection {
  struct Connection *prev, *next;
  FileDescriptor fd;
  int auth;
  struct Tty *tty;
  int raw, suspend;
  unsigned int how; /* how keys must be delivered to clients */
  BrailleWindow brailleWindow;
  BrlBufState brlbufstate;
  pthread_mutex_t brlMutex;
  KeyrangeList *unmaskedKeys;
  pthread_mutex_t maskMutex;
  time_t upTime;
  Packet packet;
} Connection;

typedef struct Tty {
  int focus;
  int number;
  struct Connection *connections;
  struct Tty *father; /* father */
  struct Tty **prevnext,*next; /* siblings */
  struct Tty *subttys; /* children */
} Tty;

#define MAXSOCKETS 4 /* who knows what users want to do... */

/* Pointer to the connection accepter thread */
static pthread_t serverThread; /* server */
static pthread_t socketThreads[MAXSOCKETS]; /* socket binding threads */
static char **socketHosts; /* socket local hosts */
static struct socketInfo {
  int addrfamily;
  FileDescriptor fd;
  char *host;
  char *port;
#ifdef WINDOWS
  OVERLAPPED overl;
#endif /* WINDOWS */
} socketInfo[MAXSOCKETS]; /* information for cleaning sockets */
static int numSockets; /* number of sockets */

/* Protects from connection addition / remove from the server thread */
static pthread_mutex_t connectionsMutex;

/* Protects the real driver's functions */
static pthread_mutex_t driverMutex;

/* Which connection currently has raw mode */
static pthread_mutex_t rawMutex;
static Connection *rawConnection = NULL;
static Connection *suspendConnection = NULL;

/* mutex lock order is connectionsMutex first, then rawMutex, then (maskMutex
 * or brlMutex) then driverMutex */

static Tty notty;
static Tty ttys;

static unsigned int unauthConnections;
static unsigned int unauthConnLog = 0;

/*
 * API states are
 * - stopped: No thread is running (hence no connection allowed).
 *   started: The server thread is running, accepting connections.
 * - unlinked: TrueBraille == &noBraille: API has no control on the driver.
 *   linked: TrueBraille != &noBraille: API controls the driver.
 * - core suspended: The core asked to keep the device closed.
 *   core active: The core asked has opened the device.
 * - device closed: API keeps the device closed.
 *   device opened: API has really opened the device.
 *
 * Combinations can be:
 * - initial: API stopped, unlinked, core suspended and device closed.
 * - started: API started, unlinked, core suspended and device closed.
 * - normal: API started, linked, core active and device opened.
 * - core suspend: API started, linked, core suspended but device opened.
 *   (BrlAPI-only output).
 * - full suspend: API started, linked, core suspended and device closed.
 * - brltty control: API started, core active and device opened, but unlinked.
 *
 * Other states don't make sense, since
 * - api needs to be started before being linked,
 * - the device can't remain closed if core is active,
 * - the core must resume before unlinking api (so as to let the api re-open
 *   the driver if necessary)
 */

/* Pointer to subroutines of the real braille driver, &noBraille when API is
 * unlinked  */
static const BrailleDriver *trueBraille;
static BrailleDriver ApiBraille;

/* Identication of the REAL braille driver currently used */

/* The following variable contains the size of the braille display */
/* stored as a pair of _network_-formatted integers */
static uint32_t displayDimensions[2] = { 0, 0 };
static unsigned int displaySize = 0;

static BrailleDisplay *disp; /* Parameter to pass to braille drivers */
static RepeatState repeatState;

static int coreActive; /* Whether core is active */
static int driverOpened; /* Whether device is really opened, protected by driverMutex */
static pthread_mutex_t suspendMutex; /* Protects use of driverOpened state */

static const char *auth = BRLAPI_DEFAUTH;
static AuthDescriptor *authDescriptor;

static Connection *last_conn_write;
static int refresh;

#ifdef WINDOWS
static WSADATA wsadata;
#endif /* WINDOWS */

/****************************************************************************/
/** SOME PROTOTYPES                                                        **/
/****************************************************************************/

extern void processParameters(char ***values, const char *const *names, const char *description, char *optionParameters, char *configuredParameters, const char *environmentVariable);
static int initializeUnmaskedKeys(Connection *c);

/****************************************************************************/
/** DRIVER CAPABILITIES                                                    **/
/****************************************************************************/

/* Function : isRawCapable */
/* Returns !0 if the specified driver is raw capable, 0 if it is not. */
static int isRawCapable(const BrailleDriver *brl)
{
  return ((brl->readPacket!=NULL) && (brl->writePacket!=NULL) && (brl->reset!=NULL));
}

/* Function : isKeyCapable */
/* Returns !0 if driver can return specific keycodes, 0 if not. */
static int isKeyCapable(const BrailleDriver *brl)
{
  return ((brl->readKey!=NULL) && (brl->keyToCommand!=NULL));
}

/* Function : suspendDriver */
/* Close driver */
static void suspendDriver(BrailleDisplay *brl) {
  if (trueBraille == &noBraille) return; /* core unlinked api */
  LogPrint(LOG_DEBUG,"driver suspended");
  pthread_mutex_lock(&suspendMutex);
  driverOpened = 0;
  closeBrailleDriver();
  pthread_mutex_unlock(&suspendMutex);
}

/* Function : resumeDriver */
/* Re-open driver */
static int resumeDriver(BrailleDisplay *brl) {
  if (trueBraille == &noBraille) return 0; /* core unlinked api */
  pthread_mutex_lock(&suspendMutex);
  driverOpened = openBrailleDriver();
  if (driverOpened) LogPrint(LOG_DEBUG,"driver resumed");
  pthread_mutex_unlock(&suspendMutex);
  return driverOpened;
}

/****************************************************************************/
/** PACKET HANDLING                                                        **/
/****************************************************************************/

/* Function : writeAck */
/* Sends an acknowledgement on the given socket */
static inline void writeAck(FileDescriptor fd)
{
  brlapiserver_writePacket(fd,BRLPACKET_ACK,NULL,0);
}

/* Function : writeError */
/* Sends the given non-fatal error on the given socket */
static void writeError(FileDescriptor fd, unsigned int err)
{
  uint32_t code = htonl(err);
  LogPrint(LOG_DEBUG,"error %u on fd %"PRIFD, err, fd);
  brlapiserver_writePacket(fd,BRLPACKET_ERROR,&code,sizeof(code));
}

/* Function : writeException */
/* Sends the given error code on the given socket */
static void writeException(FileDescriptor fd, unsigned int err, brl_type_t type, const void *packet, size_t size)
{
  int hdrsize, esize;
  unsigned char epacket[BRLAPI_MAXPACKETSIZE];
  errorPacket_t * errorPacket = (errorPacket_t *) epacket;
  LogPrint(LOG_DEBUG,"exception %u for packet type %lu on fd %"PRIFD, err, (unsigned long)type, fd);
  hdrsize = sizeof(errorPacket->code)+sizeof(errorPacket->type);
  errorPacket->code = htonl(err);
  errorPacket->type = htonl(type);
  esize = MIN(size, BRLAPI_MAXPACKETSIZE-hdrsize);
  if ((packet!=NULL) && (size!=0)) memcpy(&errorPacket->packet, packet, esize);
  brlapiserver_writePacket(fd,BRLPACKET_EXCEPTION,epacket, hdrsize+esize);
}

static void writeKey(FileDescriptor fd, brl_keycode_t key) {
  uint32_t buf[2];
  buf[0] = htonl(key >> 32);
  buf[1] = htonl(key & 0xffffffff);
  LogPrint(LOG_DEBUG,"writing key %08"PRIx32" %08"PRIx32,buf[0],buf[1]);
  brlapiserver_writePacket(fd,BRLPACKET_KEY,&buf,sizeof(buf));
}

/* Function: resetPacket */
/* Resets a Packet structure */
void resetPacket(Packet *packet)
{
#ifdef WINDOWS
  packet->state = READY;
#else /* WINDOWS */
  packet->state = READING_HEADER;
#endif /* WINDOWS */
  packet->readBytes = 0;
  packet->p = (unsigned char *) &packet->header;
  packet->n = sizeof(packet->header);
#ifdef WINDOWS
  SetEvent(packet->overl.hEvent);
#endif /* WINDOWS */
}

/* Function: initializePacket */
/* Prepares a Packet structure */
/* returns 0 on success, -1 on failure */
int initializePacket(Packet *packet)
{
#ifdef WINDOWS
  memset(&packet->overl,0,sizeof(packet->overl));
  if (!(packet->overl.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL))) {
    LogWindowsError("CreateEvent for readPacket");
    return -1;
  }
#endif /* WINDOWS */
  resetPacket(packet);
  return 0;
}

/* Function : readPacket */
/* Reads a packet for the given connection */
/* Returns -2 on EOF, -1 on error, 0 if the reading is not complete, */
/* 1 if the packet has been read. */
int readPacket(Connection *c)
{
  Packet *packet = &c->packet;
#ifdef WINDOWS
  DWORD res;
  if (packet->state!=READY) {
    /* pending read */
    if (!GetOverlappedResult(c->fd,&packet->overl,&res,FALSE)) {
      switch (GetLastError()) {
        case ERROR_IO_PENDING: return 0;
        case ERROR_HANDLE_EOF:
        case ERROR_BROKEN_PIPE: return -2;
        default: LogWindowsError("GetOverlappedResult"); setSystemErrno(); return -1;
      }
    }
read:
#else /* WINDOWS */
  int res;
read:
  res = read(c->fd, packet->p, packet->n);
  if (res==-1) {
    switch (errno) {
      case EINTR: goto read;
      case EAGAIN: return 0;
      default: return -1;
    }
  }
#endif /* WINDOWS */
  if (res==0) return -2; /* EOF */
  packet->readBytes += res;
  if ((packet->state==READING_HEADER) && (packet->readBytes==HEADERSIZE)) {
    packet->header.size = ntohl(packet->header.size);
    packet->header.type = ntohl(packet->header.type);
    if (packet->header.size==0) goto out;
    packet->readBytes = 0;
    if (packet->header.size<=BRLAPI_MAXPACKETSIZE) {
      packet->state = READING_CONTENT;
      packet->n = packet->header.size;
    } else {
      packet->state = DISCARDING;
      packet->n = BRLAPI_MAXPACKETSIZE;
    }
    packet->p = packet->content;
  } else if ((packet->state == READING_CONTENT) && (packet->readBytes==packet->header.size)) goto out;
  else if (packet->state==DISCARDING) {
    packet->p = packet->content;
    packet->n = MIN(packet->header.size-packet->readBytes, BRLAPI_MAXPACKETSIZE);
  } else {
    packet->n -= res;
    packet->p += res;
  }
#ifdef WINDOWS
  } else packet->state = READING_HEADER;
  if (!ResetEvent(packet->overl.hEvent))
    LogWindowsError("ResetEvent in readPacket");
  if (!ReadFile(c->fd, packet->p, packet->n, &res, &packet->overl)) {
    switch (GetLastError()) {
      case ERROR_IO_PENDING: return 0;
      case ERROR_HANDLE_EOF:
      case ERROR_BROKEN_PIPE: return -2;
      default: LogWindowsError("ReadFile"); setSystemErrno(); return -1;
    }
  }
#endif /* WINDOWS */
  goto read;

out:
  resetPacket(packet);
  return 1;
}

typedef int(*PacketHandler)(Connection *, brl_type_t, unsigned char *, size_t);

typedef struct { /* packet handlers */
  PacketHandler getDriverId;
  PacketHandler getDriverName;
  PacketHandler getDisplaySize;
  PacketHandler enterTtyMode;
  PacketHandler setFocus;
  PacketHandler leaveTtyMode;
  PacketHandler ignoreKeyRange;
  PacketHandler acceptKeyRange;
  PacketHandler ignoreKeySet;
  PacketHandler acceptKeySet;
  PacketHandler write;
  PacketHandler enterRawMode;  
  PacketHandler leaveRawMode;
  PacketHandler packet;
  PacketHandler suspendDriver;
  PacketHandler resumeDriver;
} PacketHandlers;

/****************************************************************************/
/** BRAILLE WINDOWS MANAGING                                               **/
/****************************************************************************/

/* Function : allocBrailleWindow */
/* Allocates and initializes the members of a BrailleWindow structure */
/* Uses displaySize to determine size of allocated buffers */
/* Returns to report success, -1 on errors */
int allocBrailleWindow(BrailleWindow *brailleWindow)
{
  if (!(brailleWindow->text = malloc(displaySize*sizeof(wchar_t)))) goto out;
  if (!(brailleWindow->andAttr = malloc(displaySize))) goto outText;
  if (!(brailleWindow->orAttr = malloc(displaySize))) goto outAnd;

  wmemset(brailleWindow->text, L' ', displaySize);
  memset(brailleWindow->andAttr, 0xFF, displaySize);
  memset(brailleWindow->orAttr, 0x00, displaySize);
  brailleWindow->cursor = 0;
  return 0;

outAnd:
  free(brailleWindow->andAttr);

outText:
  free(brailleWindow->text);

out:
  return -1;
}

/* Function: freeBrailleWindow */
/* Frees the fields of a BrailleWindow structure */
void freeBrailleWindow(BrailleWindow *brailleWindow)
{
  free(brailleWindow->text); brailleWindow->text = NULL;
  free(brailleWindow->andAttr); brailleWindow->andAttr = NULL;
  free(brailleWindow->orAttr); brailleWindow->orAttr = NULL;
}

/* Function: copyBrailleWindow */
/* Copies a BrailleWindow structure in another one */
/* No allocation is performed */
void copyBrailleWindow(BrailleWindow *dest, const BrailleWindow *src)
{
  dest->cursor = src->cursor;
  memcpy(dest->text, src->text, displaySize*sizeof(wchar_t));
  memcpy(dest->andAttr, src->andAttr, displaySize);
  memcpy(dest->orAttr, src->orAttr, displaySize);
}

/* Function: getDots */
/* Returns the braille dots corresponding to a BrailleWindow structure */
/* No allocation of buf is performed */
void getDots(const BrailleWindow *brailleWindow, unsigned char *buf)
{
  int i, c;
  wchar_t wc;
  for (i=0; i<displaySize; i++) {
    wc = brailleWindow->text[i];
    if ((wc >= BRLAPI_UC_ROW) && (wc <= (BRLAPI_UC_ROW | 0XFF)))
      buf[i] =
	(wc&(1<<(1-1))?BRLAPI_DOT1:0) |
	(wc&(1<<(2-1))?BRLAPI_DOT2:0) |
	(wc&(1<<(3-1))?BRLAPI_DOT3:0) |
	(wc&(1<<(4-1))?BRLAPI_DOT4:0) |
	(wc&(1<<(5-1))?BRLAPI_DOT5:0) |
	(wc&(1<<(6-1))?BRLAPI_DOT6:0) |
	(wc&(1<<(7-1))?BRLAPI_DOT7:0) |
	(wc&(1<<(8-1))?BRLAPI_DOT8:0);
    else
      if ((c = convertWcharToChar(wc)) != EOF)
        buf[i] = textTable[c];
      else
        /* TODO: "invalid" pattern */
        buf[i] = textTable[(unsigned char) '?'];
    buf[i] = (buf[i] & brailleWindow->andAttr[i]) | brailleWindow->orAttr[i];
  }
  if (brailleWindow->cursor) buf[brailleWindow->cursor-1] |= cursorDots();
}

/* Function: getText */
/* Returns the text corresponding to a BrailleWindow structure */
/* No allocation of buf is performed */
void getText(const BrailleWindow *brailleWindow, unsigned char *buf)
{
  int i;
  wchar_t wc;
  for (i=0; i<displaySize; i++) {
    if ((wc = brailleWindow->text[i]) >= 256)
      buf[i] = '?';
    else
      buf[i] = wc;
  }
}

static void handleResize(BrailleDisplay *brl)
{
  /* TODO: handle resize */
  LogPrint(LOG_INFO,"BrlAPI resize");
}

/****************************************************************************/
/** CONNECTIONS MANAGING                                                   **/
/****************************************************************************/

/* Function : createConnection */
/* Creates a connection */
static Connection *createConnection(FileDescriptor fd, time_t currentTime)
{
  pthread_mutexattr_t mattr;
  Connection *c =  malloc(sizeof(Connection));
  if (c==NULL) goto out;
  c->auth = 0;
  c->fd = fd;
  c->tty = NULL;
  c->raw = 0;
  c->suspend = 0;
  c->brlbufstate = EMPTY;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&c->brlMutex,&mattr);
  pthread_mutex_init(&c->maskMutex,&mattr);
  c->how = 0;
  c->unmaskedKeys = NULL;
  c->upTime = currentTime;
  c->brailleWindow.text = NULL;
  c->brailleWindow.andAttr = NULL;
  c->brailleWindow.orAttr = NULL;
  if (initializePacket(&c->packet))
    goto outmalloc;
  return c;

outmalloc:
  free(c);
out:
  writeError(fd,BRLERR_NOMEM);
  closeFileDescriptor(fd);
  return NULL;
}

/* Function : freeConnection */
/* Frees all resources associated to a connection */
static void freeConnection(Connection *c)
{
  if (c->fd>=0) closeFileDescriptor(c->fd);
  pthread_mutex_destroy(&c->brlMutex);
  pthread_mutex_destroy(&c->maskMutex);
  freeBrailleWindow(&c->brailleWindow);
  freeKeyrangeList(&c->unmaskedKeys);
  free(c);
}

/* Function : addConnection */
/* Creates a connection and adds it to the connection list */
static void __addConnection(Connection *c, Connection *connections)
{
  c->next = connections->next;
  c->prev = connections;
  connections->next->prev = c;
  connections->next = c;
}
static void addConnection(Connection *c, Connection *connections)
{
  pthread_mutex_lock(&connectionsMutex);
  __addConnection(c,connections);
  pthread_mutex_unlock(&connectionsMutex);
}

/* Function : removeConnection */
/* Removes the connection from the list */
static void __removeConnection(Connection *c)
{
  c->prev->next = c->next;
  c->next->prev = c->prev;
}
static void removeConnection(Connection *c)
{
  pthread_mutex_lock(&connectionsMutex);
  __removeConnection(c);
  pthread_mutex_unlock(&connectionsMutex);
}

/* Function: removeFreeConnection */
/* Removes the connection from the list and frees its ressources */
static void removeFreeConnection(Connection *c)
{
  removeConnection(c);
  freeConnection(c);
}

/****************************************************************************/
/** TTYs MANAGING                                                          **/
/****************************************************************************/

/* Function: newTty */
/* creates a new tty and inserts it in the hierarchy */
static inline Tty *newTty(Tty *father, int number)
{
  Tty *tty;
  if (!(tty = calloc(1,sizeof(*tty)))) goto out;
  if (!(tty->connections = createConnection(INVALID_FILE_DESCRIPTOR,0))) goto outtty;
  tty->connections->next = tty->connections->prev = tty->connections;
  tty->number = number;
  tty->focus = -1;
  tty->father = father;
  tty->prevnext = &father->subttys;
  if ((tty->next = father->subttys))
    tty->next->prevnext = &tty->next;
  father->subttys = tty;
  return tty;
  
outtty:
  free(tty);
out:
  return NULL;
}

/* Function: removeTty */
/* removes an unused tty from the hierarchy */
static inline void removeTty(Tty *toremove)
{
  if (toremove->next)
    toremove->next->prevnext = toremove->prevnext;
  *(toremove->prevnext) = toremove->next;
}

/* Function: freeTty */
/* frees a tty */
static inline void freeTty(Tty *tty)
{
  freeConnection(tty->connections);
  free(tty);
}

/****************************************************************************/
/** COMMUNICATION PROTOCOL HANDLING                                        **/
/****************************************************************************/

/* Function LogPrintRequest */
/* Logs the given request */
static inline void LogPrintRequest(int type, FileDescriptor fd)
{
  LogPrint(LOG_DEBUG, "Received %s request on fd %"PRIFD, brlapiserver_packetType(type), fd);  
}

static int handleGetDriver(Connection *c, brl_type_t type, size_t size, const char *str)
{
  int len = strlen(str);
  LogPrintRequest(type, c->fd);
  CHECKERR(size==0,BRLERR_INVALID_PACKET,"packet should be empty");
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  brlapiserver_writePacket(c->fd, type, str, len+1);
  return 0;
}

static int handleGetDriverId(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  return handleGetDriver(c, type, size, braille->definition.code);
}

static int handleGetDriverName(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  return handleGetDriver(c, type, size, braille->definition.name);
}

static int handleGetDisplaySize(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(size==0,BRLERR_INVALID_PACKET,"packet should be empty");
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  brlapiserver_writePacket(c->fd,BRLPACKET_GETDISPLAYSIZE,&displayDimensions[0],sizeof(displayDimensions));
  return 0;
}

static int handleEnterTtyMode(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  uint32_t * ints = (uint32_t *) packet;
  uint32_t nbTtys;
  int how;
  unsigned int n;
  unsigned char *p = packet;
  char name[BRLAPI_MAXNAMELENGTH+1];
  Tty *tty,*tty2,*tty3;
  uint32_t *ptty;
  size_t remaining = size;
  LogPrintRequest(type, c->fd);
  CHECKERR((!c->raw),BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKERR(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small");
  p += sizeof(uint32_t); remaining -= sizeof(uint32_t);
  nbTtys = ntohl(ints[0]);
  CHECKERR(remaining>=nbTtys*sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small for provided number of ttys");
  p += nbTtys*sizeof(uint32_t); remaining -= nbTtys*sizeof(uint32_t);
  CHECKERR(*p<=BRLAPI_MAXNAMELENGTH, BRLERR_INVALID_PARAMETER, "driver name too long");
  n = *p; p++; remaining--;
  CHECKERR(remaining==n, BRLERR_INVALID_PACKET,"packet size doesn't match format");
  memcpy(name, p, n);
  name[n] = '\0';
  if (!*name) how = BRL_COMMANDS; else {
    CHECKERR(!strcmp(name, trueBraille->definition.name), BRLERR_INVALID_PARAMETER, "wrong driver name");
    CHECKERR(isKeyCapable(trueBraille), BRLERR_OPNOTSUPP, "driver doesn't support raw keycodes");
    how = BRL_KEYCODES;
  }
  freeBrailleWindow(&c->brailleWindow); /* In case of multiple enterTtyMode requests */
  if ((initializeUnmaskedKeys(c)==-1) || (allocBrailleWindow(&c->brailleWindow)==-1)) {
    LogPrint(LOG_WARNING,"Failed to allocate some ressources");
    freeKeyrangeList(&c->unmaskedKeys);
    WERR(c->fd,BRLERR_NOMEM, "no memory for unmasked keys");
    return 0;
  }
  pthread_mutex_lock(&connectionsMutex);
  tty = tty2 = &ttys;
  for (ptty=ints+1; ptty<=ints+nbTtys; ptty++) {
    for (tty2=tty->subttys; tty2 && tty2->number!=ntohl(*ptty); tty2=tty2->next);
      if (!tty2) break;
  	tty = tty2;
  	LogPrint(LOG_DEBUG,"tty %#010lx ok",(unsigned long)ntohl(*ptty));
  }
  if (!tty2) {
    /* we were stopped at some point because the path doesn't exist yet */
    if (c->tty) {
      /* uhu, we already got a tty, but not this one, since the path
       * doesn't exist yet. This is forbidden. */
      pthread_mutex_unlock(&connectionsMutex);
      WERR(c->fd, BRLERR_INVALID_PARAMETER, "already having another tty");
      freeBrailleWindow(&c->brailleWindow);
      return 0;
    }
    /* ok, allocate path */
    /* we lock the entire subtree for easier cleanup */
    if (!(tty2 = newTty(tty,ntohl(*ptty)))) {
      pthread_mutex_unlock(&connectionsMutex);
      WERR(c->fd,BRLERR_NOMEM, "no memory for new tty");
      freeBrailleWindow(&c->brailleWindow);
      return 0;
    }
    ptty++;
    LogPrint(LOG_DEBUG,"allocated tty %#010lx",(unsigned long)ntohl(*(ptty-1)));
    for (; ptty<=ints+nbTtys; ptty++) {
      if (!(tty2 = newTty(tty2,ntohl(*ptty)))) {
        /* gasp, couldn't allocate :/, clean tree */
        for (tty2 = tty->subttys; tty2; tty2 = tty3) {
          tty3 = tty2->subttys;
          freeTty(tty2);
        }
        pthread_mutex_unlock(&connectionsMutex);
        WERR(c->fd,BRLERR_NOMEM, "no memory for new tty");
        freeBrailleWindow(&c->brailleWindow);
  	return 0;
      }
      LogPrint(LOG_DEBUG,"allocated tty %#010lx",(unsigned long)ntohl(*ptty));
    }
    tty = tty2;
  }
  if (c->tty) {
    pthread_mutex_unlock(&connectionsMutex);
    if (c->tty == tty) {
      if (c->how==how) {
	WERR(c->fd, BRLERR_ILLEGAL_INSTRUCTION, "already controlling tty %#010x", c->tty->number);
      } else {
        /* Here one is in the case where the client tries to change */
        /* from BRL_KEYCODES to BRL_COMMANDS, or something like that */
        /* For the moment this operation is not supported */
        /* A client that wants to do that should first LeaveTty() */
        /* and then get it again, risking to lose it */
        WERR(c->fd,BRLERR_OPNOTSUPP, "Switching from BRL_KEYCODES to BRL_COMMANDS not supported yet");
      }
      return 0;
    } else {
      /* uhu, we already got a tty, but not this one: this is forbidden. */
      WERR(c->fd, BRLERR_INVALID_PARAMETER, "already having a tty");
      return 0;
    }
  }
  c->tty = tty;
  c->how = how;
  __removeConnection(c);
  __addConnection(c,tty->connections);
  pthread_mutex_unlock(&connectionsMutex);
  writeAck(c->fd);
  LogPrint(LOG_DEBUG,"Taking control of tty %#010x (how=%d)",tty->number,how);
  return 0;
}

static int handleSetFocus(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  uint32_t * ints = (uint32_t *) packet;
  CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKEXC(c->tty,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of tty mode");
  c->tty->focus = ntohl(ints[0]);
  LogPrint(LOG_DEBUG,"Focus on window %#010x",c->tty->focus);
  return 0;
}

/* Function doLeaveTty */
/* handles a connection leaving its tty */
static void doLeaveTty(Connection *c)
{
  Tty *tty = c->tty;
  LogPrint(LOG_DEBUG,"Releasing tty %#010x",tty->number);
  c->tty = NULL;
  pthread_mutex_lock(&connectionsMutex);
  __removeConnection(c);
  __addConnection(c,notty.connections);
  pthread_mutex_unlock(&connectionsMutex);
  freeKeyrangeList(&c->unmaskedKeys);
  freeBrailleWindow(&c->brailleWindow);
}

static int handleLeaveTtyMode(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKERR(c->tty,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of tty mode");
  doLeaveTty(c);
  writeAck(c->fd);
  return 0;
}

static int handleKeyRange(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  int res;
  brl_keycode_t x,y;
  uint32_t * ints = (uint32_t *) packet;
  LogPrintRequest(type, c->fd);
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKERR(c->tty,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of tty mode");
  CHECKERR(size==2*sizeof(brl_keycode_t),BRLERR_INVALID_PACKET,"wrong packet size");
  x = ((brl_keycode_t)ntohl(ints[0]) << 32) | ntohl(ints[1]);
  y = ((brl_keycode_t)ntohl(ints[2]) << 32) | ntohl(ints[3]);
  LogPrint(LOG_DEBUG,"range: [%016"BRLAPI_PRIxKEYCODE"..%016"BRLAPI_PRIxKEYCODE"]",x,y);
  pthread_mutex_lock(&c->maskMutex);
  if (type==BRLPACKET_IGNOREKEYRANGE) res = removeKeyrange(x,y,&c->unmaskedKeys);
  else res = addKeyrange(x,y,&c->unmaskedKeys);
  pthread_mutex_unlock(&c->maskMutex);
  if (res==-1) WERR(c->fd,BRLERR_NOMEM,"no memory for key range");
  else writeAck(c->fd);
  return 0;
}

static int handleKeySet(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  int i = 0, res = 0;
  unsigned int nbkeys;
  uint32_t *k = (uint32_t *) packet;
  int (*fptr)(KeyrangeElem, KeyrangeElem, KeyrangeList **);
  brl_keycode_t code;
  LogPrintRequest(type, c->fd);
  if (type==BRLPACKET_IGNOREKEYSET) fptr = removeKeyrange; else fptr = addKeyrange;
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKERR(c->tty,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of tty mode");
  CHECKERR(size % sizeof(brl_keycode_t)==0,BRLERR_INVALID_PACKET,"wrong packet size");
  nbkeys = size/sizeof(brl_keycode_t);
  pthread_mutex_lock(&c->maskMutex);
  while ((res!=-1) && (i<nbkeys)) {
    code = ((brl_keycode_t)ntohl(k[2*i]) << 32) | ntohl(k[2*i+1]);
    res = fptr(code,code,&c->unmaskedKeys);
    i++;
  }
  pthread_mutex_unlock(&c->maskMutex);
  if (res==-1) WERR(c->fd,BRLERR_NOMEM,"no memory for key set");
  else writeAck(c->fd);
  return 0;
}

static int handleWrite(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  writeStruct *ws = (writeStruct *) packet;
  unsigned char *text = NULL, *orAttr = NULL, *andAttr = NULL;
  unsigned int rbeg, rsiz, textLen = 0;
  int cursor = -1;
  unsigned char *p = &ws->data;
  int remaining = size;
#ifdef HAVE_ICONV_H
  char *charset = NULL, *coreCharset = NULL;
  unsigned int charsetLen = 0;
#endif /* HAVE_ICONV_H */
  LogPrintRequest(type, c->fd);
  CHECKEXC(remaining>=sizeof(ws->flags), BRLERR_INVALID_PACKET, "packet too small for flags");
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  CHECKERR(c->tty,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of tty mode");
  ws->flags = ntohl(ws->flags);
  if ((remaining==sizeof(ws->flags))&&(ws->flags==0)) {
    c->brlbufstate = EMPTY;
    return 0;
  }
  remaining -= sizeof(ws->flags); /* flags */
  CHECKEXC((ws->flags & BRLAPI_WF_DISPLAYNUMBER)==0, BRLERR_OPNOTSUPP, "display number not yet supported");
  if (ws->flags & BRLAPI_WF_REGION) {
    CHECKEXC(remaining>2*sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small for region");
    rbeg = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* region begin */
    rsiz = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* region size */
    CHECKEXC(
      (1<=rbeg) && (rsiz>0) && (rbeg+rsiz-1<=displaySize),
      BRLERR_INVALID_PARAMETER, "wrong region");
  } else {
    LogPrint(LOG_DEBUG,"Warning: Client uses deprecated regionBegin=0 and regionSize = 0");
    rbeg = 1;
    rsiz = displaySize;
  }
  if (ws->flags & BRLAPI_WF_TEXT) {
    CHECKEXC(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small for text length");
    textLen = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* text size */
    CHECKEXC(remaining>=textLen, BRLERR_INVALID_PACKET, "packet too small for text");
    text = p;
    p += textLen; remaining -= textLen; /* text */
  }
  if (ws->flags & BRLAPI_WF_ATTR_AND) {
    CHECKEXC(remaining>=rsiz, BRLERR_INVALID_PACKET, "packet too small for And mask");
    andAttr = p;
    p += rsiz; remaining -= rsiz; /* and attributes */
  }
  if (ws->flags & BRLAPI_WF_ATTR_OR) {
    CHECKEXC(remaining>=rsiz, BRLERR_INVALID_PACKET, "packet too small for Or mask");
    orAttr = p;
    p += rsiz; remaining -= rsiz; /* or attributes */
  }
  if (ws->flags & BRLAPI_WF_CURSOR) {
    CHECKEXC(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small for cursor");
    cursor = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* cursor */
    CHECKEXC(cursor<=displaySize, BRLERR_INVALID_PACKET, "wrong cursor");
  }
#ifdef HAVE_ICONV_H
  if (ws->flags & BRLAPI_WF_CHARSET) {
    CHECKEXC(ws->flags & BRLAPI_WF_TEXT, BRLERR_INVALID_PACKET, "charset requires text");
    CHECKEXC(remaining>=1, BRLERR_INVALID_PACKET, "packet too small for charset length");
    charsetLen = *p++; remaining--; /* charset length */
    CHECKEXC(remaining>=charsetLen, BRLERR_INVALID_PACKET, "packet too small for charset");
    charset = (char *) p;
    p += charsetLen; remaining -= charsetLen; /* charset name */
  }
#else /* HAVE_ICONV_H */
  CHECKEXC(!(ws->flags & BRLAPI_WF_CHARSET), BRLERR_OPNOTSUPP, "charset conversion not supported (enable iconv?)");
#endif /* HAVE_ICONV_H */
  CHECKEXC(remaining==0, BRLERR_INVALID_PACKET, "packet too big");
  /* Here the whole packet has been checked */
  if (text) {
#ifdef HAVE_ICONV_H
    if (charset) {
      charset[charsetLen] = 0; /* we have room for this */
    } else {
      lockCharset(0);
      charset = coreCharset = (char *) getCharset();
      if (!coreCharset)
        unlockCharset();
    }
    if (charset) {
      iconv_t conv;
      wchar_t textBuf[rsiz];
      char *in = (char *) text, *out = (char *) textBuf;
      size_t sin = textLen, sout = sizeof(textBuf), res;
      LogPrint(LOG_DEBUG,"charset %s", charset);
      CHECKEXC((conv = iconv_open("WCHAR_T",charset)) != (iconv_t)(-1), BRLERR_INVALID_PACKET, "invalid charset");
      res = iconv(conv,&in,&sin,&out,&sout);
      iconv_close(conv);
      CHECKEXC(res != (size_t) -1, BRLERR_INVALID_PACKET, "invalid charset conversion");
      CHECKEXC(!sin, BRLERR_INVALID_PACKET, "text too big");
      CHECKEXC(!sout, BRLERR_INVALID_PACKET, "text too small");
      if (coreCharset) unlockCharset();
      pthread_mutex_lock(&c->brlMutex);
      memcpy(c->brailleWindow.text+rbeg-1,textBuf,rsiz*sizeof(wchar_t));
    } else
#endif /* HAVE_ICONV_H */
    {
      int i;
      pthread_mutex_lock(&c->brlMutex);
      for (i=0; i<rsiz; i++)
	/* assume latin1 */
        c->brailleWindow.text[rbeg-1+i] = text[i];
    }
    if (!andAttr) memset(c->brailleWindow.andAttr+rbeg-1,0xFF,rsiz);
    if (!orAttr)  memset(c->brailleWindow.orAttr+rbeg-1,0x00,rsiz);
  } else pthread_mutex_lock(&c->brlMutex);
  if (andAttr) memcpy(c->brailleWindow.andAttr+rbeg-1,andAttr,rsiz);
  if (orAttr) memcpy(c->brailleWindow.orAttr+rbeg-1,orAttr,rsiz);
  if (cursor>=0) c->brailleWindow.cursor = cursor;
  c->brlbufstate = TODISPLAY;
  pthread_mutex_unlock(&c->brlMutex);
  return 0;
}

static int checkDriverSpecificModePacket(Connection *c, unsigned char *packet, size_t size)
{
  getDriveSpecificModePacket_t *getDevicePacket = (getDriveSpecificModePacket_t *) packet;
  int remaining = size;
  CHECKERR(remaining>sizeof(uint32_t), BRLERR_INVALID_PACKET, "packet too small");
  remaining -= sizeof(uint32_t);
  CHECKERR(ntohl(getDevicePacket->magic)==BRLDEVICE_MAGIC,BRLERR_INVALID_PARAMETER, "wrong magic number");
  remaining--;
  CHECKERR(getDevicePacket->nameLength<=BRLAPI_MAXNAMELENGTH && getDevicePacket->nameLength == strlen(trueBraille->definition.name), BRLERR_INVALID_PARAMETER, "wrong driver length");
  CHECKERR(remaining==getDevicePacket->nameLength, BRLERR_INVALID_PACKET, "wrong packet size");
  CHECKERR(((!strncmp(&getDevicePacket->name, trueBraille->definition.name, remaining))), BRLERR_INVALID_PARAMETER, "wrong driver name");
  return 1;
}

static int handleEnterRawMode(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(!c->raw, BRLERR_ILLEGAL_INSTRUCTION,"not allowed in raw mode");
  if (!checkDriverSpecificModePacket(c, packet, size)) return 0;
  CHECKERR(isRawCapable(trueBraille), BRLERR_OPNOTSUPP, "driver doesn't support Raw mode");
  pthread_mutex_lock(&rawMutex);
  if (rawConnection || suspendConnection) {
    WERR(c->fd,BRLERR_DEVICEBUSY,"driver busy (%s)", rawConnection?"raw":"suspend");
    pthread_mutex_unlock(&rawMutex);
    return 0;
  }
  pthread_mutex_lock(&driverMutex);
  if (!driverOpened && !resumeDriver(disp)) {
    WERR(c->fd, BRLERR_DRIVERERROR,"driver resume error");
    pthread_mutex_unlock(&driverMutex);
    pthread_mutex_unlock(&rawMutex);
    return 0;
  }
  pthread_mutex_unlock(&driverMutex);
  c->raw = 1;
  rawConnection = c;
  pthread_mutex_unlock(&rawMutex);
  writeAck(c->fd);
  return 0;
}

static int handleLeaveRawMode(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of raw mode");
  LogPrint(LOG_DEBUG,"Going out of raw mode");
  pthread_mutex_lock(&rawMutex);
  c->raw = 0;
  rawConnection = NULL;
  pthread_mutex_unlock(&rawMutex);
  writeAck(c->fd);
  return 0;
}

static int handlePacket(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKEXC(c->raw,BRLERR_ILLEGAL_INSTRUCTION,"not allowed out of raw mode");
  pthread_mutex_lock(&driverMutex);
  trueBraille->writePacket(disp,packet,size);
  pthread_mutex_unlock(&driverMutex);
  return 0;
}

static int handleSuspendDriver(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  if (!checkDriverSpecificModePacket(c, packet, size)) return 0;
  CHECKERR(!c->suspend,BRLERR_ILLEGAL_INSTRUCTION, "not allowed in suspend mode");
  pthread_mutex_lock(&rawMutex);
  if (suspendConnection || rawConnection) {
    WERR(c->fd, BRLERR_DEVICEBUSY,"driver busy (%s)", rawConnection?"raw":"suspend");
    pthread_mutex_unlock(&rawMutex);
    return 0;
  }
  c->suspend = 1;
  suspendConnection = c;
  pthread_mutex_unlock(&rawMutex);
  pthread_mutex_lock(&driverMutex);
  if (driverOpened) suspendDriver(disp);
  pthread_mutex_unlock(&driverMutex);
  writeAck(c->fd);
  return 0;
}

static int handleResumeDriver(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(c->suspend,BRLERR_ILLEGAL_INSTRUCTION, "not allowed out of suspend mode");
  pthread_mutex_lock(&rawMutex);
  c->suspend = 0;
  suspendConnection = NULL;
  pthread_mutex_unlock(&rawMutex);
  pthread_mutex_lock(&driverMutex);
  if (!driverOpened) resumeDriver(disp);
  pthread_mutex_unlock(&driverMutex);
  writeAck(c->fd);
  return 0;
}

static PacketHandlers packetHandlers = {
  handleGetDriverId, handleGetDriverName, handleGetDisplaySize,
  handleEnterTtyMode, handleSetFocus, handleLeaveTtyMode,
  handleKeyRange, handleKeyRange, handleKeySet, handleKeySet,
  handleWrite,
  handleEnterRawMode, handleLeaveRawMode, handlePacket, handleSuspendDriver, handleResumeDriver
};

static void handleNewConnection(Connection *c)
{
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  int nbmethods = 0;
  authServerStruct *authPacket = (authServerStruct *) packet;
  authPacket->protocolVersion = htonl(BRLAPI_PROTOCOL_VERSION);

  /* TODO: move this inside auth.c */
  if (authDescriptor && authPerform(authDescriptor, c->fd))
    authPacket->type[nbmethods++] = htonl(BRLAPI_AUTH_NONE);
  if (auth[0] == '/')
    authPacket->type[nbmethods++] = htonl(BRLAPI_AUTH_KEY);

  brlapiserver_writePacket(c->fd,BRLPACKET_AUTH,packet,sizeof(authPacket->protocolVersion)+nbmethods*sizeof(authPacket->type));
}

/* Function : handleUnauthorizedConnection */
/* Returns 0 if connection is authorized */
/* Returns 1 if connection has to be removed */
static int handleUnauthorizedConnection(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  size_t authKeyLength = 0;
  unsigned char authKey[BRLAPI_MAXPACKETSIZE];
  int authCorrect = 0;
  authClientStruct *authPacket = (authClientStruct *) packet;
  int remaining = size;
  uint32_t authType;

  if (type!=BRLPACKET_AUTH) {
    WERR(c->fd, BRLERR_CONNREFUSED, "wrong packet type (should be auth)");
    return 1;
  }

  if ((remaining<sizeof(authPacket->protocolVersion))||(ntohl(authPacket->protocolVersion)!=BRLAPI_PROTOCOL_VERSION)) {
    writeError(c->fd, BRLERR_PROTOCOL_VERSION);
    return 1;
  }
  remaining -= sizeof(authPacket->protocolVersion);

  if (!strcmp(auth,"none"))
    authCorrect = 1;
  else {
    authType = ntohl(authPacket->type);
    remaining -= sizeof(authPacket->type);

  /* TODO: move this inside auth.c */
    switch(authType) {
      case BRLAPI_AUTH_NONE:
        if (authDescriptor) authCorrect = authPerform(authDescriptor, c->fd);
	break;
      case BRLAPI_AUTH_KEY:
        if (auth[0] == '/') {
	  if (brlapiserver_loadAuthKey(auth,&authKeyLength,authKey)==-1) {
	    LogPrint(LOG_WARNING,"Unable to load API authorization key from %s: %s in %s. You may use parameter auth=none if you don't want any authorization (dangerous)", auth, strerror(brlapi_libcerrno), brlapi_errfun);
	    break;
	  }
	  LogPrint(LOG_DEBUG, "Authorization key loaded");
	  authCorrect = (remaining==authKeyLength) && (!memcmp(&authPacket->key, authKey, authKeyLength));
	  memset(authKey, 0, authKeyLength);
	  memset(&authPacket->key, 0, authKeyLength);
	}
	break;
      default:
        LogPrint(LOG_DEBUG, "Unsupported authorization method %d\n", authType);
        break;
    }
  }

  if (!authCorrect) {
    writeError(c->fd, BRLERR_CONNREFUSED);
    LogPrint(LOG_WARNING, "BrlAPI connection fd=%"PRIFD" failed authorization", c->fd);
    return 0;
  }

  unauthConnections--;
  writeAck(c->fd);
  c->auth = 1;
  return 0;
}

/* Function : processRequest */
/* Reads a packet fro c->fd and processes it */
/* Returns 1 if connection has to be removed */
/* If EOF is reached, closes fd and frees all associated ressources */
static int processRequest(Connection *c, PacketHandlers *handlers)
{
  PacketHandler p = NULL;
  int res;
  ssize_t size;
  unsigned char *packet = c->packet.content;
  brl_type_t type;
  res = readPacket(c);
  if (res==0) return 0; /* No packet ready */
  if (res<0) {
    if (res==-1) LogPrint(LOG_WARNING,"read : %s (connection on fd %"PRIFD")",strerror(errno),c->fd);
    else {
      LogPrint(LOG_DEBUG,"Closing connection on fd %"PRIFD,c->fd);
    }
    if (c->raw) {
      pthread_mutex_lock(&rawMutex);
      c->raw = 0;
      rawConnection = NULL;
      LogPrint(LOG_WARNING,"Client on fd %"PRIFD" did not give up raw mode properly",c->fd);
      pthread_mutex_lock(&driverMutex);
      LogPrint(LOG_WARNING,"Trying to reset braille terminal");
      if (!trueBraille->reset || !trueBraille->reset(disp)) {
	if (trueBraille->reset)
          LogPrint(LOG_WARNING,"Reset failed. Restarting braille driver");
        restartBrailleDriver();
      }
      pthread_mutex_unlock(&driverMutex);
      pthread_mutex_unlock(&rawMutex);
    } else if (c->suspend) {
      pthread_mutex_lock(&rawMutex);
      c->suspend = 0;
      suspendConnection = NULL;
      LogPrint(LOG_WARNING,"Client on fd %"PRIFD" did not give up suspended mode properly",c->fd);
      pthread_mutex_lock(&driverMutex);
      if (!driverOpened && !resumeDriver(disp))
	LogPrint(LOG_WARNING,"Couldn't resume braille driver");
      if (driverOpened && trueBraille->reset) {
        LogPrint(LOG_DEBUG,"Trying to reset braille terminal");
	if (!trueBraille->reset(disp))
	  LogPrint(LOG_WARNING,"Resetting braille terminal failed, hoping it's ok");
      }
      pthread_mutex_unlock(&driverMutex);
      pthread_mutex_unlock(&rawMutex);
    }
    if (c->tty) {
      LogPrint(LOG_DEBUG,"Client on fd %"PRIFD" did not give up control of tty %#010x properly",c->fd,c->tty->number);
      doLeaveTty(c);
    }
    if (c->auth==0) unauthConnections--;
    return 1;
  }
  size = c->packet.header.size;
  type = c->packet.header.type;
  
  if (c->auth==0) return handleUnauthorizedConnection(c, type, packet, size);

  if (size>BRLAPI_MAXPACKETSIZE) {
    LogPrint(LOG_WARNING, "Discarding too large packet of type %s on fd %"PRIFD,brlapiserver_packetType(type), c->fd);
    return 0;    
  }
  switch (type) {
    case BRLPACKET_GETDRIVERID: p = handlers->getDriverId; break;
    case BRLPACKET_GETDRIVERNAME: p = handlers->getDriverName; break;
    case BRLPACKET_GETDISPLAYSIZE: p = handlers->getDisplaySize; break;
    case BRLPACKET_ENTERTTYMODE: p = handlers->enterTtyMode; break;
    case BRLPACKET_SETFOCUS: p = handlers->setFocus; break;
    case BRLPACKET_LEAVETTYMODE: p = handlers->leaveTtyMode; break;
    case BRLPACKET_IGNOREKEYRANGE: p = handlers->ignoreKeyRange; break;
    case BRLPACKET_ACCEPTKEYRANGE: p = handlers->acceptKeyRange; break;
    case BRLPACKET_IGNOREKEYSET: p = handlers->ignoreKeySet; break;
    case BRLPACKET_ACCEPTKEYSET: p = handlers->acceptKeySet; break;
    case BRLPACKET_WRITE: p = handlers->write; break;
    case BRLPACKET_ENTERRAWMODE: p = handlers->enterRawMode; break;
    case BRLPACKET_LEAVERAWMODE: p = handlers->leaveRawMode; break;
    case BRLPACKET_PACKET: p = handlers->packet; break;
    case BRLPACKET_SUSPENDDRIVER: p = handlers->suspendDriver; break;
    case BRLPACKET_RESUMEDRIVER: p = handlers->resumeDriver; break;
  }
  if (p!=NULL) p(c, type, packet, size);
  else WEXC(c->fd,BRLERR_UNKNOWN_INSTRUCTION, type, packet, size, "unknown packet type");
  return 0;
}

/****************************************************************************/
/** SOCKETS AND CONNECTIONS MANAGING                                       **/
/****************************************************************************/

/*
 * There is one server thread which first launches binding threads and then
 * enters infinite loop trying to accept connections, read packets, etc.
 *
 * Binding threads loop trying to establish some socket, waiting for 
 * filesystems to be read/write or network to be configured.
 *
 * On windows, WSAEventSelect() is emulated by a standalone thread.
 */

/* Function: loopBind */
/* tries binding while temporary errors occur */
static int loopBind(SocketDescriptor fd, struct sockaddr *addr, socklen_t len)
{
  while (bind(fd, addr, len)<0) {
    if (
#ifdef EADDRNOTAVAIL
      errno!=EADDRNOTAVAIL &&
#endif /* EADDRNOTAVAIL */
#ifdef EADDRINUSE
      errno!=EADDRINUSE &&
#endif /* EADDRINUSE */
      errno!=EROFS) {
      return -1;
    }
    approximateDelay(1000);
  }
  return 0;
}

/* Function : initializeTcpSocket */
/* Creates the listening socket for in-connections */
/* Returns the descriptor, or -1 if an error occurred */
static FileDescriptor initializeTcpSocket(struct socketInfo *info)
{
#ifdef WINDOWS
  SOCKET fd=INVALID_SOCKET;
#else /* WINDOWS */
  int fd=-1;
#endif /* WINDOWS */
  const char *fun;
  int yes=1;

#ifdef WINDOWS
  if (getaddrinfoProc) {
#endif
#if defined(HAVE_GETADDRINFO) || defined(WINDOWS)
  int err;
  struct addrinfo *res,*cur;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  err = getaddrinfo(info->host, info->port, &hints, &res);
  if (err) {
    LogPrint(LOG_WARNING,"getaddrinfo(%s,%s): "
#ifdef HAVE_GAI_STRERROR
	"%s"
#else /* HAVE_GAI_STRERROR */
	"%d"
#endif /* HAVE_GAI_STRERROR */
	,info->host,info->port
#ifdef HAVE_GAI_STRERROR
	,
#ifdef EAI_SYSTEM
	err == EAI_SYSTEM ? strerror(errno) :
#endif /* EAI_SYSTEM */
	gai_strerror(err)
#else /* HAVE_GAI_STRERROR */
	,err
#endif /* HAVE_GAI_STRERROR */
    );
    return INVALID_FILE_DESCRIPTOR;
  }
  for (cur = res; cur; cur = cur->ai_next) {
    fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (fd<0) {
      setSocketErrno();
#ifdef EAFNOSUPPORT
      if (errno != EAFNOSUPPORT)
#endif /* EAFNOSUPPORT */
        LogPrint(LOG_WARNING,"socket: %s",strerror(errno));
      continue;
    }
    /* Specifies that address can be reused */
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(void*)&yes,sizeof(yes))!=0) {
      fun = "setsockopt";
      goto cont;
    }
    if (loopBind(fd, cur->ai_addr, cur->ai_addrlen)<0) {
      fun = "bind";
      goto cont;
    }
    if (listen(fd,1)<0) {
      fun = "listen";
      goto cont;
    }
    break;
cont:
    LogSocketError(fun);
    closeSocketDescriptor(fd);
  }
  freeaddrinfo(res);
  if (cur) {
    free(info->host);
    info->host = NULL;
    free(info->port);
    info->port = NULL;

#ifdef WINDOWS
    if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
      LogWindowsError("CreateEvent");
      closeSocketDescriptor(fd);
      return INVALID_FILE_DESCRIPTOR;
    }
    LogPrint(LOG_DEBUG,"Event -> %p",info->overl.hEvent);
    WSAEventSelect(fd, info->overl.hEvent, FD_ACCEPT);
#endif /* WINDOWS */

    return (FileDescriptor)fd;
  }
  LogPrint(LOG_WARNING,"unable to find a local TCP port %s:%s !",info->host,info->port);
#endif /* HAVE_GETADDRINFO */
#ifdef WINDOWS
  } else {
#endif /* WINDOWS */
#if !defined(HAVE_GETADDRINFO) || defined(WINDOWS)

  struct sockaddr_in addr;
  struct hostent *he;

  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  if (!info->port)
    addr.sin_port = htons(BRLAPI_SOCKETPORTNUM);
  else {
    char *c;
    addr.sin_port = htons(strtol(info->port, &c, 0));
    if (*c) {
      struct servent *se;

      if (!(se = getservbyname(info->port,"tcp"))) {
        LogPrint(LOG_ERR,"port %s: "
#ifdef WINDOWS
	  "%d"
#else /* WINDOWS */
	  "%s"
#endif /* WINDOWS */
	  ,info->port,
#ifdef WINDOWS
	  WSAGetLastError()
#else /* WINDOWS */
	  hstrerror(h_errno)
#endif /* WINDOWS */
	  );
	return INVALID_FILE_DESCRIPTOR;
      }
      addr.sin_port = se->s_port;
    }
  }

  if (!info->host) {
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  } else if ((addr.sin_addr.s_addr = inet_addr(info->host)) == htonl(INADDR_NONE)) {
    if (!(he = gethostbyname(info->host))) {
      LogPrint(LOG_ERR,"gethostbyname(%s): "
#ifdef WINDOWS
	"%d"
#else /* WINDOWS */
	"%s"
#endif /* WINDOWS */
	,info->host,
#ifdef WINDOWS
	WSAGetLastError()
#else /* WINDOWS */
	hstrerror(h_errno)
#endif /* WINDOWS */
	);
      return INVALID_FILE_DESCRIPTOR;
    }
    if (he->h_addrtype != AF_INET) {
#ifdef EAFNOSUPPORT
      errno = EAFNOSUPPORT;
#else /* EAFNOSUPPORT */
      errno = EINVAL;
#endif /* EAFNOSUPPORT */
      LogPrint(LOG_ERR,"unknown address type %d",he->h_addrtype);
      return INVALID_FILE_DESCRIPTOR;
    }
    if (he->h_length > sizeof(addr.sin_addr)) {
      errno = EINVAL;
      LogPrint(LOG_ERR,"too big address: %d",he->h_length);
      return INVALID_FILE_DESCRIPTOR;
    }
    memcpy(&addr.sin_addr,he->h_addr,he->h_length);
  }

  fd = socket(addr.sin_family, SOCK_STREAM, 0);
  if (fd<0) {
    fun = "socket";
    goto err;
  }
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(void*)&yes,sizeof(yes))!=0) {
    fun = "setsockopt";
    goto err;
  }
  if (loopBind(fd, (struct sockaddr *) &addr, sizeof(addr))<0) {
    fun = "bind";
    goto err;
  }
  if (listen(fd,1)<0) {
    fun = "listen";
    goto err;
  }
  free(info->host);
  info->host = NULL;
  free(info->port);
  info->port = NULL;

#ifdef WINDOWS
  if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
    LogWindowsError("CreateEvent");
    closeSocketDescriptor(fd);
    return INVALID_FILE_DESCRIPTOR;
  }
  LogPrint(LOG_DEBUG,"Event -> %p",info->overl.hEvent);
  WSAEventSelect(fd, info->overl.hEvent, FD_ACCEPT);
#endif /* WINDOWS */

  return (FileDescriptor)fd;

err:
  LogSocketError(fun);
  if (fd >= 0) closeSocketDescriptor(fd);

#endif /* !HAVE_GETADDRINFO */
#ifdef WINDOWS
  }
#endif /* WINDOWS */

  free(info->host);
  info->host = NULL;
  free(info->port);
  info->port = NULL;
  return INVALID_FILE_DESCRIPTOR;
}

#if defined(PF_LOCAL)

#ifndef WINDOWS
static int readPid(char *path)
  /* read pid from specified file. Return 0 on any error */
{
  char pids[16], *ptr;
  pid_t pid;
  int n;
  FileDescriptor fd;
  fd = open(path, O_RDONLY);
  n = read(fd, pids, sizeof(pids)-1);
  closeFileDescriptor(fd);
  if (n == -1) return 0;
  pids[n] = 0;
  pid = strtol(pids, &ptr, 10);
  if (ptr != &pids[n]) return 0;
  return pid;
}
#endif /* WINDOWS */

/* Function : initializeLocalSocket */
/* Creates the listening socket for in-connections */
/* Returns 1, or 0 if an error occurred */
static FileDescriptor initializeLocalSocket(struct socketInfo *info)
{
  int lpath=strlen(BRLAPI_SOCKETPATH),lport=strlen(info->port);
  FileDescriptor fd;
#ifdef WINDOWS
  char path[lpath+lport+1];
  memcpy(path,BRLAPI_SOCKETPATH,lpath);
  memcpy(path+lpath,info->port,lport+1);
  if ((fd = CreateNamedPipe(path,
	  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
	  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
	  PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL)) == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      LogWindowsError("CreateNamedPipe");
    goto out;
  }
  LogPrint(LOG_DEBUG,"CreateFile -> %"PRIFD,fd);
  if (!info->overl.hEvent) {
    if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
      LogWindowsError("CreateEvent");
      goto outfd;
    }
    LogPrint(LOG_DEBUG,"Event -> %p",info->overl.hEvent);
  }
  if (!(ResetEvent(info->overl.hEvent))) {
    LogWindowsError("ResetEvent");
    goto outfd;
  }
  if (ConnectNamedPipe(fd, &info->overl)) {
    LogPrint(LOG_DEBUG,"already connected !");
    return fd;
  }

  switch(GetLastError()) {
    case ERROR_IO_PENDING: return fd;
    case ERROR_PIPE_CONNECTED: SetEvent(info->overl.hEvent); return fd;
    default: LogWindowsError("ConnectNamedPipe");
  }
  CloseHandle(info->overl.hEvent);
#else /* WINDOWS */
  struct sockaddr_un sa;
  char tmppath[lpath+lport+3];
  char lockpath[lpath+lport+2];
  struct stat st;
  char pids[16];
  pid_t pid;
  int lock,n,done,res;

  mode_t oldmode;
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0))==-1) {
    LogError("socket");
    goto out;
  }
  sa.sun_family = AF_LOCAL;
  if (lpath+lport+1>sizeof(sa.sun_path)) {
    LogPrint(LOG_ERR, "Unix path too long");
    goto outfd;
  }

  oldmode = umask(0);
  while (mkdir(BRLAPI_SOCKETPATH,01777)<0) {
    if (errno == EEXIST)
      break;
    if (errno != EROFS && errno != ENOENT) {
      LogError("making socket directory");
      goto outmode;
    }
    /* read-only, or not mounted yet, wait */
    approximateDelay(1000);
  }
  memcpy(sa.sun_path,BRLAPI_SOCKETPATH,lpath);
  memcpy(sa.sun_path+lpath,info->port,lport+1);
  memcpy(tmppath, BRLAPI_SOCKETPATH, lpath);
  tmppath[lpath]='.';
  memcpy(tmppath+lpath+1, info->port, lport);
  memcpy(lockpath, tmppath, lpath+1+lport);
  tmppath[lpath+lport+1]='_';
  tmppath[lpath+lport+2]=0;
  lockpath[lpath+lport+1]=0;
  while ((lock = open(tmppath, O_WRONLY|O_CREAT|O_EXCL, 
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
    if (errno == EROFS) {
      approximateDelay(1000);
      continue;
    }
    if (errno != EEXIST) {
      LogError("opening local socket lock");
      goto outmode;
    }
    if ((pid = readPid(tmppath)) && pid != getpid()
	&& (kill(pid, 0) != -1 || errno != ESRCH)) {
      LogPrint(LOG_ERR,"a BrlAPI server is already listening on %s (file %s exists)",info->port, tmppath);
      goto outmode;
    }
    /* bogus file, myself or non-existent process, remove */
    while (unlink(tmppath)) {
      if (errno != EROFS) {
	LogError("removing stale local socket lock");
	goto outmode;
      }
      approximateDelay(1000);
    }
  }

  n = snprintf(pids,sizeof(pids),"%d",getpid());
  done = 0;
  while ((res = write(lock,pids+done,n)) < n) {
    if (res == -1) {
      if (errno != ENOSPC) {
	LogError("writing pid in local socket lock");
	goto outtmp;
      }
      approximateDelay(1000);
    } else {
      done += res;
      n -= res;
    }
  }

  while(1) {
    if (link(tmppath, lockpath))
      LogPrint(LOG_DEBUG,"linking local socket lock: %s", strerror(errno));
      /* but no action: link() might erroneously return errors, see manpage */
    if (fstat(lock, &st)) {
      LogError("checking local socket lock");
      goto outtmp;
    }
    if (st.st_nlink == 2)
      /* success */
      break;
    /* failed to link */
    if ((pid = readPid(lockpath)) && pid != getpid()
	&& (kill(pid, 0) != -1 || errno != ESRCH)) {
      LogPrint(LOG_ERR,"a BrlAPI server already listens on %s (file %s exists)",info->port, lockpath);
      goto outtmp;
    }
    /* bogus file, myself or non-existent process, remove */
    if (unlink(lockpath)) {
      LogError("removing stale local socket lock");
      goto outtmp;
    }
  }
  closeFileDescriptor(lock);
  if (unlink(tmppath))
    LogError("removing temp local socket lock");
  if (unlink(sa.sun_path) && errno != ENOENT) {
    LogError("removing old socket");
    goto outtmp;
  }
  if (loopBind(fd, (struct sockaddr *) &sa, sizeof(sa))<0) {
    LogPrint(LOG_WARNING,"bind: %s",strerror(errno));
    goto outlock;
  }
  umask(oldmode);
  if (listen(fd,1)<0) {
    LogError("listen");
    goto outlock;
  }
  return fd;

outlock:
  unlink(lockpath);
outtmp:
  unlink(tmppath);
outmode:
  umask(oldmode);
#endif /* WINDOWS */
outfd:
  closeFileDescriptor(fd);
out:
  return INVALID_FILE_DESCRIPTOR;
}
#endif /* PF_LOCAL */

static void *establishSocket(void *arg)
{
  intptr_t num = (intptr_t) arg;
  struct socketInfo *cinfo = &socketInfo[num];
#ifndef WINDOWS
  int res;
  sigset_t blockedSignals;

  sigemptyset(&blockedSignals);
  sigaddset(&blockedSignals,SIGTERM);
  sigaddset(&blockedSignals,SIGINT);
  sigaddset(&blockedSignals,SIGPIPE);
  sigaddset(&blockedSignals,SIGCHLD);
  sigaddset(&blockedSignals,SIGUSR1);
  if ((res = pthread_sigmask(SIG_BLOCK,&blockedSignals,NULL))!=0) {
    LogPrint(LOG_WARNING,"pthread_sigmask: %s",strerror(res));
    return NULL;
  }
#endif /* WINDOWS */

#if defined(PF_LOCAL)
  if ((cinfo->addrfamily==PF_LOCAL && (cinfo->fd=initializeLocalSocket(cinfo))==INVALID_FILE_DESCRIPTOR) ||
      (cinfo->addrfamily!=PF_LOCAL && 
#else /* PF_LOCAL */
  if ((
#endif /* PF_LOCAL */
	(cinfo->fd=initializeTcpSocket(cinfo))==INVALID_FILE_DESCRIPTOR))
    LogPrint(LOG_WARNING,"Error while initializing socket %"PRIdPTR,num);
  else
    LogPrint(LOG_DEBUG,"socket %"PRIdPTR" established (fd %"PRIFD")",num,cinfo->fd);
  return NULL;
}

static void closeSockets(void *arg)
{
  int i;
  struct socketInfo *info;
  
  for (i=0;i<numSockets;i++) {
    pthread_cancel(socketThreads[i]);
    info=&socketInfo[i];
    if (info->fd>=0) {
      if (closeFileDescriptor(info->fd))
        LogError("closing socket");
      info->fd=INVALID_FILE_DESCRIPTOR;
#ifdef WINDOWS
      if ((info->overl.hEvent)) {
	CloseHandle(info->overl.hEvent);
	info->overl.hEvent = NULL;
      }
#else /* WINDOWS */
#if defined(PF_LOCAL)
      if (info->addrfamily==PF_LOCAL) {
	char *path;
	int lpath=strlen(BRLAPI_SOCKETPATH),lport=strlen(info->port);
	if ((path=malloc(lpath+lport+2))) {
	  memcpy(path,BRLAPI_SOCKETPATH,lpath);
	  memcpy(path+lpath,info->port,lport+1);
	  if (unlink(path))
	    LogError("unlinking local socket");
	  path[lpath]='.';
	  memcpy(path+lpath+1,info->port,lport+1);
	  if (unlink(path))
	    LogError("unlinking local socket lock");
	  free(path);
	}
      }
#endif /* PF_LOCAL */
#endif /* WINDOWS */
    }
    free(info->port);
    info->port = NULL;
    free(info->host);
    info->host = NULL;
  }
}

/* Function: addTtyFds */
/* recursively add fds of ttys */
#ifdef WINDOWS
static void addTtyFds(HANDLE **lpHandles, int *nbAlloc, int *nbHandles, Tty *tty) {
#else /* WINDOWS */
static void addTtyFds(fd_set *fds, int *fdmax, Tty *tty) {
#endif /* WINDOWS */
  {
    Connection *c;
    for (c = tty->connections->next; c != tty->connections; c = c -> next) {
#ifdef WINDOWS
      if (*nbHandles == *nbAlloc) {
	*nbAlloc *= 2;
	*lpHandles = realloc(*lpHandles,*nbAlloc*sizeof(**lpHandles));
      }
      (*lpHandles)[(*nbHandles)++] = c->packet.overl.hEvent;
#else /* WINDOWS */
      if (c->fd>*fdmax) *fdmax = c->fd;
      FD_SET(c->fd,fds);
#endif /* WINDOWS */
    }
  }
  {
    Tty *t;
    for (t = tty->subttys; t; t = t->next)
#ifdef WINDOWS
      addTtyFds(lpHandles, nbAlloc, nbHandles, t);
#else /* WINDOWS */
      addTtyFds(fds,fdmax,t);
#endif /* WINDOWS */
  }
}

/* Function: handleTtyFds */
/* recursively handle ttys' fds */
static void handleTtyFds(fd_set *fds, time_t currentTime, Tty *tty) {
  {
    Connection *c,*next;
    c = tty->connections->next;
    while (c!=tty->connections) {
      int remove = 0;
      next = c->next;
#ifdef WINDOWS
      if (WaitForSingleObject(c->packet.overl.hEvent,0) == WAIT_OBJECT_0)
#else /* WINDOWS */
      if (FD_ISSET(c->fd, fds))
#endif /* WINDOWS */
	remove = processRequest(c, &packetHandlers);
      else remove = c->auth==0 && currentTime-(c->upTime) > UNAUTH_DELAY;
#ifndef WINDOWS
      FD_CLR(c->fd,fds);
#endif /* WINDOWS */
      if (remove) removeFreeConnection(c);
      c = next;
    }
  }
  {
    Tty *t,*next;
    for (t = tty->subttys; t; t = next) {
      next = t->next;
      handleTtyFds(fds,currentTime,t);
    }
  }
  if (tty!=&ttys && tty!=&notty
      && tty->connections->next == tty->connections && !tty->subttys) {
    LogPrint(LOG_DEBUG,"freeing tty %#010x",tty->number);
    pthread_mutex_lock(&connectionsMutex);
    removeTty(tty);
    freeTty(tty);
    pthread_mutex_unlock(&connectionsMutex);
  }
}

/* Function : server */
/* The server thread */
/* Returns NULL in any case */
static void *server(void *arg)
{
  char *hosts = (char *)arg;
  pthread_attr_t attr;
  int i;
  int res;
  struct sockaddr_storage addr;
  socklen_t addrlen;
  Connection *c;
  time_t currentTime;
  fd_set sockset;
  FileDescriptor resfd;
#ifdef WINDOWS
  HANDLE *lpHandles;
  int nbAlloc;
  int nbHandles = 0;
#else /* WINDOWS */
  int fdmax;
  struct timeval tv;
  int n;
#endif /* WINDOWS */


#ifndef WINDOWS
  sigset_t blockedSignals;
  sigemptyset(&blockedSignals);
  sigaddset(&blockedSignals,SIGTERM);
  sigaddset(&blockedSignals,SIGINT);
  sigaddset(&blockedSignals,SIGPIPE);
  sigaddset(&blockedSignals,SIGCHLD);
  sigaddset(&blockedSignals,SIGUSR1);
  if ((res = pthread_sigmask(SIG_BLOCK,&blockedSignals,NULL))!=0) {
    LogPrint(LOG_WARNING,"pthread_sigmask : %s",strerror(res));
    pthread_exit(NULL);
  }
#endif /* WINDOWS */

  socketHosts = splitString(hosts,'+',&numSockets);
  if (numSockets>MAXSOCKETS) {
    LogPrint(LOG_ERR,"too many hosts specified (%d, max %d)",numSockets,MAXSOCKETS);
    pthread_exit(NULL);
  }
  if (numSockets == 0) {
    LogPrint(LOG_INFO,"no hosts specified");
    pthread_exit(NULL);
  }
#ifdef WINDOWS
  nbAlloc = numSockets;
#endif /* WINDOWS */

  pthread_attr_init(&attr);
#ifndef WINDOWS
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
#endif /* WINDOWS */
  /* don't care if it fails */
  pthread_attr_setstacksize(&attr,stackSize);

  for (i=0;i<numSockets;i++)
    socketInfo[i].fd = INVALID_FILE_DESCRIPTOR;

#ifdef WINDOWS
  if ((getaddrinfoProc && WSAStartup(MAKEWORD(2,0), &wsadata))
	|| (!getaddrinfoProc && WSAStartup(MAKEWORD(1,1), &wsadata))) {
    LogWindowsSocketError("Starting socket library");
    pthread_exit(NULL);
  }
#endif /* WINDOWS */

  pthread_cleanup_push(closeSockets,NULL);

  for (i=0;i<numSockets;i++) {
    socketInfo[i].addrfamily=brlapiserver_expandHost(socketHosts[i],&socketInfo[i].host,&socketInfo[i].port);
#ifdef WINDOWS
    if (socketInfo[i].addrfamily != PF_LOCAL) {
#endif /* WINDOWS */
      if ((res = pthread_create(&socketThreads[i],&attr,establishSocket,(void *)(intptr_t)i)) != 0) {
	LogPrint(LOG_WARNING,"pthread_create: %s",strerror(res));
	for (i--;i>=0;i--)
	  pthread_cancel(socketThreads[i]);
	pthread_exit(NULL);
      }
#ifdef WINDOWS
    } else {
      /* Windows doesn't have troubles with local sockets on read-only
       * filesystems, but it has with inter-thread overlapped operations,
       * so call from here */
      establishSocket((void *)i);
    }
#endif /* WINDOWS */
  }

  unauthConnections = 0; unauthConnLog = 0;
  while (1) {
#ifdef WINDOWS
    lpHandles = malloc(nbAlloc * sizeof(*lpHandles));
    nbHandles = 0;
    for (i=0;i<numSockets;i++)
      if (socketInfo[i].fd != INVALID_HANDLE_VALUE)
	lpHandles[nbHandles++] = socketInfo[i].overl.hEvent;
    pthread_mutex_lock(&connectionsMutex);
    addTtyFds(&lpHandles, &nbAlloc, &nbHandles, &notty);
    addTtyFds(&lpHandles, &nbAlloc, &nbHandles, &ttys);
    pthread_mutex_unlock(&connectionsMutex);
    if (!nbHandles) {
      free(lpHandles);
      approximateDelay(1000);
      continue;
    }
    switch (WaitForMultipleObjects(nbHandles, lpHandles, FALSE, 1000)) {
      case WAIT_TIMEOUT: continue;
      case WAIT_FAILED:  LogWindowsError("WaitForMultipleObjects");
    }
    free(lpHandles);
#else /* WINDOWS */
    /* Compute sockets set and fdmax */
    FD_ZERO(&sockset);
    fdmax=0;
    for (i=0;i<numSockets;i++)
      if (socketInfo[i].fd>=0) {
	FD_SET(socketInfo[i].fd, &sockset);
	if (socketInfo[i].fd>fdmax)
	  fdmax = socketInfo[i].fd;
      }
    pthread_mutex_lock(&connectionsMutex);
    addTtyFds(&sockset, &fdmax, &notty);
    addTtyFds(&sockset, &fdmax, &ttys);
    pthread_mutex_unlock(&connectionsMutex);
    tv.tv_sec = 1; tv.tv_usec = 0;
    if ((n=select(fdmax+1, &sockset, NULL, NULL, &tv))<0)
    {
      if (fdmax==0) continue; /* still no server socket */
      LogPrint(LOG_WARNING,"select: %s",strerror(errno));
      break;
    }
#endif /* WINDOWS */
    time(&currentTime);
    for (i=0;i<numSockets;i++) {
      char source[0X100];

#ifdef WINDOWS
      if (socketInfo[i].fd != INVALID_FILE_DESCRIPTOR &&
          WaitForSingleObject(socketInfo[i].overl.hEvent, 0) == WAIT_OBJECT_0) {
        if (socketInfo[i].addrfamily == PF_LOCAL) {
          DWORD foo;
          if (!(GetOverlappedResult(socketInfo[i].fd, &socketInfo[i].overl, &foo, FALSE)))
            LogWindowsError("GetOverlappedResult");
          resfd = socketInfo[i].fd;
          if ((socketInfo[i].fd = initializeLocalSocket(&socketInfo[i])) != INVALID_FILE_DESCRIPTOR)
            LogPrint(LOG_DEBUG,"socket %d re-established (fd %"PRIFD", was %"PRIFD")",i,socketInfo[i].fd,resfd);
          snprintf(source, sizeof(source), BRLAPI_SOCKETPATH "%s", socketInfo[i].port);
        } else {
          if (!ResetEvent(socketInfo[i].overl.hEvent))
            LogWindowsError("ResetEvent in server loop");
#else /* WINDOWS */
      if (socketInfo[i].fd>=0 && FD_ISSET(socketInfo[i].fd, &sockset)) {
#endif /* WINDOWS */
          addrlen = sizeof(addr);
          resfd = (FileDescriptor)accept((SocketDescriptor)socketInfo[i].fd, (struct sockaddr *) &addr, &addrlen);
          if (resfd == INVALID_FILE_DESCRIPTOR) {
            setSocketErrno();
            LogPrint(LOG_WARNING,"accept(%"PRIFD"): %s",socketInfo[i].fd,strerror(errno));
            continue;
          }
          formatAddress(source, sizeof(source), &addr, addrlen);

#ifdef WINDOWS
        }
#endif /* WINDOWS */

        LogPrint(LOG_NOTICE, "BrlAPI connection fd=%"PRIFD" accepted: %s", resfd, source);

        if (unauthConnections>=UNAUTH_MAX) {
          writeError(resfd, BRLERR_CONNREFUSED);
          closeFileDescriptor(resfd);
          if (unauthConnLog==0) LogPrint(LOG_WARNING, "Too many simultaneous unauthorized connections");
          unauthConnLog++;
        } else {
#ifndef WINDOWS
          if (!setBlockingIo(resfd, 0)) {
            LogPrint(LOG_WARNING, "Failed to switch to non-blocking mode: %s",strerror(errno));
            break;
          }
#endif /* WINDOWS */

          c = createConnection(resfd, currentTime);
          if (c==NULL) {
            LogPrint(LOG_WARNING,"Failed to create connection structure");
            closeFileDescriptor(resfd);
          } else {
	    unauthConnections++;
	    addConnection(c, notty.connections);
	    handleNewConnection(c);
	  }
        }
      }
    }

    handleTtyFds(&sockset,currentTime,&notty);
    handleTtyFds(&sockset,currentTime,&ttys);
  }

  pthread_cleanup_pop(1);
  return NULL;
}

/****************************************************************************/
/** MISCELLANEOUS FUNCTIONS                                                **/
/****************************************************************************/

/* Function : initializeUnmaskedKeys */
/* Specify which keys should be passed to the client by default, as soon */
/* as it controls the tty */
/* If client asked for commands, one lets it process routing cursor */
/* and screen-related commands */
/* If the client is interested in braille codes, one passes it nothing */
/* to let the user read the screen in case theree is an error */
static int initializeUnmaskedKeys(Connection *c)
{
  if (c==NULL) return 0;
  if (c->how==BRL_KEYCODES) return 0;
  if (addKeyrange(0,BRLAPI_KEYCODE_MAX,&c->unmaskedKeys)==-1) return -1;
  if (removeKeyrange(BRLAPI_KEY_CMD_NOOP,BRLAPI_KEY_CMD_NOOP|BRLAPI_KEY_FLAGS_MASK,&c->unmaskedKeys)==-1) return -1;
  if (removeKeyrange(BRLAPI_KEY_CMD_SWITCHVT_PREV,BRLAPI_KEY_CMD_SWITCHVT_NEXT|BRLAPI_KEY_FLAGS_MASK,&c->unmaskedKeys)==-1) return -1;
  if (removeKeyrange(BRLAPI_KEY_CMD_RESTARTBRL,BRLAPI_KEY_CMD_RESTARTSPEECH|BRLAPI_KEY_FLAGS_MASK,&c->unmaskedKeys)==-1) return -1;
  if (removeKeyrange(BRLAPI_KEY_CMD_SWITCHVT,BRLAPI_KEY_CMD_SWITCHVT|BRLAPI_KEY_CMD_ARG_MASK|BRLAPI_KEY_FLAGS_MASK,&c->unmaskedKeys)==-1) return -1;
  if (removeKeyrange(BRLAPI_KEY_CMD_PASSAT2,BRLAPI_KEY_CMD_PASSAT2|BRLAPI_KEY_CMD_ARG_MASK|BRLAPI_KEY_FLAGS_MASK,&c->unmaskedKeys)==-1) return -1;
  return 0;
}

/* Function: ttyTerminationHandler */
/* Recursively removes connections */
static void ttyTerminationHandler(Tty *tty)
{
  Tty *t;
  while (tty->connections->next!=tty->connections) removeFreeConnection(tty->connections->next);
  for (t = tty->subttys; t; t = t->next) ttyTerminationHandler(t);
}
/* Function : terminationHandler */
/* Terminates driver */
static void terminationHandler(void)
{
  int res;
  if ((res = pthread_cancel(serverThread)) != 0 )
    LogPrint(LOG_WARNING,"pthread_cancel: %s",strerror(res));
  ttyTerminationHandler(&notty);
  ttyTerminationHandler(&ttys);
  if (authDescriptor)
    authEnd(authDescriptor);
  authDescriptor = NULL;
#ifdef WINDOWS
  WSACleanup();
#endif /* WINDOWS */
}

/* Function: whoFillsTty */
/* Returns the connection which fills the tty */
static Connection *whoFillsTty(Tty *tty) {
  Connection *c;
  Tty *t;
  for (c=tty->connections->next; c!=tty->connections; c = c->next)
    if (c->brlbufstate!=EMPTY) goto found;

  c = NULL;
found:
  for (t = tty->subttys; t; t = t->next)
    if (tty->focus==-1 || t->number == tty->focus) {
      Connection *recur_c = whoFillsTty(t);
      return recur_c ? recur_c : c;
    }
  return c;
}

static inline void setCurrentRootTty(void) {
  ttys.focus = currentVirtualTerminal();
}

/* Function : api_writeWindow */
static void api_writeWindow(BrailleDisplay *brl)
{
  setCurrentRootTty();
  pthread_mutex_lock(&connectionsMutex);
  pthread_mutex_lock(&rawMutex);
  if (!suspendConnection && !rawConnection && !whoFillsTty(&ttys)) {
    last_conn_write=NULL;
    pthread_mutex_lock(&driverMutex);
    trueBraille->writeWindow(brl);
    pthread_mutex_unlock(&driverMutex);
  }
  pthread_mutex_unlock(&rawMutex);
  pthread_mutex_unlock(&connectionsMutex);
}

/* Function : api_writeVisual */
static void api_writeVisual(BrailleDisplay *brl)
{
  setCurrentRootTty();
  pthread_mutex_lock(&connectionsMutex);
  pthread_mutex_lock(&rawMutex);
  if (trueBraille->writeVisual &&
      !suspendConnection && !rawConnection && !whoFillsTty(&ttys)) {
    last_conn_write=NULL;
    pthread_mutex_lock(&driverMutex);
    trueBraille->writeVisual(brl);
    pthread_mutex_unlock(&driverMutex);
  }
  pthread_mutex_unlock(&rawMutex);
  pthread_mutex_unlock(&connectionsMutex);
}

brl_keycode_t coreToClient(unsigned long keycode, int how) {
  if (how == BRL_KEYCODES) {
    return keycode;
  } else {
    brl_keycode_t code;
    switch (keycode & BRL_MSK_BLK) {
    case BRL_BLK_PASSCHAR: {
      wchar_t wc = convertCharToWchar(keycode & BRL_MSK_ARG);
      if (wc < 0x100)
        /* latin1 character */
        code = wc;
      else
        /* unicode character */
        code = BRLAPI_KEY_SYM_UC | wc;
      break;
    }
    case BRL_BLK_PASSKEY:
      switch (keycode & BRL_MSK_ARG) {
      case BRL_KEY_ENTER:		code = BRLAPI_KEY_SYM_LINEFEED;break;
      case BRL_KEY_TAB:			code = BRLAPI_KEY_SYM_TAB;	break;
      case BRL_KEY_BACKSPACE:		code = BRLAPI_KEY_SYM_BACKSPACE;break;
      case BRL_KEY_ESCAPE:		code = BRLAPI_KEY_SYM_ESCAPE;	break;
      case BRL_KEY_CURSOR_LEFT:		code = BRLAPI_KEY_SYM_LEFT;	break;
      case BRL_KEY_CURSOR_RIGHT:	code = BRLAPI_KEY_SYM_RIGHT;	break;
      case BRL_KEY_CURSOR_UP:		code = BRLAPI_KEY_SYM_UP;	break;
      case BRL_KEY_CURSOR_DOWN:		code = BRLAPI_KEY_SYM_DOWN;	break;
      case BRL_KEY_PAGE_UP:		code = BRLAPI_KEY_SYM_PAGE_UP;	break;
      case BRL_KEY_PAGE_DOWN:		code = BRLAPI_KEY_SYM_PAGE_DOWN;break;
      case BRL_KEY_HOME:		code = BRLAPI_KEY_SYM_HOME;	break;
      case BRL_KEY_END:			code = BRLAPI_KEY_SYM_END;	break;
      case BRL_KEY_INSERT:		code = BRLAPI_KEY_SYM_INSERT;	break;
      case BRL_KEY_DELETE:		code = BRLAPI_KEY_SYM_DELETE;	break;
      default: code = BRLAPI_KEY_SYM_FUNCTION + (keycode & BRL_MSK_ARG) - BRL_KEY_FUNCTION; break;
      }
      break;
    default:
      code = BRLAPI_KEY_TYPE_CMD | ((keycode & BRL_MSK_BLK) << 8)
        | (keycode & BRL_MSK_ARG);
      break;
    }
    if ((keycode & BRL_MSK_BLK) == BRL_BLK_GOTOLINE)
      code = code
      | (keycode & BRL_FLG_LINE_SCALED	? BRLAPI_KEY_FLG_LINE_SCALED	: 0)
      | (keycode & BRL_FLG_LINE_TOLEFT	? BRLAPI_KEY_FLG_LINE_TOLEFT	: 0)
        ;
    if ((keycode & BRL_MSK_BLK) == BRL_BLK_PASSCHAR
     || (keycode & BRL_MSK_BLK) == BRL_BLK_PASSKEY)
      code = code
      | (keycode & BRL_FLG_CHAR_CONTROL	? BRLAPI_KEY_FLG_CONTROL	: 0)
      | (keycode & BRL_FLG_CHAR_META	? BRLAPI_KEY_FLG_META		: 0)
      | (keycode & BRL_FLG_CHAR_UPPER	? BRLAPI_KEY_FLG_UPPER		: 0)
      | (keycode & BRL_FLG_CHAR_SHIFT	? BRLAPI_KEY_FLG_SHIFT		: 0)
        ;
    else
      code = code
      | (keycode & BRL_FLG_TOGGLE_ON	? BRLAPI_KEY_FLG_TOGGLE_ON	: 0)
      | (keycode & BRL_FLG_TOGGLE_OFF	? BRLAPI_KEY_FLG_TOGGLE_OFF	: 0)
      | (keycode & BRL_FLG_ROUTE	? BRLAPI_KEY_FLG_ROUTE		: 0)
        ;
    return code
    | (keycode & BRL_FLG_REPEAT_INITIAL	? BRLAPI_KEY_FLG_REPEAT_INITIAL	: 0)
    | (keycode & BRL_FLG_REPEAT_DELAY	? BRLAPI_KEY_FLG_REPEAT_DELAY	: 0)
      ;
  }
}
/* Function: whoGetsKey */
/* Returns the connection which gets that key */
static Connection *whoGetsKey(Tty *tty, brl_keycode_t code, unsigned int how)
{
  Connection *c;
  Tty *t;
  int passKey;
  for (c=tty->connections->next; c!=tty->connections; c = c->next) {
    pthread_mutex_lock(&c->maskMutex);
    passKey = (c->how==how) && (inKeyrangeList(c->unmaskedKeys,code) != NULL);
    pthread_mutex_unlock(&c->maskMutex);
    if (passKey) goto found;
  }
  c = NULL;
found:
  for (t = tty->subttys; t; t = t->next)
    if (tty->focus==-1 || t->number == tty->focus) {
      Connection *recur_c = whoGetsKey(t, code, how);
      return recur_c ? recur_c : c;
    }
  return c;
}

/* Function : api_readCommand */
static int api_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext caller)
{
  int res;
  ssize_t size;
  Connection *c;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  int keycode, command = EOF;
  brl_keycode_t clientCode;

  pthread_mutex_lock(&connectionsMutex);
  pthread_mutex_lock(&rawMutex);
  if (suspendConnection) {
    pthread_mutex_unlock(&rawMutex);
    goto out;
  }
  if (rawConnection!=NULL) {
    pthread_mutex_lock(&driverMutex);
    size = trueBraille->readPacket(brl, packet,BRLAPI_MAXPACKETSIZE);
    pthread_mutex_unlock(&driverMutex);
    if (size<0)
      writeException(rawConnection->fd, BRLERR_DRIVERERROR, BRLPACKET_PACKET, NULL, 0);
    else if (size)
      brlapiserver_writePacket(rawConnection->fd,BRLPACKET_PACKET,packet,size);
    pthread_mutex_unlock(&rawMutex);
    goto out;
  }
  setCurrentRootTty();
  c = whoFillsTty(&ttys);
  if (c && last_conn_write!=c) {
    last_conn_write=c;
    refresh=1;
  }
  if (c) {
    pthread_mutex_lock(&c->brlMutex);
    pthread_mutex_lock(&driverMutex);
    if (!driverOpened) {
      if (!resumeDriver(brl)) {
	pthread_mutex_unlock(&driverMutex);
	pthread_mutex_unlock(&c->brlMutex);
        pthread_mutex_unlock(&rawMutex);
	goto out;
      }
      refresh=1;
    }
    pthread_mutex_unlock(&driverMutex);
    if ((c->brlbufstate==TODISPLAY) || (refresh)) {
      unsigned char *oldbuf = disp->buffer, buf[displaySize];
      disp->buffer = buf;
      if (trueBraille->writeVisual) {
        getText(&c->brailleWindow, buf);
        brl->cursor = c->brailleWindow.cursor-1;
	pthread_mutex_lock(&driverMutex);
        trueBraille->writeVisual(brl);
	pthread_mutex_unlock(&driverMutex);
      }
      getDots(&c->brailleWindow, buf);
      pthread_mutex_lock(&driverMutex);
      trueBraille->writeWindow(brl);
      pthread_mutex_unlock(&driverMutex);
      disp->buffer = oldbuf;
      c->brlbufstate = FULL;
      refresh=0;
    }
    pthread_mutex_unlock(&c->brlMutex);
  } else {
    /* no RAW, no connection filling tty, hence suspend if needed */
    pthread_mutex_lock(&driverMutex);
    if (!coreActive) {
      if (driverOpened) suspendDriver(brl);
      pthread_mutex_unlock(&driverMutex);
      pthread_mutex_unlock(&rawMutex);
      goto out;
    }
    pthread_mutex_unlock(&driverMutex);
  }
  if (trueBraille->readKey) {
    pthread_mutex_lock(&driverMutex);
    res = trueBraille->readKey(brl);
    pthread_mutex_unlock(&driverMutex);
    if (brl->resizeRequired)
      handleResize(brl);
    keycode = res;
    pthread_mutex_lock(&driverMutex);
    command = trueBraille->keyToCommand(brl,caller,keycode);
    pthread_mutex_unlock(&driverMutex);
  } else {
    /* we already ensured in ENTERTTYMODE that no connection has how == KEYCODES */
    pthread_mutex_lock(&driverMutex);
    res = trueBraille->readCommand(brl,caller);
    pthread_mutex_unlock(&driverMutex);
    if (brl->resizeRequired)
      handleResize(brl);
    keycode = EOF;
    command = res;
  }
  /* some client may get raw mode only from now */
  pthread_mutex_unlock(&rawMutex);
  clientCode = coreToClient(keycode, BRL_KEYCODES);
  if (trueBraille->readKey && keycode != EOF && (c = whoGetsKey(&ttys,clientCode,BRL_KEYCODES))) {
    /* somebody gets the raw code */
    LogPrint(LOG_DEBUG,"Transmitting unmasked key %lu",(unsigned long)keycode);
    writeKey(c->fd,clientCode);
    command = EOF;
  } else {
    handleAutorepeat(&command, &repeatState);
    if (command != EOF) {
      clientCode = coreToClient(command, BRL_COMMANDS);
      LogPrint(LOG_DEBUG,"client code %016"BRLAPI_PRIxKEYCODE, clientCode);
      /* nobody needs the raw code */
      if ((c = whoGetsKey(&ttys,clientCode,BRL_COMMANDS))) {
        LogPrint(LOG_DEBUG,"Transmitting unmasked command %lx",(unsigned long)command);
	writeKey(c->fd,clientCode);
        command = EOF;
      }
    }
  }
out:
  pthread_mutex_unlock(&connectionsMutex);
  return command;
}

void api_flush(BrailleDisplay *brl, BRL_DriverCommandContext caller) {
  (void) api_readCommand(brl, caller);
}

int api_resume(BrailleDisplay *brl) {
  /* core is resuming or opening the device for the first time, let's try to go
   * to normal state */
  pthread_mutex_lock(&rawMutex);
  pthread_mutex_lock(&driverMutex);
  if (!suspendConnection && !driverOpened) {
    resumeDriver(brl);
    if (driverOpened) {
      /* TODO: handle clients' resize */
      displayDimensions[0] = htonl(brl->x);
      displayDimensions[1] = htonl(brl->y);
      displaySize = brl->x * brl->y;
      disp = brl;
    }
  }
  pthread_mutex_unlock(&driverMutex);
  pthread_mutex_unlock(&rawMutex);
  return (coreActive = driverOpened);
}

/* try to get access to device. If suspended, returns 0 */
int api_claimDriver (BrailleDisplay *brl)
{
  pthread_mutex_lock(&suspendMutex);
  if (driverOpened) return 1;
  pthread_mutex_unlock(&suspendMutex);
  return 0;
}

void api_releaseDriver(BrailleDisplay *brl)
{
  pthread_mutex_unlock(&suspendMutex);
}

void api_suspend(BrailleDisplay *brl) {
  /* core is suspending, going to core suspend state */
  coreActive = 0;
  /* we let core's call to api_flush() go to full suspend state */
}

/* Function : api_link */
/* Does all the link stuff to let api get events from the driver and */
/* writes from brltty */
void api_link(BrailleDisplay *brl)
{
  LogPrint(LOG_DEBUG, "api link");
  resetRepeatState(&repeatState);
  trueBraille=braille;
  refresh=1;
  memcpy(&ApiBraille,braille,sizeof(BrailleDriver));
  ApiBraille.writeWindow=api_writeWindow;
  ApiBraille.writeVisual=api_writeVisual;
  ApiBraille.readCommand=api_readCommand;
  ApiBraille.readKey = NULL;
  ApiBraille.keyToCommand = NULL;
  ApiBraille.readPacket = NULL;
  ApiBraille.writePacket = NULL;
  braille=&ApiBraille;
  /* TODO: handle clients' resize */
  displayDimensions[0] = htonl(brl->x);
  displayDimensions[1] = htonl(brl->y);
  displaySize = brl->x * brl->y;
  disp = brl;
}

/* Function : api_unlink */
/* Does all the unlink stuff to remove api from the picture */
void api_unlink(BrailleDisplay *brl)
{
  LogPrint(LOG_DEBUG, "api unlink");
  braille=trueBraille;
  trueBraille=&noBraille;
  pthread_mutex_lock(&driverMutex);
  if (!coreActive && driverOpened)
    suspendDriver(disp);
  pthread_mutex_unlock(&driverMutex);
}

/* Function : api_identify */
/* Identifies BrlApi */
void api_identify(int full)
{
  LogPrint(LOG_NOTICE, RELEASE);
  if (full) {
    LogPrint(LOG_INFO, COPYRIGHT);
  }
}

/* Function : api_start */
/* Initializes BrlApi */
/* One first initialize the driver */
/* Then one creates the communication socket */
int api_start(BrailleDisplay *brl, char **parameters)
{
  int res,i;

  char *hosts=
#if defined(PF_LOCAL)
	":0+127.0.0.1:0";
#else /* PF_LOCAL */
	"127.0.0.1:0";
#endif /* PF_LOCAL */

  {
    const char *parameter = parameters[PARM_AUTH];
    if (!parameter || !*parameter) auth = BRLAPI_DEFAUTH;
    else auth = parameter;
  }

  if (auth && auth[0] != '/') 
    if (!(authDescriptor = authBeginServer(auth))) return 0;

  pthread_attr_t attr;
  pthread_mutexattr_t mattr;

  coreActive=driverOpened=1;

  if ((notty.connections = createConnection(INVALID_FILE_DESCRIPTOR,0)) == NULL) {
    LogPrint(LOG_WARNING, "Unable to create connections list");
    goto out;
  }
  notty.connections->prev = notty.connections->next = notty.connections;
  if ((ttys.connections = createConnection(INVALID_FILE_DESCRIPTOR, 0)) == NULL) {
    LogPrint(LOG_WARNING, "Unable to create ttys' connections list");
    goto outalloc;
  }
  ttys.connections->prev = ttys.connections->next = ttys.connections;
  ttys.focus = -1;

  if (*parameters[PARM_HOST]) hosts = parameters[PARM_HOST];

  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&connectionsMutex,&mattr);
  pthread_mutex_init(&driverMutex,&mattr);
  pthread_mutex_init(&rawMutex,&mattr);
  pthread_mutex_init(&suspendMutex,&mattr);

  stackSize = MAX(PTHREAD_STACK_MIN, OUR_STACK_MIN);
  {
    const char *operand = parameters[PARM_STACKSIZE];
    if (*operand) {
      int size;
      const int minSize = PTHREAD_STACK_MIN;
      if (validateInteger(&size, operand, &minSize, NULL)) {
        stackSize = size;
      } else {
        LogPrint(LOG_WARNING, "%s: %s", "invalid thread stack size", operand);
      }
    }
  }

  pthread_attr_init(&attr);
#ifndef WINDOWS
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
#endif /* WINDOWS */
  /* don't care if it fails */
  pthread_attr_setstacksize(&attr,stackSize);

  trueBraille=&noBraille;
  if ((res = pthread_create(&serverThread,&attr,server,hosts)) != 0) {
    LogPrint(LOG_WARNING,"pthread_create: %s",strerror(res));
    for (i=0;i<numSockets;i++)
      pthread_cancel(socketThreads[i]);
    goto outallocs;
  }

  return 1;
  
outallocs:
  freeConnection(ttys.connections);
outalloc:
  freeConnection(notty.connections);
out:
  authEnd(authDescriptor);
  authDescriptor = NULL;
  return 0;
}

/* Function : api_stop */
/* End of BrlApi session. Closes the listening socket */
/* destroys opened connections and associated resources */
/* Closes the driver */
void api_stop(BrailleDisplay *brl)
{
  terminationHandler();
}
