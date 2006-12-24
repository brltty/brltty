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

BEGIN {
  writeHeaderPrologue("BRLAPI_INCLUDED_CONSTANTS", "brlapi.h")
  beginDoxygenFile()
  print "/** \\ingroup brlapi_keycodes"
  print " * @{ */"
  print ""
}

END {
  print "/** @} */"
  print ""
  writeHeaderEpilogue()
}

function brlCommand(name, symbol, value, help) {
  writeMacroDefinition("BRLAPI_KEY_CMD_" name, "(BRLAPI_KEY_CMD(0) + " value ")", help)
}

function brlBlock(name, symbol, value, help) {
  if (name == "PASSCHAR") return
  if (name == "PASSKEY") return

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeMacroDefinition("BRLAPI_KEY_CMD_" name, "BRLAPI_KEY_CMD(" substr(value, 1, length(value)-2) ")", help)
  }
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
  if (value ~ /^0[xX][0-9a-fA-F]+0000$/) {
    value = substr(value, 1, length(value)-4)
    if (name ~ /^CHAR_/) {
      name = substr(name, 6)
    } else {
      value = value "00"
    }
    value = "BRLAPI_KEY_FLG(" value ")"
  } else if (value ~ /^\(/) {
    gsub("BRL_FLG_", "BRLAPI_KEY_FLG_", value)
  } else {
    return
  }
  writeMacroDefinition("BRLAPI_KEY_FLG_" name, value, help)
}

function brlDot(number, symbol, value, help) {
  writeMacroDefinition("BRLAPI_DOT" number, value, help)
}
