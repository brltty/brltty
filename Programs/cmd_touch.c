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

#include "prologue.h"

#include "cmd_queue.h"
#include "cmd_touch.h"
#include "brl_cmds.h"

static int
handleTouchCommands (int command, void *data) {
  switch (command & BRL_MSK_BLK) {
    case BRL_BLK_CMD(TOUCH):
      break;

    default:
      return 0;
  }

  return 1;
}

int
addTouchCommands (void) {
  return pushCommandHandler("touch", KTB_CTX_DEFAULT,
                            handleTouchCommands, NULL, NULL);
}
