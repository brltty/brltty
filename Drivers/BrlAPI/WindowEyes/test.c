/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 2009 by The BRLTTY Developers.
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

#include <windows.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "webrloem.h"

int main(void) {
  DWORD handle;
  DWORD key1, key2;
  int route, route2;
  bool status;
  int x, s;
  unsigned char test[] ={0, 1, 2, 4, 8, 16, 32, 64, 128, 255};
  printf("connecting\n");
  assert(WEBrailleOpen(0, 0, &handle));
  printf("getting info\n");
  assert(WEGetBrailleDisplayInfo(handle, &x, &s));
  printf("display %d+%d\n", x, s);
  assert(WEUpdateBrailleDisplay(handle, test, sizeof(test), NULL, 0));
  printf("wrote\n");
  while (!WEGetBrailleKey(handle, &key1, &key2, &route, &route2, &status))
    Sleep(50);
  printf("key %08lx%08lx %d %d %d\n", key2, key1, route, route2, status);
  while (!WEGetBrailleKey(handle, &key1, &key2, &route, &route2, &status))
    Sleep(50);
  printf("key %08lx%08lx %d %d %d\n", key2, key1, route, route2, status);
  while (!WEGetBrailleKey(handle, &key1, &key2, &route, &route2, &status))
    Sleep(50);
  printf("key %08lx%08lx %d %d %d\n", key2, key1, route, route2, status);
  while (!WEGetBrailleKey(handle, &key1, &key2, &route, &route2, &status))
    Sleep(50);
  printf("key %08lx%08lx %d %d %d\n", key2, key1, route, route2, status);
  assert(WEBrailleClose(handle));
  return 0;
}
