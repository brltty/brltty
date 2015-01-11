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

#include "ktb.h"
#include "ktb_internal.h"
#include "brl_cmds.h"

static const int commandGroupTable_modes[] = {
  BRL_CMD_HELP,
  BRL_CMD_LEARN,
  BRL_CMD_PREFMENU,
  BRL_CMD_TIME,
  BRL_CMD_INFO,
  BRL_CMD_FREEZE,
  BRL_CMD_DISPMD,
};

static const int commandGroupTable_cursor[] = {
  BRL_CMD_HOME,
  BRL_CMD_RETURN,
  BRL_CMD_BACK,
  BRL_CMD_BLK(ROUTE),
  BRL_CMD_CSRJMP_VERT,
  BRL_CMD_ROUTE_CURR_LOCN,
};

static const int commandGroupTable_vertical[] = {
  BRL_CMD_LNUP,
  BRL_CMD_LNDN,
  BRL_CMD_TOP,
  BRL_CMD_BOT,
  BRL_CMD_TOP_LEFT,
  BRL_CMD_BOT_LEFT,
  BRL_CMD_PRDIFLN,
  BRL_CMD_NXDIFLN,
  BRL_CMD_ATTRUP,
  BRL_CMD_ATTRDN,
  BRL_CMD_PRPGRPH,
  BRL_CMD_NXPGRPH,
  BRL_CMD_PRPROMPT,
  BRL_CMD_NXPROMPT,
  BRL_CMD_WINUP,
  BRL_CMD_WINDN,
  BRL_CMD_BLK(PRINDENT),
  BRL_CMD_BLK(NXINDENT),
  BRL_CMD_BLK(PRDIFCHAR),
  BRL_CMD_BLK(NXDIFCHAR),
  BRL_CMD_BLK(GOTOLINE),
};

static const int commandGroupTable_horizontal[] = {
  BRL_CMD_FWINLT,
  BRL_CMD_FWINRT,
  BRL_CMD_FWINLTSKIP,
  BRL_CMD_FWINRTSKIP,
  BRL_CMD_LNBEG,
  BRL_CMD_LNEND,
  BRL_CMD_CHRLT,
  BRL_CMD_CHRRT,
  BRL_CMD_HWINLT,
  BRL_CMD_HWINRT,
  BRL_CMD_BLK(SETLEFT),
};

static const int commandGroupTable_clipboard[] = {
  BRL_CMD_BLK(CLIP_NEW),
  BRL_CMD_BLK(CLIP_ADD),
  BRL_CMD_BLK(COPY_LINE),
  BRL_CMD_BLK(COPY_RECT),
  BRL_CMD_BLK(CLIP_COPY),
  BRL_CMD_BLK(CLIP_APPEND),
  BRL_CMD_PASTE,
  BRL_CMD_BLK(PASTE_HISTORY),
  BRL_CMD_PRSEARCH,
  BRL_CMD_NXSEARCH,
  BRL_CMD_CLIP_SAVE,
  BRL_CMD_CLIP_RESTORE,
};

static const int commandGroupTable_feature[] = {
  BRL_CMD_AUTOREPEAT,
  BRL_CMD_SIXDOTS,
  BRL_CMD_SKPIDLNS,
  BRL_CMD_SKPBLNKWINS,
  BRL_CMD_SLIDEWIN,
  BRL_CMD_CSRTRK,
  BRL_CMD_CSRSIZE,
  BRL_CMD_CSRVIS,
  BRL_CMD_CSRHIDE,
  BRL_CMD_CSRBLINK,
  BRL_CMD_ATTRVIS,
  BRL_CMD_ATTRBLINK,
  BRL_CMD_CAPBLINK,
  BRL_CMD_TUNES,
  BRL_CMD_BLK(SET_TEXT_TABLE),
  BRL_CMD_BLK(SET_ATTRIBUTES_TABLE),
  BRL_CMD_BLK(SET_CONTRACTION_TABLE),
  BRL_CMD_BLK(SET_KEYBOARD_TABLE),
  BRL_CMD_BLK(SET_LANGUAGE_PROFILE),
};

static const int commandGroupTable_menu[] = {
  BRL_CMD_MENU_FIRST_ITEM,
  BRL_CMD_MENU_LAST_ITEM,
  BRL_CMD_MENU_PREV_ITEM,
  BRL_CMD_MENU_NEXT_ITEM,
  BRL_CMD_MENU_PREV_SETTING,
  BRL_CMD_MENU_NEXT_SETTING,
  BRL_CMD_MENU_PREV_LEVEL,
  BRL_CMD_PREFLOAD,
  BRL_CMD_PREFSAVE,
};

static const int commandGroupTable_say[] = {
  BRL_CMD_AUTOSPEAK,
  BRL_CMD_MUTE,
  BRL_CMD_BLK(DESCCHAR),
  BRL_CMD_SAY_LINE,
  BRL_CMD_SAY_ABOVE,
  BRL_CMD_SAY_BELOW,
  BRL_CMD_SAY_SOFTER,
  BRL_CMD_SAY_LOUDER,
  BRL_CMD_SAY_SLOWER,
  BRL_CMD_SAY_FASTER,
  BRL_CMD_SPKHOME,
};

