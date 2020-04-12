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

static void
installKernelModules (int amRoot) {
  installSpeakerModule();
  installUinputModule();
}

#ifdef HAVE_GRP_H
#include <grp.h>

static int
sortGroupIdentifiers (const void *element1,const void *element2) {
  const gid_t *identifier1 = element1;
  const gid_t *identifier2 = element2;

  if (*identifier1 < *identifier2) return -1;
  if (*identifier1 > *identifier2) return 1;
  return 0;
}

static void
removeDuplicateGroupIdentifiers (gid_t *groups, size_t *count) {
  if (*count > 1) {
    qsort(groups, *count, sizeof(*groups), sortGroupIdentifiers);

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
  const char *action;
  const gid_t *groups;
  size_t count;
} SupplementaryGroupsLogData;

static size_t
supplementaryGroupsLogFormatter (char *buffer, size_t size, const void *data) {
  const SupplementaryGroupsLogData *sgl = data;

  size_t length;
  STR_BEGIN(buffer, size);
  STR_PRINTF("%s:", sgl->action);

  const gid_t *gid = sgl->groups;
  const gid_t *end = gid + sgl->count;

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

typedef struct {
  const char *reason;
  const char *name;
  const char *path;
} SupplementaryGroupEntry;

static SupplementaryGroupEntry supplementaryGroupTable[] = {
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

static void
setSupplementaryGroups (int amRoot) {
  size_t size = ARRAY_COUNT(supplementaryGroupTable);
  gid_t groups[size * 2];
  size_t count = 0;

  {
    const SupplementaryGroupEntry *sge = supplementaryGroupTable;
    const SupplementaryGroupEntry *end = sge + size;

    while (sge < end) {
      {
        const char *name = sge->name;

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
        const char *path = sge->path;

        if (path) {
          struct stat s;

          if (stat(path, &s) != -1) {
            groups[count++] = s.st_gid;
          } else {
            logMessage(LOG_WARNING, "path access error: %s: %s", path, strerror(errno));
          }
        }
      }

      sge += 1;
    }

    removeDuplicateGroupIdentifiers(groups, &count);
  }

  {
    SupplementaryGroupsLogData sgl = {
      .action = "setting supplementary groups",
      .groups = groups,
      .count = count
    };

    logData(LOG_DEBUG, supplementaryGroupsLogFormatter, &sgl);
  }

  if (setgroups(count, groups) == -1) {
    logSystemError("setgroups");
  }

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
  logSystemError("PR_CAP_AMBIENT_RAISE");
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
    logSystemError("PR_CAP_AMBIENT_CLEAR_ALL");
  }
#endif /* PR_CAP_AMBIENT */
}

static int
addRequiredCapability (cap_t caps, cap_value_t capability) {
  static const cap_flag_t sets[] = {CAP_PERMITTED, CAP_EFFECTIVE, CAP_INHERITABLE};
  const cap_flag_t *set = sets;
  const cap_flag_t *end = set + ARRAY_COUNT(sets);;

  while (set < end) {
    if (!addCapability(caps, *set, capability)) return 0;
    set += 1;
  }

  return 1;
}

static void
assignRequiredCapabilities (int amRoot) {
  cap_t newCaps, oldCaps;

  if (amRoot) {
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

typedef void PrivilegesAcquisitionFunction (int amRoot);
typedef void MissingPrivilegesLogger (void);

typedef struct {
  PrivilegesAcquisitionFunction *acquirePrivileges;
  MissingPrivilegesLogger *logMissingPrivileges;

  #ifdef CAP_IS_SUPPORTED
  cap_value_t capability;
  #endif /* CAP_IS_SUPPORTED */
} PrivilegesSetterEntry;

static const PrivilegesSetterEntry privilegesSetterTable[] = {
  { .acquirePrivileges = installKernelModules,

    #ifdef CAP_SYS_MODULE
    .capability = CAP_SYS_MODULE,
    #endif /* CAP_SYS_MODULE, */
  },

#ifdef HAVE_GRP_H
  { .acquirePrivileges = setSupplementaryGroups,

    #ifdef CAP_SETGID
    .capability = CAP_SETGID,
    #endif /* CAP_SETGID, */
  },
#endif /* HAVE_GRP_H */

#ifdef CAP_IS_SUPPORTED
  { .acquirePrivileges = assignRequiredCapabilities,
    .logMissingPrivileges = logUnassignedCapabilities,
  },
#endif /* CAP_IS_SUPPORTED */
};

void
setProgramPrivileges (void) {
  int amRoot = !geteuid();

  if (amRoot) {
    const PrivilegesSetterEntry *pse = privilegesSetterTable;
    const PrivilegesSetterEntry *end = pse + ARRAY_COUNT(privilegesSetterTable);

    while (pse < end) {
      pse->acquirePrivileges(amRoot);
      pse += 1;
    }
  }

#ifdef CAP_IS_SUPPORTED
  else {
    cap_t caps;

    if ((caps = cap_get_proc())) {
      const PrivilegesSetterEntry *pse = privilegesSetterTable;
      const PrivilegesSetterEntry *end = pse + ARRAY_COUNT(privilegesSetterTable);

      while (pse < end) {
        cap_value_t capability = (pse++)->capability;

        if (capability) {
          if (hasCapability(caps, CAP_EFFECTIVE, capability)) continue;
          if (!hasCapability(caps, CAP_PERMITTED, capability)) continue;
          if (!addCapability(caps, CAP_EFFECTIVE, capability)) continue;
          if (!addCapability(caps, CAP_INHERITABLE, capability)) continue;
          if (!setCapabilities(caps)) continue;
          if (!addAmbientCapability(capability)) continue;
        }

        (pse-1)->acquirePrivileges(amRoot);
      }

      cap_free(caps);
    } else {
      logSystemError("cap_get_proc");
    }
  }
#endif /* CAP_IS_SUPPORTED */

  {
    const PrivilegesSetterEntry *pse = privilegesSetterTable;
    const PrivilegesSetterEntry *end = pse + ARRAY_COUNT(privilegesSetterTable);

    while (pse < end) {
      MissingPrivilegesLogger *logger = pse->logMissingPrivileges;
      if (logger) logger();
      pse += 1;
    }
  }
}
