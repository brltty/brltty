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

#ifndef BRLTTY_INCLUDED_PARAMETERS
#define BRLTTY_INCLUDED_PARAMETERS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PID_FILE_RETRY_INTERVAL 5000
#define BRAILLE_DRIVER_RETRY_INTERVAL 5000
#define SPEECH_DRIVER_RETRY_INTERVAL 5000
#define SCREEN_DRIVER_RETRY_INTERVAL 5000
#define KEYBOARD_MONITOR_RETRY_INTERVAL 5000

#define MOUNT_TABLE_UPDATE_RETRY_INTERVAL 5000

#define GPM_CONNECTION_RESET_INTERVAL 5000

#define INPUT_POLL_INTERVAL 40

#define UPDATE_POLL_INTERVAL 40
#define UPDATE_SCHEDULE_DELAY 10

#define MESSAGE_HOLD_TIME 4000

#define LEARN_MODE_TIMEOUT 10000

#define BLUETOOTH_NAME_TIMEOUT 5000
#define BLUETOOTH_BUSY_DEVICE_RETRY_TIMEOUT 2000
#define BLUETOOTH_BUSY_DEVICE_RETRY_INTERVAL 100
#define BLUETOOTH_CONNECT_TIMEOUT 15000

#define LINUX_INPUT_DEVICE_OPEN_DELAY 1000

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PARAMETERS */
