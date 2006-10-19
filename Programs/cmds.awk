function writeCommandEntry(name, value, help) {
  print "{"
  print "  .name = \"" name "\","
  print "  .code = " value ","
  print "  .description = \"" help "\""
  print "},"
}

function brlCommand(name, value, help) {
  writeCommandEntry(name, value, help)
}

function brlBlock(name, value, help) {
  writeCommandEntry(name, value, help)
}

function brlKey(name, value, help) {
  writeCommandEntry(name, value, help)
}

function brlFlag(name, value, help) {
}

function brlDot(number, value, help) {
}
