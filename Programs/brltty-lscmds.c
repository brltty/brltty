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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "program.h"
#include "options.h"
#include "ktb_cmds.h"
#include "cmd.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

static void
writeCharacter (char character) {
  putc(character, stdout);
}

static void
writeCharacters (char character, size_t count) {
  while (count > 0) {
    writeCharacter(character);
    count -= 1;
  }
}

static void
endLine (void) {
  writeCharacter('\n');
}

static void
endHeader (char character, size_t length) {
  endLine();
  writeCharacters(character, length);
  endLine();
  endLine();
}

static void
writeString (const char *string) {
  while (*string) writeCharacter(*string++);
}

static void
writeStringHeader (const char *header, char character) {
  writeString(header);
  endHeader(character, strlen(header));
}

static void
writeText (const wchar_t *text) {
  printf("%ls", text);
}

static void
writeTextHeader (const wchar_t *header, char character) {
  writeText(header);
  endHeader(character, wcslen(header));
}

void
commandGroupHook_hotkeys (CommandGroupHookData *data) {
}

void
commandGroupHook_keyboardFunctions (CommandGroupHookData *data) {
}

static void
listModifiers (int include, const char *type, int *started, const CommandModifierEntry *modifiers) {
  if (include) {
    if (!*started) {
      *started = 1;
      printf("The following modifiers may be specified:\n\n");
    }

    printf("* %s", type);

    if (modifiers) {
      const CommandModifierEntry *modifier = modifiers;
      char punctuation = ':';

      while (modifier->name) {
        printf("%c %s", punctuation, modifier->name);
        punctuation = ',';
        modifier += 1;
      }
    }

    endLine();
  }
}

static void
listCommand (const CommandEntry *cmd) {
  writeStringHeader(cmd->name, '~');

  {
    const char *description = cmd->description;
    writeCharacter(toupper(*description++));
    printf("%s.\n\n", description);
  }

  {
    int started = 0;

    listModifiers(cmd->isColumn, "a column number", &started, NULL);
    listModifiers(cmd->isOffset, "an offset", &started, NULL);

    listModifiers(
      cmd->isToggle, "Toggle", &started,
      commandModifierTable_toggle
    );

    listModifiers(
      cmd->isMotion, "Motion", &started,
      commandModifierTable_motion
    );

    listModifiers(
      cmd->isRow, "Row", &started,
      commandModifierTable_row
    );

    listModifiers(
      cmd->isVertical, "Vertical", &started,
      commandModifierTable_vertical
    );

    listModifiers(
      cmd->isInput, "Input", &started,
      commandModifierTable_input
    );

    listModifiers(
      (cmd->isCharacter || cmd->isBraille), "Character", &started,
      commandModifierTable_character
    );

    listModifiers(
      cmd->isBraille, "Braille", &started,
      commandModifierTable_braille
    );

    listModifiers(
      cmd->isKeyboard, "Keyboard", &started,
      commandModifierTable_keyboard
    );

    if (started) endLine();
  }
}

static void
listCommandGroup (const CommandGroupEntry *grp) {
  writeTextHeader(grp->name, '-');

  const CommandListEntry *cmd = grp->commands.table;
  const CommandListEntry *cmdEnd = cmd + grp->commands.count;

  while (cmd < cmdEnd) {
    listCommand(findCommandEntry(cmd->command));
    cmd += 1;
  }
}

static void
listCommandGroups (void) {
  const CommandGroupEntry *grp = commandGroupTable;
  const CommandGroupEntry *grpEnd = grp + commandGroupCount;

  while (grp < grpEnd) {
    listCommandGroup(grp);
    grp += 1;
  }
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-lscmds"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }
fflush(stdout);

  writeStringHeader("BRLTTY Command List", '=');
  writeString(".. contents::\n\n");

  listCommandGroups();
  return PROG_EXIT_SUCCESS;
}
