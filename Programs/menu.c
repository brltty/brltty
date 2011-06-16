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
  const char *label;                      /* item name for presentation */

  TestItem *test;                     /* returns true if item should be presented */
  SettingChanged *changed;

  const char *const *names;               /* symbolic names of values */

  unsigned char minimum;                  /* lowest valid value */
  unsigned char maximum;                  /* highest valid value */
  unsigned char divisor;                  /* present only multiples of this value */
};

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
getCurrentItem (Menu *menu) {
  return getMenuItem(menu, menu->index);
}

static int
testMenuItem (Menu *menu, unsigned int index) {
  MenuItem *item = getMenuItem(menu, index);
  return item && (!item->test || item->test());
}

const char *
getItemLabel (MenuItem *item) {
  return gettext(item->label);
}

const char *
getItemValue (MenuItem *item) {
  if (item->names) {
    const char *name = item->names[*item->setting - item->minimum];
    if (!*name) name = strtext("<off>");
    return gettext(name);
  }

  {
    Menu *menu = item->menu;
    snprintf(menu->valueBuffer, sizeof(menu->valueBuffer), "%u", *item->setting);
    return menu->valueBuffer;
  }
}

MenuItem *
newMenuItem (Menu *menu, unsigned char *setting, const char *label) {
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
    item->label = label;

    item->test = NULL;
    item->changed = NULL;

    item->names = NULL;

    item->minimum = 0;
    item->maximum = 0;
    item->divisor = 1;

    return item;
  }
}

void
setTestItem (MenuItem *item, TestItem *test) {
  item->test = test;
}

void
setSettingChanged (MenuItem *item, SettingChanged *changed) {
  item->changed = changed;
}

void
setItemNames (MenuItem *item, const char *const *names, unsigned char count) {
  item->names = names;
  item->minimum = 0;
  item->maximum = count - 1;
  item->divisor = 1;
}

MenuItem *
newNumericItem (
  Menu *menu, unsigned char *setting, const char *label,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
) {
  MenuItem *item = newMenuItem(menu, setting, label);

  if (item) {
    item->minimum = minimum;
    item->maximum = maximum;
    item->divisor = divisor;
  }

  return item;
}

MenuItem *
newTextItem (
  Menu *menu, unsigned char *setting, const char *label,
  const char *const *names, unsigned char count
) {
  MenuItem *item = newMenuItem(menu, setting, label);

  if (item) {
    setItemNames(item, names, count);
  }

  return item;
}

MenuItem *
newBooleanItem (Menu *menu, unsigned char *setting, const char *label) {
  static const char *names[] = {
    strtext("No"),
    strtext("Yes")
  };

  return newSymbolicItem(menu, setting, label, names);
}

int
setPreviousItem (Menu *menu) {
  do {
    if (!menu->index) menu->index = menu->count;
    if (!menu->index) return 0;
  } while (!testMenuItem(menu, --menu->index));

  return 1;
}

int
setNextItem (Menu *menu) {
  if (menu->index >= menu->count) return 0;

  do {
    if (++menu->index == menu->count) menu->index = 0;
  } while (!testMenuItem(menu, menu->index));

  return 1;
}

int
setFirstItem (Menu *menu) {
  if (!menu->count) return 0;
  menu->index = 0;
  return testMenuItem(menu, menu->index) || setNextItem(menu);
}

int
setLastItem (Menu *menu) {
  if (!menu->count) return 0;
  menu->index = menu->count - 1;
  return testMenuItem(menu, menu->index) || setPreviousItem(menu);
}

static int
adjustItemSetting (MenuItem *item, void (*adjust) (MenuItem *ktem)) {
  int count = item->maximum - item->minimum + 1;

  do {
    adjust(item);
    if (!--count) break;
  } while ((*item->setting % item->divisor) || (item->changed && !item->changed(*item->setting)));

  return !!count;
}

static void
decrementItemSetting (MenuItem *item) {
  if ((*item->setting)-- <= item->minimum) *item->setting = item->maximum;
}

int
previousItemSetting (MenuItem *item) {
  return adjustItemSetting(item, decrementItemSetting);
}

static void
incrementItemSetting (MenuItem *item) {
  if ((*item->setting)++ >= item->maximum) *item->setting = item->minimum;
}

int
nextItemSetting (MenuItem *item) {
  return adjustItemSetting(item, incrementItemSetting);
}

int
selectItemSetting (MenuItem *item, unsigned int index, unsigned int count) {
  unsigned char newSetting;

  if (item->names) {
    newSetting = index % (item->maximum + 1);
  } else {
    newSetting = rescaleInteger(index, count-1, item->maximum-item->minimum) + item->minimum;
  }

  if (item->changed && !item->changed(newSetting)) return 0;
  *item->setting = newSetting;
  return 1;
}
