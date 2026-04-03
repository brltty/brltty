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

local function showInterpreterVersion ()
  print(_VERSION)
end

local function listGlobalObjects ()
  listGlobalTable()
end

local function listOSObjects ()
  listTable(os)
end

local function listIOObjects ()
  listTable(io)
end

local function listPackageObjects ()
  listTable(package)
end

local function listStringObjects ()
  listTable(string)
end

local function listTableObjects ()
  listTable(table)
end

local function listMathObjects ()
  listTable(math)
end

local function listUTF8Objects ()
  listTable(utf8)
end

local function showLibraryDirectory ()
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

local actionHandlers = {
  version = showInterpreterVersion,
  global = listGlobalObjects,

  os = listOSObjects,
  io = listIOObjects,
  package = listPackageObjects,

  string = listStringObjects,
  table = listTableObjects,
  math = listMathObjects,
  utf8 = listUTF8Objects,

  libdir = showLibraryDirectory
}

local function listActionNames ()
  for _, key in ipairs(getSortedTableKeys(actionHandlers))
  do
    print(key)
  end
end

actionHandlers.actions = listActionNames
local actionName = nextProgramArgument("action")
local actionHandler = actionHandlers[actionName]

if actionHandler
then
  actionHandler()
else
  syntaxError(string.format("unknown action: %s", actionName))
end

os.exit()
