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

#include "prologue.h"

#include <string.h>
#include <errno.h>

#ifdef WINDOWS
#include <ws2tcpip.h>

#ifdef __MINGW32__
#include <io.h>

#define GET_INT_FD(fd) (_open_osfhandle((long)(fd), O_RDWR))
#else /* __MINGW32__ */
#include <sys/cygwin.h>

#define GET_INT_FD(fd) (cygwin_attach_handle_to_fd("auth descriptor", 1, (fd), TRUE, GENERIC_READ|GENERIC_WRITE))
#endif /* __MINGW32__ */
#else /* WINDOWS */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define GET_INT_FD(fd) (fd)
#endif /* WINDOWS */

#if !defined(AF_LOCAL) && defined(AF_UNIX)
#define AF_LOCAL AF_UNIX
#endif /* !defined(AF_LOCAL) && defined(AF_UNIX) */

#if !defined(PF_LOCAL) && defined(PF_UNIX)
#define PF_LOCAL PF_UNIX
#endif /* !defined(PF_LOCAL) && defined(PF_UNIX) */

#include "misc.h"
#include "auth.h"

/* peer credentials */
#undef CAN_CHECK_CREDENTIALS

#if defined(HAVE_GETPEERUCRED)
#define CAN_CHECK_CREDENTIALS

#include <ucred.h>

#ifdef HAVE_GETZONEID
#include <zone.h>
#endif /* HAVE_GETZONEID */

typedef ucred_t *PeerCredentials;

static int
retrievePeerCredentials (PeerCredentials *credentials, int fd) {
  *credentials = NULL;
  if (getpeerucred(fd, credentials) == -1) {
    LogError("getpeerucred");
    return 0;
  }

#ifdef HAVE_GETZONEID
  if (ucred_getzoneid(*credentials) != getzoneid()) {
    ucred_free(*credentials);
    return 0;
  }
#endif /* HAVE_GETZONEID */

  return 1;
}

static void
releasePeerCredentials (PeerCredentials *credentials) {
  ucred_free(*credentials);
}

static int
checkPeerUser (PeerCredentials *credentials, uid_t user) {
  if (user == ucred_geteuid(*credentials)) return 1;
  return 0;
}

static int
checkPeerGroup (PeerCredentials *credentials, gid_t group) {
  if (group == ucred_getegid(*credentials)) return 1;

  {
    const gid_t *groups;
    int count = ucred_getgroups(*credentials, &groups);
    while (count > 0)
      if (group == groups[--count])
        return 1;
  }

  return 0;
}

#elif defined(SO_PEERCRED)
#define CAN_CHECK_CREDENTIALS

typedef struct ucred PeerCredentials;

static int
retrievePeerCredentials (PeerCredentials *credentials, int fd) {
  socklen_t length = sizeof(*credentials);
  if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, credentials, &length) != -1) return 1;
  LogError("getsockopt[SO_PEERCRED]");
  return 0;
}

static void
releasePeerCredentials (PeerCredentials *credentials) {
}

static int
checkPeerUser (PeerCredentials *credentials, uid_t user) {
  return user == credentials->uid;
}

static int
checkPeerGroup (PeerCredentials *credentials, gid_t group) {
  return group == credentials->gid;
}

#elif defined(HAVE_GETPEEREID)
#define CAN_CHECK_CREDENTIALS

typedef struct {
  uid_t euid;
  gid_t egid;
} PeerCredentials;

static int
retrievePeerCredentials (PeerCredentials *credentials, int fd) {
  if (getpeereid(fd, &credentials->euid, &credentials->egid) != -1) return 1;
  LogError("getpeereid");
  return 0;
}

static void
releasePeerCredentials (PeerCredentials *credentials) {
}

static int
checkPeerUser (PeerCredentials *credentials, uid_t user) {
  return user == credentials->euid;
}

static int
checkPeerGroup (PeerCredentials *credentials, gid_t group) {
  return group == credentials->egid;
}

#else /* peer credentials method */
#warning peer credentials support not available on this platform
#endif /* peer credentials method */

/* general type definitions */

typedef int (*MethodPerformer) (AuthDescriptor *auth, FileDescriptor fd, void *data);

typedef struct {
  const char *name;
  void *(*initialize) (const char *parameter);
  void (*release) (void *data);
  MethodPerformer client;
  MethodPerformer server;
} MethodDefinition;

typedef struct {
  const MethodDefinition *definition;
  MethodPerformer perform;
  void *data;
} MethodDescriptor;

#ifdef CAN_CHECK_CREDENTIALS
typedef enum {
  PCS_NEED,
  PCS_CANT,
  PCS_HAVE
} PeerCredentialsState;
#endif /* CAN_CHECK_CREDENTIALS */

struct AuthDescriptorStruct {
  int count;
  char **parameters;
  MethodDescriptor *methods;

#ifdef CAN_CHECK_CREDENTIALS
  PeerCredentialsState peerCredentialsState;
  PeerCredentials peerCredentials;
#endif /* CAN_CHECK_CREDENTIALS */
};

/* the keyfile method */

