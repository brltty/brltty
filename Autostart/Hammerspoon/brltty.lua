-- Hammerspoon configuration for BRLTTY on macOS.
--
-- Automatically hands off the braille display between BRLTTY (for terminal
-- use) and VoiceOver (for everything else) whenever focus moves between the
-- terminal emulator running brltty and any other application.
--
-- Installation (standalone): copy or symlink to ~/.hammerspoon/init.lua
-- Installation (module): place in ~/.hammerspoon/ and add require("brltty")
-- See Documents/README.macOS for prerequisites and full setup instructions.
--
-- Copyright (C) 1995-2026 by The BRLTTY Developers.
-- Web Page: http://brltty.app/
-- This software is maintained by Dave Mielke <dave@mielke.cc>.

pcall(require, "hs.ipc")

local HANDOFF = "/usr/local/bin/brltty-handoff"

-- Fallback allowlist: recognised before brltty has started and automatic
-- detection is possible.  Add bundle IDs of any terminal emulators you use
-- that are not listed here.
local TERMINAL_BUNDLES = {
  ["com.apple.Terminal"]     = true,  -- Terminal.app
  ["com.googlecode.iterm2"]  = true,  -- iTerm2
  ["io.alacritty"]           = true,  -- Alacritty
  ["net.kovidgoyal.kitty"]   = true,  -- kitty
  ["com.github.wez.wezterm"] = true,  -- WezTerm
}

-- Bundle ID of the terminal that started this brltty session (set at runtime).
local detectedTerminalBundle = nil

local BRLTTY_POLL_INTERVAL = 0.5  -- seconds between brltty-running checks
-- After toggling VoiceOver off, macOS fires an activated event for the
-- previously focused app.  Ignore away-from-terminal events for this many
-- seconds to avoid a spurious handoff reversal.
local HANDOFF_COOLDOWN = 3.0

local blowSound = hs.sound.getByFile("/System/Library/Sounds/Blow.aiff")

local log = hs.logger.new("brltty", "info")

local function dbg(msg)
  log:i(msg)
  local f = io.open("/tmp/hs-brltty.log", "a")
  if f then
    local ms = math.floor(hs.timer.absoluteTime() / 1e6)
    f:write(string.format("[%d ms] %s\n", ms, msg))
    f:close()
  end
end

-- Walk up brltty's ancestor chain to find the terminal emulator that started
-- it.  The process tree is: brltty → sudo → shell → terminal app.
-- Returns the bundle ID of the first ancestor that is a running GUI
-- application, or nil if not found (e.g. brltty was started by launchd).
local function detectTerminalApp()
  local out = hs.execute("pgrep -x brltty 2>/dev/null")
  if not out or out == "" then return nil end
  local pid = tonumber(out:match("%d+"))
  while pid and pid > 1 do
    local ppid_out = hs.execute("ps -o ppid= -p " .. pid .. " 2>/dev/null")
    pid = tonumber(ppid_out and ppid_out:match("%d+"))
    if pid then
      local app = hs.application.applicationForPID(pid)
      if app then return app:bundleID() end
    end
  end
  return nil
end

-- Returns true if bid belongs to the terminal running brltty, or to any
-- terminal in the fallback allowlist (used before brltty has started).
local function isTerminalApp(bid)
  if not bid then return false end
  if detectedTerminalBundle then return bid == detectedTerminalBundle end
  return TERMINAL_BUNDLES[bid] == true
end

local function voiceoverOn()
  if hs.application.get("VoiceOver") then return end
  dbg("voiceoverOn: launching VoiceOverStarter")
  hs.task.new("/System/Library/CoreServices/VoiceOver.app/Contents/MacOS/VoiceOverStarter", nil, {}):start()
end

local function voiceoverOff()
  if not hs.application.get("VoiceOver") then return end
  if blowSound then blowSound:play() end
  dbg("voiceoverOff: quitting VoiceOver via AppleScript")
  -- VoiceOver Utility keeps BrailleTranslationService alive even after VoiceOver
  -- itself quits, which prevents BRLTTY from reclaiming the USB device.  Quit it
  -- before quitting VoiceOver so that BrailleTranslationService exits promptly.
  if hs.application.get("VoiceOver Utility") then
    dbg("voiceoverOff: also quitting VoiceOver Utility")
    hs.task.new("/usr/bin/osascript", nil, {"-e", "tell application \"VoiceOver Utility\" to quit"}):start()
  end
  -- AppleScript quit triggers VoiceOver's normal shutdown path, which releases
  -- the USB braille device cleanly through IOKit.  kill() leaves the IOKit
  -- user-client seize unreleased, causing USBDeviceOpenSeize to fail with
  -- kIOReturnExclusiveAccess for 60+ seconds afterwards.
  hs.task.new("/usr/bin/osascript", nil, {"-e", "tell application \"VoiceOver\" to quit"}):start()
