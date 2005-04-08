/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

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
#include "api_common.h"
#include "rangelist.h"
#include "brl.h"
#include "brltty.h"
#include "misc.h"
#include "iomisc.h"
#include "scr.h"
#include "tunes.h"

#ifdef WINDOWS
#define close(fd) CloseHandle((HANDLE)(fd))
#endif /* WINDOWS */

#define UNAUTH_MAX 5
#define UNAUTH_DELAY 30

typedef enum {
  PARM_HOST,
  PARM_KEYFILE
} Parameters;

const char *const api_parameters[] = { "host", "keyfile", NULL };

#define RELEASE "BRLTTY API Library: release " BRLAPI_RELEASE
#define COPYRIGHT "   Copyright Sebastien HINDERER <Sebastien.Hinderer@ens-lyon.org>, \
Samuel THIBAULT <samuel.thibault@ens-lyon.org>"

/* These CHECK* macros check whether a condition is true, and, if not, */
/* send back either a non-fatal error, or an excepti)n */
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

#ifdef brlapi_errno
#undef brlapi_errno
#endif

int brlapi_errno;
int *brlapi_errno_location(void) { return &brlapi_errno; }

/** ask for \e brltty commands */
#define BRLCOMMANDS 0
/** ask for raw driver keycodes */
#define BRLKEYCODES 1

/****************************************************************************/
/** GLOBAL TYPES AND VARIABLES                                              */
/****************************************************************************/

extern char *opt_brailleParameters;
extern char *cfg_brailleParameters;

typedef struct {
  unsigned int cursor;
  unsigned char *text;
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
  rangeList *unmaskedKeys;
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
  struct Tty *subttys; /* childs */
} Tty;
#define MAXTTYRECUR 16

static int connectionsAllowed = 0;

/* Pointer on the connection accepter thread */
#define MAXSOCKETS 4 /* who knows what users want to do... */
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
static HANDLE socketSelectEvent;
#endif /* WINDOWS */

static pthread_mutex_t connectionsMutex = PTHREAD_MUTEX_INITIALIZER;
static Tty notty;
static Tty ttys;

static unsigned int unauthConnections;
static unsigned int unauthConnLog = 0;

/* These variables let access to some information be synchronized */
static pthread_mutex_t driverMutex = PTHREAD_MUTEX_INITIALIZER;
static Connection *rawConnection = NULL;

/* Pointer on subroutines of the real braille driver */
static const BrailleDriver *trueBraille;
static BrailleDriver ApiBraille;

/* Identication of the REAL braille driver currently used */

/* The following variable contains the size of the braille display */
/* stored as a pair of _network_-formatted integers */
static uint32_t displayDimensions[2] = { 0, 0 };
static unsigned int displaySize = 0;

static BrailleDisplay *disp; /* Parameter to pass to braille drivers */

static size_t authKeyLength = 0;
static unsigned char authKey[BRLAPI_MAXPACKETSIZE];

static Connection *last_conn_write;

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

/****************************************************************************/
/** PACKET MANAGING                                                        **/
/****************************************************************************/

/* Function : writeAck */
/* Sends an acknowledgement on the given socket */
static inline void writeAck(int fd)
{
  brlapi_writePacket(fd,BRLPACKET_ACK,NULL,0);
}

/* Function : writeError */
/* Sends the given non-fatal error on the given socket */
static void writeError(int fd, unsigned int err)
{
  uint32_t code = htonl(err);
  LogPrint(LOG_DEBUG,"error %u on fd %d", err, fd);
  brlapi_writePacket(fd,BRLPACKET_ERROR,&code,sizeof(code));
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
  brlapi_writePacket(fd,BRLPACKET_EXCEPTION,epacket, hdrsize+esize);
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
  packet->p = (char *) &packet->header;
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
      switch (errno = GetLastError()) {
        case ERROR_IO_PENDING: return 0;
	case 0:
	case ERROR_BROKEN_PIPE: return -2;
	default: LogWindowsError("GetOverlappedResult"); return -1;
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
  } else if (packet->readBytes==packet->header.size) goto out;
  else if (packet->state==DISCARDING) {
    packet->p = packet->content;
    packet->n = MIN(packet->header.size-packet->readBytes, BRLAPI_MAXPACKETSIZE);
  } else {
    packet->n -= res;
    packet->p += res;
  }
#ifdef WINDOWS
  } else packet->state = READING_HEADER;
  if (!ReadFile((HANDLE)c->fd, packet->p, packet->n, &res, &packet->overl)) {
    switch (errno = GetLastError()) {
      case ERROR_IO_PENDING: return 0;
      case 0:
      case ERROR_BROKEN_PIPE: return -2;
      default: LogWindowsError("ReadFile"); return -1;
    }
  }
#endif /* WINDOWS */
  goto read;

