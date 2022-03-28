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

local brlapi = require("brlapi")
require("../../brltty-prologue")

local function showProperty (name, value)
  io.stdout:write(string.format("%s: %s\n", name, tostring(value)))
end

local function performTests ()
  showProperty("BrlAPI Version", string.format("%d.%d.%d", brlapi.getLibraryVersion()))

  local brl <close> = brlapi.openConnection(arg[2], arg[3])

  showProperty("File Descriptor", brl:getFileDescriptor())
  showProperty("Driver Name", brl:getDriverName())
  showProperty("Model Identifier", brl:getModelIdentifier())

  do
    local columns, rows = brl:getDisplaySize()
    showProperty("Display Size", string.format("%dx%d", columns, rows))
  end
end

do
  local ok, error = pcall(performTests)

  if not ok then
    writeProgramMessage(error)
    os.exit(9)
  end
end

os.exit()
