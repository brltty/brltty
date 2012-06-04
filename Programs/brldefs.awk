###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2012 by The BRLTTY Developers.
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

BEGIN {
  brlCommandValue = 0
  brlKeyValue = 0

  brlBlockAlias["CLIP_NEW"] = "CUTBEGIN"
  brlBlockAlias["CLIP_ADD"] = "CUTAPPEND"
  brlBlockAlias["COPY_LINE"] = "CUTLINE"
  brlBlockAlias["COPY_RECT"] = "CUTRECT"
  brlBlockAlias["CLIP_COPY"] = "COPYCHARS"
  brlBlockAlias["CLIP_APPEND"] = "APNDCHARS"
}

/^ *BRL_CMD_/ {
  brlCommand(substr($1, 9), $1, brlCommandValue++, getComment($0))
  next
}

/#define[ \t]*BRL_BLK_/ {
  prefix = substr($2, 1, 8)
  name = substr($2, 9)
  value = getDefineValue()
  help = getComment($0)
  brlBlock(name, $2, value, help)

  if (name in brlBlockAlias) {
    alias = brlBlockAlias[name]
    brlBlock(alias, prefix alias, value, "alias for " name " - " help)
  }

  next
}

/^ *BRL_KEY_/ {
  gsub(",", "", $1)
  key = tolower(substr($1, 9))
  gsub("_", "-", key)
  brlKey(substr($1, 9), $1, brlKeyValue++, key " key")
  next
}

/#define[ \t]*BRL_FLG_/ {
  brlFlag(substr($2, 9), $2, getDefineValue(), getComment($0))
  next
}

/#define[ \t]*BRL_DOT/ {
  value = getDefineValue()
  sub("^.*\\(", "", value)
  sub("\\).*$", "", value)
  brlDot(substr($2, 8), $2, value, getComment($0))
  next
}

function writeHeaderPrologue(symbol, copyrightFile) {
  writeCopyright(copyrightFile)

  headerSymbol = symbol
  print "#ifndef " headerSymbol
  writeMacroDefinition(headerSymbol)
  print ""

  print "#ifdef __cplusplus"
  print "extern \"C\" {"
  print "#endif /* __cplusplus */"
  print ""
}

function writeCopyright(file) {
  if (length(file) > 0) {
    while (getline line <file == 1) {
      if (match(line, "^ */\\* *$")) {
        print line

        while (getline line <file == 1) {
          print line
          if (match(line, "^ *\\*/ *$")) break
        }

        print ""
        break
      }
    }
  }
}

function writeHeaderEpilogue() {
  print ""
  print "#ifdef __cplusplus"
  print "}"
  print "#endif /* __cplusplus */"

  print ""
  print "#endif /* " headerSymbol " */"
}

function writeMacroDefinition(name, definition, help) {
  statement = "#define " name
  if (length(definition) > 0) statement = statement " " definition

  if (length(help) > 0) print makeDoxygenComment(help)
  print statement
}

function getComment(line) {
  if (!match(line, "/\\*.*\\*/")) return ""
  line = substr(line, RSTART+2, RLENGTH-4)

  match(line, "^\\** *")
  line = substr(line, RLENGTH+1)

  match(line, " *$")
  return substr(line, 1, RSTART-1)
}

function getDefineValue() {
  if ($3 !~ "^\\(") return getNormalizedConstant($3)
  if (match($0, "\\([^)]*\\)")) return substr($0, RSTART, RLENGTH)
  return ""
}

function getNormalizedConstant(value) {
   if (value ~ "^UINT64_C\\(.+\\)$") value = substr(value, 10, length(value)-10)
   return value
}

function makeDoxygenComment(text) {
  return "/** " text " */"
}

function beginDoxygenFile() {
  print "/** \\file"
  print " */"
  print ""
}

function getBrlapiKeyName(name) {
  if (name == "ENTER") return "LINEFEED"
  if (name ~ /^CURSOR_/) return substr(name, 8)
  return name
}
