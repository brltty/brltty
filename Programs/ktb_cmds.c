/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "ktb.h"
#include "ktb_list.h"
#include "brl_cmds.h"

static const CommandListEntry commandList_modes[] = {
  { .command = BRL_CMD_HELP },
  { .command = BRL_CMD_LEARN },
  { .command = BRL_CMD_PREFMENU },
  { .command = BRL_CMD_INFO },
  { .command = BRL_CMD_DISPMD },
  { .command = BRL_CMD_FREEZE },
  { .command = BRL_CMD_BLK(DESCCHAR) },
  { .command = BRL_CMD_TIME },
  { .command = BRL_CMD_BLK(CONTEXT) },
};

static const CommandListEntry commandList_cursor[] = {
  { .command = BRL_CMD_HOME },
  { .command = BRL_CMD_RETURN },
  { .command = BRL_CMD_BACK },
  { .command = BRL_CMD_BLK(ROUTE) },
  { .command = BRL_CMD_CSRJMP_VERT },
  { .command = BRL_CMD_ROUTE_CURR_LOCN },
};

static const CommandListEntry commandList_vertical[] = {
  { .command = BRL_CMD_LNUP },
  { .command = BRL_CMD_LNDN },
  { .command = BRL_CMD_TOP },
  { .command = BRL_CMD_BOT },
  { .command = BRL_CMD_TOP_LEFT },
  { .command = BRL_CMD_BOT_LEFT },
  { .command = BRL_CMD_PRDIFLN },
  { .command = BRL_CMD_NXDIFLN },
  { .command = BRL_CMD_ATTRUP },
  { .command = BRL_CMD_ATTRDN },
  { .command = BRL_CMD_PRPGRPH },
  { .command = BRL_CMD_NXPGRPH },
  { .command = BRL_CMD_PRPROMPT },
  { .command = BRL_CMD_NXPROMPT },
  { .command = BRL_CMD_WINUP },
  { .command = BRL_CMD_WINDN },
  { .command = BRL_CMD_BLK(PRINDENT) },
  { .command = BRL_CMD_BLK(NXINDENT) },
  { .command = BRL_CMD_BLK(PRDIFCHAR) },
  { .command = BRL_CMD_BLK(NXDIFCHAR) },
  { .command = BRL_CMD_BLK(GOTOLINE) },
};

static const CommandListEntry commandList_horizontal[] = {
  { .command = BRL_CMD_FWINLT },
  { .command = BRL_CMD_FWINRT },
  { .command = BRL_CMD_FWINLTSKIP },
  { .command = BRL_CMD_FWINRTSKIP },
  { .command = BRL_CMD_PRNBWIN},
  { .command = BRL_CMD_NXNBWIN},
  { .command = BRL_CMD_LNBEG },
  { .command = BRL_CMD_LNEND },
  { .command = BRL_CMD_CHRLT },
  { .command = BRL_CMD_CHRRT },
  { .command = BRL_CMD_HWINLT },
  { .command = BRL_CMD_HWINRT },
  { .command = BRL_CMD_BLK(SETLEFT) },
};

static const CommandListEntry commandList_clipboard[] = {
  { .command = BRL_CMD_BLK(CLIP_NEW) },
  { .command = BRL_CMD_BLK(CLIP_ADD) },
  { .command = BRL_CMD_BLK(COPY_LINE) },
  { .command = BRL_CMD_BLK(COPY_RECT) },
  { .command = BRL_CMD_BLK(CLIP_COPY) },
  { .command = BRL_CMD_BLK(CLIP_APPEND) },
  { .command = BRL_CMD_PASTE },
  { .command = BRL_CMD_BLK(PASTE_HISTORY) },
  { .command = BRL_CMD_PRSEARCH },
  { .command = BRL_CMD_NXSEARCH },
  { .command = BRL_CMD_CLIP_SAVE },
  { .command = BRL_CMD_CLIP_RESTORE },
};

