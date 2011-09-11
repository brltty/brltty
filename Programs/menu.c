/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

#include "log.h"
#include "menu.h"
#include "parse.h"

struct MenuStruct {
  MenuItem *items;
  unsigned int size;
  unsigned int count;
  unsigned int index;

  char valueBuffer[0X10];
};

struct MenuItemStruct {
  Menu *menu;
  unsigned char *setting;                 /* pointer to current value */
  MenuString name;                      /* item name for presentation */

  MenuItemTester *test;                     /* returns true if item should be presented */
  MenuItemChanged *changed;

  unsigned char minimum;                  /* lowest valid value */
  unsigned char maximum;                  /* highest valid value */
  unsigned char divisor;                  /* present only multiples of this value */

  const char * (*getValue) (const MenuItem *item);
  const char * (*getComment) (const MenuItem *item);
  const void *data;
};

char *
getLocalText (const char *string) {
  return (string && *string)? gettext(string): "";
}

Menu *
newMenu (void) {
  Menu *menu = malloc(sizeof(*menu));

  if (menu) {
    menu->items = NULL;
    menu->size = 0;
    menu->count = 0;
    menu->index = 0;
  }

  return menu;
}

void
deallocateMenu (Menu *menu) {
  if (menu) {
    if (menu->items) free(menu->items);
    free(menu);
  }
}

static MenuItem *
getMenuItem (Menu *menu, unsigned int index) {
  return (index < menu->count)? &menu->items[index]: NULL;
}

MenuItem *
getCurrentMenuItem (Menu *menu) {
  return getMenuItem(menu, menu->index);
}

static int
testMenuItem (Menu *menu, unsigned int index) {
  MenuItem *item = getMenuItem(menu, index);
  return item && (!item->test || item->test());
}

const MenuString *
getMenuItemName (const MenuItem *item) {
  return &item->name;
}

static const char *
getMenuItemValue_numeric (const MenuItem *item) {
  Menu *menu = item->menu;
  snprintf(menu->valueBuffer, sizeof(menu->valueBuffer), "%u", *item->setting);
  return menu->valueBuffer;
}

static const char *
getMenuItemValue_strings (const MenuItem *item) {
  const MenuString *strings = item->data;
  const char *label = strings[*item->setting - item->minimum].label;
  if (!label || !*label) label = strtext("<off>");
  return getLocalText(label);
}

const char *
getMenuItemValue (const MenuItem *item) {
  return item->getValue(item);
}

static const char *
getMenuItemComment_none (const MenuItem *item) {
  return "";
}

static const char *
getMenuItemComment_strings (const MenuItem *item) {
  const MenuString *strings = item->data;
  return getLocalText(strings[*item->setting - item->minimum].comment);
}

const char *
getMenuItemComment (const MenuItem *item) {
  return item->getComment(item);
}

MenuItem *
newMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  if (menu->count == menu->size) {
    unsigned int newSize = menu->size? (menu->size << 1): 0X10;
    MenuItem *newItems = realloc(menu->items, (newSize * sizeof(*newItems)));

    if (!newItems) {
      logMallocError();
      return NULL;
    }

    menu->items = newItems;
    menu->size = newSize;
  }

  {
    MenuItem *item = getMenuItem(menu, menu->count++);

    item->menu = menu;
    item->setting = setting;

    item->name.label = getLocalText(name->label);
    item->name.comment = getLocalText(name->comment);

    item->test = NULL;
    item->changed = NULL;

    item->minimum = 0;
    item->maximum = 0;
    item->divisor = 1;

    item->getValue = getMenuItemValue_numeric;
    item->getComment = getMenuItemComment_none;
    item->data = NULL;

    return item;
  }
}

void
setMenuItemTester (MenuItem *item, MenuItemTester *handler) {
  item->test = handler;
}

void
setMenuItemChanged (MenuItem *item, MenuItemChanged *handler) {
  item->changed = handler;
}

void
setMenuItemStrings (MenuItem *item, const MenuString *strings, unsigned char count) {
  item->getValue = getMenuItemValue_strings;
  item->getComment = getMenuItemComment_strings;
  item->data = strings;

  item->minimum = 0;
  item->maximum = count - 1;
  item->divisor = 1;
}

void
setMenuItemValues (MenuItem *item, const char *const *values, unsigned char count) {
  const MenuString *strings = item->data;
  unsigned int index;

  for (index=0; index<count; index+=1) {
    *((const char **)&strings[index].label) = values[index];
  }

  item->maximum = count - 1;
}

MenuItem *
newNumericMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
) {
  MenuItem *item = newMenuItem(menu, setting, name);

  if (item) {
    item->minimum = minimum;
    item->maximum = maximum;
    item->divisor = divisor;
  }

  return item;
}

MenuItem *
newStringsMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  const MenuString *strings, unsigned char count
) {
  MenuItem *item = newMenuItem(menu, setting, name);

  if (item) {
    setMenuItemStrings(item, strings, count);
  }

  return item;
}

MenuItem *
newBooleanMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  static const MenuString strings[] = {
    {.label=strtext("No")},
    {.label=strtext("Yes")}
  };

  return newEnumeratedMenuItem(menu, setting, name, strings);
}

static int
adjustMenuItem (const MenuItem *item, void (*adjust) (const MenuItem *item)) {
  int count = item->maximum - item->minimum + 1;

  do {
    adjust(item);
    if (!--count) break;
  } while ((*item->setting % item->divisor) || (item->changed && !item->changed(*item->setting)));

  return !!count;
}

static void
decrementMenuItem (const MenuItem *item) {
  if ((*item->setting)-- <= item->minimum) *item->setting = item->maximum;
}

int
changeMenuItemPrevious (const MenuItem *item) {
  return adjustMenuItem(item, decrementMenuItem);
}

static void
incrementMenuItem (const MenuItem *item) {
  if ((*item->setting)++ >= item->maximum) *item->setting = item->minimum;
}

int
changeMenuItemNext (const MenuItem *item) {
  return adjustMenuItem(item, incrementMenuItem);
}

int
changeMenuItemScaled (const MenuItem *item, unsigned int index, unsigned int count) {
  unsigned char oldSetting = *item->setting;

  if (item->data) {
    *item->setting = index % (item->maximum + 1);
  } else {
    *item->setting = rescaleInteger(index, count-1, item->maximum-item->minimum) + item->minimum;
  }

  if (!item->changed || item->changed(*item->setting)) return 1;
  *item->setting = oldSetting;
  return 0;
}

int
setMenuPreviousItem (Menu *menu) {
  do {
    if (!menu->index) menu->index = menu->count;
    if (!menu->index) return 0;
  } while (!testMenuItem(menu, --menu->index));

  return 1;
}

int
setMenuNextItem (Menu *menu) {
  if (menu->index >= menu->count) return 0;

  do {
    if (++menu->index == menu->count) menu->index = 0;
  } while (!testMenuItem(menu, menu->index));

  return 1;
}

int
setMenuFirstItem (Menu *menu) {
  if (!menu->count) return 0;
  menu->index = 0;
  return testMenuItem(menu, menu->index) || setMenuNextItem(menu);
}

int
setMenuLastItem (Menu *menu) {
  if (!menu->count) return 0;
  menu->index = menu->count - 1;
  return testMenuItem(menu, menu->index) || setMenuPreviousItem(menu);
}
