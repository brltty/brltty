/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif /* HAVE_SYS_PRCTL_H */

#include "log.h"
#include "strfmt.h"
#include "pgmprivs.h"
#include "system_linux.h"
#include "program.h"
#include "file.h"

typedef struct {
  const char *reason;
  int (*install) (void);
} KernelModuleEntry;

static const KernelModuleEntry kernelModuleTable[] = {
  { .reason = "for playing alert tunes via the built-in PC speaker",
    .install = installSpeakerModule,
  },

  { .reason = "for creating virtual devices",
    .install = installUinputModule,
  },
};

static void
installKernelModules (int amPrivilegedUser) {
  const KernelModuleEntry *kme = kernelModuleTable;
  const KernelModuleEntry *end = kme + ARRAY_COUNT(kernelModuleTable);

  while (kme < end) {
    kme->install();
    kme += 1;
  }
}

#ifdef HAVE_GRP_H
#include <grp.h>

typedef struct {
  const char *message;
  const gid_t *groups;
  size_t count;
} GroupsLogData;

static size_t
groupsLogFormatter (char *buffer, size_t size, const void *data) {
  const GroupsLogData *gld = data;

  size_t length;
  STR_BEGIN(buffer, size);
  STR_PRINTF("%s:", gld->message);

  const gid_t *gid = gld->groups;
  const gid_t *end = gid + gld->count;

  while (gid < end) {
    STR_PRINTF(" %d", *gid);

    const struct group *grp = getgrgid(*gid);
    if (grp) STR_PRINTF("(%s)", grp->gr_name);

    gid += 1;
  }

  length = STR_LENGTH;
  STR_END;
  return length;
}

static void
logGroups (int level, const char *message, const gid_t *groups, size_t count) {
  GroupsLogData gld = {
    .message = message,
    .groups = groups,
    .count = count
  };

  logData(level, groupsLogFormatter, &gld);
}

static void
logGroup (int level, const char *message, gid_t group) {
  logGroups(level, message, &group, 1);
}

typedef struct {
  const char *reason;
  const char *name;
  const char *path;
} RequiredGroupEntry;

static RequiredGroupEntry requiredGroupTable[] = {
  { .reason = "for reading screen content",
    .name = "tty",
    .path = "/dev/vcs1",
  },

  { .reason = "for virtual console monitoring and control",
    .name = "tty",
    .path = "/dev/tty1",
  },

  { .reason = "for serial I/O",
    .name = "dialout",
    .path = "/dev/ttyS0",
  },

  { .reason = "for USB I/O via USBFS",
    .path = "/dev/bus/usb",
  },

  { .reason = "for playing sound via the ALSA framework",
    .name = "audio",
    .path = "/dev/snd/seq",
  },

  { .reason = "for playing sound via the Pulse Audio daemon",
    .name = "pulse-access",
  },

  { .reason = "for monitoring keyboard input",
    .name = "input",
    .path = "/dev/input/mice",
  },

  { .reason = "for creating virtual devices",
    .path = "/dev/uinput",
  },
};

static void
processRequiredGroups (GroupsProcessor *processGroups, void *data) {
  size_t size = ARRAY_COUNT(requiredGroupTable);
  gid_t groups[size * 2];
  size_t count = 0;

  {
    const RequiredGroupEntry *rge = requiredGroupTable;
    const RequiredGroupEntry *end = rge + size;

    while (rge < end) {
      {
        const char *name = rge->name;

        if (name) {
          const struct group *grp = getgrnam(name);

          if (grp) {
            groups[count++] = grp->gr_gid;
          } else {
            logMessage(LOG_WARNING, "unknown user group: %s", name);
          }
        }
      }

      {
        const char *path = rge->path;

        if (path) {
          struct stat status;

          if (stat(path, &status) != -1) {
            groups[count++] = status.st_gid;
          } else {
            logMessage(LOG_WARNING, "path access error: %s: %s", path, strerror(errno));
          }
        }
      }

      rge += 1;
    }
  }

  removeDuplicateGroups(groups, &count);
  processGroups(groups, count, data);
}

static void
setSupplementaryGroups (const gid_t *groups, size_t count, void *data) {
  logGroups(LOG_DEBUG, "setting supplementary groups", groups, count);

  if (setgroups(count, groups) == -1) {
    logSystemError("setgroups");
  }
}

static void
joinRequiredGroups (int amPrivilegedUser) {
  processRequiredGroups(setSupplementaryGroups, NULL);
}

typedef struct {
  const gid_t *groups;
  size_t count;
} CurrentGroupsData;

