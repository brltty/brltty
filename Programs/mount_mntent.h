/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_MOUNT_MNTENT
#define BRLTTY_INCLUDED_MOUNT_MNTENT

#include <mntent.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(MOUNTED)
#define MOUNTS_TABLE_PATH MOUNTED
#elif defined(MNT_MNTTAB)
#define MOUNTS_TABLE_PATH MNT_MNTTAB
#endif /* MOUNTS_TABLE_PATH */

typedef struct mntent MountEntry;
#define mountPath mnt_dir
#define mountReference mnt_fsname
#define mountType mnt_type
#define mountOptions mnt_opts

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MOUNT_MNTENT */
