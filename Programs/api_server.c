/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* brlapi.c : Main file for BrlApi library */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

#include "api.h"
#include "api_common.h"
#include "rangelist.h"
#include "brl.h"
#include "brltty.h"
#include "misc.h"
#include "scr.h"
#include "tunes.h"

/* This defines the message that is initially prepared to be printed */
/* on the braille display, when a client takes control of a tty */
#define INITIAL_MESSAGE "Connected to BrlApi"

/* Total number of authorized connections */
#define CONNECTIONS 64

typedef enum {
  PARM_HOST,
  PARM_KEYFILE
} Parameters;

const char *const api_parameters[] = { "host", "keyfile", NULL };

#define RELEASE "BRLTTY API Library: release " BRLAPI_RELEASE
#define COPYRIGHT "   Copyright Sebastien HINDERER <shindere@ens-lyon.fr> \
& Samuel THIBAULT <samuel.thibault@ens-lyon.org>"

/* This CHECK macro checks whether a condition is true, and, if not, does */
/* a "continue" and writes an error on the socket c->fd, associated to the */
/* currently handled connection */
/* It is used in ProcessConnection function */
#define CHECK(condition, error) \
if (!( condition )) { \
  writeErrorPacket(c->fd, error ); \
  continue; \
} else { }

#ifdef brlapi_errno
#undef brlapi_errno
#endif

int brlapi_errno;
int *brlapi_errno_location(void) { return &brlapi_errno; }

/****************************************************************************/
/** GLOBAL TYPES AND VARIABLES                                              */
/****************************************************************************/

extern char *opt_brailleParameters;
extern char *cfg_brailleParameters;

typedef enum { FULL, EMPTY} TBrlBufState;

typedef struct
{
  pthread_t thread;
  int id;
  int fd;
  int tty;
  int raw;
  uint32_t how; /* how keys must be delivered to clients */
  BrailleDisplay brl;
  unsigned int cursor;
  TBrlBufState brlbufstate;
  pthread_mutex_t brlmutex;
  Trangelist *UnmaskedKeys;
  pthread_mutex_t maskmutex;
} Tconnection;

static int connectionsAllowed = 0;

/* Pointer on the connection accepter thread */
static pthread_t t; /* ConnectionsManager thread */

/* This mutex protects data shared by several connections */
static pthread_mutex_t connections_mutex = PTHREAD_MUTEX_INITIALIZER;

/* These 4 variables let a connection manager thread to signal its */
/* termination by storing the address of its connection in a shared variable. */
/* Connection resources will be destroyed by the main thread as soon as */
/* possible. */
static pthread_mutex_t destroy_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t destroy_condition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t todestroy_condition = PTHREAD_COND_INITIALIZER;
static Tconnection *ConnectionToDestroy = NULL;

static Tconnection *connections[CONNECTIONS];

/* These variables let access to some information be synchronized */
static pthread_mutex_t raw_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t packet_mutex = PTHREAD_MUTEX_INITIALIZER;
static Tconnection *RawConnection = NULL;

/* Variable which holds every signal any thread must ignore since brltty */
/* already use them. */
static sigset_t BlockedSignals;

/* Pointer on subroutines of the real braille driver */
static const BrailleDriver *TrueBraille;
static BrailleDriver ApiBraille;

/* Identication of the REAL braille driver currently used */
static uint32_t DisplaySize[2] = { 0, 0 };

static int authKeyLength = 0;
static unsigned char authKey[BRLAPI_MAXPACKETSIZE];

/****************************************************************************/
/** SOME PROTOTYPES                                                        **/
/****************************************************************************/

extern void processParameters(char ***values, const char *const *names, const char *description, char *optionParameters, char *configuredParameters, const char *environmentVariable);
static int initializeUnmaskedKeys(Tconnection *c);

/****************************************************************************/
/** SIGNALS HANDLING                                                       **/
/****************************************************************************/

/* Function : initBlockedSignals */
/* Prepares BlockedSignals */
/* *Must* be called before any call to pthread_create */
static inline void initBlockedSignals()
{
  sigemptyset(&BlockedSignals);
  sigaddset(&BlockedSignals,SIGTERM);
  sigaddset(&BlockedSignals,SIGINT);
  sigaddset(&BlockedSignals,SIGPIPE);
  sigaddset(&BlockedSignals,SIGCHLD);
  sigaddset(&BlockedSignals,SIGUSR1);
}

/****************************************************************************/
/** PACKET MANAGING                                                        **/
/****************************************************************************/

