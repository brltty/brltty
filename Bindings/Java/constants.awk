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
  print "public interface BrlapiConstants {"
}

END {
  print "}"
}

function brlCommand(name, symbol, value, help) {
  writeCommandDefinition(name, value)
}

function brlBlock(name, symbol, value, help) {
  if (name == "PASSCHAR") return
  if (name == "PASSKEY") return

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeCommandDefinition(name, toupper(value) "00")
  }
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
  if (value ~ /^0[xX][0-9a-fA-F]+0000$/) {
    value = toupper(substr(value, 1, length(value)-4))
    if (name ~ /^CHAR_/) {
      name = substr(name, 6)
    } else {
      value = value "00"
    }
  } else if (value ~ /^\(/) {
    gsub("BRL_FLG_", "KEY_FLG_", value)
  } else {
    return
  }
  writeJavaConstant("KEY_FLG_" name, value)
}

function brlDot(number, symbol, value, help) {
  writeJavaConstant("BRL_DOT" number, value)
}

function apiType(name, symbol, value, help) {
  writeJavaConstant("KEY_TYPE_" name, hexadecimalValue(value));
}

function apiKey(name, symbol, value, help) {
  value = hexadecimalValue(value)

  if (name == "FUNCTION") {
    for (i=0; i<35; ++i) {
      writeKeyDefinition("F" (i+1), "(" value " + " i ")")
    }
  } else {
    writeKeyDefinition(name, value)
  }
}

function writeCommandDefinition(name, value) {
  writeJavaConstant("KEY_CMD_" name, "(KEY_TYPE_CMD | " value ")")
}

function writeKeyDefinition(name, value) {
  writeJavaConstant("KEY_SYM_" name, "(KEY_TYPE_SYM | " value ")")
}

function writeJavaConstant(name, value) {
  print "  public static final int " name " = " value ";"
}

function hexadecimalValue(value) {
  gsub("[uU]?[lL]*$", "", value)
  return value
}

