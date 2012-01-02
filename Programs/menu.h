/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

typedef struct {
  const char *label;
  const char *comment;
} MenuString;

extern Menu *newMenu (void);
extern void deallocateMenu (Menu *menu);
extern MenuItem *newMenuItem (Menu *menu, unsigned char *setting, const MenuString *name);

extern MenuItem *newNumericMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
);

extern MenuItem *newStringMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  const MenuString *strings, unsigned char count
);

#define newEnumeratedMenuItem(menu, setting, name, strings) newStringMenuItem(menu, setting, name, strings, ARRAY_COUNT(strings))
extern MenuItem *newBooleanMenuItem (Menu *menu, unsigned char *setting, const MenuString *name);

extern MenuItem *newFileMenuItem (
  Menu *menu, const MenuString *name,
  const char *directory, const char *extension,
  const char *initial, int none
);

typedef int MenuItemTester (void);
extern void setMenuItemTester (MenuItem *item, MenuItemTester *handler);

typedef int MenuItemChanged (const MenuItem *item, unsigned char setting);
extern void setMenuItemChanged (MenuItem *item, MenuItemChanged *handler);

extern void setMenuItemStrings (MenuItem *item, const MenuString *strings, unsigned char count);

extern unsigned int getMenuSize (const Menu *menu);
extern unsigned int getMenuIndex (const Menu *menu);
extern MenuItem *getMenuItem (Menu *menu, unsigned int index);
extern MenuItem *getCurrentMenuItem (Menu *menu);

extern const MenuString *getMenuItemName (const MenuItem *item);
extern const char *getMenuItemValue (const MenuItem *item);
extern const char *getMenuItemComment (const MenuItem *item);

extern int changeMenuItemPrevious (const MenuItem *item);
extern int changeMenuItemNext (const MenuItem *item);
extern int changeMenuItemScaled (const MenuItem *item, unsigned int index, unsigned int count);

extern int setMenuPreviousItem (Menu *menu);
extern int setMenuNextItem (Menu *menu);
extern int setMenuFirstItem (Menu *menu);
extern int setMenuLastItem (Menu *menu);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MENU */
