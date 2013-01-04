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

#include "prologue.h"

#include <stdio.h>

#include "brltty.h"
#include "brldefs.h"
#include "tunes.h"
#include "charset.h"
#include "scr.h"
#include "scr_menu.h"

static Menu *rootMenu = NULL;
static unsigned int screenWidth;

static unsigned int lineLength;
static unsigned int settingIndent;

static inline Menu *
getSubmenu (void) {
  return getCurrentSubmenu(rootMenu);
}

static inline MenuItem *
getItem (void) {
  return getCurrentMenuItem(getSubmenu());
}

static void
formatMenuItem (const MenuItem *item, wchar_t *buffer, size_t size) {
  char labelString[0X100];
  size_t labelLength;

  char settingString[0X100];
  size_t settingLength;

  {
    const MenuString *name = getMenuItemName(item);

    STR_BEGIN(labelString, ARRAY_COUNT(labelString));
    STR_PRINTF("%s", name->label);
    if (*name->comment) STR_PRINTF(" %s", name->comment);
    if (isSettableMenuItem(item)) STR_PRINTF(":");
    STR_PRINTF(" ");
    labelLength = STR_LENGTH;
    STR_END;
  }

  {
    const char *value = getMenuItemValue(item);
    const char *comment = getMenuItemComment(item);
    if (!*value) value = gettext("<off>");

    STR_BEGIN(settingString, ARRAY_COUNT(settingString));
    STR_PRINTF("%s", value);
    if (*comment) STR_PRINTF(" (%s)", comment);
    settingLength = STR_LENGTH;
    STR_END;
  }

  {
    size_t maximumLength = labelLength + settingLength;
    wchar_t characters[maximumLength];
    size_t currentLength = 0;

    currentLength += convertTextToWchars(&characters[currentLength], labelString, maximumLength-currentLength);
    settingIndent = MIN(currentLength, size);

    currentLength += convertTextToWchars(&characters[currentLength], settingString, maximumLength-currentLength);
    lineLength = MIN(currentLength, size);

    if (screenWidth < currentLength) screenWidth = currentLength;
    if (currentLength > size) currentLength = size;
    wmemcpy(buffer, characters, currentLength);
    wmemset(&buffer[currentLength], WC_C(' '), size-currentLength);
  }
}

static void
checkScreenWidth (void) {
  wchar_t line[screenWidth];

  formatMenuItem(getItem(), line, ARRAY_COUNT(line));
}

static int
construct_MenuScreen (Menu *menu) {
  rootMenu = menu;
  screenWidth = 1;

  checkScreenWidth();
  return 1;
}

static void
destruct_MenuScreen (void) {
  rootMenu = NULL;
}

static int
currentVirtualTerminal_MenuScreen (void) {
  return userVirtualTerminal(2);
}

static size_t
formatTitle_MenuScreen (char *buffer, size_t size) {
  size_t length;

  STR_BEGIN(buffer, size);
  STR_PRINTF("%s", gettext("Preferences Menu"));
  length = STR_LENGTH;
  STR_END
  return length;
}

static void
describe_MenuScreen (ScreenDescription *description) {
  Menu *submenu = getSubmenu();

  description->posx = 0;
  description->posy = getMenuIndex(submenu);
  description->cols = screenWidth;
  description->rows = getMenuSize(submenu);
  description->number = currentVirtualTerminal_MenuScreen();
}

static int
readCharacters_MenuScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  Menu *submenu = getSubmenu();

  if (validateScreenBox(box, screenWidth, getMenuSize(submenu))) {
    wchar_t line[screenWidth];
    unsigned int row = box->height;

    while (row > 0) {
      unsigned int column;

      formatMenuItem(getMenuItem(submenu, box->top+--row),
                     line, ARRAY_COUNT(line));

      for (column=0; column<box->width; column+=1) {
        int index = box->left + column;
        ScreenCharacter *character = buffer++;

        character->text = (index < lineLength)? line[index]: WC_C(' ');
        character->attributes = SCR_COLOUR_DEFAULT;
      }
    }

    return 1;
  }

  return 0;
}

static void
commandRejected (void) {
  playTune(&tune_command_rejected);
}

static void
itemChanged (void) {
  ses->winx = 0;
  checkScreenWidth();
}

static void
settingChanged (void) {
  unsigned int textLength = textCount * brl.textRows;

  if (((lineLength - ses->winx) > textLength) && (ses->winx < settingIndent)) {
    ses->winx = settingIndent;
  }

  checkScreenWidth();
}