typedef struct {
  const char *path;
} MethodDescriptor_keyfile;

static void *
authKeyfile_initialize (const char *parameter) {
  MethodDescriptor_keyfile *keyfile;

  if ((keyfile = malloc(sizeof(*keyfile)))) {
    if (*parameter) {
      keyfile->path = parameter;
      return keyfile;
    } else {
      LogPrint(LOG_ERR, "path to key file not specified");
    }

    free(keyfile);
  } else {
    LogError("malloc");
  }

  return NULL;
}

static void
authKeyfile_release (void *data) {
  MethodDescriptor_keyfile *keyfile = data;
  free(keyfile);
}

static int
authKeyfile_client (AuthDescriptor *auth, FileDescriptor fd, void *data) {
  return 1;
}

static int
authKeyfile_server (AuthDescriptor *auth, FileDescriptor fd, void *data) {
  MethodDescriptor_keyfile *keyfile = data;
  LogPrint(LOG_DEBUG, "checking key file: %s", keyfile->path);
  return 1;
}

#ifdef CAN_CHECK_CREDENTIALS
static int
getPeerCredentials (AuthDescriptor *auth, FileDescriptor fd) {
  if (auth->peerCredentialsState == PCS_NEED) {
    auth->peerCredentialsState = retrievePeerCredentials(&auth->peerCredentials, GET_INT_FD(fd))? PCS_HAVE: PCS_CANT;
  }
  return auth->peerCredentialsState == PCS_HAVE;
}

/* the user method */

#include <pwd.h>

typedef struct {
  uid_t id;
} MethodDescriptor_user;

static void *
authUser_initialize (const char *parameter) {
  MethodDescriptor_user *user;

  if ((user = malloc(sizeof(*user)))) {
    if (!*parameter) {
      user->id = geteuid();
      return user;
    }

    {
      int value;
      if (isInteger(&value, parameter)) {
        user->id = value;
        return user;
      }
    }

    {
      const struct passwd *p = getpwnam(parameter);
      if (p) {
        user->id = p->pw_uid;
        return user;
      }
    }

    LogPrint(LOG_ERR, "unknown user: %s", parameter);
    free(user);
  } else {
    LogError("malloc");
  }

  return NULL;
}

static void
authUser_release (void *data) {
  MethodDescriptor_user *user = data;
  free(user);
}

static int
authUser_server (AuthDescriptor *auth, FileDescriptor fd, void *data) {
  MethodDescriptor_user *user = data;
  return getPeerCredentials(auth, fd) &&
         checkPeerUser(&auth->peerCredentials, user->id);
}

/* the group method */

#include <grp.h>

typedef struct {
  gid_t id;
} MethodDescriptor_group;

static void *
authGroup_initialize (const char *parameter) {
  MethodDescriptor_group *group;

  if ((group = malloc(sizeof(*group)))) {
    if (!*parameter) {
      group->id = getegid();
      return group;
    }

    {
      int value;
      if (isInteger(&value, parameter)) {
        group->id = value;
        return group;
      }
    }

    {
      const struct group *g = getgrnam(parameter);
      if (g) {
        group->id = g->gr_gid;
        return group;
      }
    }

    LogPrint(LOG_ERR, "unknown group: %s", parameter);
    free(group);
  } else {
    LogError("malloc");
  }

  return NULL;
}

static void
authGroup_release (void *data) {
  MethodDescriptor_group *group = data;
  free(group);
}

static int
authGroup_server (AuthDescriptor *auth, FileDescriptor fd, void *data) {
  MethodDescriptor_group *group = data;
  return getPeerCredentials(auth, fd) &&
         checkPeerGroup(&auth->peerCredentials, group->id);
}
#endif /* CAN_CHECK_CREDENTIALS */

/* general functions */

static const MethodDefinition methodDefinitions[] = {
  { "keyfile",
    authKeyfile_initialize, authKeyfile_release,
    authKeyfile_client, authKeyfile_server
  },

#ifdef CAN_CHECK_CREDENTIALS
  { "user",
    authUser_initialize, authUser_release,
    NULL, authUser_server
  },

  { "group",
    authGroup_initialize, authGroup_release,
    NULL, authGroup_server
  },
#endif /* CAN_CHECK_CREDENTIALS */

  {NULL}
};

static void
releaseMethodDescriptor (MethodDescriptor *method) {
  if (method->data) {
    if (method->definition->release) method->definition->release(method->data);
    method->data = NULL;
  }
}

static void
releaseMethodDescriptors (AuthDescriptor *auth, int count) {
  while (count > 0) releaseMethodDescriptor(&auth->methods[--count]);
}

