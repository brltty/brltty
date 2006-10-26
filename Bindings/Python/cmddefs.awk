###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2006 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

function brlCommand(name, symbol, value, help) {
  writeKeyDefinition(name, value)
}

function brlBlock(name, symbol, value, help) {
  if (name == "PASSCHAR") return
  if (name == "PASSKEY") return

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeKeyDefinition(name, tolower(value) "00")
  }
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
}

function brlDot(number, symbol, value, help) {
}

function writeKeyDefinition(name, value) {
  print "KEY_CMD_" name " = " value
}
