/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SYSTEM_LINUX
#define BRLTTY_INCLUDED_SYSTEM_LINUX

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int installKernelModule (const char *name, unsigned char *status);

extern int openCharacterDevice (const char *name, int flags, int major, int minor);

typedef struct UinputObjectStruct UinputObject;
extern UinputObject *newUinputObject (const char *name);
extern void destroyUinputObject (UinputObject *uinput);
extern int createUinputDevice (UinputObject *uinput);

extern int enableUinputEventType (UinputObject *uinput, int type);
extern int writeInputEvent (UinputObject *uinput, uint16_t type, uint16_t code, int32_t value);

extern int enableUinputKey (UinputObject *uinput, int key);
extern int writeKeyEvent (UinputObject *uinput, int key, int press);
extern int releasePressedKeys (UinputObject *uinput);

extern int writeRepeatDelay (UinputObject *uinput, int delay);
extern int writeRepeatPeriod (UinputObject *uinput, int period);

extern UinputObject *newUinputKeyboard (const char *name);

typedef uint8_t LinuxKeyCode;
#define LINUX_KEY_TABLE_NAME(type) linuxKeyTable_ ## type
#define LINUX_KEY_TABLE(type) const LinuxKeyCode LINUX_KEY_TABLE_NAME(type)[0X100]

extern LINUX_KEY_TABLE(xt00);
extern LINUX_KEY_TABLE(xtE0);
extern LINUX_KEY_TABLE(xtE1);
extern LINUX_KEY_TABLE(at00);
extern LINUX_KEY_TABLE(atE0);
extern LINUX_KEY_TABLE(atE1);
extern LINUX_KEY_TABLE(ps2);
extern LINUX_KEY_TABLE(hid);

typedef struct {
  const char *name;
  const LinuxKeyCode *keys;
  unsigned int count;
} LinuxKeyTableDescriptor;

extern const LinuxKeyTableDescriptor linuxKeyTableDescriptors[];
extern const unsigned char linuxKeyTableCount;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYSTEM_LINUX */
