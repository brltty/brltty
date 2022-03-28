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
programArgumentCount = #arg
programArgumentNumber = 1

function writeProgramMessage (message)
  io.stderr:write(string.format("%s: %s\n", programName, message))
end

function syntaxError (message)
  writeProgramMessage(message)
  os.exit(2)
end

function haveMoreProgramArguments () 
  return programArgumentNumber <= programArgumentCount
end

function nextProgramArgument (label)
  if not haveMoreProgramArguments() then
    syntaxError(string.format("missing %s", label))
  end

  local argument = arg[programArgumentNumber]
  programArgumentNumber = programArgumentNumber + 1
  return argument
end

function stringContains (string, substring)
  return not not string:find(substring, 1, true)
end

function splitString (string, delimiter)
  local components = {}
  local count = 0
  local oldPosition = 1

  while true do
    local newPosition = string:find(delimiter, oldPosition, true)
    if not newPosition then break end

    count = count + 1
    components[count] = string:sub(oldPosition, newPosition-1)

    oldPosition = newPosition + #delimiter
  end

  count = count + 1
  components[count] = string:sub(oldPosition)

  return components
end

