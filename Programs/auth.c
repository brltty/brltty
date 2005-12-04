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
authKeyfile_client (int fd, void *data) {
  return 1;
}

static int
authKeyfile_server (int fd, void *data) {
  MethodDescriptor_keyfile *keyfile = data;
  LogPrint(LOG_DEBUG, "checking key file: %s", keyfile->path);
  return 1;
}

typedef int (*MethodPerformHandler) (int fd, void *data);

typedef struct {
  const char *name;
  void *(*initialize) (const char *parameter);
  void (*release) (void *data);
  MethodPerformHandler client;
  MethodPerformHandler server;
} MethodDefinition;

static const MethodDefinition methodDefinitions[] = {
  { "keyfile",
    authKeyfile_initialize, authKeyfile_release,
    authKeyfile_client, authKeyfile_server
  },

  {NULL}
};

typedef struct {
  const MethodDefinition *definition;
  MethodPerformHandler perform;
  void *data;
} MethodDescriptor;

struct AuthDescriptorStruct {
  int count;
  char **parameters;
  MethodDescriptor *methods;
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
      MethodPerformHandler perform = client? definition->client: definition->server;
      if (perform &&
          (nameLength == strlen(definition->name)) &&
          (strncmp(name, definition->name, nameLength) == 0)) {
        method->definition = definition;
        method->perform = perform;
        return (method->data = definition->initialize(parameter)) != NULL;
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
    if ((auth->parameters = splitString(parameter, '+', &auth->count))) {
      if ((auth->methods = malloc(auth->count * sizeof(*auth->methods)))) {
        if (initializeMethodDescriptors(auth, client)) {
          return auth;
        }

        free(auth->methods);
      }

      deallocateStrings(auth->parameters);
    }

    free(auth);
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
  int index;
  for (index=0; index<auth->count; ++index) {
    MethodDescriptor *method = &auth->methods[index];
    if (!method->perform(fd, method->data)) return 0;
  }
  return 1;
}