/* Function : writeAckPacket */
/* Sends an acknowledgement on the given socket */
static inline void writeAckPacket(int fd)
{
  brlapi_writePacket(fd,BRLPACKET_ACK,NULL,0);
}

/* Function : writeErrorPacket */
/* Sends the given error code on the given socket */
static void writeErrorPacket(int fd,unsigned long int err)
{
  uint32_t code = htonl(err);
  brlapi_writePacket(fd,BRLPACKET_ERROR,&code,sizeof(code));
}

/****************************************************************************/
/** COMMUNICATION PROTOCOLE HANDLING                                       **/
/****************************************************************************/

/* Function : createConnection */
/* Creates a struct of type Tconnection and stores suitable values */
/* x and y correspond to the braille display size */
/* This function also record the connection in an array */
/* If an error occur, one returns NULL, and an error message is written on */
/* the socket before closing it */
static Tconnection *createConnection(int fd,int x, int y)
{
  int i;
  Tconnection *c;
  pthread_mutex_lock(&connections_mutex);
  for (i=0; i<CONNECTIONS; i++) if (connections[i]==NULL) {
    connections[i] = (Tconnection *) malloc(sizeof(Tconnection));
    if (connections[i]==NULL) {
      pthread_mutex_unlock(&connections_mutex);
      writeErrorPacket(fd,BRLERR_NOMEM);
      close(fd);
      return NULL;
    }
    c = connections[i];
    if (x*y > 0) {
      c->brl.buffer = (unsigned char *) malloc(x*y);
      if (c->brl.buffer==NULL) {
        connections[i] = NULL;
        pthread_mutex_unlock(&connections_mutex);
        writeErrorPacket(fd,BRLERR_NOMEM);
        close(fd);
        free(c);
        return NULL;
      }
    } else c->brl.buffer = NULL;
    /* From now on, nothing can prevent us from settling connection */
    /* That's why one can already release the mutex : one can assume one won't */
    /* have to modify connections for now... */
    pthread_mutex_unlock(&connections_mutex);
    c->brl.x = x; c->brl.y = y;
    c->id = i;
    c->fd = fd;
    c->tty = -1;
    c->raw = 0;
    c->cursor = 0;
    c->brlbufstate = EMPTY;
    pthread_mutex_init(&c->brlmutex,NULL);
    pthread_mutex_init(&c->maskmutex,NULL);
    c->how = 0;
    c->UnmaskedKeys = NULL;
    return c;
  }
  /* One got here ? Uh, no connection identicator could be found... */
  pthread_mutex_unlock(&connections_mutex);
  writeErrorPacket(fd,BRLERR_NOMEM);
  close(fd);
  return NULL;
}

/* Function : freeConnection */
/* Frees all resources associated to a connection, ie : */
/* - updates the connections array */
/* - destroys resources associated to brlmutex */
/* - frees memory used by struct Tconnection */
static void freeConnection(Tconnection *c)
{
  pthread_mutex_destroy(&c->brlmutex);
  pthread_mutex_destroy(&c->maskmutex);
  pthread_mutex_lock(&connections_mutex);
  connections[c->id] = NULL;
  pthread_mutex_unlock(&connections_mutex);
  if (c->brl.buffer!=NULL) free(c->brl.buffer);
  FreeRangeList(&c->UnmaskedKeys);
  free(c);
}

/* Function : endThread */
/* Lets the thread associated to the given connection to signal its */
/* termination */
/* No resource is freed here (would somewhat gargble everything !) */
static void endThread(void *arg)
{
  Tconnection *c = (Tconnection *) arg;
  LogPrint(LOG_DEBUG,"Ending thread associated to connection %d",c->id);
  close(c->fd); /* Pour libérer au plus vite les descripteurs */
  pthread_mutex_lock(&destroy_mutex);
  while (ConnectionToDestroy!=NULL)
    pthread_cond_wait(&destroy_condition,&destroy_mutex);
  ConnectionToDestroy = c;
  pthread_cond_signal(&todestroy_condition);
  pthread_mutex_unlock(&destroy_mutex);
}

