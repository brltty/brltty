###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2013 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

function writeCommandEntry(name, symbol, help) {
  if (help ~ /^deprecated /) return

  print "{"
  print "  .name = \"" name "\","
  print "  .code = " symbol ","

  if (help ~ /^set .*\//) print "  .isToggle = 1,"
  if (help ~ /^go /) print "  .isMotion = 1,"
  if (help ~ /^bring /) print "  .isRouting = 1,"

  if (symbol ~ /^BRL_BLK_/) {
    if (symbol ~ /^BRL_BLK_PASS/) {
      if (symbol ~ /PASS(CHAR|DOTS|KEY)/) {
        print "  .isInput = 1,"
      }

      if (symbol ~ /PASSCHAR/) {
        print "  .isCharacter = 1,"
      }

      if (symbol ~ /PASSDOTS/) {
        print "  .isBraille = 1,"
      }

      if (symbol ~ /PASSKEY.*KEY_FUNCTION/) {
        print "  .isOffset = 1,"
      }

      if (symbol ~ /PASS(XT|AT|PS2)/) {
        print "  .isKeyboard = 1,"
      }
    } else if (help ~ / character$/) {
      print "  .isColumn = 1,"
    } else if (help ~ / characters /) {
      print "  .isRange = 1,"
    } else if (help ~ / line$/) {
      print "  .isRow = 1,"
    } else {
      print "  .isOffset = 1,"
    }
  }

  print "  .description = strtext(\"" help "\")"
  print "}"
  print ","
}

function brlCommand(name, symbol, value, help) {
  writeCommandEntry(name, symbol, help)
}

function brlBlock(name, symbol, value, help) {
  writeCommandEntry(name, symbol, help)
}

function brlKey(name, symbol, value, help) {
  writeCommandEntry("KEY_" name, "BRL_BLK_PASSKEY+" symbol, help)
}

function brlFlag(name, symbol, value, help) {
}

function brlDot(number, symbol, value, help) {
}