static const int commandGroupTable_speak[] = {
  BRL_CMD_SPEAK_CURR_CHAR,
  BRL_CMD_DESC_CURR_CHAR,
  BRL_CMD_SPEAK_PREV_CHAR,
  BRL_CMD_SPEAK_NEXT_CHAR,
  BRL_CMD_SPEAK_FRST_CHAR,
  BRL_CMD_SPEAK_LAST_CHAR,
  BRL_CMD_SPEAK_CURR_WORD,
  BRL_CMD_SPELL_CURR_WORD,
  BRL_CMD_SPEAK_PREV_WORD,
  BRL_CMD_SPEAK_NEXT_WORD,
  BRL_CMD_SPEAK_CURR_LINE,
  BRL_CMD_SPEAK_PREV_LINE,
  BRL_CMD_SPEAK_NEXT_LINE,
  BRL_CMD_SPEAK_FRST_LINE,
  BRL_CMD_SPEAK_LAST_LINE,
  BRL_CMD_SPEAK_CURR_LOCN,
  BRL_CMD_SHOW_CURR_LOCN,
  BRL_CMD_ASPK_SEL_LINE,
  BRL_CMD_ASPK_SEL_CHAR,
  BRL_CMD_ASPK_INS_CHARS,
  BRL_CMD_ASPK_DEL_CHARS,
  BRL_CMD_ASPK_REP_CHARS,
  BRL_CMD_ASPK_CMP_WORDS,
};

static const int commandGroupTable_input[] = {
  BRL_CMD_BRLKBD,
  BRL_CMD_BRLUCDOTS,
  BRL_CMD_KEY(ENTER),
  BRL_CMD_KEY(TAB),
  BRL_CMD_KEY(BACKSPACE),
  BRL_CMD_KEY(ESCAPE),
  BRL_CMD_KEY(CURSOR_LEFT),
  BRL_CMD_KEY(CURSOR_RIGHT),
  BRL_CMD_KEY(CURSOR_UP),
  BRL_CMD_KEY(CURSOR_DOWN),
  BRL_CMD_KEY(PAGE_UP),
  BRL_CMD_KEY(PAGE_DOWN),
  BRL_CMD_KEY(HOME),
  BRL_CMD_KEY(END),
  BRL_CMD_KEY(INSERT),
  BRL_CMD_KEY(DELETE),
  BRL_CMD_KEY(FUNCTION),
  BRL_CMD_SHIFT,
  BRL_CMD_UPPER,
  BRL_CMD_CONTROL,
  BRL_CMD_META,
  BRL_CMD_BLK(PASSCHAR),
  BRL_CMD_BLK(PASSDOTS),
  BRL_CMD_BLK(SWITCHVT),
  BRL_CMD_SWITCHVT_PREV,
  BRL_CMD_SWITCHVT_NEXT,
};

static const int commandGroupTable_special[] = {
  BRL_CMD_BLK(SETMARK),
  BRL_CMD_BLK(GOTOMARK),
  BRL_CMD_RESTARTBRL,
  BRL_CMD_RESTARTSPEECH,
};

static const int commandGroupTable_internal[] = {
  BRL_CMD_NOOP,
  BRL_CMD_OFFLINE,
  BRL_CMD_BLK(ALERT),
  BRL_CMD_BLK(CONTEXT),
  BRL_CMD_BLK(PASSXT),
  BRL_CMD_BLK(PASSAT),
  BRL_CMD_BLK(PASSPS2),
  BRL_CMD_BLK(TOUCH),
};

#define COMMAND_GROUP_TABLE(name) .commands = { \
  .table = commandGroupTable_##name, \
  .count = ARRAY_COUNT(commandGroupTable_##name), \
}

const CommandGroupEntry commandGroupTable[] = {
  { COMMAND_GROUP_TABLE(modes),
    .name = strtext("Special Modes")
  },

  { COMMAND_GROUP_TABLE(cursor),
    .name = strtext("Cursor Functions")
  },

  { COMMAND_GROUP_TABLE(vertical),
    .name = strtext("Vertical Navigation")
  },

  { COMMAND_GROUP_TABLE(horizontal),
    .name = strtext("Horizontal Navigation")
  },

  { COMMAND_GROUP_TABLE(clipboard),
    .name = strtext("Clipboard Functions")
  },

  { COMMAND_GROUP_TABLE(feature),
    .name = strtext("Feature Enabling")
  },

  { COMMAND_GROUP_TABLE(menu),
    .name = strtext("Menu Functions")
  },

  { COMMAND_GROUP_TABLE(say),
    .name = strtext("Speech Functions")
  },

  { COMMAND_GROUP_TABLE(speak),
    .name = strtext("Speech Navigation")
  },

  { COMMAND_GROUP_TABLE(input),
    .name = strtext("Input Functions")
  },

  { COMMAND_GROUP_TABLE(special),
    .name = strtext("Special Functions")
  },

  { COMMAND_GROUP_TABLE(internal),
    .name = strtext("Internal Functions")
  },
};

const unsigned char commandGroupCount = ARRAY_COUNT(commandGroupTable);
