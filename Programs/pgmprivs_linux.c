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

static void
installKernelModules (int amPrivilegedUser) {
  installSpeakerModule();
  installUinputModule();
}

#ifdef HAVE_GRP_H
#include <grp.h>

static int
compareGroups (gid_t group1, gid_t group2) {
  if (group1 < group2) return -1;
  if (group1 > group2) return 1;
  return 0;
}

static int
sortGroups (const void *element1,const void *element2) {
  const gid_t *group1 = element1;
  const gid_t *group2 = element2;
  return compareGroups(*group1, *group2);
}

static void
removeDuplicateGroups (gid_t *groups, size_t *count) {
  if (*count > 1) {
    qsort(groups, *count, sizeof(*groups), sortGroups);

    gid_t *to = groups;
    const gid_t *from = to + 1;
    const gid_t *end = to + *count;

    while (from < end) {
      if (*from != *to) *++to = *from;
      from += 1;
    }

    *count = ++to - groups;
  }
}

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
    .path = "/dev/tty1",
  },

  { .reason = "for serial I/O",
    .name = "dialout",
    .path = "/dev/ttyS0",
  },

  { .reason = "for USB I/O",
    .path = "/dev/bus/usb",
  },

  { .reason = "for playing sound via the ALSA framework",
    .name = "audio",
    .path = "/dev/snd/seq",
  },

  { .reason = "for playing sound via the Pulse Audio daemon",
    .name = "pulse-access",
  },

  { .reason = "for intercepting keyboard key events",
    .name = "input",
    .path = "/dev/input/mice",
  },

  { .reason = "for creating virtual keyboards",
    .path = "/dev/uinput",
  },
};

typedef void RequiredGroupsProcessor (const gid_t *groups, size_t count, void *data);

static void
processRequiredGroups (RequiredGroupsProcessor *processGroups, void *data) {
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
          struct stat s;

          if (stat(path, &s) != -1) {
            groups[count++] = s.st_gid;
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
logMissingGroups (const gid_t *groups, size_t count, void *data) {
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
logUnjoinedGroups (void) {
  ssize_t size = getgroups(0, NULL);

  if (size != -1) {
    gid_t groups[size];
    ssize_t count = getgroups(size, groups);

    if (count != -1) {
      CurrentGroupsData cgd = {
        .groups = groups,
        .count = count
      };

      removeDuplicateGroups(groups, &cgd.count);
      processRequiredGroups(logMissingGroups, &cgd);
    } else {
      logSystemError("getgroups");
    }
  } else {
    logSystemError("getgroups");
  }
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

static int
IsPermittedCapability (cap_t caps, cap_value_t capability) {
  if (!caps) return 1;
  return hasCapability(caps, CAP_PERMITTED, capability);
}

typedef struct {
  const char *reason;
  cap_value_t value;
} RequiredCapabilityEntry;

static const RequiredCapabilityEntry requiredCapabilityTable[] = {
  { .reason = "for inserting input characters typed on a braille device",
    .value = CAP_SYS_ADMIN,
  },

  { .reason = "for playing alert tunes via the built-in PC speaker",
    .value = CAP_SYS_TTY_CONFIG,
  },

  { .reason = "for creating needed but missing special device files",
    .value = CAP_MKNOD,
  },
};

static int
addAmbientCapability (cap_value_t capability) {
#ifdef PR_CAP_AMBIENT_RAISE
  if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, capability, 0, 0) != -1) return 1;
  logSystemError("prctl[PR_CAP_AMBIENT_RAISE]");
#endif /*( PR_CAP_AMBIENT_RAISE */
  return 0;
}

static void
setAmbientCapabilities (cap_t caps) {
#ifdef PR_CAP_AMBIENT
  if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) != -1) {
    const RequiredCapabilityEntry *rce = requiredCapabilityTable;
    const RequiredCapabilityEntry *end = rce + ARRAY_COUNT(requiredCapabilityTable);

    while (rce < end) {
      cap_value_t capability = rce->value;

      if (IsPermittedCapability(caps, capability)) {
        if (!addAmbientCapability(capability)) {
          break;
        }
      }

      rce += 1;
    }
  } else {
    logSystemError("prctl[PR_CAP_AMBIENT_CLEAR_ALL]");
  }
#endif /* PR_CAP_AMBIENT */
}

typedef struct {
  const char *name;
  cap_flag_t value;
} CapabilitySetEntry;

static const CapabilitySetEntry capabilitySetTable[] = {
  { .value = CAP_PERMITTED,
    .name = "permitted"
  },

  { .value = CAP_EFFECTIVE,
    .name = "effective"
  },

  { .value = CAP_INHERITABLE,
    .name = "inheritable"
  },
};

/*
static void
logCapabilities (const char *label) {
  cap_t caps;

  if ((caps = cap_get_proc())) {
    const CapabilitySetEntry *cse = capabilitySetTable;
    const CapabilitySetEntry *end = cse + ARRAY_COUNT(capabilitySetTable);

    while (cse < end) {
      char buffer[0X1000];
      STR_BEGIN(buffer, sizeof(buffer));
      char delimiter = ':';

      for (cap_value_t capability=0; capability<=CAP_LAST_CAP; capability+=1) {
        if (!hasCapability(caps, cse->value, capability)) continue;
        STR_PRINTF("%c %s", delimiter, cap_to_name(capability));
        delimiter = ',';
      }

      STR_END;
      logMessage(LOG_DEBUG, "%s capabilities: %s%s", cse->name, label, buffer);

      cse += 1;
    }

    cap_free(caps);
  } else {
    logSystemError("cap_get_proc");
  }
}
*/

