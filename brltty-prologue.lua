--[[
  libbrlapi - A library providing access to braille terminals for applications.
 
  Copyright (C) 2006-2026 by Dave Mielke <dave@mielke.cc>
 
  libbrlapi comes with ABSOLUTELY NO WARRANTY.
 
  This is free software, placed under the terms of the
  GNU Lesser General Public License, as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any
  later version. Please see the file LICENSE-LGPL for details.
 
  Web Page: http://brltty.app/
 
  This software is maintained by Dave Mielke <dave@mielke.cc>.
]]

if arg
then
  script = {}

  script.path = arg[0]
  script.directory = script.path:match("^(.*/)") or "."
  script.name = script.path:match("([^/]*)$")
  script.argumentList = arg
  script.argumentCount = #arg
  script.argumentNumber = 1

  function script.writeMessage (message)
    io.stderr:write(string.format("%s: %s\n", script.name, message))
  end

  function script.syntaxError (message)
    script.writeMessage(message)
    os.exit(2)
  end

  function script.haveMoreArguments () 
    return script.argumentNumber <= script.argumentCount
  end

  function script.nextArgument (label)
    if not script.haveMoreArguments()
    then
      script.syntaxError(string.format("missing %s", label))
    end

    local argument = script.argumentList[script.argumentNumber]
    script.argumentNumber = script.argumentNumber + 1
    return argument
  end
end

function getSortedObjectKeys (tbl)
  local keys = {}

  for key in pairs(tbl)
  do
    table.insert(keys, key)
  end

  table.sort(keys)
  return keys
end

function listObjectKeys (tbl)
  print("begin " .. type(tbl) .. " listing")

  if tbl
  then
    for _, key in ipairs(getSortedObjectKeys(tbl))
    do
      local value = tbl[key]
      print(type(value) .. " " .. key)
    end
  end

  print("end " .. type(tbl) .. " listing")
end

function stringContains (string, substring)
  return not not string:find(substring, 1, true)
end

function splitString (string, delimiter)
  local components = {}
  local count = 0
  local oldPosition = 1

  while true
  do
    local newPosition = string:find(delimiter, oldPosition, true)

    if not newPosition
    then
      break
    end

    count = count + 1
    components[count] = string:sub(oldPosition, newPosition-1)
    oldPosition = newPosition + #delimiter
  end

  count = count + 1
  components[count] = string:sub(oldPosition)

  return components
end

