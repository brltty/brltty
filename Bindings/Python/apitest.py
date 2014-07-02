#!/usr/bin/env python
###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2014 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

import sys
import sysconfig
import os
import ctypes

def getProgramName ():
  return os.path.basename(sys.argv[0])

def writeError (message):
  file = sys.stderr
  file.write((getProgramName() + ": " + message + "\n"))
  file.flush()

def getBuildDirectory (directory):
  name = "{prefix}.{platform}-{version[0]}.{version[1]}".format(
    prefix = directory,
    platform = sysconfig.get_platform(),
    version = sys.version_info
  )

  return os.path.join(os.getcwd(), "build", name)

def getLibraryDirectory ():
  return os.path.join(os.getcwd(), "..", "..", "Programs")

def loadLibrary (directory, name):
  platform = sysconfig.get_platform()

  if platform == "win32":
    pattern = name + "-*.dll"

    import fnmatch
    names = [name
      for name in os.listdir(directory)
      if fnmatch.fnmatch(name, pattern)
    ]

    return ctypes.WinDLL(os.path.join(directory, names[0]))

  return ctypes.CDLL(os.path.join(directory, ("lib" + name + ".so")))

sys.path.insert(0, getBuildDirectory("lib"))
brlapiHandle = loadLibrary(getLibraryDirectory(), "brlapi")

if __name__ == "__main__":
  import brlapi
  import errno
  import Xlib.keysymdef.miscellany

  try:
    brl = brlapi.Connection()

    try:
      brl.enterTtyMode()
      brl.ignoreKeys(brlapi.rangeType_all, [0])

      # Accept the home, window up, and window down braille commands
      brl.acceptKeys(brlapi.rangeType_command, [
        brlapi.KEY_TYPE_CMD | brlapi.KEY_CMD_HOME,
        brlapi.KEY_TYPE_CMD | brlapi.KEY_CMD_WINUP,
        brlapi.KEY_TYPE_CMD | brlapi.KEY_CMD_WINDN
      ])

      # Accept the tab key
      brl.acceptKeys(brlapi.rangeType_key, [
        brlapi.KEY_TYPE_SYM | Xlib.keysymdef.miscellany.XK_Tab
      ])

      brl.writeText("Press home, winup/dn or tab to continue ...")
      key = brl.readKey()

      k = brl.expandKeyCode(key)
      brl.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
      brl.writeText(None, 1)
      brl.readKey()

      underline = chr(brlapi.DOT7 + brlapi.DOT8)
      # Note: center() can take two arguments only starting from python 2.4
      brl.write(
        regionBegin = 1,
        regionSize = 40,
        text = "Press any key to exit                   ",
        orMask = "".center(21,underline) + "".center(19,chr(0))
      )

      brl.acceptKeys(brlapi.rangeType_all, [0])
      brl.readKey()

      brl.leaveTtyMode()
    finally:
      del brl
  except brlapi.ConnectionError as e:
    if e.brlerrno == brlapi.ERROR_CONNREFUSED:
      writeError("Connection to %s refused. BRLTTY is too busy..." % e.host)
    elif e.brlerrno == brlapi.ERROR_AUTHENTICATION:
      writeError("Authentication with %s failed. Please check the permissions of %s" % (e.host, e.auth))
    elif e.brlerrno == brlapi.ERROR_LIBCERR and (e.libcerrno == errno.ECONNREFUSED or e.libcerrno == errno.ENOENT):
      writeError("Connection to %s failed. Is BRLTTY really running?" % (e.host))
    else:
      writeError("Connection to BRLTTY at %s failed: " % (e.host))
    print(e)
    print(e.brlerrno)
    print(e.libcerrno)