static void
logUnjoinedGroups (const gid_t *groups, size_t count, void *data) {
  const CurrentGroupsData *cgd = data;

  const gid_t *cur = cgd->groups;
  const gid_t *curEnd = cur + cgd->count;

  const gid_t *req = groups;
  const gid_t *reqEnd = req + count;

  while (req < reqEnd) {
    int relation = (cur < curEnd)? compareGroups(*cur, *req): 1;

    if (relation > 0) {
      logGroup(LOG_WARNING, "group not joined", *req++);
    } else {
      if (!relation) req += 1;
      cur += 1;
    }
  }
}

static void
logWantedGroups (const gid_t *groups, size_t count, void *data) {
  CurrentGroupsData cgd = {
    .groups = groups,
    .count = count
  };

  processRequiredGroups(logUnjoinedGroups, &cgd);
}

static void
logMissingGroups (void) {
  processSupplementaryGroups(logWantedGroups, NULL);
}

static void
closeGroupsDatabase (void) {
  endgrent();
}
#endif /* HAVE_GRP_H */

#ifdef HAVE_LIBCAP
#ifdef HAVE_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif /* HAVE_SYS_CAPABILITY_H */
#endif /* HAVE_LIBCAP */

#ifdef CAP_IS_SUPPORTED
typedef struct {
  const char *label;
  cap_t caps;
} CapabilitiesLogData;

static size_t
capabilitiesLogFormatter (char *buffer, size_t size, const void *data) {
  const CapabilitiesLogData *cld = data;

  size_t length;
  STR_BEGIN(buffer, size);
  STR_PRINTF("capabilities: %s:", cld->label);

  cap_t caps = cld->caps;
  int capsAllocated = 0;

  if (!caps) {
    if (!(caps = cap_get_proc())) {
      logSystemError("cap_get_proc");
      goto done;
    }

    capsAllocated = 1;
  }

  {
    char *text;

    if ((text = cap_to_text(caps, NULL))) {
      STR_PRINTF(" %s", text);
      cap_free(text);
    } else {
      logSystemError("cap_to_text");
    }
  }

  if (capsAllocated) {
    cap_free(caps);
    caps = NULL;
  }

done:
  length = STR_LENGTH;
  STR_END;
  return length;
}

static void
logCapabilities (cap_t caps, const char *label) {
  CapabilitiesLogData cld = { .label=label, .caps=caps };
  logData(LOG_DEBUG, capabilitiesLogFormatter, &cld);
}

static void
logCurrentCapabilities (const char *label) {
  logCapabilities(NULL, label);
}

static int
setCapabilities (cap_t caps) {
  if (cap_set_proc(caps) != -1) return 1;
  logSystemError("cap_set_proc");
  return 0;
}

static int
hasCapability (cap_t caps, cap_flag_t set, cap_value_t capability) {
  cap_flag_value_t value;
  if (cap_get_flag(caps, capability, set, &value) != -1) return value == CAP_SET;
  logSystemError("cap_get_flag");
  return 0;
}

static int
addCapability (cap_t caps, cap_flag_t set, cap_value_t capability) {
  if (cap_set_flag(caps, set, 1, &capability, CAP_SET) != -1) return 1;
  logSystemError("cap_set_flag");
  return 0;
}

typedef struct {
  const char *reason;
  cap_value_t value;
} RequiredCapabilityEntry;

static const RequiredCapabilityEntry requiredCapabilityTable[] = {
  { .reason = "for injecting input characters typed on a braille device",
    .value = CAP_SYS_ADMIN,
  },

  { .reason = "for playing alert tunes via the built-in PC speaker",
    .value = CAP_SYS_TTY_CONFIG,
  },

  { .reason = "for creating needed but missing special device files",
    .value = CAP_MKNOD,
  },
};

static void
setRequiredCapabilities (int amPrivilegedUser) {
  cap_t newCaps, oldCaps;

  if (amPrivilegedUser) {
    oldCaps = NULL;
  } else if (!(oldCaps = cap_get_proc())) {
    logSystemError("cap_get_proc");
    return;
  }

  if ((newCaps = cap_init())) {
    {
      const RequiredCapabilityEntry *rce = requiredCapabilityTable;
      const RequiredCapabilityEntry *end = rce + ARRAY_COUNT(requiredCapabilityTable);

      while (rce < end) {
        cap_value_t capability = rce->value;

        if (!oldCaps || hasCapability(oldCaps, CAP_PERMITTED, capability)) {
          if (!addCapability(newCaps, CAP_PERMITTED, capability)) break;
          if (!addCapability(newCaps, CAP_EFFECTIVE, capability)) break;
        }

        rce += 1;
      }
    }

    setCapabilities(newCaps);
    cap_free(newCaps);
  } else {
    logSystemError("cap_init");
  }

#ifdef PR_CAP_AMBIENT
  if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) == -1) {
    logSystemError("prctl[PR_CAP_AMBIENT_CLEAR_ALL]");
  }
