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
  import time
  import errno

  try:
    brl = brlapi.Connection()

    try:
      brl.enterTtyMode()
      brl.writeText("The Python bindings for BrlAPI seem to be working.")

      key = brl.readKey()
      properties = brl.expandKeyCode(key)
      brl.writeText("Key:%ld (Typ:%x Cmd:%x Arg:%x Flg:%x)" % (
        key,
        properties["type"],
        properties["command"],
        properties["argument"],
        properties["flags"]
      ))

      time.sleep(10)
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