static int
executeCommand_MenuScreen (int *command) {
  switch (*command) {
    case BRL_BLK_PASSKEY+BRL_KEY_ESCAPE:
    case BRL_BLK_PASSKEY+BRL_KEY_END:
      *command = BRL_CMD_PREFMENU;
      return 0;

    case BRL_BLK_PASSKEY+BRL_KEY_HOME:
      *command = BRL_CMD_PREFLOAD;
      return 0;

    case BRL_BLK_PASSKEY+BRL_KEY_ENTER:
      *command = BRL_CMD_PREFSAVE;
      return 0;

    case BRL_CMD_TOP_LEFT:
    case BRL_CMD_BOT_LEFT:
    case BRL_CMD_MENU_PREV_LEVEL: {
      Menu *menu = getSubmenu();

      if (menu == rootMenu) {
        commandRejected();
      } else if (!changeMenuItemNext(getMenuItem(menu, 0))) {
        commandRejected();
      }

      return 1;
    }

    case BRL_CMD_TOP:
    case BRL_BLK_PASSKEY+BRL_KEY_PAGE_UP:
    case BRL_CMD_MENU_FIRST_ITEM:
      if (setMenuFirstItem(getSubmenu())) {
        itemChanged();
      } else {
        commandRejected();
      }
      return 1;

    case BRL_CMD_BOT:
    case BRL_BLK_PASSKEY+BRL_KEY_PAGE_DOWN:
    case BRL_CMD_MENU_LAST_ITEM:
      if (setMenuLastItem(getSubmenu())) {
        itemChanged();
      } else {
        commandRejected();
      }
      return 1;

    case BRL_CMD_LNUP:
    case BRL_CMD_PRDIFLN:
    case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_UP:
    case BRL_CMD_MENU_PREV_ITEM:
      if (setMenuPreviousItem(getSubmenu())) {
        itemChanged();
      } else {
        commandRejected();
      }
      return 1;

    case BRL_CMD_LNDN:
    case BRL_CMD_NXDIFLN:
    case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_DOWN:
    case BRL_CMD_MENU_NEXT_ITEM:
      if (setMenuNextItem(getSubmenu())) {
        itemChanged();
      } else {
        commandRejected();
      }
      return 1;

    case BRL_CMD_WINUP:
    case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_LEFT:
    case BRL_CMD_BACK:
    case BRL_CMD_MENU_PREV_SETTING:
      if (changeMenuItemPrevious(getItem())) {
        settingChanged();
      } else {
        commandRejected();
      }
      return 1;

    case BRL_CMD_WINDN:
    case BRL_BLK_PASSKEY+BRL_KEY_CURSOR_RIGHT:
    case BRL_CMD_HOME:
    case BRL_CMD_RETURN:
    case BRL_CMD_MENU_NEXT_SETTING:
      if (changeMenuItemNext(getItem())) {
        settingChanged();
      } else {
        commandRejected();
      }
      return 1;

    default:
      if ((*command & BRL_MSK_BLK) == BRL_BLK_ROUTE) {
        int key = *command & BRL_MSK_ARG;

        if ((key >= textStart) && (key < (textStart + textCount))) {
          if (changeMenuItemScaled(getItem(), key-textStart, textCount)) {
            settingChanged();
          } else {
            commandRejected();
          }
        } else if ((key >= statusStart) && (key < (statusStart + statusCount))) {
          switch (key - statusStart) {
            default:
              commandRejected();
              break;
          }
        } else {
          commandRejected();
        }

        return 1;
      }

      break;
  }

  return 0;
}

static KeyTableCommandContext
getCommandContext_MenuScreen (void) {
  return KTB_CTX_MENU;
}

void
initializeMenuScreen (MenuScreen *menu) {
  initializeBaseScreen(&menu->base);
  menu->base.currentVirtualTerminal = currentVirtualTerminal_MenuScreen;
  menu->base.formatTitle = formatTitle_MenuScreen;
  menu->base.describe = describe_MenuScreen;
  menu->base.readCharacters = readCharacters_MenuScreen;
  menu->base.executeCommand = executeCommand_MenuScreen;
  menu->base.getCommandContext = getCommandContext_MenuScreen;
  menu->construct = construct_MenuScreen;
  menu->destruct = destruct_MenuScreen;
}