#endif /* PR_CAP_AMBIENT */

  if (oldCaps) cap_free(oldCaps);
}

static void
logUnassignedCapability (cap_value_t capability, int required, const char *reason) {
  logMessage(LOG_WARNING,
             "%s capability not assigned: %s (%s)",
             (required? "required": "temporary"),
             cap_to_name(capability), reason);
}

static void
logMissingCapabilities (void) {
  cap_t caps;

  if ((caps = cap_get_proc())) {
    const RequiredCapabilityEntry *rce = requiredCapabilityTable;
    const RequiredCapabilityEntry *end = rce + ARRAY_COUNT(requiredCapabilityTable);

    while (rce < end) {
      cap_value_t capability = rce->value;

      if (!hasCapability(caps, CAP_EFFECTIVE, capability)) {
        logUnassignedCapability(capability, 1, rce->reason);
      }

      rce += 1;
    }

    cap_free(caps);
  } else {
    logSystemError("cap_get_proc");
  }
}

static int
requestCapability (cap_t caps, cap_value_t capability, int inheritable) {
  if (!hasCapability(caps, CAP_EFFECTIVE, capability)) {
    if (!hasCapability(caps, CAP_PERMITTED, capability)) {
      logMessage(LOG_WARNING, "capability not permitted: %s", cap_to_name(capability));
      return 0;
    }

    if (!addCapability(caps, CAP_EFFECTIVE, capability)) return 0;
    if (!inheritable) return setCapabilities(caps);
  } else if (!inheritable) {
    return 1;
  }

  if (!hasCapability(caps, CAP_INHERITABLE, capability)) {
    if (!addCapability(caps, CAP_INHERITABLE, capability)) {
      return 0;
    }
  }

  if (setCapabilities(caps)) {
    #ifdef PR_CAP_AMBIENT_RAISE
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, capability, 0, 0) != -1) return 1;
    logSystemError("prctl[PR_CAP_AMBIENT_RAISE]");
    #endif /* PR_CAP_AMBIENT_RAISE */
  }

  return 0;
}

static void
wantTemporaryCapability (int *can, cap_t caps, cap_value_t capability, const char *reason) {
  if (!*can) {
    if (requestCapability(caps, capability, 0)) {
      *can = 1;
    } else {
      logUnassignedCapability(capability, 0, reason);
    }
  }
}
#endif /* CAP_IS_SUPPORTED */

typedef void PrivilegesAcquisitionFunction (int amPrivilegedUser);
typedef void MissingPrivilegesLogger (void);
typedef void ReleaseResourcesFunction (void);

typedef struct {
  const char *reason;
  PrivilegesAcquisitionFunction *acquirePrivileges;
  MissingPrivilegesLogger *logMissingPrivileges;
  ReleaseResourcesFunction *releaseResources;

  #ifdef CAP_IS_SUPPORTED
  cap_value_t capability;
  unsigned char inheritable:1;
  #endif /* CAP_IS_SUPPORTED */
} PrivilegesAcquisitionEntry;

static const PrivilegesAcquisitionEntry privilegesAcquisitionTable[] = {
  { .reason = "for installing kernel modules",
    .acquirePrivileges = installKernelModules,

    #ifdef CAP_SYS_MODULE
    .capability = CAP_SYS_MODULE,
    .inheritable = 1,
    #endif /* CAP_SYS_MODULE, */
  },

#ifdef HAVE_GRP_H
  { .reason = "for joining required groups",
    .acquirePrivileges = joinRequiredGroups,
    .logMissingPrivileges = logMissingGroups,
    .releaseResources = closeGroupsDatabase,

    #ifdef CAP_SETGID
    .capability = CAP_SETGID,
    #endif /* CAP_SETGID, */
  },
#endif /* HAVE_GRP_H */

// This one must be last because it relinquishes the temporary capabilities.
#ifdef CAP_IS_SUPPORTED
  { .reason = "for assigning required capabilities",
    .acquirePrivileges = setRequiredCapabilities,
    .logMissingPrivileges = logMissingCapabilities,
  }
#endif /* CAP_IS_SUPPORTED */
};

