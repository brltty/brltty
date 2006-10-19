BEGIN {
  writeHeaderPrologue("BRLAPI_INCLUDED_BRLDEFS", "api.h")
}

END {
  writeMacroDefinition("BRL_MSK_BLK", "(BRLAPI_KEY_TYPE_MASK | BRLAPI_KEY_CMD_BLK_MASK)", "mask for command type")
  writeMaskDefinition("ARG", "mask for command value/argument")
  writeMacroDefinition("BRL_MSK_FLG", "BRLAPI_KEY_FLAGS_MASK", "mask for command flags")
  writeMacroDefinition("BRL_MSK_CMD", "(BRL_MSK_BLK | BRL_MSK_ARG)", "mask for command")

  writeHeaderEpilogue()
}

function writeMaskDefinition(name, help) {
  writeSymbolDefinition(name, "BRL_MSK_", "", "BRLAPI_KEY_CMD_", "_MASK", help)
}

function writeSymbolDefinition(name, brlPrefix, brlSuffix, apiPrefix, apiSuffix, help) {
  writeMacroDefinition(brlPrefix name brlSuffix, apiPrefix name apiSuffix, help)
}

function brlCommand(name, value, help) {
  writeSymbolDefinition(name, "BRL_CMD_", "", "BRLAPI_KEY_CMD_", "", help)
}

function brlBlock(name, value, help) {
  if (name == "PASSCHAR") {
    writeMacroDefinition("BRL_KEY_" name, "(BRLAPI_KEY_TYPE_SYM | 0X0000)", help)
  } else if (name == "PASSKEY") {
    writeMacroDefinition("BRL_KEY_" name, "(BRLAPI_KEY_TYPE_SYM | 0XFF00)", help)
  } else {
    writeSymbolDefinition(name, "BRL_BLK_", "", "BRLAPI_KEY_CMD_", "", help)
  }
}

function brlKey(name, value, help) {
  writeMacroDefinition("BRL_KEY_" name, "(BRLAPI_KEY_SYM_" name " & 0XFF)", help)
}

function brlFlag(name, value, help) {
  if (match(name, "^[^_]*_")) {
    type = substr(name, 1, RLENGTH-1)
    if (type == "CHAR") {
      name = substr(name, RLENGTH+1)
      writeSymbolDefinition(name, "BRL_FLG_" type "_", "", "BRLAPI_KEY_FLG_", "", help)
      return
    }
  }
  writeSymbolDefinition(name, "BRL_FLG_", "", "BRLAPI_KEY_FLG_", "", help)
}

function brlDot(number, value, help) {
  writeSymbolDefinition(number, "BRL_DOT", "", "BRLAPI_DOT", "", help)
}
