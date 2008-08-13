###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2008 by
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
#   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

BEGIN {
  print "package org.a11y.BrlAPI;"
  print ""
  print "public interface Constants {"
}

END {
  print "}"
}

function brlCommand(name, symbol, value, help) {
  writeCommandDefinition(name, value, help)
}

function brlBlock(name, symbol, value, help) {
  if (name == "PASSCHAR") return
  if (name == "PASSKEY") return

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeCommandDefinition(name, hexadecimalValue(value) "00", help)
  }
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
  if (value ~ /^0[xX][0-9a-fA-F]+0000$/) {
    value = hexadecimalValue(substr(value, 1, length(value)-4))
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
  writeJavaConstant("int", "KEY_FLG_" name, value, help)
}

function brlDot(number, symbol, value, help) {
  writeJavaConstant("int", "DOT" number, value, help)
}

function apiConstant(name, symbol, value, help) {
  writeJavaConstant("int", name, value);
}

function apiMask(name, symbol, value, help) {
  writeJavaConstant("long", "KEY_" name, hexadecimalValue(value) "L");
}

function apiShift(name, symbol, value, help) {
  writeJavaConstant("int", "KEY_" name, hexadecimalValue(value));
}

function apiType(name, symbol, value, help) {
  writeJavaConstant("int", "KEY_TYPE_" name, hexadecimalValue(value));
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

function apiRangeType(name, symbol, value, help) {
  writeJavaConstant("int", "rangeType_" name, value, help)
}

function writeCommandDefinition(name, value, help) {
  writeJavaConstant("int", "KEY_CMD_" name, value, help)
}

function writeKeyDefinition(name, value) {
  writeJavaConstant("int", "KEY_SYM_" name, value)
}

function writeJavaConstant(type, name, value, help) {
  writeJavadocComment(help)
  print "  " type " " name " = " value ";"
}

function writeJavadocComment(text) {
  if (length(text) > 0) print "  /** " text " */"
}

function hexadecimalValue(value) {
  value = tolower(value)
  gsub("x0+", "x", value)
  gsub("x$", "x0", value)
  return value
}