out:
  resetPacket(packet);
  return 1;
}

/****************************************************************************/
/** BRAILLE WINDOWS MANAGING                                               **/
/****************************************************************************/

/* Function : allocBrailleWindow */
/* Allocates and initializes the members of a BrailleWindow structure */
/* Uses displaySize to determine size of allocated buffers */
/* Returns to report success, -1 on errors */
int allocBrailleWindow(BrailleWindow *brailleWindow)
{
  if (!(brailleWindow->text = malloc(displaySize))) goto out;
  if (!(brailleWindow->andAttr = malloc(displaySize))) goto outText;
  if (!(brailleWindow->orAttr = malloc(displaySize))) goto outAnd;

  memset(brailleWindow->text, ' ', displaySize);
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
  memcpy(dest->text, src->text, displaySize);
  memcpy(dest->andAttr, src->andAttr, displaySize);
  memcpy(dest->orAttr, src->orAttr, displaySize);
}

/* Function: getDots */
/* Returns the braille dots corresponding to a BrailleWindow structure */
/* No allocation of buf is performed */
void getDots(const BrailleWindow *brailleWindow, unsigned char *buf)
{
  int i;
  for (i=0; i<displaySize; i++)
    buf[i] = (textTable[*(brailleWindow->text+i)] &
              brailleWindow->andAttr[i]) |
             brailleWindow->orAttr[i];              
  if (brailleWindow->cursor) buf[brailleWindow->cursor-1] |= cursorDots();
}

/* Function: getText */
/* Returns the text corresponding to a BrailleWindow structure */
/* No allocation of buf is performed */
void getText(const BrailleWindow *brailleWindow, unsigned char *buf)
{
  memcpy(buf,brailleWindow->text,displaySize);
}

/****************************************************************************/
/** CONNECTIONS MANAGING                                                   **/
/****************************************************************************/

/* Function : createConnection */
/* Creates a struct of type Connection and stores suitable values */
/* x and y correspond to the braille display size */
/* This function also records the connection in an array */
/* If an error occurs, one returns NULL, and an error message is written on */
/* the socket before closing it */
static Connection *createConnection(int fd, time_t currentTime)
{
  Connection *c =  malloc(sizeof(Connection));
  if (c==NULL) goto out;
  c->auth = 0;
  c->fd = fd;
  c->tty = NULL;
  c->raw = 0;
  c->brlbufstate = EMPTY;
  pthread_mutex_init(&c->brlMutex,NULL);
  pthread_mutex_init(&c->maskMutex,NULL);
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
  close(fd);
  return NULL;
}

/* Function : freeConnection */
/* Frees all resources associated to a connection */
static void freeConnection(Connection *c)
{
  if (c->fd>=0) close(c->fd);
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
  LogPrint(LOG_DEBUG, "Received %s request on fd %d", brlapi_packetType(type), fd);  
}

