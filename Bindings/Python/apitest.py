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
import time
import ctypes

def getProgramName ():
  return os.path.basename(sys.argv[0])

def logMessage (message, file=sys.stderr, label=getProgramName()):
  file.write((label + ": " + message + "\n"))
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
library = loadLibrary(getLibraryDirectory(), "brlapi")
import brlapi

def readKey (connection, timeout=0, interval=0.1, target=None, args=(), kwargs={}):
  start = time.time()

  while True:
    key = connection.readKey(wait=0)

    if key != None:
      return key

    if (time.time() - start) >= timeout:
      break

    time.sleep(interval)

  if target != None:
    return target(*args, **kwargs)

  return None

def handleTimeout ():
  logMessage("done")
  exit(0)

if __name__ == "__main__":
  import errno

  def writeProperty (name, value):
    sys.stdout.write(name + ": " + value + "\n")

  try:
    brl = brlapi.Connection()

    try:
      writeProperty("Driver", brl.driverName)
      writeProperty("Columns", str(brl.displaySize[0]))
      writeProperty("Rows", str(brl.displaySize[1]))

      brl.enterTtyMode()
      brl.writeText("The Python bindings for BrlAPI seem to be working.")

      while True:
        key = readKey(
          connection = brl,
          timeout = 10,
          target = handleTimeout
        )

        properties = brl.expandKeyCode(key)
        line = "Key:%ld (Typ:%x Cmd:%x Arg:%x Flg:%x)" % (
          key,
          properties["type"],
          properties["command"],
          properties["argument"],
          properties["flags"]
        )

        brl.writeText(line)
        sys.stdout.write(line + "\n")

      brl.leaveTtyMode()
    finally:
      del brl
  except brlapi.ConnectionError as e:
    if e.brlerrno == brlapi.ERROR_CONNREFUSED:
      logMessage("Connection to %s refused. BRLTTY is too busy..." % e.host)
    elif e.brlerrno == brlapi.ERROR_AUTHENTICATION:
      logMessage("Authentication with %s failed. Please check the permissions of %s" % (e.host, e.auth))
    elif e.brlerrno == brlapi.ERROR_LIBCERR and (e.libcerrno == errno.ECONNREFUSED or e.libcerrno == errno.ENOENT):
      logMessage("Connection to %s failed. Is BRLTTY really running?" % (e.host))
    else:
      logMessage("Connection to BRLTTY at %s failed: " % (e.host))
    print(e)
    print(e.brlerrno)
    print(e.libcerrno)
