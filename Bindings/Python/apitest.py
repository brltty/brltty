###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2018 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
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

if __name__ == "__main__":
  import errno

  def writeProperty (name, value):
    sys.stdout.write(name + ": " + value + "\n")

  writeProperty("BrlAPI Version", ".".join(map(str, brlapi.getLibraryVersion())))

  try:
    brl = brlapi.Connection()

    try:
      writeProperty("File Descriptor", str(brl.fileDescriptor))
      writeProperty("Server Host", brl.host)
      writeProperty("Authorization Schemes", brl.auth)
      writeProperty("Driver Name", brl.driverName)
      writeProperty("Model Identifier", brl.modelIdentifier)
      writeProperty("Display Width", str(brl.displaySize[0]))
      writeProperty("Display Height", str(brl.displaySize[1]))

      brl.enterTtyMode()
      timeout = 10
      brl.writeText("press keys (timeout is %d seconds)" % (timeout, ))

      while True:
        code = brl.readKeyWithTimeout(timeout * 1000)
        if not code: break

        properties = brlapi.describeKeyCode(code)
        properties["code"] = "0X%X" % code

        for name in ("flags", ):
          properties[name] = ",".join(properties[name])

        for property in (
          ("command" , "cmd"),
          ("argument", "arg"),
          ("flags"   , "flg"),
        ):
          (oldName, newName) = property
          properties[newName] = properties[oldName]
          del properties[oldName]

        names = ("code", "type", "cmd", "arg", "flg")
        values = map(lambda name: ("%s=%s" % (name, str(properties[name]))), names)

        text = " ".join(values)
        brl.writeText(text)
        writeProperty("Key", text);

      brl.leaveTtyMode()
      brl.closeConnection()
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
