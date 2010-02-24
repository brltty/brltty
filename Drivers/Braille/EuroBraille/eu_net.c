/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/** io_net.c -- EuroBraille Driver IO subroutines for Ethernet support.
 ** Maintained by Yannick PLASSIARD <yan@mistigri.org> 
 */


#ifndef __MSDOS__
/**
*** What we do here is dealing with Ethernet IOs, but also 
*** we handle some Protocol stuff specific to ethernet, especially the 
*** initialization stuff (see eubrl_netInit for details).
**/

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "eu_io.h"

#ifdef __MINGW32__
#include <ws2tcpip.h>
#include "sys_windows.h"
#else /* __MINGW32__ */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* __MINGW32__ */

#if !defined(AF_LOCAL) && defined(AF_UNIX)
#define AF_LOCAL AF_UNIX
#endif /* !defined(AF_LOCAL) && defined(AF_UNIX) */
 
#if !defined(PF_LOCAL) && defined(PF_UNIX)
#define PF_LOCAL PF_UNIX
#endif /* !defined(PF_LOCAL) && defined(PF_UNIX) */
 
#ifdef __MINGW32__
#undef AF_LOCAL
#define close(fd) CloseHandle((HANDLE)(fd))
#define LogSocketError(msg) LogWindowsSocketError(msg)
#else /* __MINGW32__ */
#define LogSocketError(msg) LogError(msg)
#endif /* __MINGW32__ */

#include "log.h"
#include "timing.h"
#include "io_misc.h"
#include "cmd.h"

#include "eu_braille.h"

/**
 ** Define our local variables 
 */

static int broadcastFd = -1;
static int realConnectionFd = -1;

typedef enum e_netstate {
  NET_UNITIALIZED,
  NET_LISTENING_BROADCAST,
  NET_MAKING_CONNECTION,
  NET_CONNECTED,
  NET_DISCONNECTED
}		t_netstate;


# define	NET_SEARCH_IRIS_MSG	"IRIS_NET_FFFF_?"
# define	BROADCAST_PORT	1100
# define	IRIS_RECV_INIT_LEN	15

static t_netstate connectionStatus = NET_UNITIALIZED;


