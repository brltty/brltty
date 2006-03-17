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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <wchar.h>
#include <inttypes.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

#ifdef WINDOWS
#include <ws2tcpip.h>

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
#include "rangelist.h"
#include "cmd.h"
#include "brl.h"
#include "brltty.h"
#include "misc.h"
#include "auth.h"
#include "io_misc.h"
#include "scr.h"
#include "tunes.h"

#ifdef WINDOWS
#define closeFd(fd) CloseHandle((HANDLE)(fd))
#define LogSocketError(msg) LogWindowsSocketError(msg)
#else /* WINDOWS */
#define closeFd(fd) close(fd)
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
  PARM_KEYFILE,
  PARM_STACKSIZE
} Parameters;

const char *const api_parameters[] = { "auth", "host", "keyfile", "stacksize", NULL };

static size_t stackSize;

#define RELEASE "BRLTTY API Library: release " BRLAPI_RELEASE
#define COPYRIGHT "   Copyright (C) 2002-2006 by Sebastien Hinderer <Sebastien.Hinderer@ens-lyon.org>, \
Samuel Thibault <samuel.thibault@ens-lyon.org>"

/* These CHECK* macros check whether a condition is true, and, if not, */
/* send back either a non-fatal error, or an exception */
#define CHECKERR(condition, error) \
if (!( condition )) { \
  writeError(c->fd, error); \
  return 0; \
} else { }
#define CHECKEXC(condition, error) \
if (!( condition )) { \
  writeException(c->fd, error, type, packet, size); \
  return 0; \
} else { }

#define WERR(x, y) writeError(x, y)
#define WEXC(x, y) writeException(x, y, type, packet, size)

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
  unsigned char content[BRLAPI_MAXPACKETSIZE];
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
  int fd;
  int auth;
  struct Tty *tty;
  int raw;
  unsigned int how; /* how keys must be delivered to clients */
  BrailleWindow brailleWindow;
  BrlBufState brlbufstate;
  pthread_mutex_t brlMutex;
  RangeList *unmaskedKeys;
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
  int fd;
  char *hostname;
  char *port;
#ifdef WINDOWS
  OVERLAPPED overl;
#endif /* WINDOWS */
} socketInfo[MAXSOCKETS]; /* information for cleaning sockets */
static int numSockets; /* number of sockets */
#ifdef WINDOWS
static pthread_t socketSelectThread;
#endif /* WINDOWS */

/* Protects from connection addition / remove from the server thread */
static pthread_mutex_t connectionsMutex;

/* Protects the real driver's functions */
static pthread_mutex_t driverMutex;

/* Which connection currently has raw mode */
static pthread_mutex_t rawMutex;
static Connection *rawConnection = NULL;

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

static int unauthorizedConnectionsAllowed = 0;
static char *keyfile;

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

/* Function : driverSuspend */
/* Close driver */
static void driverSuspend(BrailleDisplay *brl) {
  if (trueBraille == &noBraille) return; /* core unlinked api */
  LogPrint(LOG_DEBUG,"driver suspended");
  driverOpened = 0;
  closeBrailleDriver();
}

/* Function : driverResume */
/* Re-open driver */
static int driverResume(BrailleDisplay *brl) {
  if (trueBraille == &noBraille) return 0; /* core unlinked api */
  driverOpened = openBrailleDriver();
  if (driverOpened) LogPrint(LOG_DEBUG,"driver resumed");
  return driverOpened;
}

/****************************************************************************/
/** PACKET HANDLING                                                        **/
/****************************************************************************/

/* Function : writeAck */
/* Sends an acknowledgement on the given socket */
static inline void writeAck(int fd)
{
  brlapiserver_writePacket(fd,BRLPACKET_ACK,NULL,0);
}

/* Function : writeError */
/* Sends the given non-fatal error on the given socket */
static void writeError(int fd, unsigned int err)
{
  uint32_t code = htonl(err);
  LogPrint(LOG_DEBUG,"error %u on fd %d", err, fd);
  brlapiserver_writePacket(fd,BRLPACKET_ERROR,&code,sizeof(code));
}

