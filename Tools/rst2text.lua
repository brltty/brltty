--[[
  BRLTTY - A background process providing access to the console screen (when in
           text mode) for a blind person using a refreshable braille display.
 
  Copyright (C) 1995-2026 by The BRLTTY Developers.
 
  BRLTTY comes with ABSOLUTELY NO WARRANTY.
 
  This is free software, placed under the terms of the
  GNU Lesser General Public License, as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any
  later version. Please see the file LICENSE-LGPL for details.
 
  Web Page: http://brltty.app/
 
  This software is maintained by Dave Mielke <dave@mielke.cc>.
]]

function Div (element)
  if element.identifier == "contents"
  then
    return {}
  end
end

function Link (element)
  local content = element.content
  local target = element.target
  local isInternal = target:match("^#")

  if not isInternal
  then
    table.insert(content, 1, pandoc.Str("["))
    table.insert(content, pandoc.Str("]"))
    table.insert(content, pandoc.Str(" {" .. target .. "}"))
  end

  return content
end

function Header (element)
  local content = element.content

  local marker = string.rep("#", element.level)
  table.insert(content, 1, pandoc.Str(marker .. " "))
  table.insert(content, pandoc.Str(" " .. marker))

  return pandoc.Plain(content)
end

local function addHighlightIndicator (element, indicator)
  return {
    pandoc.Str(indicator),
    element,
    pandoc.Str(indicator)
  }
end

function Strong (element)
  return addHighlightIndicator(element, "**")
end

function Emph (element)
  return addHighlightIndicator(element, "_")
end

function Code (element)
  return addHighlightIndicator(element, "`")
end

function CodeBlock (element)
  return addHighlightIndicator(element, "```")
end

function BlockQuote (element)
  return addHighlightIndicator(element, '"""')
end