static int
isCapabilityEnabled (cap_t caps, cap_value_t capability) {
  return hasCapability(caps, CAP_EFFECTIVE, capability);
}

static int
enableCapability (cap_t caps, cap_value_t capability) {
  return hasCapability(caps, CAP_PERMITTED, capability)
      && addCapability(caps, CAP_EFFECTIVE, capability)
      && addCapability(caps, CAP_INHERITABLE, capability)
      && setCapabilities(caps)
      && addAmbientCapability(capability)
      ;
}

static int
ensureCapability (cap_t caps, cap_value_t capability) {
  if (isCapabilityEnabled(caps, capability)) return 1;
  return enableCapability(caps, capability);
}

static int
addRequiredCapability (cap_t caps, cap_value_t capability) {
  const CapabilitySetEntry *cse = capabilitySetTable;
  const CapabilitySetEntry *end = cse + ARRAY_COUNT(capabilitySetTable);;

  while (cse < end) {
    if (!addCapability(caps, cse->value, capability)) return 0;
    cse += 1;
  }

  return 1;
}

static void
assignRequiredCapabilities (int amPrivilegedUser) {
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

        if (IsPermittedCapability(oldCaps, capability)) {
          if (!addRequiredCapability(newCaps, capability)) {
            break;
          }
        }

        rce += 1;
      }
    }

    if (setCapabilities(newCaps)) setAmbientCapabilities(oldCaps);
    cap_free(newCaps);
  } else {
    logSystemError("cap_init");
  }

  if (oldCaps) cap_free(oldCaps);
}

static void
logUnassignedCapabilities (void) {
  cap_t caps;

  if ((caps = cap_get_proc())) {
    const RequiredCapabilityEntry *rce = requiredCapabilityTable;
    const RequiredCapabilityEntry *end = rce + ARRAY_COUNT(requiredCapabilityTable);

    while (rce < end) {
      cap_value_t capability = rce->value;

      if (!hasCapability(caps, CAP_EFFECTIVE, capability)) {
        logMessage(LOG_WARNING,
          "capability not assigned: %s (%s)",
          cap_to_name(capability), rce->reason
        );
      }

      rce += 1;
    }

    cap_free(caps);
  } else {
    logSystemError("cap_get_proc");
  }
}
#endif /* CAP_IS_SUPPORTED */

typedef void PrivilegesAcquisitionFunction (int amPrivilegedUser);
typedef void MissingPrivilegesLogger (void);
typedef void ReleaseResourcesFunction (void);

typedef struct {
  PrivilegesAcquisitionFunction *acquirePrivileges;
  MissingPrivilegesLogger *logMissingPrivileges;
  ReleaseResourcesFunction *releaseResources;

  #ifdef CAP_IS_SUPPORTED
  cap_value_t capability;
  #endif /* CAP_IS_SUPPORTED */
} PrivilegesAcquisitionEntry;

static const PrivilegesAcquisitionEntry privilegesAcquisitionTable[] = {
  { .acquirePrivileges = installKernelModules,

    #ifdef CAP_SYS_MODULE
    .capability = CAP_SYS_MODULE,
    #endif /* CAP_SYS_MODULE, */
  },

#ifdef HAVE_GRP_H
  { .acquirePrivileges = joinRequiredGroups,
    .logMissingPrivileges = logUnjoinedGroups,
    .releaseResources = closeGroupsDatabase,

    #ifdef CAP_SETGID
    .capability = CAP_SETGID,
    #endif /* CAP_SETGID, */
  },
#endif /* HAVE_GRP_H */

// This one must be last because it relinquishes the temporary capabilities.
#ifdef CAP_IS_SUPPORTED
  { .acquirePrivileges = assignRequiredCapabilities,
    .logMissingPrivileges = logUnassignedCapabilities,
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
        int callHandler = 1;

        if (capability) {
          if (!isCapabilityEnabled(caps, capability)) {
            if (!enableCapability(caps, capability)) {
              callHandler = 0;
            }
          }
        }

        if (callHandler) pae->acquirePrivileges(amPrivilegedUser);
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

    logMessage(LOG_ERR, "can't switch to explicitly specified user: %s", user);
    exit(PROG_EXIT_FATAL);
  }

  if (*(user = UNPRIVILEGED_USER)) {
    if (switchToUser(user)) return 1;
    logMessage(LOG_WARNING, "couldn't switch to default unprivileged user: %s", user);
  }

  return 0;
}
#endif /* HAVE_PWD_H */

void
establishProgramPrivileges (const char *user) {
  int amPrivilegedUser = !geteuid();
  int canSwitchUser = amPrivilegedUser;
  int canSwitchGroup = amPrivilegedUser;

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
        if (!canSwitchUser) {
          if (ensureCapability(newCaps, CAP_SETUID)) {
            canSwitchUser = 1;
          }
        }

        if (!canSwitchGroup) {
          if (ensureCapability(newCaps, CAP_SETGID)) {
            canSwitchGroup = 1;
          }
        }

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
  if (canSwitchUser) {
    if (switchUser(user, amPrivilegedUser)) {
      amPrivilegedUser = 0;
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

      logMessage(LOG_ERR, "continuing to execute as invoking user: %s", name);
    }

    endpwent();
  }
#endif /* HAVE_PWD_H */

  acquirePrivileges(amPrivilegedUser);
}
