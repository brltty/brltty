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

local function addHighlightIndicator (element, start, finish)
  start = start or ""
  finish = finish or start

  return {
    pandoc.Str(start),
    element,
    pandoc.Str(finish)
  }
end

function Div (element)
  if element.identifier == "contents"
  then
    return {}
  end
end

function Link (element)
  local content = element.content

  if element.classes and not element.classes:includes("toc-backref")
  then
    content:insert(1, pandoc.Str("["))
    content:insert(pandoc.Str("]"))

    local target = element.target
    local isInternal = target:match("^#")

    if not isInternal
    then
      content:insert(pandoc.Str(" {" .. target .. "}"))
    end
  end

  return content
end

function Header (element)
  local start = string.rep("#", element.level)
  local finish = start

  if element.classes and element.classes:includes("title")
  then
    start = "[" .. start
    finish = finish .. "]"
  end

  local content = element.content
  content:insert(1, pandoc.Str(start .. " "))
  content:insert(pandoc.Str(" " .. finish))

  return pandoc.Plain(content)
end

function Strong (element)
  return addHighlightIndicator(element, "**")
end

function Emph (element)
  return addHighlightIndicator(element, "<", ">")
end

function Underline (element)
  return addHighlightIndicator(element, "__")
end

function Code (element)
  return addHighlightIndicator(element, "`")
end

function CodeBlock (element)
  return addHighlightIndicator(element, "<```", "```>")
end

function BlockQuote (element)
  return addHighlightIndicator(element, '<"""', '""">')
end