/* Function : processConnection */
/* Root function for connection manager threads */
static void *processConnection(void *arg)
{
  int i, go_on = 1;
  ssize_t size;
  Tconnection *c = (Tconnection *) arg;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  uint32_t * ints = (uint32_t *) &packet[0];
  brl_type_t type;
  pthread_cleanup_push(endThread, arg);
  LogPrint(LOG_DEBUG,"Beginning to process connection %d",c->id);
  while (go_on) {
    if ((size=brlapi_readPacket(c->fd,&type,packet,BRLAPI_MAXPACKETSIZE))<0) {
      if (size==-1) LogPrint(LOG_WARNING,"read : %s (connection %d)",strerror(errno),c->id);
      else LogPrint(LOG_WARNING,"Unexpected end of file");
      break;
    }
    switch (type) {
      case BRLPACKET_GETTTY: {            uint32_t tty, how, n;
        unsigned char *p = INITIAL_MESSAGE;
        LogPrint(LOG_DEBUG,"Received GetTty Request");
        CHECK((!c->raw),BRLERR_ILLEGAL_INSTRUCTION);
        CHECK((size==2*sizeof(uint32_t)),BRLERR_INVALID_PACKET);
        how = ntohl(ints[1]);
        CHECK(((how == BRLKEYCODES) || (how == BRLCOMMANDS)),BRLERR_INVALID_PARAMETER);
        tty = ntohl(ints[0]);
        if (c->tty==tty) {
          if (c->how==how) {
            LogPrint(LOG_WARNING,"One already controls tty %d",tty);
            writeAckPacket(c->fd);
          } else {
            /* Here one is in the case where the client tries to change */
            /* from BRLKEYCODES to BRLCOMMANDS, or something like that */
            /* For the moment this operation is not supported */
            /* A client that wants to do that should first LeaveTty() */
            /* and then get it again, risking to lose it */
            LogPrint(LOG_DEBUG,"Switching from BRLKEYCODES to BRLCOMMANDS not supported yet");
            writeErrorPacket(c->fd,BRLERR_OPNOTSUPP);
          }
          continue;
        }
        if (how==BRLKEYCODES) { /* We must check if the braille driver supports that */
          if ((TrueBraille->readKey==NULL) || (TrueBraille->keyToCommand==NULL)) {
            writeErrorPacket(c->fd, BRLERR_KEYSNOTSUPP);
            continue;
          }
        }
        pthread_mutex_lock(&connections_mutex);
        for (i=0; i<CONNECTIONS; i++)
          if ((connections[i]!=NULL) && (connections[i]->tty == tty)) break;
        if (i<CONNECTIONS) {
          LogPrint(LOG_WARNING,"tty %d is already controled by someone else",tty);
          writeErrorPacket(c->fd,BRLERR_TTYBUSY);
        } else {
          c->tty = tty;
          c->how = how;
          if (initializeUnmaskedKeys(c)==-1) {
            LogPrint(LOG_WARNING,"Failed to initialize unmasked keys");
            FreeRangeList(&c->UnmaskedKeys);
            writeErrorPacket(c->fd,BRLERR_NOMEM);
          } else {
            /* The following code fills braille display with blank spaces */
            /* this avoids random display, when client sends no text immediately */
            n = MIN(strlen(p),c->brl.x*c->brl.y);
            for (i=0; i<n; i++) *(c->brl.buffer+i) = textTable[*(p+i)];
            for (i=n; i<c->brl.x*c->brl.y; i++) *(c->brl.buffer+i) = textTable[' '];
            c->brlbufstate = FULL;
            writeAckPacket(c->fd);
            LogPrint(LOG_DEBUG,"Taking control of tty %d (how=%d)",tty,how);
          }
        }
        pthread_mutex_unlock(&connections_mutex);
        continue;
      }
      case BRLPACKET_LEAVETTY: {
        CHECK(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        CHECK(c->tty!=-1,BRLERR_ILLEGAL_INSTRUCTION);
        pthread_mutex_lock(&connections_mutex);
        LogPrint(LOG_DEBUG,"Releasing tty %d",c->tty);
        c->tty = -1;
        FreeRangeList(&c->UnmaskedKeys);
        pthread_mutex_unlock(&connections_mutex);
        writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_MASKKEYS:
      case BRLPACKET_UNMASKKEYS: {
        int res;
        brl_keycode_t x,y;
        if (type==BRLPACKET_MASKKEYS) LogPrint(LOG_DEBUG,"Received MaskKeys request");
        else LogPrint(LOG_DEBUG,"Received UnmaskKeys request");
        CHECK(( (!c->raw) && (c->tty!=-1) ),BRLERR_ILLEGAL_INSTRUCTION);
        CHECK(size==2*sizeof(brl_keycode_t),BRLERR_INVALID_PACKET);
        x = ntohl(ints[0]);
        y = ntohl(ints[1]);
        LogPrint(LOG_DEBUG,"range: [%u..%u]",x,y);
        pthread_mutex_lock(&c->maskmutex);
        if (type==BRLPACKET_MASKKEYS) res = RemoveRange(x,y,&c->UnmaskedKeys);
        else res = AddRange(x,y,&c->UnmaskedKeys);
        pthread_mutex_unlock(&c->maskmutex);
        if (res==-1) writeErrorPacket(c->fd,BRLERR_NOMEM);
        else writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_WRITEDOTS: {
        LogPrint(LOG_DEBUG,"Received WriteDots request... ");
        CHECK(((!c->raw)&&(c->tty!=-1)),BRLERR_ILLEGAL_INSTRUCTION);
        CHECK(size==c->brl.x*c->brl.y,BRLERR_INVALID_PACKET);
        pthread_mutex_lock(&c->brlmutex);
        memcpy(c->brl.buffer, packet,c->brl.x*c->brl.y);
        c->brlbufstate = FULL;
        pthread_mutex_unlock(&c->brlmutex);
        writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_STATWRITE: {
        CHECK(((!c->raw)&&(c->tty!=-1)),BRLERR_ILLEGAL_INSTRUCTION);
        writeErrorPacket(c->fd,BRLERR_OPNOTSUPP);
        continue;
      }
      case BRLPACKET_EXTWRITE: {
        extWriteStruct *ews = (extWriteStruct *) packet;
        uint32_t dispSize, cursor, rbeg, rend, strLen;
        unsigned char *p = &ews->data;
        unsigned char buf[200]; /* dirty! */
        LogPrint(LOG_DEBUG,"Received Extended Write request...");
        CHECK(size>=sizeof(ews->flags), BRLERR_INVALID_PACKET);
        CHECK(((!c->raw)&&(c->tty!=-1)),BRLERR_ILLEGAL_INSTRUCTION);
        pthread_mutex_lock(&c->brlmutex);
        cursor = c->cursor;
        dispSize = c->brl.x*c->brl.y;
        memcpy(buf, c->brl.buffer, dispSize);
        pthread_mutex_unlock(&c->brlmutex);
        size -= sizeof(ews->flags); /* flags */
        CHECK((ews->flags & BRLAPI_EWF_DISPLAYNUMBER)==0, BRLERR_OPNOTSUPP);
        if (ews->flags & BRLAPI_EWF_REGION) {
          CHECK(size>2*sizeof(uint32_t), BRLERR_INVALID_PACKET);
          rbeg = ntohl( *((uint32_t *) p) );
          p += sizeof(uint32_t); size -= sizeof(uint32_t); /* region begin */
          rend = ntohl( *((uint32_t *) p) );
          p += sizeof(uint32_t); size -= sizeof(uint32_t); /* region end */
          CHECK(
            (1<=rbeg) && (rbeg<=dispSize) && (1<=rend) && (rend<=dispSize)
            && (rbeg<=rend), BRLERR_INVALID_PARAMETER);
        } else {
          rbeg = 1;
          rend = dispSize;
        }
        strLen = (rend-rbeg) + 1;
        if (ews->flags & BRLAPI_EWF_TEXT) {
          CHECK(size>=strLen, BRLERR_INVALID_PACKET);
          for (i=rbeg-1; i<rend; i++)
            buf[i] = textTable[*(p+i)];
          p += strLen; size -= strLen; /* text */
        }
        if (ews->flags & BRLAPI_EWF_ATTR_AND) {
          CHECK(size>=strLen, BRLERR_INVALID_PACKET);
          for (i=rbeg-1; i<rend; i++)
            buf[i] &= *(p+i);
          p += strLen; size -= strLen; /* and attributes */
        }
        if (ews->flags & BRLAPI_EWF_ATTR_OR) {
          CHECK(size>=strLen, BRLERR_INVALID_PACKET);
          for (i=rbeg-1; i<rend; i++)
            buf[i] |= *(p+i);
          p += strLen; size -= strLen; /* or attributes */
        }
        if (ews->flags & BRLAPI_EWF_CURSOR) {
          CHECK(size>=sizeof(uint32_t), BRLERR_INVALID_PACKET);
          cursor = ntohl( *((uint32_t *) p) );
          p += sizeof(uint32_t); size -= sizeof(uint32_t); /* cursor */
          CHECK(cursor<=dispSize, BRLERR_INVALID_PACKET);
        }
        CHECK(size==0, BRLERR_INVALID_PACKET);
        /* Here all the packet has been processed. */
        /* We can now set the cursor if any, and update the actual buffer */
        /* with the new information to display */
        if (cursor) buf[cursor-1] |= cursorDots();
        pthread_mutex_lock(&c->brlmutex);
        c->cursor = cursor;
        memcpy(c->brl.buffer, buf, dispSize);
        c->brlbufstate = FULL;
        pthread_mutex_unlock(&c->brlmutex);
        writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_GETRAW: {
        uint32_t raw_magic;
        LogPrint(LOG_DEBUG,"Received GetRaw request");
        CHECK(((TrueBraille->readPacket!=NULL) && (TrueBraille->writePacket!=NULL)), BRLERR_RAWNOTSUPP);
        if (c->raw) {
          LogPrint(LOG_DEBUG,"satisfied immediately since one already is in raw mode");
          writeAckPacket(c->fd);
          continue;
        }
        raw_magic = ntohl(ints[0]);
        CHECK(raw_magic==BRLRAW_MAGIC,BRLERR_INVALID_PARAMETER);
        pthread_mutex_lock(&raw_mutex);
        c->raw = 1;
        RawConnection = c;
        writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_LEAVERAW: {
        LogPrint(LOG_DEBUG,"Received LeaveRaw request");
        CHECK(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        LogPrint(LOG_DEBUG,"Going out of raw mode");
        c->raw = 0;
        RawConnection = NULL;
        pthread_mutex_unlock(&raw_mutex);
        writeAckPacket(c->fd);
        continue;
      }
      case BRLPACKET_PACKET: {
        LogPrint(LOG_DEBUG,"Received PutPacket request");
        CHECK(c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        LogPrint(LOG_DEBUG,"Size: %d Contents: %c %c %c %x %x ",size,*packet,packet[1],packet[2],packet[3],packet[4]);
        pthread_mutex_lock(&packet_mutex);
        TrueBraille->writePacket(&c->brl,packet,size);
        pthread_mutex_unlock(&packet_mutex);
        continue;
      }
      case BRLPACKET_GETDRIVERID: {
        CHECK(size==0,BRLERR_INVALID_PACKET);
        LogPrint(LOG_DEBUG,"Received GetDriverId request");
        CHECK(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        brlapi_writePacket(c->fd,BRLPACKET_GETDRIVERID,braille->identifier,strlen(braille->identifier)+1);
        continue;
      }
      case BRLPACKET_GETDRIVERNAME: {
        CHECK(size==0,BRLERR_INVALID_PACKET);
        LogPrint(LOG_DEBUG,"Received GetDriverName request");
        CHECK(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        brlapi_writePacket(c->fd,BRLPACKET_GETDRIVERNAME,braille->name,strlen(braille->name)+1);
        continue;
      }
      case BRLPACKET_GETDISPLAYSIZE: {
        CHECK(size==0,BRLERR_INVALID_PACKET);
        LogPrint(LOG_DEBUG,"GetDisplaySize request");
        CHECK(!c->raw,BRLERR_ILLEGAL_INSTRUCTION);
        brlapi_writePacket(c->fd,BRLPACKET_GETDISPLAYSIZE,&DisplaySize[0],sizeof(DisplaySize));
        continue;
      }
      case BRLPACKET_BYE: {
        LogPrint(LOG_DEBUG,"Client %d said Bye",c->id);
        writeAckPacket(c->fd);
        go_on = 0;
        continue;
      }
      default:
        writeErrorPacket(c->fd,BRLERR_UNKNOWN_INSTRUCTION);
    }
  }
  if (c->raw) {
    c->raw = 0;
    RawConnection = NULL;
    pthread_mutex_unlock(&raw_mutex);
    LogPrint(LOG_WARNING,"Client %d did not give up raw mode properly",c->id);
    LogPrint(LOG_WARNING,"Trying to reset braille terminal");
    if (!TrueBraille->reset(&c->brl)) {
      LogPrint(LOG_WARNING,"Reset failed. Restarting braille driver");
      restartBrailleDriver();
    }
  }
  if (c->tty!=-1) {
    LogPrint(LOG_WARNING,"Client %d did not give up control of tty %d properly",c->id,c->tty);
    /* One frees the terminal so as brltty to take back its control as soon as */
    /* possible... */
    pthread_mutex_lock(&connections_mutex);
    c->tty = -1;
    FreeRangeList(&c->UnmaskedKeys);
    pthread_mutex_unlock(&connections_mutex);
  }
  pthread_cleanup_pop(1);
  return NULL;
}

/****************************************************************************/
/** SOCKETS AND CONNECTIONS MANAGING                                       **/
/****************************************************************************/

/* Function : initializeSocket */
/* Creates the listening socket for in-connections */
/* Returns the descriptor, or -1 if an error occurred */
static int initializeSocket(const char *host)
{
  int fd=-1, err, yes=1;
  struct addrinfo *res,*cur;
  struct addrinfo hints;
  char *hostname,*port;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  brlapi_splitHost(host,&hostname,&port);

  err = getaddrinfo(hostname, port, &hints, &res);
  if (hostname)
    free(hostname);
  free(port);
  if (err) {
    LogPrint(LOG_WARNING,"getaddrinfo(%s:%s,%s) : %s",host,hostname,port,gai_strerror(err));
    return -1;
  }
  for (cur = res; cur; cur = cur->ai_next) {
    fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
    if (fd<0) {
      if (errno != EAFNOSUPPORT)
        LogPrint(LOG_WARNING,"socket : %s",strerror(errno));
      continue;
    }
    /* Specifies that address can be reused */
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))!=0) {
      LogError("setsockopt");
      close(fd);
       continue;
    }
    if (bind(fd, cur->ai_addr, cur->ai_addrlen)<0) {
      if (errno!=EADDRNOTAVAIL && errno!=EADDRINUSE) {
        LogPrint(LOG_WARNING,"bind : %s",strerror(errno));
        close(fd);
        continue;
      }
      do {
        sleep(1);
        err = bind(fd, cur->ai_addr, cur->ai_addrlen);
      } while ((err == -1) && (errno==EADDRNOTAVAIL));
    }
    if (listen(fd,1)<0) {
      LogPrint(LOG_WARNING,"listen : %s",strerror(errno));
      close(fd);
      continue;
    }
    break;
  }
  freeaddrinfo(res);
  if (cur) return fd;
  LogPrint(LOG_WARNING,"unable to find a local TCP port %s !",host);
  return -1;
}

/* Function : closeSocket */
/* Closes the listening socket for in-connections */
static void closeSocket(int fd)
{
  close(fd);
}

/* Function : connectionsManager */
/* Creates a socket to listen incoming connections as soon as possible */
/* (i.e. when the loopback interface is configured), accepts incoming connections */
/* and areates data sructures aod threads to process them properly*/
/* Argument : The port on which the socket will be bound */
/* Returns NULL in any case */
static void *connectionsManager(void *arg)
{
  int res, mainsock;
  struct sockaddr addr;
  socklen_t addrlen = sizeof(addr);
  char *host = (char *) arg;
  Tconnection *c;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  authStruct *auth = (authStruct *) packet;
  brl_type_t type;
  ssize_t size;
  int ok;

  if ((res = pthread_sigmask(SIG_BLOCK,&BlockedSignals,NULL))!=0) {
    LogPrint(LOG_WARNING,"pthread_sigmask : %s",strerror(res));
    pthread_exit(NULL);
  }
  if ((mainsock = initializeSocket(host))==-1) {
    LogPrint(LOG_WARNING,"Error while initializing socket : %s",strerror(errno));
    return NULL;
  }
  pthread_cleanup_push((void (*) (void *)) closeSocket,(void *) mainsock);
  for (; (res = accept(mainsock, &addr, &addrlen))>=0; addrlen=sizeof(addr)) {
    LogPrint(LOG_DEBUG,"Connection accepted on socket %d",res);
    ok=0;
    size = brlapi_readPacket(res,&type,packet,BRLAPI_MAXPACKETSIZE);
    LogPrint(LOG_DEBUG,"read packet of size %d and type %d",size,type);
    if (type==BRLPACKET_AUTHKEY) {
      if ((size == authKeyLength) && (!memcmp(packet, authKey, size)))
        writeErrorPacket(res, BRLERR_PROTOCOL_VERSION);
      else {
        if (size==sizeof(auth->protocolVersion)+authKeyLength) {
          if (auth->protocolVersion==BRLAPI_PROTOCOL_VERSION) {
            if (!memcmp(&auth->key, authKey, authKeyLength)) {
              writeAckPacket(res);
              ok = 1;
            } else writeErrorPacket(res, BRLERR_CONNREFUSED);
          } else writeErrorPacket(res, BRLERR_PROTOCOL_VERSION);
        } else writeErrorPacket(res, BRLERR_CONNREFUSED);
      }
    } else writeErrorPacket(res, BRLERR_CONNREFUSED);
    if (!ok) { close(res); continue; }
    c = createConnection(res,ntohl(DisplaySize[0]),ntohl(DisplaySize[1]));
    if (c==NULL) {
      LogPrint(LOG_WARNING,"Failed to create connection structure");
      /* send a BRLPACKET_BYE, or ERROR ? */
      close(res);
      continue;
    }
    LogPrint(LOG_DEBUG,"Connection on socket %d has identifier %d",c->fd,c->id);
    if ((res = pthread_create(&(c->thread),NULL,processConnection,(void *) c))!=0) {
      LogPrint(LOG_WARNING,"pthread_create : %s",strerror(res));
      /* send a BRLPACKET_BYE, or ERROR ? */
      close(c->fd);
      freeConnection(c);
      continue;
    }
  }
  if (res < 0)
    LogPrint(LOG_WARNING,"accept : %s",strerror(errno));
  pthread_cleanup_pop(1);
  return NULL;
}

/****************************************************************************/
/** MISCELLANEOUS FUNCTIONS                                                **/
/****************************************************************************/

/* Function : initializeUnmaskedKeys */
/* Specify which keys should be passed to the client by default, as soon */
/* as he controls the tty */
/* If clients asked for commands, one lets him process routing cursor */
/* and screen-related commands */
/* If the client is interested in braille codes, one passes him every key */
static int initializeUnmaskedKeys(Tconnection *c)
{
  if (c==NULL) return 0;
  if (AddRange(0,BRL_KEYCODE_MAX,&c->UnmaskedKeys)==-1) return -1;
  if (c->how==BRLKEYCODES) return 0;
  if (RemoveRange(CMD_SWITCHVT_PREV,CMD_SWITCHVT_NEXT,&c->UnmaskedKeys)==-1) return -1;
  if (RemoveRange(CMD_RESTARTBRL,CMD_RESTARTSPEECH,&c->UnmaskedKeys)==-1) return -1;
  if (RemoveRange(CR_SWITCHVT,CR_SWITCHVT|0XFF,&c->UnmaskedKeys)==-1) return -1;
  return 0;
}

/* Function : cleanUp */
/* Checks whether a connection is to be destroyed, and if so, calls */
/* freeConnection */
static void cleanUp()
{
  int res;
  pthread_mutex_lock(&destroy_mutex);
  if (ConnectionToDestroy!=NULL) {
    if ((res = pthread_join(ConnectionToDestroy->thread,NULL))!=0) {
      LogPrint(LOG_WARNING,"pthread_join : %s",strerror(res));
    }
    freeConnection(ConnectionToDestroy);
    ConnectionToDestroy = NULL;
    pthread_cond_signal(&destroy_condition);
  }
  pthread_mutex_unlock(&destroy_mutex);
}

/* Function : terminationHandler */
/* Terminates driver */
static void terminationHandler()
{
  int i,res;
  if (connectionsAllowed!=0) {
    if ((res = pthread_cancel(t))!=0) {
      LogPrint(LOG_WARNING,"pthread_cancel (1) : %s",strerror(res));
    } else if ((res = pthread_join(t,NULL))!=0) {
      LogPrint(LOG_WARNING,"pthread_join : %s",strerror(res));
    }
    for (i=0;i<CONNECTIONS;i++) if (connections[i]!=NULL) {
      if ((res = pthread_cancel(connections[i]->thread))!=0) {
        LogPrint(LOG_WARNING,"pthread_cancel (2) : %s",strerror(res));
        freeConnection(connections[i]);
        connections[i]=NULL;
      }
    }
    for (i=0;i<CONNECTIONS;i++) if (connections[i]!=NULL) {
      int ok=0;
      while (!ok) {
        pthread_mutex_lock(&destroy_mutex);
        while (!ConnectionToDestroy)
          pthread_cond_wait(&todestroy_condition,&destroy_mutex);
        ok=ConnectionToDestroy==connections[i];
        if ((res = pthread_join(connections[ConnectionToDestroy->id]->thread,NULL))!=0)
          LogPrint(LOG_WARNING,"pthread_join : %s",strerror(res));
        connections[ConnectionToDestroy->id]=NULL;
        freeConnection(ConnectionToDestroy);
        ConnectionToDestroy=NULL;
        pthread_cond_broadcast(&destroy_condition);
        pthread_mutex_unlock(&destroy_mutex);
      }
    }
  }
  api_unlink();
}

/* Function : api_writeWindow */
static void api_writeWindow(BrailleDisplay *brl)
{
  int i,tty;
  if (RawConnection!=NULL) return;
  pthread_mutex_lock(&connections_mutex);
  tty = currentVirtualTerminal();
  for (i=0;  i<CONNECTIONS; i++) if ((connections[i]!=NULL) && (connections[i]->tty==tty)) {
    pthread_mutex_unlock(&connections_mutex);
    return;
  }
  pthread_mutex_unlock(&connections_mutex);
  TrueBraille->writeWindow(brl);
}

/* Function : api_readCommand */
static int api_readCommand(BrailleDisplay *disp, DriverCommandContext caller)
{
  int i,tty,res,refresh = 0, masked;
  static int oldtty = 0;
  Tconnection *c = NULL;
  unsigned char packet[BRLAPI_MAXPACKETSIZE];
  brl_keycode_t keycode = 0;

  cleanUp(); /* destroys remaining connections if any */
  if (RawConnection!=NULL) {
    pthread_mutex_lock(&packet_mutex);
    res = TrueBraille->readPacket(&c->brl,packet,BRLAPI_MAXPACKETSIZE);
    pthread_mutex_unlock(&packet_mutex);
    if (res>0) {
      LogPrint(LOG_DEBUG,"Size: %d Contents: %c %c %c %x %x ",res,*packet,packet[1],packet[2],packet[3],packet[4]);
      brlapi_writePacket(RawConnection->fd,BRLPACKET_PACKET,packet,res);
    }
    return EOF;
  }
  pthread_mutex_lock(&connections_mutex);
  tty = currentVirtualTerminal();
  if (tty!=oldtty) {
    refresh = 1;
    oldtty = tty;
  }
  for (i=0; i<CONNECTIONS; i++) if ((connections[i]!=NULL) && (connections[i]->tty==tty)) {
    c = connections[i];
    break;
  }
  pthread_mutex_unlock(&connections_mutex);
  if (c!=NULL) {
    pthread_mutex_lock(&c->brlmutex);
    if ((c->brlbufstate==FULL) || (refresh)) {
      TrueBraille->writeWindow(&c->brl);
      c->brlbufstate = EMPTY;
    }
    pthread_mutex_unlock(&c->brlmutex);
    if (c->how==BRLKEYCODES) res = TrueBraille->readKey(&c->brl);
    else res = TrueBraille->readCommand(&c->brl,caller);
    if (res==EOF) return EOF;
    keycode = (brl_keycode_t) res;
    pthread_mutex_lock(&c->maskmutex);
    masked = (contains(c->UnmaskedKeys,keycode) == NULL);
    pthread_mutex_unlock(&c->maskmutex);
    if (!masked) {
      if (c->how==BRLKEYCODES) {
        LogPrint(LOG_DEBUG,"Transmitting unmasked key %u",keycode);
        keycode = htonl(keycode);
        brlapi_writePacket(c->fd,BRLPACKET_KEY,&keycode,sizeof(keycode));
      } else {
        LogPrint(LOG_DEBUG,"Transmitting unmasked command %u",keycode);
        keycode = htonl(keycode);
        brlapi_writePacket(c->fd,BRLPACKET_COMMAND,&keycode,sizeof(keycode));
      }
      return EOF;
    }
    if (c->how==BRLKEYCODES) keycode = TrueBraille->keyToCommand(&c->brl,caller,keycode);
  }
  if (keycode==0) {
    res = TrueBraille->readCommand(disp,caller);
    if (res==EOF) return EOF;
    keycode = (brl_keycode_t) res;
  }
  return keycode;
}

/* Function : api_link */
/* Does all the link stuff to let api get events from the driver and */
/* writes from brltty */
void api_link(void)
{
  TrueBraille=braille;
  memcpy(&ApiBraille,braille,sizeof(BrailleDriver));
  ApiBraille.writeWindow=api_writeWindow;
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
  braille=TrueBraille;
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
void api_open(BrailleDisplay *brl, char **parameters)
{
  static char *host;
  int res;

  DisplaySize[0] = htonl(brl->x);
  DisplaySize[1] = htonl(brl->y);

  api_link();

  res = brlapi_loadAuthKey((*parameters[PARM_KEYFILE]?parameters[PARM_KEYFILE]:BRLAPI_DEFAUTHPATH),
                           &authKeyLength,authKey);
  if (res==-1) {
    LogPrint(LOG_WARNING,"Unable to load API authentication key: no connections will be accepted.");
    connectionsAllowed = 0;
    return;
  }
  for (res=0; res<CONNECTIONS; res++) connections[res] = NULL;
  initBlockedSignals();

  if (*parameters[PARM_HOST]) host = parameters[PARM_HOST];
  else host = "127.0.0.1";

  if ((res = pthread_create(&t,NULL,connectionsManager,(void *) host)) != 0) {
    LogPrint(LOG_WARNING,"pthread_create : %s",strerror(res));
    return;
  } else connectionsAllowed = 1;
}

/* Function : api_close */
/* End of BrlApi session. Closes the listening socket */
/* destroys opened connections and associated resources */
/* Closes the driver */
void api_close(BrailleDisplay *brl)
{
  terminationHandler();
}
