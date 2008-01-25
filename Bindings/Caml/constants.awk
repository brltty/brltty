###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2008 by The BRLTTY Developers.
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
}

END {
}

function brlCommand(name, symbol, value, help) {
  writeCommandDefinition("cmd_" name, "Int32.of_int " value, help)
}

function brlBlock(name, symbol, value, help) {
  if (value ~ /^0[xX][0-9a-fA-F]+00$/) {
    writeCommandDefinition("blk_" name, "Int32.of_int " hexadecimalValue(value) "00", help)
  }
}

function brlKey(name, symbol, value, help) {
# not implemented for other bindings either
#  print "braille key:"
#  print name symbol value
#  print "(** " help " *)"
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
    return
  } else {
    return
  }
  writeCamlConstant("int32", "KEY_FLG_" name, "Int32.of_int " tolower(value), help)
}

function brlDot(number, symbol, value, help) {
  writeCamlConstant("int", "dot" number, value, help)
}

function apiConstant(name, symbol, value, help) {
  writeCamlConstant("int", name, value);
}

function apiMask(name, symbol, value, help) {
  writeCamlConstant("int64", "key_" tolower(name), hexadecimalValue(value) "L");
}

function apiShift(name, symbol, value, help) {
  writeCamlConstant("int", "key_" tolower(name), hexadecimalValue(value));
}

function apiType(name, symbol, value, help) {
  writeCamlConstant("int", "key_type_" tolower(name), hexadecimalValue(value));
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
  writeCamlConstant("int", "rangeType_" name, value, help)
}

function writeCommandDefinition(name, value, help) {
  writeCamlConstant("int32", name, value, help)
}

function writeKeyDefinition(name, value) {
  writeCamlConstant("int32", "KEY_SYM_" name, "Int32.of_int " value)
}

function writeCamlConstant(type, name, value, help) {
  print "let " tolower(name) " : " type " = " value " " camldocComment(help)
}

function camldocComment(text) {
  if (length(text) > 0) value = "(** " text " *)";
  else value = "";
  return value;
}

function hexadecimalValue(value) {
  value = tolower(value)
  gsub("x0+", "x", value)
  gsub("x$", "x0", value)
  return value
}
