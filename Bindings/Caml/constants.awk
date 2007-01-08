###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2007 by The BRLTTY Developers.
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
  print "type t ="
}

END {
}

function brlCommand(name, symbol, value, help) {
  writeCommandDefinition(name, value, help)
}

function brlBlock(name, symbol, value, help) {

  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeCommandDefinition(name, hexadecimalValue(value) "00", help)
  }
}

function brlKey(name, symbol, value, help) {
#  print "braille key:"
#  print name symbol value
#  print "(** " help " *)"
}

function brlFlag(name, symbol, value, help) {
#  if (value ~ /^0[xX][0-9a-fA-F]+0000$/) {
#    value = hexadecimalValue(substr(value, 1, length(value)-4))
#    if (name ~ /^CHAR_/) {
#      name = substr(name, 6)
#    } else {
#      value = value "00"
#    }
#  } else if (value ~ /^\(/) {
#    gsub("BRL_FLG_", "KEY_FLG_", value)
#  } else {
#    return
#  }
#  writeCamlConstant("int", "KEY_FLG_" name, value, help)
}

function brlDot(number, symbol, value, help) {
#  writeCamlConstant("int", "DOT" number, value, help)
}

function apiConstant(name, symbol, value, help) {
#  writeCamlConstant("int", name, value);
}

function apiMask(name, symbol, value, help) {
#  writeCamlConstant("long", "KEY_" name, hexadecimalValue(value) "L");
}

function apiShift(name, symbol, value, help) {
#  writeCamlConstant("int", "KEY_" name, hexadecimalValue(value));
}

function apiType(name, symbol, value, help) {
#  writeCamlConstant("int", "KEY_TYPE_" name, hexadecimalValue(value));
}

function apiKey(name, symbol, value, help) {
#  value = hexadecimalValue(value)
#
#  if (name == "FUNCTION") {
#    for (i=0; i<35; ++i) {
#      writeKeyDefinition("F" (i+1), "(" value " + " i ")")
#    }
#  } else {
#    writeKeyDefinition(name, value)
#  }
}

function writeCommandDefinition(name, value, help) {
  print "  | CMD_" name " (** " help " *)"
}

function writeKeyDefinition(name, value) {
#  writeCamlConstant("int", "KEY_SYM_" name, value)
}

function writeCamlConstant(type, name, value, help) {
#  writeCamldocComment(help)
#  print "  public static final " type " " name " = " value ";"
}

function writeCamldocComment(text) {
#  if (length(text) > 0) print " (** " text " *)"
}

function hexadecimalValue(value) {
  value = tolower(value)
  gsub("x0+", "x", value)
  gsub("x$", "x0", value)
  return value
}

