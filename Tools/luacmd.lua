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

require("brltty-prologue")

local actionHandlers = {
  version = function ()
    print(_VERSION)
  end,

  global = function ()
    listTable(_G)
  end,

  os = function ()
    listTable(os)
  end,

  io = function ()
    listTable(io)
  end,

  package = function ()
    listTable(package)
  end,

  coroutine = function ()
    listTable(coroutine)
  end,

  debug = function ()
    listTable(debug)
  end,

  string = function ()
    listTable(string)
  end,

  table = function ()
    listTable(table)
  end,

  math = function ()
    listTable(math)
  end,

  utf8 = function ()
    listTable(utf8)
  end,

  libdir = function ()
    local localDirectory = nil

    for _, component in ipairs(splitString(package.cpath, ";"))
    do
      local directory, name = component:match("^(.*)/(.-)$")

      if name ~= "?.so"
      then
        goto nextComponent
      end

      if not directory
      then
        goto nextComponent
      end

      if #directory == 0
      then
        goto nextComponent
      end

      if directory:sub(1,1) ~= "/"
      then
        goto nextComponent
      end

      if stringContains(directory, "?")
      then
        goto nextComponent
      end

      if not stringContains(directory, "/local/")
      then
        print(directory)
        return
      end

      if not localDirectory
      then
        localDirectory = directory
      end

    ::nextComponent::
    end

    if localDirectory
    then
      print(localDirectory)
    end
  end
}

actionHandlers.help = function ()
  for _, key in ipairs(getSortedTableKeys(actionHandlers))
  do
    print(key)
  end
end

do
  local action = nextProgramArgument("action")
  local handler = actionHandlers[action]

  if handler
  then
    handler()
  else
    syntaxError(string.format("unknown action: %s", action))
  end
end

os.exit()
