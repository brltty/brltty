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

#define MENU_MAXIMUM_ITEM_VALUE 0XFF

typedef struct MenuStruct Menu;
typedef struct MenuItemStruct MenuItem;

typedef struct {
  const char *keyword;
  const char *comment;
} MenuString;

extern Menu *newMenu (void);
extern void deallocateMenu (Menu *menu);
extern MenuItem *newMenuItem (Menu *menu, unsigned char *setting, const char *label);

extern MenuItem *newNumericMenuItem (
  Menu *menu, unsigned char *setting, const char *label,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
);

extern MenuItem *newStringsMenuItem (
  Menu *menu, unsigned char *setting, const char *label,
  const MenuString *strings, unsigned char count
);

#define newEnumeratedMenuItem(menu, setting, label, strings) newStringsMenuItem(menu, setting, label, strings, ARRAY_COUNT(strings))
extern MenuItem *newBooleanMenuItem (Menu *menu, unsigned char *setting, const char *label);

typedef int MenuItemTester (void);
extern void setMenuItemTester (MenuItem *item, MenuItemTester *handler);

typedef int MenuItemChanged (unsigned char setting);
extern void setMenuItemChanged (MenuItem *item, MenuItemChanged *handler);

extern void setMenuItemStrings (MenuItem *item, const MenuString *strings, unsigned char count);
extern void setMenuItemKeywords (MenuItem *item, const char *const *keywords, unsigned char count);

extern MenuItem *getCurrentMenuItem (Menu *menu);
extern const char *getMenuItemLabel (MenuItem *item);
extern const char *getMenuItemValue (MenuItem *item);
extern const char *getMenuItemComment (MenuItem *item);

extern int changeMenuItemPrevious (MenuItem *item);
extern int changeMenuItemNext (MenuItem *item);
extern int changeMenuItemScaled (MenuItem *item, unsigned int index, unsigned int count);

extern int setMenuPreviousItem (Menu *menu);
extern int setMenuNextItem (Menu *menu);
extern int setMenuFirstItem (Menu *menu);
extern int setMenuLastItem (Menu *menu);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MENU */