static const CommandListEntry commandList_feature[] = {
  { .command = BRL_CMD_TOUCH_NAV },
  { .command = BRL_CMD_AUTOREPEAT },
  { .command = BRL_CMD_SIXDOTS },
  { .command = BRL_CMD_SKPIDLNS },
  { .command = BRL_CMD_SKPBLNKWINS },
  { .command = BRL_CMD_SLIDEWIN },
  { .command = BRL_CMD_CSRTRK },
  { .command = BRL_CMD_CSRSIZE },
  { .command = BRL_CMD_CSRVIS },
  { .command = BRL_CMD_CSRHIDE },
  { .command = BRL_CMD_CSRBLINK },
  { .command = BRL_CMD_ATTRVIS },
  { .command = BRL_CMD_ATTRBLINK },
  { .command = BRL_CMD_CAPBLINK },
  { .command = BRL_CMD_TUNES },
  { .command = BRL_CMD_BLK(SET_TEXT_TABLE) },
  { .command = BRL_CMD_BLK(SET_ATTRIBUTES_TABLE) },
  { .command = BRL_CMD_BLK(SET_CONTRACTION_TABLE) },
  { .command = BRL_CMD_BLK(SET_KEYBOARD_TABLE) },
  { .command = BRL_CMD_BLK(SET_LANGUAGE_PROFILE) },
};

static const CommandListEntry commandList_menu[] = {
  { .command = BRL_CMD_MENU_PREV_ITEM },
  { .command = BRL_CMD_MENU_NEXT_ITEM },
  { .command = BRL_CMD_MENU_FIRST_ITEM },
  { .command = BRL_CMD_MENU_LAST_ITEM },
  { .command = BRL_CMD_MENU_PREV_SETTING },
  { .command = BRL_CMD_MENU_NEXT_SETTING },
  { .command = BRL_CMD_MENU_PREV_LEVEL },
  { .command = BRL_CMD_PREFLOAD },
  { .command = BRL_CMD_PREFSAVE },
};

static const CommandListEntry commandList_say[] = {
  { .command = BRL_CMD_MUTE },
  { .command = BRL_CMD_SAY_LINE },
  { .command = BRL_CMD_SAY_ABOVE },
  { .command = BRL_CMD_SAY_BELOW },
  { .command = BRL_CMD_SPKHOME },
  { .command = BRL_CMD_SAY_SOFTER },
  { .command = BRL_CMD_SAY_LOUDER },
  { .command = BRL_CMD_SAY_SLOWER },
  { .command = BRL_CMD_SAY_FASTER },
  { .command = BRL_CMD_AUTOSPEAK },
  { .command = BRL_CMD_ASPK_SEL_LINE },
  { .command = BRL_CMD_ASPK_SEL_CHAR },
  { .command = BRL_CMD_ASPK_INS_CHARS },
  { .command = BRL_CMD_ASPK_DEL_CHARS },
  { .command = BRL_CMD_ASPK_REP_CHARS },
  { .command = BRL_CMD_ASPK_CMP_WORDS },
};

static const CommandListEntry commandList_speak[] = {
  { .command = BRL_CMD_SPEAK_CURR_CHAR },
  { .command = BRL_CMD_DESC_CURR_CHAR },
  { .command = BRL_CMD_SPEAK_PREV_CHAR },
  { .command = BRL_CMD_SPEAK_NEXT_CHAR },
  { .command = BRL_CMD_SPEAK_FRST_CHAR },
  { .command = BRL_CMD_SPEAK_LAST_CHAR },
  { .command = BRL_CMD_SPEAK_CURR_WORD },
  { .command = BRL_CMD_SPELL_CURR_WORD },
  { .command = BRL_CMD_SPEAK_PREV_WORD },
  { .command = BRL_CMD_SPEAK_NEXT_WORD },
  { .command = BRL_CMD_SPEAK_CURR_LINE },
  { .command = BRL_CMD_SPEAK_PREV_LINE },
  { .command = BRL_CMD_SPEAK_NEXT_LINE },
  { .command = BRL_CMD_SPEAK_FRST_LINE },
  { .command = BRL_CMD_SPEAK_LAST_LINE },
  { .command = BRL_CMD_SPEAK_CURR_LOCN },
  { .command = BRL_CMD_SHOW_CURR_LOCN },
};