static int
initializeMethodDescriptor (MethodDescriptor *method, const char *parameter, int client) {
  const char *name;
  int nameLength;

  if ((parameter = strchr(name=parameter, ':'))) {
    nameLength = parameter++ - name;
  } else {
    parameter = name + (nameLength = strlen(name));
  }

  {
    const MethodDefinition *definition = methodDefinitions;
    while (definition->name) {
      MethodPerformer perform = client? definition->client: definition->server;
      if (perform &&
          (nameLength == strlen(definition->name)) &&
          (strncmp(name, definition->name, nameLength) == 0)) {
        void *data = definition->initialize(parameter);
        if (!data) return 0;

        method->definition = definition;
        method->perform = perform;
        method->data = data;
        return 1;
      }

      ++definition;
    }
  }

  LogPrint(LOG_WARNING, "unknown authentication/authorization method: %.*s", nameLength, name);
  return 0;
}

static int
initializeMethodDescriptors (AuthDescriptor *auth, int client) {
  int index = 0;
  while (index < auth->count) {
    if (!initializeMethodDescriptor(&auth->methods[index], auth->parameters[index], client)) {
      releaseMethodDescriptors(auth, index);
      return 0;
    }
    ++index;
  }
  return 1;
}

static AuthDescriptor *
authBegin (const char *parameter, int client) {
  AuthDescriptor *auth;

  if ((auth = malloc(sizeof(*auth)))) {
    if (!parameter) parameter = "";
    if (!*parameter) {
      parameter = client? "": "user";
    } else if (strcmp(parameter, "none") == 0) {
      parameter = "";
    }

    if ((auth->parameters = splitString(parameter, '+', &auth->count))) {
      if ((auth->methods = malloc(ARRAY_SIZE(auth->methods, auth->count)))) {
        if (initializeMethodDescriptors(auth, client)) {
          return auth;
        }

        free(auth->methods);
      } else {
        LogError("malloc");
      }

      deallocateStrings(auth->parameters);
    }

    free(auth);
  } else {
    LogError("malloc");
  }

  return NULL;
}

AuthDescriptor *
authBeginClient (const char *parameter) {
  return authBegin(parameter, 1);
}

AuthDescriptor *
authBeginServer (const char *parameter) {
  return authBegin(parameter, 0);
}

void
authEnd (AuthDescriptor *auth) {
  releaseMethodDescriptors(auth, auth->count);
  free(auth->methods);
  deallocateStrings(auth->parameters);
  free(auth);
}

int
authPerform (AuthDescriptor *auth, FileDescriptor fd) {
  int ok = 0;

#ifdef CAN_CHECK_CREDENTIALS
  auth->peerCredentialsState = PCS_NEED;
#endif /* CAN_CHECK_CREDENTIALS */

  {
    int index;
    for (index=0; index<auth->count; ++index) {
      const MethodDescriptor *method = &auth->methods[index];
      if (method->perform(auth, fd, method->data)) {
        ok = 1;
        break;
      }
    }
  }

#ifdef CAN_CHECK_CREDENTIALS
  if (auth->peerCredentialsState != PCS_NEED) {
    if (auth->peerCredentialsState == PCS_HAVE) releasePeerCredentials(&auth->peerCredentials);
    if (!ok) LogPrint(LOG_ERR, "no matching user or group");
  }
#endif /* CAN_CHECK_CREDENTIALS */

  return ok;
}

void
formatAddress (char *buffer, int bufferSize, const void *address, int addressSize) {
  const struct sockaddr *sa = address;
  switch (sa->sa_family) {
#ifndef WINDOWS
    case AF_LOCAL: {
      const struct sockaddr_un *local = address;
      snprintf(buffer, bufferSize, "local %s", local->sun_path);
      break;
    }
#endif /* WINDOWS */

    case AF_INET: {
      const struct sockaddr_in *inet = address;
      snprintf(buffer, bufferSize, "inet %s:%d", inet_ntoa(inet->sin_addr), ntohs(inet->sin_port));
      break;
    }

    default:
#if defined(HAVE_GETNAMEINFO) && !defined(WINDOWS)
      {
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        int err;

        if (!(err = getnameinfo(address, addressSize,
                                host, sizeof(host), service, sizeof(service),
                                NI_NUMERICHOST | NI_NUMERICSERV))) {
          snprintf(buffer, bufferSize, "af=%d %s:%s", sa->sa_family, host, service);
          break;
        }

        if (err != EAI_FAMILY) {
#ifdef HAVE_GAI_STRERROR
          snprintf(buffer, bufferSize, "reverse lookup error for address family %d: %s", 
                   sa->sa_family,
#ifdef EAI_SYSTEM
                   (err == EAI_SYSTEM)? strerror(errno):
#endif /* EAI_SYSTEM */
                   gai_strerror(err));
#else /* HAVE_GAI_STRERROR */
          snprintf(buffer, bufferSize, "reverse lookup error %d for address family %d.",
                   err, sa->sa_family);
#endif /* HAVE_GAI_STRERROR */
          break;
        }
      }
#endif /* GETNAMEINFO */

      {
        int length;
        snprintf(buffer, bufferSize, "address family %d:%n", sa->sa_family, &length);

        {
          const unsigned char *byte = address;
          const unsigned char *end = byte + addressSize;
          while (byte < end) {
            int count;
            snprintf(&buffer[length], bufferSize-length,
                     " %02X%n", *byte++, &count);
            length += count;
          }
        }
      }
      break;
  }
}