end

local function brlttyRunning()
  local out = hs.execute("pgrep -x brltty")
  return out ~= nil and out ~= ""
end

local brlttyPoller = nil
local handoffCooldown = false

local function brlttyResume()
  hs.task.new(HANDOFF, function(exitCode)
    dbg("resume exitCode=" .. tostring(exitCode))
  end, {"resume"}):start()
end

local function brlttySuspend(callback)
  -- SIGKILL: any exit (including SIGTERM) closes the BrlAPI fd, which briefly
  -- resumes the driver via the server.  SIGKILL avoids atexit/library cleanup
  -- that could trigger that path before the fresh suspend below takes effect.
  hs.execute("pkill -SIGKILL -x brltty-handoff 2>/dev/null; rm -f /tmp/brltty-handoff.pid")
  local _, ok = hs.execute(HANDOFF .. " suspend")
  dbg("suspend ok=" .. tostring(ok))
  callback()
end

local function brailleTranslationServiceRunning()
  local out = hs.execute("pgrep -x BrailleTranslationService")
  return out ~= nil and out ~= ""
end

-- Maximum seconds to wait for VoiceOver and BrailleTranslationService to
-- release the USB device.  If exceeded, VoiceOver is restored so the user
-- is not left without any braille output.
local VOICEOVER_EXIT_TIMEOUT = 10.0

local inTerminal = false

-- Poll until VoiceOver and BrailleTranslationService have both exited, then
-- signal brltty to reclaim the display.  Both processes must release the USB
-- device before brltty can open it.
local function waitForVoiceOverExitThenResume(elapsed)
  if not inTerminal then return end
  elapsed = elapsed or 0
  if hs.application.get("VoiceOver") or brailleTranslationServiceRunning() then
    if elapsed >= VOICEOVER_EXIT_TIMEOUT then
      dbg("VoiceOver exit timed out after " .. VOICEOVER_EXIT_TIMEOUT .. "s; restoring VoiceOver")
      voiceoverOn()
    else
      hs.timer.doAfter(0.1, function() waitForVoiceOverExitThenResume(elapsed + 0.1) end)
    end
  else
    dbg("VoiceOver and BrailleTranslationService gone, resuming brltty")
    brlttyResume()
  end
end

-- Poll until brltty starts, detect the terminal app, turn off VoiceOver,
-- then signal brltty to resume.
local function waitForBrlttyThenHandoff()
  if brlttyRunning() then
    brlttyPoller = nil
    if not detectedTerminalBundle then
      detectedTerminalBundle = detectTerminalApp()
      if detectedTerminalBundle then
        dbg("terminal app detected: " .. detectedTerminalBundle)
      else
        dbg("terminal app not detected (brltty may have been started by launchd)")
      end
    end
    voiceoverOff()
    waitForVoiceOverExitThenResume()
  else
    dbg("waiting for brltty")
    brlttyPoller = hs.timer.doAfter(BRLTTY_POLL_INTERVAL, waitForBrlttyThenHandoff)
  end
end

local function onAppSwitch(name, event, app)
  if event == hs.application.watcher.activated then
    local bid = app and app:bundleID() or "nil"
    if isTerminalApp(bid) then
      -- Re-trigger if VoiceOver is still on (state got out of sync).
      local voRunning = hs.application.get("VoiceOver") ~= nil
      if not inTerminal or voRunning then
        inTerminal = true
        handoffCooldown = true
        hs.timer.doAfter(HANDOFF_COOLDOWN, function() handoffCooldown = false end)
        dbg("switching TO terminal (" .. bid .. ") voRunning=" .. tostring(voRunning))
        if brlttyPoller then brlttyPoller:stop(); brlttyPoller = nil end
        waitForBrlttyThenHandoff()
      else
        dbg("TO terminal ignored: already inTerminal and VoiceOver off")
      end
    else
      -- Always update inTerminal state; cooldown only gates the suspend action.
      local wasInTerminal = inTerminal
      if inTerminal then
        inTerminal = false
        dbg("switching FROM terminal (cooldown=" .. tostring(handoffCooldown) .. ")")
        if brlttyPoller then brlttyPoller:stop(); brlttyPoller = nil end
      end
      if handoffCooldown then return end
      if wasInTerminal then
        if brlttyRunning() then
          brlttySuspend(voiceoverOn)
        else
          voiceoverOn()
        end
      end
    end
  end
end

appWatcher = hs.application.watcher.new(onAppSwitch)  -- global: locals are GC'd
appWatcher:start()
dbg("brltty handoff watcher started")
