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

#ifndef BRLTTY_INCLUDED_MENU
#define BRLTTY_INCLUDED_MENU

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct MenuStruct Menu;
typedef struct MenuItemStruct MenuItem;

extern Menu *newMenu (void);
extern void deallocateMenu (Menu *menu);
extern MenuItem *newMenuItem (Menu *menu, unsigned char *setting, const char *label);

extern MenuItem *newNumericItem (
  Menu *menu, unsigned char *setting, const char *label,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
);

extern MenuItem *newTextItem (
  Menu *menu, unsigned char *setting, const char *label,
  const char *const *names, unsigned char count
);

#define newSymbolicItem(menu, setting, label, names) newTextItem(menu, setting, label, names, ARRAY_COUNT(names))
extern MenuItem *newBooleanItem (Menu *menu, unsigned char *setting, const char *label);

typedef int TestItem (void);
extern void setTestItem (MenuItem *item, TestItem *test);

typedef int SettingChanged (unsigned char setting);
extern void setSettingChanged (MenuItem *item, SettingChanged *changed);

extern void setItemNames (MenuItem *item, const char *const *names, unsigned char count);

extern MenuItem *getCurrentItem (Menu *menu);
extern const char *getItemLabel (MenuItem *item);
extern const char *getItemValue (MenuItem *item);

extern int setFirstItem (Menu *menu);
extern int setLastItem (Menu *menu);
extern int setPreviousItem (Menu *menu);
extern int setNextItem (Menu *menu);

extern int previousItemSetting (MenuItem *item);
extern int nextItemSetting (MenuItem *item);
extern int selectItemSetting (MenuItem *item, unsigned int index, unsigned int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MENU */