/* Function : writeException */
/* Sends the given error code on the given socket */
static void writeException(int fd, unsigned int err, brl_type_t type, const void *packet, size_t size)
{
  int hdrsize, esize;
  unsigned char epacket[BRLAPI_MAXPACKETSIZE];
  errorPacket_t * errorPacket = (errorPacket_t *) epacket;
  LogPrint(LOG_DEBUG,"exception %u for packet type %lu on fd %d", err, (unsigned long)type, fd);
  hdrsize = sizeof(errorPacket->code)+sizeof(errorPacket->type);
  errorPacket->code = htonl(err);
  errorPacket->type = htonl(type);
  esize = MIN(size, BRLAPI_MAXPACKETSIZE-hdrsize);
  if ((packet!=NULL) && (size!=0)) memcpy(&errorPacket->packet, packet, esize);
  brlapiserver_writePacket(fd,BRLPACKET_EXCEPTION,epacket, hdrsize+esize);
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
    if (!GetOverlappedResult((HANDLE) c->fd,&packet->overl,&res,FALSE)) {
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
  if (!ReadFile((HANDLE)c->fd, packet->p, packet->n, &res, &packet->overl)) {
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
  PacketHandler getTty;
  PacketHandler setFocus;
  PacketHandler leaveTty;
  PacketHandler ignoreKeyRange;
  PacketHandler unignoreKeyRange;
  PacketHandler ignoreKeySet;
  PacketHandler unignoreKeySet;
  PacketHandler write;
  PacketHandler getRaw;  
  PacketHandler leaveRaw;
  PacketHandler packet;
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
  int i;
  wchar_t wc;
  for (i=0; i<displaySize; i++) {
    if ((wc = brailleWindow->text[i]) < 0x100)
      buf[i] = textTable[wc];
    else
      if ((wc >= BRL_UC_ROW) && (wc <= (BRL_UC_ROW | 0XFF)))
	buf[i] =
	  (wc&(1<<(1-1))?BRL_DOT1:0) |
	  (wc&(1<<(2-1))?BRL_DOT2:0) |
	  (wc&(1<<(3-1))?BRL_DOT3:0) |
	  (wc&(1<<(4-1))?BRL_DOT4:0) |
	  (wc&(1<<(5-1))?BRL_DOT5:0) |
	  (wc&(1<<(6-1))?BRL_DOT6:0) |
	  (wc&(1<<(7-1))?BRL_DOT7:0) |
	  (wc&(1<<(8-1))?BRL_DOT8:0);
      else
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
/* Creates a connectiN */
static Connection *createConnection(int fd, time_t currentTime)
{
  pthread_mutexattr_t mattr;
  Connection *c =  malloc(sizeof(Connection));
  if (c==NULL) goto out;
  c->auth = 0;
  c->fd = fd;
  c->tty = NULL;
  c->raw = 0;
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
  closeFd(fd);
  return NULL;
}

/* Function : freeConnection */
/* Frees all resources associated to a connection */
static void freeConnection(Connection *c)
{
  if (c->fd>=0) closeFd(c->fd);
  pthread_mutex_destroy(&c->brlMutex);
  pthread_mutex_destroy(&c->maskMutex);
  freeBrailleWindow(&c->brailleWindow);
  freeRangeList(&c->unmaskedKeys);
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
  if (!(tty->connections = createConnection(-1,0))) goto outtty;
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
static inline void LogPrintRequest(int type, int fd)
{
  LogPrint(LOG_DEBUG, "Received %s request on fd %d", brlapiserver_packetType(type), fd);  
}

static int handleGetDriver(Connection *c, brl_type_t type, size_t size, const char *str)
{
  int len = strlen(str);
  LogPrintRequest(type, c->fd);
  CHECKERR(size==0,BRLERR_INVALID_PACKET);
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  brlapiserver_writePacket(c->fd, type, str, len+1);
  return 0;
}

static int handleGetDriverId(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  return handleGetDriver(c, type, size, braille->code);
}

static int handleGetDriverName(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  return handleGetDriver(c, type, size, braille->name);
}

static int handleGetDisplaySize(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(size==0,BRLERR_INVALID_PACKET);
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  brlapiserver_writePacket(c->fd,BRLPACKET_GETDISPLAYSIZE,&displayDimensions[0],sizeof(displayDimensions));
  return 0;
}

static int handleGetTty(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
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
  CHECKERR((!c->raw),BRLERR_ILLEGAL_INSTRUCTION);
  CHECKERR(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
  p += sizeof(uint32_t); remaining -= sizeof(uint32_t);
  nbTtys = ntohl(ints[0]);
  CHECKERR(remaining>=nbTtys*sizeof(uint32_t), BRLERR_INVALID_PACKET);
  p += nbTtys*sizeof(uint32_t); remaining -= nbTtys*sizeof(uint32_t);
  CHECKERR(*p<=BRLAPI_MAXNAMELENGTH, BRLERR_INVALID_PARAMETER);
  n = *p; p++; remaining--;
  CHECKERR(remaining==n, BRLERR_INVALID_PACKET);
  memcpy(name, p, n);
  name[n] = '\0';
  if (!*name) how = BRL_COMMANDS; else {
    if ((!strcmp(name, trueBraille->name)) && (isKeyCapable(trueBraille)))
      how = BRL_KEYCODES;
    else how = -1;
  }
  CHECKERR(((how == BRL_KEYCODES) || (how == BRL_COMMANDS)),BRLERR_OPNOTSUPP);
  c->how = how;
  freeBrailleWindow(&c->brailleWindow); /* In case of multiple gettty requests */
  if ((initializeUnmaskedKeys(c)==-1) || (allocBrailleWindow(&c->brailleWindow)==-1)) {
    LogPrint(LOG_WARNING,"Failed to allocate some ressources");
    freeRangeList(&c->unmaskedKeys);
    WERR(c->fd,BRLERR_NOMEM);
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
      WERR(c->fd, BRLERR_INVALID_PARAMETER);
      freeBrailleWindow(&c->brailleWindow);
      return 0;
    }
    /* ok, allocate path */
    /* we lock the entire subtree for easier cleanup */
    if (!(tty2 = newTty(tty,ntohl(*ptty)))) {
      pthread_mutex_unlock(&connectionsMutex);
      WERR(c->fd,BRLERR_NOMEM);
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
        WERR(c->fd,BRLERR_NOMEM);
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
        LogPrint(LOG_WARNING,"One already controls tty %#010x !",c->tty->number);
        WERR(c->fd, BRLERR_ILLEGAL_INSTRUCTION);
      } else {
        /* Here one is in the case where the client tries to change */
        /* from BRL_KEYCODES to BRL_COMMANDS, or something like that */
        /* For the moment this operation is not supported */
        /* A client that wants to do that should first LeaveTty() */
        /* and then get it again, risking to lose it */
        LogPrint(LOG_INFO,"Switching from BRL_KEYCODES to BRL_COMMANDS not supported yet");
        WERR(c->fd,BRLERR_OPNOTSUPP);
      }
      return 0;
    } else {
      /* uhu, we already got a tty, but not this one: this is forbidden. */
      WERR(c->fd, BRLERR_INVALID_PARAMETER);
      return 0;
    }
  }
  c->tty = tty;
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
  CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  CHECKEXC(c->tty,BRLERR_ILLEGAL_INSTRUCTION);
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
  freeRangeList(&c->unmaskedKeys);
  freeBrailleWindow(&c->brailleWindow);
}

static int handleLeaveTty(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  CHECKERR(c->tty,BRLERR_ILLEGAL_INSTRUCTION);
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
  CHECKERR(( (!c->raw) && c->tty ),BRLERR_ILLEGAL_INSTRUCTION);
  CHECKERR(size==2*sizeof(brl_keycode_t),BRLERR_INVALID_PACKET);
  x = ntohl(ints[0]);
  y = ntohl(ints[1]);
  LogPrint(LOG_DEBUG,"range: [%lu..%lu]",(unsigned long)x,(unsigned long)y);
  pthread_mutex_lock(&c->maskMutex);
  if (type==BRLPACKET_IGNOREKEYRANGE) res = removeRange(x,y,&c->unmaskedKeys);
  else res = addRange(x,y,&c->unmaskedKeys);
  pthread_mutex_unlock(&c->maskMutex);
  if (res==-1) WERR(c->fd,BRLERR_NOMEM);
  else writeAck(c->fd);
  return 0;
}

static int handleKeySet(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  int i = 0, res = 0;
  unsigned int nbkeys;
  brl_keycode_t *k = (brl_keycode_t *) packet;
  int (*fptr)(uint32_t, uint32_t, RangeList **);
  LogPrintRequest(type, c->fd);
  if (type==BRLPACKET_IGNOREKEYSET) fptr = removeRange; else fptr = addRange;
  CHECKERR(( (!c->raw) && c->tty ),BRLERR_ILLEGAL_INSTRUCTION);
  CHECKERR(size % sizeof(brl_keycode_t)==0,BRLERR_INVALID_PACKET);
  nbkeys = size/sizeof(brl_keycode_t);
  pthread_mutex_lock(&c->maskMutex);
  while ((res!=-1) && (i<nbkeys)) {
    res = fptr(k[i],k[i],&c->unmaskedKeys);
    i++;
  }
  pthread_mutex_unlock(&c->maskMutex);
  if (res==-1) WERR(c->fd,BRLERR_NOMEM);
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
  unsigned char *charset = NULL;
  unsigned int charsetLen = 0;
#endif /* HAVE_ICONV_H */
  LogPrintRequest(type, c->fd);
  CHECKEXC(remaining>=sizeof(ws->flags), BRLERR_INVALID_PACKET);
  CHECKEXC(((!c->raw)&&c->tty),BRLERR_ILLEGAL_INSTRUCTION);
  ws->flags = ntohl(ws->flags);
  if ((remaining==sizeof(ws->flags))&&(ws->flags==0)) {
    c->brlbufstate = EMPTY;
    return 0;
  }
  remaining -= sizeof(ws->flags); /* flags */
  CHECKEXC((ws->flags & BRLAPI_WF_DISPLAYNUMBER)==0, BRLERR_OPNOTSUPP);
  if (ws->flags & BRLAPI_WF_REGION) {
    CHECKEXC(remaining>2*sizeof(uint32_t), BRLERR_INVALID_PACKET);
    rbeg = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* region begin */
    rsiz = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* region size */
    CHECKEXC(
      (1<=rbeg) && (rsiz>0) && (rbeg+rsiz-1<=displaySize),
      BRLERR_INVALID_PARAMETER);
  } else {
    rbeg = 1;
    rsiz = displaySize;
  }
  if (ws->flags & BRLAPI_WF_TEXT) {
    CHECKEXC(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
    textLen = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* text size */
    CHECKEXC(remaining>=textLen, BRLERR_INVALID_PACKET);
    text = p;
    p += textLen; remaining -= textLen; /* text */
  }
  if (ws->flags & BRLAPI_WF_ATTR_AND) {
    CHECKEXC(remaining>=rsiz, BRLERR_INVALID_PACKET);
    andAttr = p;
    p += rsiz; remaining -= rsiz; /* and attributes */
  }
  if (ws->flags & BRLAPI_WF_ATTR_OR) {
    CHECKEXC(remaining>=rsiz, BRLERR_INVALID_PACKET);
    orAttr = p;
    p += rsiz; remaining -= rsiz; /* or attributes */
  }
  if (ws->flags & BRLAPI_WF_CURSOR) {
    CHECKEXC(remaining>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
    cursor = ntohl( *((uint32_t *) p) );
    p += sizeof(uint32_t); remaining -= sizeof(uint32_t); /* cursor */
    CHECKEXC(cursor<=displaySize, BRLERR_INVALID_PACKET);
  }
#ifdef HAVE_ICONV_H
  if (ws->flags & BRLAPI_WF_CHARSET) {
    CHECKEXC(ws->flags & BRLAPI_WF_TEXT, BRLERR_INVALID_PACKET);
    CHECKEXC(remaining>=1, BRLERR_INVALID_PACKET);
    charsetLen = *p++; remaining--; /* charset length */
    CHECKEXC(remaining>=charsetLen, BRLERR_INVALID_PACKET);
    charset = p;
    p += charsetLen; remaining -= charsetLen; /* charset name */
  }
#else /* HAVE_ICONV_H */
  CHECKEXC(!(ws->flags & BRLAPI_WF_CHARSET), BRLERR_OPNOTSUPP);
#endif /* HAVE_ICONV_H */
  CHECKEXC(remaining==0, BRLERR_INVALID_PACKET);
  /* Here the whole packet has been checked */
  if (text) {
#ifdef HAVE_ICONV_H
    if (charset) {
      char _charset[charsetLen+1];
      iconv_t conv;
      wchar_t textBuf[rsiz];
      char *in = (char *) text, *out = (char *) textBuf;
      size_t sin = textLen, sout = sizeof(textBuf), res;
      memcpy(_charset,charset,charsetLen);
      _charset[charsetLen] = '\0';
      CHECKEXC((conv = iconv_open("WCHAR_T",_charset)) != (iconv_t)(-1), BRLERR_INVALID_PACKET);
      res = iconv(conv,&in,&sin,&out,&sout);
      iconv_close(conv);
      CHECKEXC(res != (size_t) -1 && !sin && !sout, BRLERR_INVALID_PACKET);
      pthread_mutex_lock(&c->brlMutex);
      memcpy(c->brailleWindow.text+rbeg-1,textBuf,rsiz*sizeof(wchar_t));
    } else
#endif /* HAVE_ICONV_H */
    {
      int i;
      pthread_mutex_lock(&c->brlMutex);
      for (i=0; i<rsiz; i++)
        c->brailleWindow.text[rbeg-1+i] = text[i];
    }
  } else pthread_mutex_lock(&c->brlMutex);
  if (andAttr) memcpy(c->brailleWindow.andAttr+rbeg-1,andAttr,rsiz);
  if (orAttr) memcpy(c->brailleWindow.orAttr+rbeg-1,orAttr,rsiz);
  if (cursor>=0) c->brailleWindow.cursor = cursor;
  c->brlbufstate = TODISPLAY;
  pthread_mutex_unlock(&c->brlMutex);
  return 0;
}

static int handleGetRaw(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  getRawPacket_t *getRawPacket = (getRawPacket_t *) packet;
  char name[BRLAPI_MAXNAMELENGTH+1];
  int remaining = size;
  LogPrintRequest(type, c->fd);
  CHECKERR(!c->raw, BRLERR_ILLEGAL_INSTRUCTION);
  CHECKERR(remaining>sizeof(uint32_t), BRLERR_INVALID_PACKET);
  remaining -= sizeof(uint32_t);
  getRawPacket->magic = ntohl(getRawPacket->magic);
  CHECKERR(getRawPacket->magic==BRLRAW_MAGIC,BRLERR_INVALID_PARAMETER);
  CHECKERR(getRawPacket->nameLength<=BRLAPI_MAXNAMELENGTH, BRLERR_INVALID_PARAMETER);
  remaining--;
  CHECKERR(remaining==getRawPacket->nameLength, BRLERR_INVALID_PACKET);
  memcpy(name, &getRawPacket->name, getRawPacket->nameLength);
  name[getRawPacket->nameLength] = '\0';
  CHECKERR(((!strcmp(name, trueBraille->name)) && isRawCapable(trueBraille)), BRLERR_OPNOTSUPP);
  CHECKERR(rawConnection==NULL,BRLERR_RAWMODEBUSY);
  pthread_mutex_lock(&rawMutex);
  pthread_mutex_lock(&driverMutex);
  if (!driverOpened && !driverResume(disp)) {
    WERR(c->fd, BRLERR_DRIVERERROR);
    pthread_mutex_unlock(&driverMutex);
    pthread_mutex_unlock(&rawMutex);
    return 0;
  }
  pthread_mutex_unlock(&driverMutex);
  c->raw = 1;
  rawConnection = c;
  writeAck(c->fd);
  pthread_mutex_unlock(&rawMutex);
  return 0;
}

static int handleLeaveRaw(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKERR(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  LogPrint(LOG_DEBUG,"Going out of raw mode");
  pthread_mutex_lock(&rawMutex);
  c->raw = 0;
  rawConnection = NULL;
  writeAck(c->fd);
  pthread_mutex_unlock(&rawMutex);
  return 0;
}

static int handlePacket(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  LogPrintRequest(type, c->fd);
  CHECKEXC(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
  pthread_mutex_lock(&driverMutex);
  trueBraille->writePacket(disp,packet,size);
  pthread_mutex_unlock(&driverMutex);
  return 0;
}

static PacketHandlers packetHandlers = {
  handleGetDriverId, handleGetDriverName, handleGetDisplaySize,
  handleGetTty, handleSetFocus, handleLeaveTty,
  handleKeyRange, handleKeyRange, handleKeySet, handleKeySet,
  handleWrite,
  handleGetRaw, handleLeaveRaw, handlePacket  
};

static AuthDescriptor *authDescriptor;

/* Function : handleUnauthorizedConnection */
/* Returns 0 if connection is authorized */
/* Returns 1 if connection has to be removed */
static int handleUnauthorizedConnection(Connection *c, brl_type_t type, unsigned char *packet, size_t size)
{
  size_t authKeyLength = 0;
  unsigned char authKey[BRLAPI_MAXPACKETSIZE];
  int authKeyCorrect = 0;
  authStruct *auth = (authStruct *) packet;
  int remaining = size;

  if (type!=BRLPACKET_AUTHKEY) {
    WERR(c->fd, BRLERR_CONNREFUSED);
    return 1;
  }

  if ((remaining<sizeof(auth->protocolVersion))||(ntohl(auth->protocolVersion)!=BRLAPI_PROTOCOL_VERSION)) {
    writeError(c->fd, BRLERR_PROTOCOL_VERSION);
    return 1;
  }
  remaining -= sizeof(auth->protocolVersion);

  if (!unauthorizedConnectionsAllowed) {
    if (brlapiserver_loadAuthKey(keyfile,&authKeyLength,authKey)==-1) {
      LogPrint(LOG_WARNING,"Unable to load API authorization key from %s: %s in %s. You may use parameter keyfile=none if you don't want any authorization (dangerous)", keyfile, strerror(brlapi_libcerrno), brlapi_errfun);
      return 1;
    }
    LogPrint(LOG_DEBUG, "Authorization key loaded");
  }

  authKeyCorrect = (remaining==authKeyLength) && (!memcmp(&auth->key, authKey, authKeyLength));
  memset(authKey, 0, authKeyLength);
  memset(&auth->key, 0, authKeyLength);
  unauthConnections--;
  if (!authKeyCorrect) {
    writeError(c->fd, BRLERR_CONNREFUSED);
    LogPrint(LOG_WARNING, "BrlAPI connection fd=%d failed authorization", c->fd);
    return 1;
  }

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
    if (res==-1) LogPrint(LOG_WARNING,"read : %s (connection on fd %d)",strerror(errno),c->fd);
    else {
      LogPrint(LOG_DEBUG,"Closing connection on fd %d",c->fd);
    }
    if (c->raw) {
      c->raw = 0;
      rawConnection = NULL;
      LogPrint(LOG_WARNING,"Client on fd %d did not give up raw mode properly",c->fd);
      LogPrint(LOG_WARNING,"Trying to reset braille terminal");
      pthread_mutex_lock(&driverMutex);
      if (trueBraille->reset && !trueBraille->reset(disp)) {
        LogPrint(LOG_WARNING,"Reset failed. Restarting braille driver");
        restartBrailleDriver();
      }
      pthread_mutex_unlock(&driverMutex);
    }
    if (c->tty) {
      LogPrint(LOG_DEBUG,"Client on fd %d did not give up control of tty %#010x properly",c->fd,c->tty->number);
      doLeaveTty(c);
    }
    if (c->auth==0) unauthConnections--;
    return 1;
  }
  size = c->packet.header.size;
  type = c->packet.header.type;
  
  if (c->auth==0) return handleUnauthorizedConnection(c, type, packet, size);

  if (size>BRLAPI_MAXPACKETSIZE) {
    LogPrint(LOG_WARNING, "Discarding too large packet of type %s on fd %d",brlapiserver_packetType(type), c->fd);
    return 0;    
  }
  switch (type) {
    case BRLPACKET_GETDRIVERID: p = handlers->getDriverId; break;
    case BRLPACKET_GETDRIVERNAME: p = handlers->getDriverName; break;
    case BRLPACKET_GETDISPLAYSIZE: p = handlers->getDisplaySize; break;
    case BRLPACKET_GETTTY: p = handlers->getTty; break;
    case BRLPACKET_SETFOCUS: p = handlers->setFocus; break;
    case BRLPACKET_LEAVETTY: p = handlers->leaveTty; break;
    case BRLPACKET_IGNOREKEYRANGE: p = handlers->ignoreKeyRange; break;
    case BRLPACKET_UNIGNOREKEYRANGE: p = handlers->unignoreKeyRange; break;
    case BRLPACKET_IGNOREKEYSET: p = handlers->ignoreKeySet; break;
    case BRLPACKET_UNIGNOREKEYSET: p = handlers->unignoreKeySet; break;
    case BRLPACKET_WRITE: p = handlers->write; break;
    case BRLPACKET_GETRAW: p = handlers->getRaw; break;
    case BRLPACKET_LEAVERAW: p = handlers->leaveRaw; break;
    case BRLPACKET_PACKET: p = handlers->packet; break;
  }
  if (p!=NULL) p(c, type, packet, size);
  else WEXC(c->fd,BRLERR_UNKNOWN_INSTRUCTION);
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
static int loopBind(int fd, struct sockaddr *addr, socklen_t len)
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
static int initializeTcpSocket(struct socketInfo *info)
{
  int fd=-1;
  const char *fun;
  int yes=1;

#ifdef HAVE_GETADDRINFO

  int err;
  struct addrinfo *res,*cur;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  err = getaddrinfo(info->hostname, info->port, &hints, &res);
  if (err) {
    LogPrint(LOG_WARNING,"getaddrinfo(%s,%s): "
#ifdef HAVE_GAI_STRERROR
	"%s"
#else /* HAVE_GAI_STRERROR */
	"%d"
#endif /* HAVE_GAI_STRERROR */
	,info->hostname,info->port
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
    return -1;
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
    closeFd(fd);
  }
  freeaddrinfo(res);
  if (cur) {
    free(info->hostname);
    info->hostname = NULL;
    free(info->port);
    info->port = NULL;

#ifdef WINDOWS
    if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
      LogWindowsError("CreateEvent");
      closeFd(fd);
      return -1;
    }
    LogPrint(LOG_DEBUG,"Event -> %p",info->overl.hEvent);
#endif /* WINDOWS */

    return fd;
  }
  LogPrint(LOG_WARNING,"unable to find a local TCP port %s:%s !",info->hostname,info->port);

#else /* HAVE_GETADDRINFO */

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
	return -1;
      }
      addr.sin_port = se->s_port;
    }
  }

  if (!info->hostname) {
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  } else if ((addr.sin_addr.s_addr = inet_addr(info->hostname)) == htonl(INADDR_NONE)) {
    if (!(he = gethostbyname(info->hostname))) {
      LogPrint(LOG_ERR,"gethostbyname(%s): "
#ifdef WINDOWS
	"%d"
#else /* WINDOWS */
	"%s"
#endif /* WINDOWS */
	,info->hostname,
#ifdef WINDOWS
	WSAGetLastError()
#else /* WINDOWS */
	hstrerror(h_errno)
#endif /* WINDOWS */
	);
      return -1;
    }
    if (he->h_addrtype != AF_INET) {
#ifdef EAFNOSUPPORT
      errno = EAFNOSUPPORT;
#else /* EAFNOSUPPORT */
      errno = EINVAL;
#endif /* EAFNOSUPPORT */
      LogPrint(LOG_ERR,"unknown address type %d",he->h_addrtype);
      return -1;
    }
    if (he->h_length > sizeof(addr.sin_addr)) {
      errno = EINVAL;
      LogPrint(LOG_ERR,"too big address: %d",he->h_length);
      return -1;
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
  free(info->hostname);
  info->hostname = NULL;
  free(info->port);
  info->port = NULL;

#ifdef WINDOWS
  if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
    LogWindowsError("CreateEvent");
    closeFd(fd);
    return -1;
  }
  LogPrint(LOG_DEBUG,"Event -> %p",info->overl.hEvent);
#endif /* WINDOWS */

  return fd;

err:
  LogSocketError(fun);
  if (fd >= 0) closeFd(fd);

#endif /* HAVE_GETADDRINFO */

  free(info->hostname);
  info->hostname = NULL;
  free(info->port);
  info->port = NULL;
  return -1;
}

#if defined(PF_LOCAL) && (!defined(WINDOWS) || defined(HAVE_CREATENAMEDPIPE))

#ifndef WINDOWS
static int readPid(char *path)
  /* read pid from specified file. Return 0 on any error */
{
  char pids[16], *ptr;
  pid_t pid;
  int n, fd;
  fd = open(path, O_RDONLY);
  n = read(fd, pids, sizeof(pids)-1);
  closeFd(fd);
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
static int initializeLocalSocket(struct socketInfo *info)
{
  int lpath=strlen(BRLAPI_SOCKETPATH),lport=strlen(info->port);
  int fd;
#ifdef WINDOWS
  char path[lpath+lport+1];
  memcpy(path,BRLAPI_SOCKETPATH,lpath);
  memcpy(path+lpath,info->port,lport+1);
  if ((HANDLE) (fd = (int) CreateNamedPipe(path,
	  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
	  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
	  PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL)) == INVALID_HANDLE_VALUE) {
    LogWindowsError("CreateNamedPipe");
    goto out;
  }
  LogPrint(LOG_DEBUG,"CreateFile -> %p",(HANDLE) fd);
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
  if (ConnectNamedPipe((HANDLE) fd, &info->overl)) {
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
  while ((lock = open(tmppath, O_WRONLY|O_CREAT|O_EXCL, 0644)) == -1) {
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
      LogPrint(LOG_ERR,"a BrlAPI server already listens on %s (file %s exists)",info->port, tmppath);
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
  closeFd(lock);
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
  closeFd(fd);
out:
  return -1;
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

#if defined(PF_LOCAL) && (!defined(WINDOWS) || defined(HAVE_CREATENAMEDPIPE))
  if ((cinfo->addrfamily==PF_LOCAL && (cinfo->fd=initializeLocalSocket(cinfo))==-1) ||
      (cinfo->addrfamily!=PF_LOCAL && 
#else /* PF_LOCAL */
  if ((
#endif /* PF_LOCAL */
	(cinfo->fd=initializeTcpSocket(cinfo))==-1))
    LogPrint(LOG_WARNING,"Error while initializing socket %"PRIdPTR,num);
  else
    LogPrint(LOG_DEBUG,"socket %"PRIdPTR" established (fd %d)",num,cinfo->fd);
  return NULL;
}

#ifdef WINDOWS
static void *socketSelect(void *foo)
{
  fd_set sockset;
  int fdmax;
  struct timeval tv;
  int n,i;

  while(1) {
    FD_ZERO(&sockset);
    fdmax=0;
    for (i=0;i<numSockets;i++)
      if (socketInfo[i].fd>=0 && socketInfo[i].addrfamily != PF_LOCAL) {
	FD_SET(socketInfo[i].fd, &sockset);
	if (socketInfo[i].fd>fdmax)
	  fdmax = socketInfo[i].fd;
      }
    tv.tv_sec = 1; tv.tv_usec = 0;
    if (fdmax == 0) {
      /* still no server socket */
      approximateDelay(1000);
      continue;
    }
    if ((n=select(fdmax+1, &sockset, NULL, NULL, &tv))<0) {
      setSocketErrno();
      LogPrint(LOG_WARNING,"select: %s",strerror(errno));
      break;
    }
    if (n==0) continue;
    for (i=0;i<numSockets;i++)
      if (socketInfo[i].fd>=0 && socketInfo[i].addrfamily != PF_LOCAL
	  && FD_ISSET(socketInfo[i].fd, &sockset))
	if (!(SetEvent(socketInfo[i].overl.hEvent)))
	  LogWindowsError("SetEvent(socketInfo)");
  }
  return NULL;
}
#endif /* WINDOWS */

static void closeSockets(void *arg)
{
  int i;
  struct socketInfo *info;
  
#ifdef WINDOWS
  pthread_cancel(socketSelectThread);
#endif /* WINDOWS */

  for (i=0;i<numSockets;i++) {
    pthread_cancel(socketThreads[i]);
    info=&socketInfo[i];
    if (info->fd>=0) {
      if (closeFd(info->fd))
        LogError("closing socket");
      info->fd=-1;
#ifdef WINDOWS
      if ((info->overl.hEvent)) {
	CloseHandle(info->overl.hEvent);
	info->overl.hEvent = NULL;
      }
#else /* WINDOWS */
#if defined(PF_LOCAL) && (!defined(WINDOWS) || defined(HAVE_CREATENAMEDPIPE))
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
	}
      }
#endif /* PF_LOCAL */
#endif /* WINDOWS */
    }
    free(info->port);
    info->port = NULL;
    free(info->hostname);
    info->hostname = NULL;
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
      (*lpHandles)[(*nbHandles)++] = (HANDLE) c->packet.overl.hEvent;
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
    socketInfo[i].fd = -1;

#ifdef WINDOWS
  if (WSAStartup(
    /* TODO: if UNIMPLEMENTED, tell that some tcp/ip stuff is needed */
#ifdef HAVE_GETADDRINFO
	  MAKEWORD(2,0),
#else /* HAVE_GETADDRINFO */
	  MAKEWORD(1,1),
#endif /* HAVE_GETADDRINFO */
	  &wsadata)) {
    LogWindowsSocketError("Starting socket library");
    pthread_exit(NULL);
  }
#endif /* WINDOWS */

  pthread_cleanup_push(closeSockets,NULL);

  for (i=0;i<numSockets;i++) {
    socketInfo[i].addrfamily=brlapiserver_splitHost(socketHosts[i],&socketInfo[i].hostname,&socketInfo[i].port);
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
#ifdef WINDOWS
  if ((pthread_create(&socketSelectThread, NULL, socketSelect, NULL))) {
    LogWindowsError("pthread_create");
    pthread_exit(NULL);
  }
#endif /* WINDOWS */

  unauthConnections = 0; unauthConnLog = 0;
  while (1) {
#ifdef WINDOWS
    lpHandles = malloc(nbAlloc * sizeof(*lpHandles));
    nbHandles = 0;
    for (i=0;i<numSockets;i++)
      if ((HANDLE) socketInfo[i].fd != INVALID_HANDLE_VALUE)
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
      if (socketInfo[i].fd != -1 &&
          WaitForSingleObject(socketInfo[i].overl.hEvent, 0) == WAIT_OBJECT_0) {
#if defined(HAVE_CREATENAMEDPIPE)
        if (socketInfo[i].addrfamily == PF_LOCAL) {
          DWORD foo;
          if (!(GetOverlappedResult((HANDLE) socketInfo[i].fd, &socketInfo[i].overl, &foo, FALSE)))
            LogWindowsError("GetOverlappedResult");
          res = socketInfo[i].fd;
          if ((socketInfo[i].fd = initializeLocalSocket(&socketInfo[i])) != -1)
            LogPrint(LOG_DEBUG,"socket %d re-established (fd %p, was %p)",i,(HANDLE) socketInfo[i].fd,(HANDLE) res);
          snprintf(source, sizeof(source), BRLAPI_SOCKETPATH "%s", socketInfo[i].port);
        } else {
#endif /* defined(HAVE_CREATENAMEDPIPE) */
          if (!ResetEvent(socketInfo[i].overl.hEvent))
            LogWindowsError("ResetEvent in server loop");
#else /* WINDOWS */
      if (socketInfo[i].fd>=0 && FD_ISSET(socketInfo[i].fd, &sockset)) {
#endif /* WINDOWS */
          addrlen = sizeof(addr);
          res = accept(socketInfo[i].fd, (struct sockaddr *) &addr, &addrlen);
          if (res<0) {
            setSocketErrno();
            LogPrint(LOG_WARNING,"accept(%d): %s",socketInfo[i].fd,strerror(errno));
            continue;
          }
          formatAddress(source, sizeof(source), &addr, addrlen);

#ifdef WINDOWS
#if defined(HAVE_CREATENAMEDPIPE)
        }
#endif /* defined(HAVE_CREATENAMEDPIPE) */
#endif /* WINDOWS */

        if (authDescriptor && !authPerform(authDescriptor, res)) {
          closeFd(res);
          continue;
        }
        LogPrint(LOG_NOTICE, "BrlAPI connection fd=%d accepted: %s", res, source);

        if (unauthConnections>=UNAUTH_MAX) {
          writeError(res, BRLERR_CONNREFUSED);
          closeFd(res);
          if (unauthConnLog==0) LogPrint(LOG_WARNING, "Too many simultaneous unauthorized connections");
          unauthConnLog++;
        } else {
#ifndef WINDOWS
          if (!setBlockingIo(res, 0)) {
            LogPrint(LOG_WARNING, "Failed to switch to non-blocking mode: %s",strerror(errno));
            break;
          }
#endif /* WINDOWS */

          c = createConnection(res, currentTime);
          if (c==NULL) {
            LogPrint(LOG_WARNING,"Failed to create connection structure");
            closeFd(res);
          }

          unauthConnections++;
          addConnection(c, notty.connections);
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
  if (addRange(0,BRL_KEYCODE_MAX,&c->unmaskedKeys)==-1) return -1;
  if (removeRange(BRL_CMD_SWITCHVT_PREV,BRL_CMD_SWITCHVT_NEXT,&c->unmaskedKeys)==-1) return -1;
  if (removeRange(BRL_CMD_RESTARTBRL,BRL_CMD_RESTARTSPEECH,&c->unmaskedKeys)==-1) return -1;
  if (removeRange(BRL_BLK_SWITCHVT,BRL_BLK_SWITCHVT|BRL_MSK_ARG,&c->unmaskedKeys)==-1) return -1;
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
  if (rawConnection!=NULL) return;
  pthread_mutex_lock(&connectionsMutex);
  if (whoFillsTty(&ttys)!=NULL) {
    pthread_mutex_unlock(&connectionsMutex);
    return;
  }
  pthread_mutex_unlock(&connectionsMutex);
  last_conn_write=NULL;
  pthread_mutex_lock(&driverMutex);
  trueBraille->writeWindow(brl);
  pthread_mutex_unlock(&driverMutex);
}

/* Function : api_writeVisual */
static void api_writeVisual(BrailleDisplay *brl)
{
  setCurrentRootTty();
  if (!trueBraille->writeVisual) return;
  if (rawConnection!=NULL) return;
  pthread_mutex_lock(&connectionsMutex);
  if (whoFillsTty(&ttys)!=NULL) {
    pthread_mutex_unlock(&connectionsMutex);
    return;
  }
  pthread_mutex_unlock(&connectionsMutex);
  last_conn_write=NULL;
  pthread_mutex_lock(&driverMutex);
  trueBraille->writeVisual(brl);
  pthread_mutex_unlock(&driverMutex);
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
    passKey = (c->how==how) && (inRangeList(c->unmaskedKeys,code) != NULL);
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
  if (rawConnection!=NULL) {
    pthread_mutex_lock(&driverMutex);
    size = trueBraille->readPacket(brl, packet,BRLAPI_MAXPACKETSIZE);
    pthread_mutex_unlock(&driverMutex);
    if (size<0)
      writeException(rawConnection->fd, BRLERR_DRIVERERROR, BRLPACKET_PACKET, NULL, 0);
    else 
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
      if (!driverResume(brl)) {
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
      if (driverOpened) driverSuspend(brl);
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
    /* we already ensured in GETTTY that no connection has how == KEYCODES */
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
  if (trueBraille->readKey && keycode != EOF && (c = whoGetsKey(&ttys,keycode,BRL_KEYCODES))) {
    /* somebody gets the raw code */
    LogPrint(LOG_DEBUG,"Transmitting unmasked key %lu",(unsigned long)keycode);
    clientCode = htonl(keycode);
    brlapiserver_writePacket(c->fd,BRLPACKET_KEY,&clientCode,sizeof(clientCode));
    command = EOF;
  } else {
    handleAutorepeat(&command, &repeatState);
    if (command != EOF) {
      /* nobody needs the raw code */
      if ((c = whoGetsKey(&ttys,command&BRL_MSK_CMD,BRL_COMMANDS))) {
        LogPrint(LOG_DEBUG,"Transmitting unmasked command %lu",(unsigned long)command);
        clientCode = htonl(command);
        brlapiserver_writePacket(c->fd,BRLPACKET_KEY,&clientCode,sizeof(clientCode));
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
  if (!driverOpened) {
    driverResume(brl);
    if (driverOpened) {
      /* TODO: handle clients' resize */
      displayDimensions[0] = htonl(brl->x);
      displayDimensions[1] = htonl(brl->y);
      displaySize = brl->x * brl->y;
      disp = brl;
    }
  }
  return (coreActive = driverOpened);
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
  if (!coreActive && driverOpened)
    driverSuspend(disp);
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
#if defined(PF_LOCAL) && (!defined(WINDOWS) || defined(HAVE_CREATENAMEDPIPE))
	":0+127.0.0.1:0";
#else /* PF_LOCAL */
	"127.0.0.1:0";
#endif /* PF_LOCAL */

  {
    const char *parameter = parameters[PARM_AUTH];
    if (!parameter) parameter = "";
    if (!*parameter) parameter = "none";
    if (!(authDescriptor = authBeginServer(parameter))) return 0;
  }

  keyfile = *parameters[PARM_KEYFILE]?parameters[PARM_KEYFILE]:BRLAPI_DEFAUTHPATH;
  pthread_attr_t attr;
  pthread_mutexattr_t mattr;

  coreActive=driverOpened=1;

  if (!strcmp(keyfile,"none")) unauthorizedConnectionsAllowed = 1;

  if ((notty.connections = createConnection(-1,0)) == NULL) {
    LogPrint(LOG_WARNING, "Unable to create connections list");
    goto out;
  }
  notty.connections->prev = notty.connections->next = notty.connections;
  if ((ttys.connections = createConnection(-1, 0)) == NULL) {
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

  stackSize = MAX(PTHREAD_STACK_MIN, OUR_STACK_MIN);
  {
    const char *operand = parameters[PARM_STACKSIZE];
    if (*operand) {
      int size;
      static const int minSize = PTHREAD_STACK_MIN;
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
