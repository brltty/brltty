--[[
  libbrlapi - A library providing access to braille terminals for applications.
 
  Copyright (C) 2006-2023 by Dave Mielke <dave@mielke.cc>
 
  libbrlapi comes with ABSOLUTELY NO WARRANTY.
 
  This is free software, placed under the terms of the
  GNU Lesser General Public License, as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any
  later version. Please see the file LICENSE-LGPL for details.
 
  Web Page: http://brltty.app/
 
  This software is maintained by Dave Mielke <dave@mielke.cc>.
]]

require("brltty-prologue")

function showLibraryDirectory ()
  for number, component in ipairs(splitString(package.cpath, ";")) do
    local directory, name = component:match("^(.*)/(.-)$")
    if name ~= "?.so" then goto next end

    if not directory then goto next end
    if #directory == 0 then goto next end

    if directory:sub(1,1) ~= "/" then goto next end
    if stringContains(directory, "?") then goto next end

    print(directory)
    break

  ::next::
  end
end

action = nextProgramArgument("action")

if action == "libdir" then
  showLibraryDirectory()
else
  syntaxError(string.format("unknown action: %s", action))
end

os.exit()