static const CommandListEntry commandList_input[] = {
  { .command = BRL_CMD_BLK(PASSDOTS) },
  { .command = BRL_CMD_BLK(PASSCHAR) },
  { .command = BRL_CMD_KEY(BACKSPACE) },
  { .command = BRL_CMD_KEY(ENTER) },
  { .command = BRL_CMD_KEY(TAB) },
  { .command = BRL_CMD_KEY(CURSOR_LEFT) },
  { .command = BRL_CMD_KEY(CURSOR_RIGHT) },
  { .command = BRL_CMD_KEY(CURSOR_UP) },
  { .command = BRL_CMD_KEY(CURSOR_DOWN) },
  { .command = BRL_CMD_KEY(PAGE_UP) },
  { .command = BRL_CMD_KEY(PAGE_DOWN) },
  { .command = BRL_CMD_KEY(HOME) },
  { .command = BRL_CMD_KEY(END) },
  { .command = BRL_CMD_KEY(INSERT) },
  { .command = BRL_CMD_KEY(DELETE) },
  { .command = BRL_CMD_UNSTICK },
  { .command = BRL_CMD_UPPER },
  { .command = BRL_CMD_SHIFT },
  { .command = BRL_CMD_CONTROL },
  { .command = BRL_CMD_META },
  { .command = BRL_CMD_ALTGR },
  { .command = BRL_CMD_GUI },
  { .command = BRL_CMD_KEY(ESCAPE) },
  { .command = BRL_CMD_KEY(FUNCTION) },
  { .command = BRL_CMD_BLK(SWITCHVT) },
  { .command = BRL_CMD_SWITCHVT_PREV },
  { .command = BRL_CMD_SWITCHVT_NEXT },
  { .command = BRL_CMD_BLK(SELECTVT) },
  { .command = BRL_CMD_SELECTVT_PREV },
  { .command = BRL_CMD_SELECTVT_NEXT },
  { .command = BRL_CMD_BRLKBD },
  { .command = BRL_CMD_BRLUCDOTS },
};

static const CommandListEntry commandList_special[] = {
  { .command = BRL_CMD_BLK(SETMARK) },
  { .command = BRL_CMD_BLK(GOTOMARK) },
  { .command = BRL_CMD_RESTARTBRL },
  { .command = BRL_CMD_BRL_STOP },
  { .command = BRL_CMD_BRL_START },
  { .command = BRL_CMD_RESTARTSPEECH },
  { .command = BRL_CMD_SPK_STOP },
  { .command = BRL_CMD_SPK_START },
  { .command = BRL_CMD_SCR_STOP },
  { .command = BRL_CMD_SCR_START },
};

static const CommandListEntry commandList_internal[] = {
  { .command = BRL_CMD_NOOP },
  { .command = BRL_CMD_OFFLINE },
  { .command = BRL_CMD_BLK(ALERT) },
  { .command = BRL_CMD_BLK(PASSXT) },
  { .command = BRL_CMD_BLK(PASSAT) },
  { .command = BRL_CMD_BLK(PASSPS2) },
  { .command = BRL_CMD_BLK(TOUCH_AT) },
};

#define COMMAND_LIST(name) .commands = { \
  .table = commandList_##name, \
  .count = ARRAY_COUNT(commandList_##name), \
}

const CommandGroupEntry commandGroupTable[] = {
  { COMMAND_LIST(modes),
    .listAfter = listHotkeys,
    .name = WS_C("Special Modes")
  },

  { COMMAND_LIST(cursor),
    .name = WS_C("Cursor Functions")
  },

  { COMMAND_LIST(vertical),
    .name = WS_C("Vertical Navigation")
  },

  { COMMAND_LIST(horizontal),
    .name = WS_C("Horizontal Navigation")
  },

  { COMMAND_LIST(clipboard),
    .name = WS_C("Clipboard Functions")
  },

  { COMMAND_LIST(feature),
    .name = WS_C("Configuration Functions")
  },

  { COMMAND_LIST(menu),
    .name = WS_C("Menu Operations")
  },

  { COMMAND_LIST(say),
    .name = WS_C("Speech Functions")
  },

  { COMMAND_LIST(speak),
    .name = WS_C("Speech Navigation")
  },

  { COMMAND_LIST(input),
    .listBefore = listKeyboardFunctions,
    .name = WS_C("Keyboard Input")
  },

  { COMMAND_LIST(special),
    .name = WS_C("Special Functions")
  },

  { COMMAND_LIST(internal),
    .name = WS_C("Internal Functions")
  },
};

const unsigned char commandGroupCount = ARRAY_COUNT(commandGroupTable);
