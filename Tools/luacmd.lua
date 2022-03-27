--[[
  libbrlapi - A library providing access to braille terminals for applications.
 
  Copyright (C) 2006-2022 by Dave Mielke <dave@mielke.cc>
 
  libbrlapi comes with ABSOLUTELY NO WARRANTY.
 
  This is free software, placed under the terms of the
  GNU Lesser General Public License, as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any
  later version. Please see the file LICENSE-LGPL for details.
 
  Web Page: http://brltty.app/
 
  This software is maintained by Dave Mielke <dave@mielke.cc>.
]]

programName = string.match(arg[0], "([^/]*)$")
argumentCount = #arg
argumentIndex = 1

function writeProgramMessage (message)
  io.stderr:write(string.format("%s: %s\n", programName, message))
end

function syntaxError (message)
  writeProgramMessage(message)
  os.exit(2)
end

function noMoreArguments () 
  return argumentIndex > argumentCount
end

function nextArgument (label)
  if noMoreArguments()
  then
    writeProgramMessage(string.format("missing %s", label))
  end

  local argument = arg[argumentIndex]
  argumentIndex = argumentIndex + 1
  return argument
end

action = nextArgument("action")

if action == "pkgdir"
then
  os.exit()
end

syntaxError(string.format("unknown action: %s", action))
