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

local brlapi = require"brlapi"
local getDriverName, getModelIdentifier, getDisplaySize, closeConnection =
    brlapi.getDriverName,
    brlapi.getModelIdentifier,
    brlapi.getDisplaySize,
    brlapi.closeConnection

local brl = brlapi.openConnection()
print(brl:getDriverName(), brl:getModelIdentifier(), brl:getDisplaySize())
brl:closeConnection()