static void
acquirePrivileges (int amPrivilegedUser) {
  if (amPrivilegedUser) {
    const PrivilegesAcquisitionEntry *pae = privilegesAcquisitionTable;
    const PrivilegesAcquisitionEntry *end = pae + ARRAY_COUNT(privilegesAcquisitionTable);

    while (pae < end) {
      pae->acquirePrivileges(amPrivilegedUser);
      pae += 1;
    }
  }

#ifdef CAP_IS_SUPPORTED
  else {
    cap_t caps;

    if ((caps = cap_get_proc())) {
      const PrivilegesAcquisitionEntry *pae = privilegesAcquisitionTable;
      const PrivilegesAcquisitionEntry *end = pae + ARRAY_COUNT(privilegesAcquisitionTable);

      while (pae < end) {
        cap_value_t capability = pae->capability;

        if (!capability || requestCapability(caps, capability, pae->inheritable)) {
          pae->acquirePrivileges(amPrivilegedUser);
        } else {
          logUnassignedCapability(capability, 0, pae->reason);
        }

        pae += 1;
      }

      cap_free(caps);
    } else {
      logSystemError("cap_get_proc");
    }
  }
#endif /* CAP_IS_SUPPORTED */

  {
    const PrivilegesAcquisitionEntry *pae = privilegesAcquisitionTable;
    const PrivilegesAcquisitionEntry *end = pae + ARRAY_COUNT(privilegesAcquisitionTable);

    while (pae < end) {
      {
        MissingPrivilegesLogger *log = pae->logMissingPrivileges;
        if (log) log();
      }

      {
        ReleaseResourcesFunction *release = pae->releaseResources;
        if (release) release();
      }

      pae += 1;
    }
  }
}

#ifdef HAVE_PWD_H
#include <pwd.h>

static int
switchToUser (const char *user) {
  const struct passwd *pwd = getpwnam(user);

  if (pwd) {
    uid_t newUid = pwd->pw_uid;

    if (newUid) {
      gid_t oldRgid, oldEgid, oldSgid;

      if (getresgid(&oldRgid, &oldEgid, &oldSgid) != -1) {
        gid_t newGid = pwd->pw_gid;

        if (setresgid(newGid, newGid, newGid) != -1) {
          if (setresuid(newUid, newUid, newUid) != -1) {
            logMessage(LOG_NOTICE, "switched to user: %s", user);
            return 1;
          } else {
            logSystemError("setresuid");
          }

          setresgid(oldRgid, oldEgid, oldSgid);
        } else {
          logSystemError("setresgid");
        }
      } else {
        logSystemError("getresgid");
      }
    } else {
      logMessage(LOG_WARNING, "user is privileged: %s", user);
    }
  } else {
    logMessage(LOG_WARNING, "user not found: %s", user);
  }

  return 0;
}

static int
switchUser (const char *user, int amPrivilegedUser) {
  if (*user) {
    if (!amPrivilegedUser) {
      logMessage(LOG_WARNING, "not executing as a privileged user");
    } else if (switchToUser(user)) {
      return 1;
    }

    logMessage(LOG_ERR, "won't switch to an explicitly specified user: %s", user);
    exit(PROG_EXIT_FATAL);
  }

  if (*(user = UNPRIVILEGED_USER)) {
    if (switchToUser(user)) return 1;
    logMessage(LOG_WARNING, "couldn't switch to default unprivileged user: %s", user);
  }

  return 0;
}
#endif /* HAVE_PWD_H */

static const char *
getSocketsDirectory (void) {
  const char *path = BRLAPI_SOCKETPATH;
  if (!ensureDirectory(path)) path = NULL;
  return path;
}

typedef struct {
  const char *reason;
  const char * (*get) (void);
  const char *name;
} StateDirectoryEntry;

static const StateDirectoryEntry stateDirectoryTable[] = {
  { .reason = "updatable directory",
    .get = getUpdatableDirectory,
    .name = "brltty",
  },

  { .reason = "writable directory",
    .get = getWritableDirectory,
    .name = "brltty",
  },

  { .reason = "sockets directory",
    .get = getSocketsDirectory,
    .name = "BrlAPI",
  },
};

typedef struct {
  gid_t owningGroup;
  unsigned char canChangeOwnership:1;
  unsigned char canChangePermissions:1;
} StateDirectoryData;