/* Function doLeaveTty */
/* handles a connection leaving its tty */
static void doLeaveTty(Connection *c) {
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

/* Function : processRequest */
/* Reads a packet fro c->fd and processes it */
/* Returns 1 if connection has to be removed */
/* If EOF is reached, closes fd and frees all associated ressources */
static int processRequest(Connection *c)
{
  int res;
  ssize_t size;
  unsigned char *packet = c->packet.content;
  authStruct *auth = (authStruct *) packet;
  uint32_t * ints = (uint32_t *) packet;
  brl_type_t type;
  const char *str;
  unsigned int len;
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
      if (trueBraille->reset && !trueBraille->reset(disp)) {
        LogPrint(LOG_WARNING,"Reset failed. Restarting braille driver");
        restartBrailleDriver();
      }
    }
    if (c->tty) {
      LogPrint(LOG_DEBUG,"Client on fd %d did not give up control of tty %#010x properly",c->fd,c->tty->number);
      doLeaveTty(c);
    }
    return 1;
  }
  size = c->packet.header.size;
  type = c->packet.header.type;
  
  if (c->auth==0) {
    if (ntohl(auth->protocolVersion)!=BRLAPI_PROTOCOL_VERSION) {
      writeError(c->fd, BRLERR_PROTOCOL_VERSION);
      return 1;
    }
    if ((type==BRLPACKET_AUTHKEY) && (size==sizeof(auth->protocolVersion)+authKeyLength) && (!memcmp(&auth->key, authKey, authKeyLength))) {
      writeAck(c->fd);
      c->auth = 1;
      unauthConnections--;
      return 0;
    } else {
      writeError(c->fd, BRLERR_CONNREFUSED);
      return 1;
    }
  }
  if (size>BRLAPI_MAXPACKETSIZE) {
    LogPrint(LOG_WARNING, "Discarding too large packet of type %s on fd %d",brlapi_packetType(type), c->fd);
    return 0;    
  }
  switch (type) {
    case BRLPACKET_GETTTY: {
      uint32_t nbTtys;
      int how;
      unsigned int n;
      unsigned char *p = packet, name[BRLAPI_MAXNAMELENGTH+1];
      Tty *tty,*tty2,*tty3;
      uint32_t *ptty;
      LogPrintRequest(type, c->fd);
      CHECKEXC((!c->raw),BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(size>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
      p += sizeof(uint32_t); size -= sizeof(uint32_t);
      nbTtys = ntohl(ints[0]);
      CHECKEXC(nbTtys<=MAXTTYRECUR, BRLERR_TOORECURSE);
      CHECKEXC(size>=nbTtys*sizeof(uint32_t), BRLERR_INVALID_PACKET);
      p += nbTtys*sizeof(uint32_t); size -= nbTtys*sizeof(uint32_t);
      CHECKEXC(*p<=BRLAPI_MAXNAMELENGTH, BRLERR_INVALID_PARAMETER);
      n = *p; p++; size--;
      CHECKEXC(size==n, BRLERR_INVALID_PACKET);
      memcpy(name, p, n);
      name[n] = '\0';
      if (!*name) how = BRLCOMMANDS; else {
        if ((!strcmp(name, trueBraille->name)) && (isKeyCapable(trueBraille)))
          how = BRLKEYCODES;
        else how = -1;
      }
      CHECKEXC(((how == BRLKEYCODES) || (how == BRLCOMMANDS)),BRLERR_KEYSNOTSUPP);
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
          WEXC(c->fd, BRLERR_INVALID_PARAMETER);
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
	for (; ptty<&ints[size/sizeof(uint32_t)-1]; ptty++) {
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
	    WEXC(c->fd, BRLERR_ILLEGAL_INSTRUCTION);
          } else {
            /* Here one is in the case where the client tries to change */
            /* from BRLKEYCODES to BRLCOMMANDS, or something like that */
            /* For the moment this operation is not supported */
            /* A client that wants to do that should first LeaveTty() */
            /* and then get it again, risking to lose it */
            LogPrint(LOG_INFO,"Switching from BRLKEYCODES to BRLCOMMANDS not supported yet");
            WERR(c->fd,BRLERR_OPNOTSUPP);
          }
          return 0;
	} else {
	  /* uhu, we already got a tty, but not this one: this is forbidden. */
          WEXC(c->fd, BRLERR_INVALID_PARAMETER);
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
    case BRLPACKET_SETFOCUS: {
      CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(c->tty,BRLERR_ILLEGAL_INSTRUCTION);
      c->tty->focus = ntohl(ints[0]);
      LogPrint(LOG_DEBUG,"Focus on window %#010x",c->tty->focus);
      return 0;
    }
    case BRLPACKET_LEAVETTY: {
      LogPrintRequest(type, c->fd);
      CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(c->tty,BRLERR_ILLEGAL_INSTRUCTION);
      doLeaveTty(c);
      writeAck(c->fd);
      return 0;
    }
    case BRLPACKET_IGNOREKEYRANGE:
    case BRLPACKET_UNIGNOREKEYRANGE: {
      int res;
      brl_keycode_t x,y;
      LogPrintRequest(type, c->fd);
      CHECKEXC(( (!c->raw) && c->tty ),BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(size==2*sizeof(brl_keycode_t),BRLERR_INVALID_PACKET);
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
    case BRLPACKET_IGNOREKEYSET:
    case BRLPACKET_UNIGNOREKEYSET: {
      int i = 0, res = 0;
      unsigned int nbkeys;
      brl_keycode_t *k = (brl_keycode_t *) packet;
      int (*fptr)(uint32_t, uint32_t, rangeList **);
      LogPrintRequest(type, c->fd);
      if (type==BRLPACKET_IGNOREKEYSET) fptr = removeRange; else fptr = addRange;
      CHECKEXC(( (!c->raw) && c->tty ),BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(size % sizeof(brl_keycode_t)==0,BRLERR_INVALID_PACKET);
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
    case BRLPACKET_WRITE: {
      writeStruct *ws = (writeStruct *) packet;
      BrailleWindow brailleWindow;
      /* Hack to avoid allocating fields with malloc */
      unsigned char x[displaySize], y[displaySize], z[displaySize];
      unsigned int rbeg, rend, strLen;
      unsigned char *p = &ws->data;
      LogPrintRequest(type, c->fd);
      CHECKEXC(size>=sizeof(ws->flags), BRLERR_INVALID_PACKET);
      CHECKEXC(((!c->raw)&&c->tty),BRLERR_ILLEGAL_INSTRUCTION);
      brailleWindow.text = x;
      brailleWindow.andAttr = y;
      brailleWindow.orAttr = z;
      ws->flags = ntohl(ws->flags);
      pthread_mutex_lock(&c->brlMutex);
      if ((size==sizeof(ws->flags))&&(ws->flags==0)) {
        c->brlbufstate = EMPTY;
        pthread_mutex_unlock(&c->brlMutex);
        return 0;
      }
      copyBrailleWindow(&brailleWindow, &c->brailleWindow);
      pthread_mutex_unlock(&c->brlMutex);
      size -= sizeof(ws->flags); /* flags */
      CHECKERR((ws->flags & BRLAPI_WF_DISPLAYNUMBER)==0, BRLERR_OPNOTSUPP);
      if (ws->flags & BRLAPI_WF_REGION) {
        CHECKEXC(size>2*sizeof(uint32_t), BRLERR_INVALID_PACKET);
        rbeg = ntohl( *((uint32_t *) p) );
        p += sizeof(uint32_t); size -= sizeof(uint32_t); /* region begin */
        rend = ntohl( *((uint32_t *) p) );
        p += sizeof(uint32_t); size -= sizeof(uint32_t); /* region end */
        CHECKEXC(
          (1<=rbeg) && (rbeg<=rend) && (rend<=displaySize),
          BRLERR_INVALID_PARAMETER);
      } else {
        rbeg = 1;
        rend = displaySize;
      }
      strLen = (rend-rbeg) + 1;
      if (ws->flags & BRLAPI_WF_TEXT) {
        CHECKEXC(size>=strLen, BRLERR_INVALID_PACKET);
        memcpy(brailleWindow.text+rbeg-1, p, strLen);
        p += strLen; size -= strLen; /* text */
      }
      if (ws->flags & BRLAPI_WF_ATTR_AND) {
        CHECKEXC(size>=strLen, BRLERR_INVALID_PACKET);
        memcpy(brailleWindow.andAttr+rbeg-1, p, strLen);
        p += strLen; size -= strLen; /* and attributes */
      }
      if (ws->flags & BRLAPI_WF_ATTR_OR) {
        CHECKEXC(size>=strLen, BRLERR_INVALID_PACKET);
        memcpy(brailleWindow.orAttr+rbeg-1, p, strLen);
        p += strLen; size -= strLen; /* or attributes */
      }
      if (ws->flags & BRLAPI_WF_CURSOR) {
        CHECKEXC(size>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
        brailleWindow.cursor = ntohl( *((uint32_t *) p) );
        p += sizeof(uint32_t); size -= sizeof(uint32_t); /* cursor */
        CHECKEXC(brailleWindow.cursor<=displaySize, BRLERR_INVALID_PACKET);
      }
      CHECKEXC(size==0, BRLERR_INVALID_PACKET);
      /* Here all the packet has been processed. */
      pthread_mutex_lock(&c->brlMutex);
      copyBrailleWindow(&c->brailleWindow, &brailleWindow);
      c->brlbufstate = TODISPLAY;
      pthread_mutex_unlock(&c->brlMutex);
      return 0;
    }
    case BRLPACKET_GETRAW: {
      getRawPacket_t *getRawPacket = (getRawPacket_t *) packet;
      unsigned char name[BRLAPI_MAXNAMELENGTH+1];
      LogPrintRequest(type, c->fd);
      CHECKEXC(!c->raw, BRLERR_ILLEGAL_INSTRUCTION);
      CHECKEXC(size>sizeof(uint32_t), BRLERR_INVALID_PACKET);
      size -= sizeof(uint32_t);
      getRawPacket->magic = ntohl(getRawPacket->magic);
      CHECKEXC(getRawPacket->magic==BRLRAW_MAGIC,BRLERR_INVALID_PARAMETER);
      CHECKEXC(getRawPacket->nameLength<=BRLAPI_MAXNAMELENGTH, BRLERR_INVALID_PARAMETER);
      size--;
      CHECKEXC(size==getRawPacket->nameLength, BRLERR_INVALID_PACKET);
      memcpy(name, &getRawPacket->name, getRawPacket->nameLength);
      name[getRawPacket->nameLength] = '\0';
      CHECKEXC(((!strcmp(name, trueBraille->name)) && isRawCapable(trueBraille)), BRLERR_RAWNOTSUPP);
      CHECKERR(rawConnection==NULL,BRLERR_RAWMODEBUSY);
      c->raw = 1;
      rawConnection = c;
      writeAck(c->fd);
      return 0;
    }
    case BRLPACKET_LEAVERAW: {
      LogPrintRequest(type, c->fd);
      CHECKEXC(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      LogPrint(LOG_DEBUG,"Going out of raw mode");
      c->raw = 0;
      rawConnection = NULL;
      writeAck(c->fd);
      return 0;
    }
    case BRLPACKET_PACKET: {
      LogPrintRequest(type, c->fd);
      CHECKEXC(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      pthread_mutex_lock(&driverMutex);
      trueBraille->writePacket(disp,packet,size);
      pthread_mutex_unlock(&driverMutex);
      return 0;
    }
    case BRLPACKET_GETDRIVERID: str = braille->identifier; goto l1;
    case BRLPACKET_GETDRIVERNAME: {
      str = braille->name;
l1:   len = strlen(str);
      LogPrintRequest(type, c->fd);
      CHECKEXC(size==0,BRLERR_INVALID_PACKET);
      CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      brlapi_writePacket(c->fd, type, str, len+1);
      return 0;
      }
    case BRLPACKET_GETDISPLAYSIZE: {
      LogPrintRequest(type, c->fd);
      CHECKEXC(size==0,BRLERR_INVALID_PACKET);
      CHECKEXC(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
      brlapi_writePacket(c->fd,BRLPACKET_GETDISPLAYSIZE,&displayDimensions[0],sizeof(displayDimensions));
      return 0;
    }
    default:
      WEXC(c->fd,BRLERR_UNKNOWN_INSTRUCTION);
  }
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

#ifdef WINDOWS
  WSAStartup(MAKEWORD(2,0),&wsadata);
#endif /* WINDOWS */

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
	,gai_strerror(err)
#else /* HAVE_GAI_STRERROR */
	,err
#endif /* HAVE_GAI_STRERROR */
    );
    return -1;
  }
  for (cur = res; cur; cur = cur->ai_next) {
    fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (fd<0) {
      if (errno != EAFNOSUPPORT)
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
    close(fd);
    LogError(fun);
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
      close(fd);
      return -1;
    }
#endif /* WINDOWS */

    return fd;
  }
  LogPrint(LOG_WARNING,"unable to find a local TCP port %s:%s !",info->hostname,info->port);

#else /* HAVE_GETADDRINFO */

  struct sockaddr_in addr;
  struct hostent *he;

#ifdef WINDOWS
  WSAStartup(MAKEWORD(1,0),&wsadata);
#endif /* WINDOWS */

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
  } else {
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
    goto errfd;
  }
  if (loopBind(fd, (struct sockaddr *) &addr, sizeof(addr))<0) {
    fun = "bind";
    goto errfd;
  }
  if (listen(fd,1)<0) {
    fun = "listen";
    goto errfd;
  }
  free(info->hostname);
  info->hostname = NULL;
  free(info->port);
  info->port = NULL;

#ifdef WINDOWS
  if (!(info->overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
    LogWindowsError("CreateEvent");
    close(fd);
    return -1;
  }
#endif /* WINDOWS */

  return fd;

errfd:
  close(fd);
err:
  LogError(fun);

#endif /* HAVE_GETADDRINFO */

  free(info->hostname);
  info->hostname = NULL;
  free(info->port);
  info->port = NULL;
  return -1;
}

#ifdef PF_LOCAL
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
    LogPrint(LOG_DEBUG,"Event -> %d",(int)info->overl.hEvent);
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
  mode_t oldmode;
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0))==-1) {
    LogError("socket");
    goto out;
  }
  sa.sun_family = AF_LOCAL;
  if (lpath+lport+1>sizeof(sa.sun_path)) {
    LogError("Unix path too long");
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
    sleep(1);
  }
  memcpy(sa.sun_path,BRLAPI_SOCKETPATH,lpath);
  memcpy(sa.sun_path+lpath,info->port,lport+1);
  /* hacky, TODO: find a better way to keep secure */
  unlink(sa.sun_path);
  if (loopBind(fd, (struct sockaddr *) &sa, sizeof(sa))<0) {
    LogPrint(LOG_WARNING,"bind: %s",strerror(errno));
    goto outmode;
  }
  umask(oldmode);
  if (listen(fd,1)<0) {
    LogError("listen");
    goto outfd;
  }
  return fd;
  
outmode:
  umask(oldmode);
#endif /* WINDOWS */
outfd:
  close(fd);
out:
  return -1;
}
#endif

static void *establishSocket(void *arg)
{
  int num = (int) arg;
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
  if ((cinfo->addrfamily==PF_LOCAL && (cinfo->fd=initializeLocalSocket(cinfo))==-1) ||
      (cinfo->addrfamily!=PF_LOCAL && 
#else
  if ((
#endif
	(cinfo->fd=initializeTcpSocket(cinfo))==-1))
    LogPrint(LOG_WARNING,"Error while initializing socket %d",num);
  else
    LogPrint(LOG_DEBUG,"socket %d established (fd %d)",num,cinfo->fd);
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
      Sleep(1000);
      continue;
    }
    if ((n=select(fdmax+1, &sockset, NULL, NULL, &tv))<0) {
      LogPrint(LOG_WARNING,"select: %s",strerror(errno));
      break;
    }
    if (n==0) continue;
    if (!ResetEvent(socketSelectEvent))
      LogWindowsError("ResetEvent(socketSelectEvent)");
    for (i=0;i<numSockets;i++)
      if (socketInfo[i].fd>=0 && FD_ISSET(socketInfo[i].fd, &sockset))
	if (!(SetEvent(socketInfo[i].overl.hEvent)))
	  LogWindowsError("SetEvent(socketInfo)");
    if (WaitForSingleObject(socketSelectEvent, INFINITE) == WAIT_FAILED) {
      LogWindowsError("WaitForSingleObject(socketSelectEvent");
      break;
    }
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
      if (close(info->fd))
        LogError("closing socket");
      info->fd=-1;
    }
#ifdef WINDOWS
    if ((info->overl.hEvent)) {
      CloseHandle(info->overl.hEvent);
      info->overl.hEvent = NULL;
    }
#else /* WINDOWS */
#ifdef PF_LOCAL
    if (info->addrfamily==PF_LOCAL) {
      char *path;
      int lpath=strlen(BRLAPI_SOCKETPATH),lport=strlen(info->port);
      if ((path=malloc(lpath+lport+1))) {
        memcpy(path,BRLAPI_SOCKETPATH,lpath);
        memcpy(path+lpath,info->port,lport+1);
        if (unlink(path)<0)
          LogError("unlinking socket");
      }
    }
#endif /* PF_LOCAL */
#endif /* WINDOWS */
    free(info->port);
    info->port = NULL;
    free(info->hostname);
    info->hostname = NULL;
  }
}

/* Function: addTtyFds */
/* recursively add fds of ttys */
#ifdef WINDOWS
static void addTtyFds(HANDLE *lpHandles, int *nbAlloc, int *nbHandles, Tty *tty) {
#else /* WINDOWS */
static void addTtyFds(fd_set *fds, int *fdmax, Tty *tty) {
#endif /* WINDOWS */
  {
    Connection *c;
    for (c = tty->connections->next; c != tty->connections; c = c -> next) {
#ifdef WINDOWS
      if (*nbHandles == *nbAlloc) {
	*nbAlloc *= 2;
	*lpHandles = realloc(lpHandles,*nbAlloc*sizeof(*lpHandles));
      }
      lpHandles[(*nbHandles)++] = (HANDLE) c->packet.overl.hEvent;
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
	remove = processRequest(c);
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
  struct sockaddr addr;
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
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

  for (i=0;i<numSockets;i++)
    socketInfo[i].fd = -1;

  pthread_cleanup_push(closeSockets,NULL);
  for (i=0;i<numSockets;i++) {
    socketInfo[i].addrfamily=brlapi_splitHost(socketHosts[i],&socketInfo[i].hostname,&socketInfo[i].port);
#ifdef WINDOWS
    if (socketInfo[i].addrfamily != PF_LOCAL) {
#endif /* WINDOWS */
      if ((res = pthread_create(&socketThreads[i],&attr,establishSocket,(void *)i)) != 0) {
	LogPrint(LOG_WARNING,"pthread_create: %s",strerror(res));
	for (i--;i>=0;i--)
	  pthread_cancel(socketThreads[i]);
	pthread_exit(NULL);
      }
#ifdef WINDOWS
    } else {
      /* Windows doesn't have troubles with local sockets on read-only
       * filesystems, but it has with inter-thread events, so call from here */
      establishSocket((void *)i);
    }
#endif /* WINDOWS */
  }
#ifdef WINDOWS
  if (!(socketSelectEvent = CreateEvent(NULL,TRUE,TRUE,NULL))) {
    LogWindowsError("CreateEvent");
    pthread_exit(NULL);
  }
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
    addTtyFds(lpHandles, &nbAlloc, &nbHandles, &notty);
    addTtyFds(lpHandles, &nbAlloc, &nbHandles, &ttys);
    pthread_mutex_unlock(&connectionsMutex);
    if (!nbHandles) {
      free(lpHandles);
      Sleep(1000);
      continue;
    }
    switch (WaitForMultipleObjects(nbHandles, lpHandles, FALSE, 1000)) {
      case WAIT_TIMEOUT: continue;
      case WAIT_FAILED: LogWindowsError("WaitForMultipleObjects");
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
    for (i=0;i<numSockets;i++)
#ifdef WINDOWS
    if (socketInfo[i].fd != -1 &&
	WaitForSingleObject(socketInfo[i].overl.hEvent, 0) == WAIT_OBJECT_0) {
      if (socketInfo[i].addrfamily != PF_LOCAL) {
	ResetEvent(socketInfo[i].overl.hEvent);
#else /* WINDOWS */
    if (socketInfo[i].fd>=0 && FD_ISSET(socketInfo[i].fd, &sockset)) {
#endif /* WINDOWS */
	addrlen = sizeof(addr);
	res = accept(socketInfo[i].fd, &addr, &addrlen);
	if (res<0) {
	  LogPrint(LOG_WARNING,"accept(%d): %s",socketInfo[i].fd,strerror(errno));
	  break;
	}
#ifdef WINDOWS
      } else /* PF_LOCAL */ {
        DWORD foo;
	if (!(GetOverlappedResult((HANDLE) socketInfo[i].fd, &socketInfo[i].overl, &foo, FALSE)))
	  LogWindowsError("GetOverlappedResult");
        res = socketInfo[i].fd;
	if ((socketInfo[i].fd = initializeLocalSocket(&socketInfo[i])) != -1)
	  LogPrint(LOG_DEBUG,"socket %d re-established (fd %p, was %p)",i,(HANDLE) socketInfo[i].fd,(HANDLE) res);
      }
#endif /* WINDOWS */
      LogPrint(LOG_DEBUG,"Connection accepted on fd %d",res);
      if (unauthConnections>=UNAUTH_MAX) {
        writeError(res, BRLERR_CONNREFUSED);
        close(res);
        if (unauthConnLog==0) LogPrint(LOG_WARNING, "Too many simultaneous unauthenticated connections");
        unauthConnLog++;
      } else {
        unauthConnections++;
#ifndef WINDOWS
        if (!setBlockingIo(res, 0)) {
          LogPrint(LOG_WARNING, "Failed to switch to non-blocking mode: %s",strerror(errno));
          break;
        }
#endif /* WINDOWS */
        c = createConnection(res, currentTime);
        if (c==NULL) {
          LogPrint(LOG_WARNING,"Failed to create connection structure");
          close(res);
        }
	addConnection(c, notty.connections);
      }
    }
#ifdef WINDOWS
    if (!SetEvent(socketSelectEvent))
      LogWindowsError("SetEvent(socketSelectEvent)");
#endif /* WINDOWS */
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
  if (c->how==BRLKEYCODES) return 0;
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
  if (connectionsAllowed!=0) {
    if ((res = pthread_cancel(serverThread)) != 0 )
      LogPrint(LOG_WARNING,"pthread_cancel: %s",strerror(res));
    ttyTerminationHandler(&notty);
    ttyTerminationHandler(&ttys);
    api_unlink();
#ifdef WINDOWS
    WSACleanup();
#endif /* WINDOWS */
  }
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
  int tty;
  if ((tty = currentVirtualTerminal()))
    ttys.focus = tty;
  else
    ttys.focus = -1;
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
  trueBraille->writeWindow(brl);
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
  trueBraille->writeVisual(brl);
}

/* Function: whoGetsKey */
/* Returns the connection which gets that key */
static Connection *whoGetsKey(Tty *tty, brl_keycode_t command, brl_keycode_t keycode)
{
  Connection *c;
  Tty *t;
  {
    int masked;
    for (c=tty->connections->next; c!=tty->connections; c = c->next) {
      pthread_mutex_lock(&c->maskMutex);
      if (c->how==BRLKEYCODES)
        masked = (contains(c->unmaskedKeys,keycode) == NULL);
      else
        masked = (contains(c->unmaskedKeys,command & BRL_MSK_CMD) == NULL);
      pthread_mutex_unlock(&c->maskMutex);
      if (!masked) goto found;
    }
  }
  c = NULL;
found:
  for (t = tty->subttys; t; t = t->next)
    if (tty->focus==-1 || t->number == tty->focus) {
      Connection *recur_c = whoGetsKey(t, command, keycode);
      return recur_c ? recur_c : c;
    }
  return c;
}

/* Function : api_readCommand */
static int api_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext caller)
{
  int res, refresh = 0;
  ssize_t size;
  Connection *c;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brl_keycode_t keycode, command;

  if (rawConnection!=NULL) {
    pthread_mutex_lock(&driverMutex);
    size = trueBraille->readPacket(brl, packet,BRLAPI_MAXPACKETSIZE);
    pthread_mutex_unlock(&driverMutex);
    if (size<0)
      writeException(rawConnection->fd, BRLERR_DRIVERERROR, BRLPACKET_PACKET, NULL, 0);
    else 
      brlapi_writePacket(rawConnection->fd,BRLPACKET_PACKET,packet,size);
    return EOF;
  }
  setCurrentRootTty();
  pthread_mutex_lock(&connectionsMutex);
  c = whoFillsTty(&ttys);
  if (c && last_conn_write!=c) {
    last_conn_write=c;
    refresh=1;
  }
  pthread_mutex_unlock(&connectionsMutex);
  if (c) {
    pthread_mutex_lock(&c->brlMutex);
    if ((c->brlbufstate==TODISPLAY) || (refresh)) {
      unsigned char *oldbuf = disp->buffer, buf[displaySize];
      disp->buffer = buf;
      if (trueBraille->writeVisual) {
        getText(&c->brailleWindow, buf);
        brl->cursor = c->brailleWindow.cursor-1;
        trueBraille->writeVisual(brl);
      }
      getDots(&c->brailleWindow, buf);
      trueBraille->writeWindow(brl);
      disp->buffer = oldbuf;
      c->brlbufstate = FULL;
    }
    pthread_mutex_unlock(&c->brlMutex);
  }
  if (trueBraille->readKey) {
    res = trueBraille->readKey(brl);
    if (res==EOF) return EOF;
    keycode = (brl_keycode_t) res;
    command = trueBraille->keyToCommand(brl,caller,keycode);
  } else {
    /* we already ensured in GETTTY that no connection has how == KEYCODES */
    res = trueBraille->readCommand(brl,caller);
    if (res==EOF) return EOF;
    keycode = 0;
    command = (brl_keycode_t) res;
  }
  c = whoGetsKey(&ttys,command,keycode);
  if (c) {
    if (c->how==BRLKEYCODES) {
      LogPrint(LOG_DEBUG,"Transmitting unmasked key %lu",(unsigned long)keycode);
      keycode = htonl(keycode);
      brlapi_writePacket(c->fd,BRLPACKET_KEY,&keycode,sizeof(keycode));
    } else {
      if ((command!=BRL_CMD_NOOP) && (command!=EOF)) {
        LogPrint(LOG_DEBUG,"Transmitting unmasked command %lu",(unsigned long)command);
        keycode = htonl(command);
        brlapi_writePacket(c->fd,BRLPACKET_KEY,&keycode,sizeof(command));
      }
    }
    return EOF;
  }
  return command;
}

/* Function : api_link */
/* Does all the link stuff to let api get events from the driver and */
/* writes from brltty */
void api_link(void)
{
  trueBraille=braille;
  memcpy(&ApiBraille,braille,sizeof(BrailleDriver));
  ApiBraille.writeWindow=api_writeWindow;
  ApiBraille.writeVisual=api_writeVisual;
  ApiBraille.readCommand=api_readCommand;
  ApiBraille.readKey = NULL;
  ApiBraille.keyToCommand = NULL;
  ApiBraille.readPacket = NULL;
  ApiBraille.writePacket = NULL;
  braille=&ApiBraille;
}

/* Function : api_unlink */
/* Does all the unlink stuff to remove api from the picture */
void api_unlink(void)
{
  braille=trueBraille;
}

/* Function : api_identify */
/* Identifies BrlApi */
void api_identify(void)
{
  LogPrint(LOG_NOTICE, RELEASE);
  LogPrint(LOG_INFO,   COPYRIGHT);
}

/* Function : api_open */
/* Initializes BrlApi */
/* One first initialize the driver */
/* Then one creates the communication socket */
int api_open(BrailleDisplay *brl, char **parameters)
{
  int res,i;
  char *hosts=
#ifdef PF_LOCAL
	":0+127.0.0.1:0";
#else
	"127.0.0.1:0";
#endif
  char *keyfile = *parameters[PARM_KEYFILE]?parameters[PARM_KEYFILE]:BRLAPI_DEFAUTHPATH;
  pthread_attr_t attr;

  displayDimensions[0] = htonl(brl->x);
  displayDimensions[1] = htonl(brl->y);
  displaySize = brl->x * brl->y;
  disp = brl;

  res = brlapi_loadAuthKey(keyfile,&authKeyLength,authKey);
  if (res==-1) {
    LogPrint(LOG_WARNING,"Unable to load API authentication key from %s: %s in %s, no connections will be accepted.", keyfile, strerror(brlapi_libcerrno), brlapi_libcerrfun);
    goto out;
  }
  LogPrint(LOG_DEBUG, "Authentication key loaded");
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

  if (*parameters[PARM_HOST]) hosts = parameters[PARM_HOST];

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

  trueBraille=braille;
  if ((res = pthread_create(&serverThread,&attr,server,hosts)) != 0) {
    LogPrint(LOG_WARNING,"pthread_create: %s",strerror(res));
    for (i=0;i<numSockets;i++)
      pthread_cancel(socketThreads[i]);
    goto outallocs;
  }
  api_link();
  connectionsAllowed = 1;
  return 1;
  
outallocs:
  freeConnection(ttys.connections);
outalloc:
  freeConnection(notty.connections);
out:
  connectionsAllowed = 0;
  return 0;
}

/* Function : api_close */
/* End of BrlApi session. Closes the listening socket */
/* destroys opened connections and associated resources */
/* Closes the driver */
void api_close(BrailleDisplay *brl)
{
  terminationHandler();
}