int
eubrl_netInit(BrailleDisplay *brl, char **params, const char* device)
{
  connectionStatus = NET_UNITIALIZED;
  broadcastFd = -1;
  struct sockaddr_in broadcastAddress, listenAddress;
  socklen_t len = sizeof(broadcastAddress), listenLen = sizeof(listenAddress);
  char inbuf[256], outbuf[256];
  int optBroadcast = 1;

  memset(inbuf, 0, 256);
  memset(outbuf, 0, 256);
  broadcastAddress.sin_family = AF_INET;
  broadcastAddress.sin_port = htons(BROADCAST_PORT); 
  if ((broadcastFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      LogError("eu: netinit: Error while creating socket");
      return (0);
    }
  broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST;
  if (setsockopt(broadcastFd, SOL_SOCKET, SO_BROADCAST, &optBroadcast, sizeof(optBroadcast)) == -1)
    {
      LogError("Cannot set broadcast flag to true");
      close(broadcastFd);
      broadcastFd = -1;
      return (0);
    }
  strcpy(outbuf, NET_SEARCH_IRIS_MSG);
  if (sendto(broadcastFd, (void *)outbuf, strlen(outbuf), 0,
	     (struct sockaddr *)&broadcastAddress,
	     len) == -1)
    {
      LogError("eu: netinit: Cannot send data.");
      return (0);
    }
  struct sockaddr_in sourceAddress;
  socklen_t sourceLen = sizeof(sourceAddress);
  int nBytes = recvfrom(broadcastFd, (void *)inbuf, IRIS_RECV_INIT_LEN, 0, 
			(struct sockaddr *)&sourceAddress,
			&sourceLen);
  if (nBytes <= 0)
    {
      LogError("eu: netinit: Failed to receive data.");
      close(broadcastFd);
      broadcastFd = -1;
      return (0);
    }
  LogPrint(LOG_DEBUG, "Received %s response from %s:%d.", 
	   inbuf, inet_ntoa(sourceAddress.sin_addr), 
	   ntohs(sourceAddress.sin_port));
  memset(outbuf, 0, sizeof(outbuf));
  strcpy(outbuf, "IRIS_NET_");
  strncat(outbuf + 9, inbuf + 5, 4);
  strcat(outbuf + 13, "_?");
  memset(inbuf, 0, sizeof(inbuf));
  LogPrint(LOG_DEBUG, "Sending %s ...", outbuf);
  if (sendto(broadcastFd, (void *)outbuf, strlen(outbuf), 0,
	     (struct sockaddr *)&broadcastAddress,
	     len) == -1)
    {
      LogError("eu: netinit: Cannot send data.");
      return (0);
    }
  nBytes = recvfrom(broadcastFd, (void *)inbuf, IRIS_RECV_INIT_LEN, 0, 
			(struct sockaddr *)&sourceAddress,
			&sourceLen);
  if (nBytes <= 0)
    {
      LogError("eu: netinit: Failed to receive data.");
      close(broadcastFd);
      broadcastFd = -1;
      return (0);
    }
  LogPrint(LOG_DEBUG, "Received %s response.", inbuf);
  int ourPort;
  struct sockaddr_in localAddress;
  socklen_t localLen;
  if (getsockname(broadcastFd, (struct sockaddr*)&localAddress, &localLen)== -1)
    {
      LogError("Cannot get local address description");
      close(broadcastFd);
      broadcastFd = -1;
      return (0);
    }
  ourPort = ntohs(localAddress.sin_port);
  LogPrint(LOG_DEBUG, "Sourde Address: %s:%d",
	   inet_ntoa(localAddress.sin_addr), ourPort);
  listenAddress.sin_family = AF_INET;
  listenAddress.sin_port = htons(ourPort);
  listenAddress.sin_addr.s_addr = INADDR_ANY;
  realConnectionFd = socket(AF_INET, SOCK_STREAM, 0);
  if (realConnectionFd == -1)
    {
      LogError("eu: netinit: Failed to establish TCP socket server");
      close(broadcastFd);
      broadcastFd = -1;
    }
  if (bind(realConnectionFd, (struct sockaddr *)&listenAddress, listenLen) == -1)
    {
      LogError("eu: netinit: Cannot bind socket");
      close(realConnectionFd);
      close(broadcastFd);
      broadcastFd = realConnectionFd = -1;
      return (0);
    }
  if (listen(realConnectionFd, 5) == -1)
    {
      LogError("eu: netinit: Failed to listen for TCP connection");
      close(realConnectionFd);
      close(broadcastFd);
      realConnectionFd = broadcastFd = -1;
      return (0);
    }
  approximateDelay(200);
  if (ourPort == 0)
    {
      LogPrint(LOG_INFO, "eu: netinit: Failed to negotiate port.");
      close(broadcastFd);
      close(realConnectionFd);
      realConnectionFd = broadcastFd = -1;
      return (0);
    }
  LogPrint(LOG_DEBUG, "eu: netinit: Listening on port %d", ourPort);
  memset(outbuf, 0, sizeof(outbuf));
  strcpy(outbuf, "IRIS_NET_DO_CONNECT");
  LogPrint(LOG_DEBUG, "Sending %s", outbuf);
  if (sendto(broadcastFd, (void *)outbuf, strlen(outbuf), 0,
	     (struct sockaddr *)&broadcastAddress,
	     sourceLen) == -1)
    {
      LogError("eu: netinit: Cannot send data.");
      return (0);
    }
  close(broadcastFd);
  broadcastFd = -1;
  int fd = -1;
  LogPrint(LOG_DEBUG, "Waiting for incoming connection from remote device.");
  if ((fd = accept(realConnectionFd, (struct sockaddr *)&sourceAddress, 
	     &sourceLen)) == -1)
    {
      LogError("eu: netinit: Cannot accept connection");
      close(realConnectionFd);
      realConnectionFd = -1;
      return (0);
    }
  close(realConnectionFd);
  realConnectionFd = fd;
  setBlockingIo(realConnectionFd, 0);
  LogPrint(LOG_INFO, "eu: Ethernet transport initialized, fd=%d.",
	   realConnectionFd);
  connectionStatus = NET_CONNECTED;
  return (1);
}

ssize_t
eubrl_netRead(BrailleDisplay *brl, void *buf, size_t bufsize)
{
  if (connectionStatus != NET_CONNECTED)
    {
      LogPrint(LOG_ERR, "EuroBraille: NET read while not connected.");
      return (-1);
    }
  ssize_t nBytes = readData(realConnectionFd, buf, bufsize, 0, 0);
  return nBytes;
}

ssize_t
eubrl_netWrite(BrailleDisplay *brl, const void *buf, size_t length)
{
  if (connectionStatus != NET_CONNECTED)
    return (-1);
  ssize_t nBytes = writeData(realConnectionFd, buf, length);
  if (nBytes == -1)
    {
      connectionStatus = NET_DISCONNECTED;
    }
  return nBytes;
}

int
eubrl_netClose(BrailleDisplay *brl)
{
  if (broadcastFd > -1)
    {
      close(broadcastFd);
      broadcastFd = -1;
    }
  if (realConnectionFd > -1)
    {
      close(realConnectionFd);
      realConnectionFd = -1;
    }
  connectionStatus = NET_UNITIALIZED;
  return 0;
}
#endif /* __MSDOS__ */
