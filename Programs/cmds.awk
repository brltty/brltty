###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2009 by The BRLTTY Developers.
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
  print "{"
  print "  .name = \"" name "\","
  print "  .code = " symbol ","

  if (help ~ /^go /) print "  .isMotion = 1,"
  if (help ~ /^toggle /) print "  .isToggle = 1,"

  if (symbol ~ /^BRL_BLK_/) {
    if (symbol ~ /^BRL_BLK_PASS/) {
      if (symbol ~ /^BRL_BLK_PASSKEY.*KEY_FUNCTION$/) {
        print "  .isOffset = 1,"
      }
    } else if (help ~ / character$/) {
      print "  .isColumn = 1,"
    } else if (help ~ / line$/) {
      print "  .isRow = 1,"
    } else {
      print "  .isOffset = 1,"
    }
  }

  print "  .description = \"" help "\""
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
