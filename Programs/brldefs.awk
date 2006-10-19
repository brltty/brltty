/^ *BRL_CMD_/ {
  brlCommand(substr($1, 9), $1, getComment($0))
  next
}

/#define[ \t]*BRL_BLK_/ {
  brlBlock(substr($2, 9), $2, getComment($0))
  next
}

/^ *BRL_KEY_/ {
  gsub(",", "", $1)
  key = tolower(substr($1, 9))
  gsub("_", "-", key)
  brlKey(substr($1, 9), "BRL_BLK_PASSKEY+" $1, "simulate press of " key " key")
  next
}

/#define[ \t]*BRL_FLG_/ {
  brlFlag(substr($2, 9), $2, getComment($0))
  next
}

/#define[ \t]*BRL_DOT/ {
  brlDot(substr($2, 8), $2, getComment($0))
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

function makeDoxygenComment(text) {
  return "/** " text " */"
}

function getComment(line) {
  if (!match(line, "/\\*.*\\*/")) return ""
  line = substr(line, RSTART+2, RLENGTH-4)

  match(line, "^\\** *")
  line = substr(line, RLENGTH+1)

  match(line, " *$")
  return substr(line, 1, RSTART-1)
}