static int
claimStateDirectory (const PathProcessorParameters *parameters) {
  const StateDirectoryData *sdd = parameters->data;
  const char *path = parameters->path;
  gid_t group = sdd->owningGroup;
  struct stat status;

  if (stat(path, &status) != -1) {
    int addPermissions = 0;

    if (status.st_gid == group) {
      addPermissions = 1;
    } else if (!sdd->canChangeOwnership) {
      logMessage(LOG_WARNING, "can't claim group ownership: %s", path);
    } else if (chown(path, -1, group) != -1) {
      logMessage(LOG_INFO, "group ownership claimed: %s", path);
      addPermissions = 1;
    } else {
      logSystemError("chown");
    }

    if (addPermissions) {
      mode_t oldMode = status.st_mode;
      mode_t newMode = oldMode;

      newMode |= S_IRGRP | S_IWGRP;
      if (S_ISDIR(newMode)) newMode |= S_IXGRP | S_ISGID;

      if (newMode != oldMode) {
        if (!sdd->canChangePermissions) {
          logMessage(LOG_WARNING, "can't add group permissions: %s", path);
        } else if (chmod(path, newMode) != -1) {
          logMessage(LOG_INFO, "group permissions added: %s", path);
        } else {
          logSystemError("chmod");
        }
      }
    }
  } else {
    logSystemError("stat");
  }

  return 1;
}

static void
claimStateDirectories (int canChangeOwnership, int canChangePermissions) {
  StateDirectoryData sdd = {
    .owningGroup = getegid(),
    .canChangeOwnership = canChangeOwnership,
    .canChangePermissions = canChangePermissions,
  };

  const StateDirectoryEntry *sde = stateDirectoryTable;
  const StateDirectoryEntry *end = sde + ARRAY_COUNT(stateDirectoryTable);

  while (sde < end) {
    const char *path = sde->get();

    if (path && *path) {
      const char *name = locatePathName(path);

      if (strcasecmp(name, sde->name) == 0) {
        processPathTree(path, claimStateDirectory, &sdd);
      }
    }

    sde += 1;
  }
}

void
establishProgramPrivileges (const char *user) {
  logCurrentCapabilities("at start");

  int amPrivilegedUser = !geteuid();
  int canSwitchUser = amPrivilegedUser;
  int canSwitchGroup = amPrivilegedUser;
  int canChangeOwnership = amPrivilegedUser;
  int canChangePermissions = amPrivilegedUser;
  int canOverridePermissions = amPrivilegedUser;

#ifdef PR_SET_KEEPCAPS
  if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1) {
    logSystemError("prctl[PR_SET_KEEPCAPS]");
  }
#endif /* PR_SET_KEEPCAPS */

#ifdef CAP_IS_SUPPORTED
  {
    cap_t curCaps;

    if ((curCaps = cap_get_proc())) {
      cap_t newCaps;

      if ((newCaps = cap_dup(curCaps))) {
        wantTemporaryCapability(
          &canSwitchUser, newCaps, CAP_SETUID,
          "for switching to the default unprivileged user"
        );

        wantTemporaryCapability(
          &canSwitchGroup, newCaps, CAP_SETGID,
          "for switching to the writable group"
        );

        wantTemporaryCapability(
          &canChangeOwnership, newCaps, CAP_CHOWN,
          "for claiming group ownership of the state directories"
        );

        wantTemporaryCapability(
          &canChangePermissions, newCaps, CAP_FOWNER,
          "for adding group permissions to the state directories"
        );

        wantTemporaryCapability(
          &canOverridePermissions, newCaps, CAP_DAC_OVERRIDE,
          "for creating missing state directories"
        );

        if (cap_compare(newCaps, curCaps) != 0) setCapabilities(newCaps);
        cap_free(newCaps);
      } else {
        logSystemError("cap_dup");
      }

      cap_free(curCaps);
    } else {
      logSystemError("cap_get_proc");
    }
  }
#endif /* CAP_IS_SUPPORTED */

#ifdef HAVE_PWD_H
  {
    if (canSwitchUser && canSwitchGroup && switchUser(user, amPrivilegedUser)) {
      amPrivilegedUser = 0;
      umask(umask(0) & ~S_IRWXG);
      claimStateDirectories(canChangeOwnership, canChangePermissions);
    } else {
      uid_t uid = geteuid();
      const struct passwd *pwd = getpwuid(uid);

      const char *name;
      char number[0X10];

      if (pwd) {
        name = pwd->pw_name;
      } else {
        snprintf(number, sizeof(number), "%d", uid);
        name = number;
      }

      logMessage(LOG_NOTICE, "continuing to execute as invoking user: %s", name);
    }

    endpwent();
  }
#endif /* HAVE_PWD_H */

  acquirePrivileges(amPrivilegedUser);
  logCurrentCapabilities("after relinquish");
}
