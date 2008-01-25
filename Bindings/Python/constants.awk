###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2008 by
#   Alexis Robert <alexissoft@free.fr>
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License,
# or (at your option) any later version.
# Please see the file COPYING-API for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

function brlCommand(name, symbol, value, help) {
  writeCommandDefinition(name, value, help)
}

function brlBlock(name, symbol, value, help) {
  if (name == "PASSCHAR") return
  if (name == "PASSKEY") return

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeCommandDefinition(name, tolower(value) "00", help)
  }
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
  if (value ~ /^0[xX][0-9a-fA-F]+0000$/) {
    value = tolower(substr(value, 1, length(value)-4))
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
  writePythonAssignment("KEY_FLG_" name, value, help)
}

function brlDot(number, symbol, value, help) {
  writePythonAssignment("DOT" number, value, help)
}

function apiConstant(name, symbol, value, help) {
  writePythonAssignment(name, value, help)
}

function apiMask(name, symbol, value, help) {
  writePythonAssignment("KEY_" name, "c_brlapi." symbol, help)
}

function apiShift(name, symbol, value, help) {
  value = hexadecimalValue(value)
  writePythonAssignment("KEY_" name, value, help)
}

function apiType(name, symbol, value, help) {
  value = hexadecimalValue(value)
  writePythonAssignment("KEY_TYPE_" name, value, help)
}

function apiKey(name, symbol, value, help) {
  value = hexadecimalValue(value)

  if (name == "FUNCTION") {
    for (i=0; i<35; ++i) {
      key = "F" (i+1)
      writeKeyDefinition(key, "(" value " + " i ")", "the " key " key")
    }
  } else {
    writeKeyDefinition(name, value, help)
  }
}

function apiRangeType(name, symbol, value, help) {
  writePythonAssignment("rangeType_" name, value, help)
}

function writeCommandDefinition(name, value, help) {
  writePythonAssignment("KEY_CMD_" name, value, help)
}

function writeKeyDefinition(name, value, help) {
  writePythonAssignment("KEY_SYM_" name, value, help)
}

function writePythonAssignment(name, value, help) {
  if (length(help) > 0) print "## " help
  print name " = " value
}

function hexadecimalValue(value) {
  value = tolower(value)
  return value
}

