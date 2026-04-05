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

require("brltty-prologue")

function Pandoc (document)
  local blocks = document.blocks
  local meta = document.meta

  return document
end

function Div (element)
  if element.identifier == "contents"
  then
    return {}
  end
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

local function addElementTitle (element, title)
  return {
    pandoc.Para{pandoc.Str("<-- " .. element.t .. " - " .. title .. " -->")},
    element
  }
end

function Table (element)
  local caption = pandoc.utils.stringify(element.caption.long)
  element.caption = {short={}, long={}}
  return addElementTitle(element, caption)
end

local function addElementIndicator (element, start, finish)
  start = start or ""
  finish = finish or start

  return {
    pandoc.Str(start),
    element,
    pandoc.Str(finish)
  }
end

function Strong (element)
  return addElementIndicator(element, "**")
end

function Emph (element)
  return addElementIndicator(element, "<", ">")
end

function Underline (element)
  return addElementIndicator(element, "__")
end

function Code (element)
  return addElementIndicator(element, "`")
end

local function addTextIndicator (element, start, finish)
  local text = element.text
  text = text:match("^%s*\n(.-)%s*$") or text
  text = start .. "\n" .. text
  text = text .. "\n" .. finish
  element.text = text
  return element
end

function CodeBlock (element)
  return addTextIndicator(element, "<```", "```>")
end

local function addContentIndicator (element, start, finish)
  local content = element.content
  content:insert(1, pandoc.Str(start))
  content:insert(pandoc.Str(finish))
  element.content = content
  return element
end

function BlockQuote (element)
  local start = '<"""'
  local finish = '""">'

  if #element.content == 1
  then
    local quote = element.content[1]

    if quote.t == "Plain"
    then
      local content = quote.content
      content:insert(1, pandoc.LineBreak())
      content:insert(1, pandoc.Str(start))
      content:insert(pandoc.LineBreak())
      content:insert(pandoc.Str(finish))
      return element
    end
  end

  return addContentIndicator(element, start, finish)
end

