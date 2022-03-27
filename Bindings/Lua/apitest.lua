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

require("../../brltty-prologue")
brlapi = require("brlapi")

getDriverName, getModelIdentifier, getDisplaySize, closeConnection =
  brlapi.getDriverName,
  brlapi.getModelIdentifier,
  brlapi.getDisplaySize,
  brlapi.closeConnection

function showProperty (name, value)
  io.stdout:write(string.format("%s: %s\n", name, tostring(value)))
end

connected, result = pcall(
  function ()
    return brlapi.openConnection()
  end
)

if connected
then
  brl = result
  showProperty("Driver", brl:getDriverName())
  showProperty("Model", brl:getModelIdentifier())

  do
    local columns, rows = brl:getDisplaySize()
    showProperty("Size", string.format("%dx%d", columns, rows))
  end

  brl:closeConnection()
else
  writeProgramMessage(result)
  os.exit(9)
end

os.exit()
