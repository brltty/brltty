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

#include "prologue.h"

#include <string.h>

#include "misc.h"
#include "auth.h"

/* peer credentials */

#include <sys/socket.h>

#if defined(HAVE_GETPEERUCRED)

#include <ucred.h>

#ifdef HAVE_GETZONEID
#include <zone.h>
#endif /* HAVE_GETZONEID */

typedef ucred_t *PeerCredentials;

static int
initializePeerCredentials (PeerCredentials *credentials, int fd) {
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

typedef struct ucred PeerCredentials;

static int
initializePeerCredentials (PeerCredentials *credentials, int fd) {
  size_t length = sizeof(*credentials);
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

#else /* peer credentials method */
#warning peer credentials support not available on this platform

typedef void *PeerCredentials;

static int
initializePeerCredentials (PeerCredentials *credentials, int fd) {
  return 0;
}

static void
releasePeerCredentials (PeerCredentials *credentials) {
}

static int
checkPeerUser (PeerCredentials *credentials, uid_t user) {
  return 0;
}

static int
checkPeerGroup (PeerCredentials *credentials, gid_t group) {
  return 0;
}

#endif /* peer credentials method */

/* general type definitions */

typedef int (*MethodPerformer) (AuthDescriptor *auth, int fd, void *data);

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

typedef enum {
  PCS_NEED,
  PCS_HAVE,
  PCS_GOOD
} PeerCredentialsState;

struct AuthDescriptorStruct {
  int count;
  char **parameters;
  MethodDescriptor *methods;

  PeerCredentialsState peerCredentialsState;
  PeerCredentials peerCredentials;
};

static int
getPeerCredentials (AuthDescriptor *auth, int fd) {
  if (auth->peerCredentialsState == PCS_NEED) {
    if (!initializePeerCredentials(&auth->peerCredentials, fd)) return 0;
    auth->peerCredentialsState = PCS_HAVE;
  }
  return 1;
}

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
authKeyfile_client (AuthDescriptor *auth, int fd, void *data) {
  return 1;
}

static int
authKeyfile_server (AuthDescriptor *auth, int fd, void *data) {
  MethodDescriptor_keyfile *keyfile = data;
  LogPrint(LOG_DEBUG, "checking key file: %s", keyfile->path);
  return 1;
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
authUser_server (AuthDescriptor *auth, int fd, void *data) {
  MethodDescriptor_user *user = data;
  if (auth->peerCredentialsState != PCS_GOOD) {
    if (!getPeerCredentials(auth, fd)) return 0;
    if (checkPeerUser(&auth->peerCredentials, user->id)) auth->peerCredentialsState = PCS_GOOD;
  }
  return 1;
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
authGroup_server (AuthDescriptor *auth, int fd, void *data) {
  MethodDescriptor_group *group = data;
  if (auth->peerCredentialsState != PCS_GOOD) {
    if (!getPeerCredentials(auth, fd)) return 0;
    if (checkPeerGroup(&auth->peerCredentials, group->id)) auth->peerCredentialsState = PCS_GOOD;
  }
  return 1;
}

/* general functions */

static const MethodDefinition methodDefinitions[] = {
  { "keyfile",
    authKeyfile_initialize, authKeyfile_release,
    authKeyfile_client, authKeyfile_server
  },

  { "user",
    authUser_initialize, authUser_release,
    NULL, authUser_server
  },

  { "group",
    authGroup_initialize, authGroup_release,
    NULL, authGroup_server
  },

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
        void *data;
        if ((data = definition->initialize(parameter))) {
          method->definition = definition;
          method->perform = perform;
          method->data = data;
          return 1;
        }
        return 0;
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
      if ((auth->methods = malloc(auth->count * sizeof(*auth->methods)))) {
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
authPerform (AuthDescriptor *auth, int fd) {
  int ok = 1;
  auth->peerCredentialsState = PCS_NEED;

  {
    int index;
    for (index=0; index<auth->count; ++index) {
      const MethodDescriptor *method = &auth->methods[index];
      if (!method->perform(auth, fd, method->data)) {
        ok = 0;
        break;
      }
    }
  }

  if (auth->peerCredentialsState == PCS_HAVE) {
    LogPrint(LOG_ERR, "no matching user or group");
    ok = 0;
  }
  if (auth->peerCredentialsState != PCS_NEED) releasePeerCredentials(&auth->peerCredentials);

  return ok;
}
