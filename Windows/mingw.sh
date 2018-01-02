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
# Web Page: http://brltty.com/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

ahkName="AutoHotkey"
ahkLocation="/c/Program Files (x86)/AutoHotkey"
ahkDownload="http://www.autohotkey.com/"

nsisName="NSIS"
nsisLocation="/c/Program Files (x86)/NSIS"
nsisDownload="http://nsis.sourceforge.net/"

libusb0Name="LibUSB-Win32"
libusb0Version="1.2.6.0"
libusb0Location="/c/LibUSB-Win32"
libusb0Files="bin/libusb-win32-bin-README.txt"
libusb0Download="http://libusb-win32.sourceforge.net/"
libusb0Pattern="usb0"

libusb1Name="LibUSB-1.0"
libusb1Version="1.0.18"
libusb1Location="/c/LibUSB-1.0"
libusb1Files="libusb-1.0.def"
libusb1Download="http://www.libusb.org/wiki/windows_backend"
libusb1Pattern="usb-1.0"

winusbName="WinUSB"
winusbLocation="/c/WinUSB"
winusbFiles="libusb_device.inf"
winusbDownload="http://www.libusb.org/wiki/windows_backend"

msvcName="MSVC"
msvcLocation=""
msvcFiles="lib.exe link.exe"
msvcDownload=""

pythonName="Python"
pythonLocation=""
pythonFiles="Lib/site-packages Scripts"
pythonDownload="http://www.python.org/"

cythonName="Cython"
cythonLocation=""
cythonDownload="http://www.cython.org/"

icuName="ICU"
icuLocation="/c/ICU"
icuFiles="include/unicode"
icuDownload="http://icu-project.org/"

pthreadsName="mingw32-pthreads-w32"
pthreadsLocation="/"
pthreadsFiles="mingw/include/pthread.h"

cursesName="mingw32-pdcurses"
cursesLocation="/"
cursesFiles="mingw/include/curses.h"

zipPackage="msys-zip"
unix2dosPackage="msys-dos2unix"
groffPackage="msys-groff"

verifyPackage() {
   local package="${1}"
   shift 1

   local name="$(getVariable "${package}Name")"

   local root="$(getVariable "${package}Root")"
   [ -n "${root}" ] || root="$(getVariable "${package}Location")"

   if [ -z "${root}" ]
   then
      logMessage warning "package location not defined: ${name}"
      root=""
   elif [ ! -e "${root}" ]
   then
      logMessage warning "directory not found: ${root}"
      root=""
   elif [ ! -d "${root}" ]
   then
      logMessage warning "not a directory: ${root}"
      root=""
   else
      set -- $(getVariable "${package}Files")
      local prefix="${root%/}"
      local file

      for file
      do
         local path="${prefix}/${file}"

         [ -e "${path}" ] || {
            logMessage warning "${name} file not found: ${path}"
            root=""
         }
      done
   fi

   setVariable "${package}Root" "${root}"
   [ -n "${root}" ] || return 1
   return 0
}

addWindowsPackageOption() {
   local letter="${1}"
   local package="${2}"

   local name="$(getVariable "${package}Name")"
   local location="$(getVariable "${package}Location")"
   addProgramOption "${letter}" string.directory "${package}Root" "where the ${name} package has been installed" "${location}"
}

verifyWindowsPackage() {
   local package="${1}"

   verifyPackage "${package}" || {
      local message="Windows package not installed:"

      local name="$(getVariable "${package}Name")"
      [ -z "${name}" ] || message="${message} ${name}"

      local download="$(getVariable "${package}Download")"
      [ -z "${download}" ] || message="${message} (download from ${download})"

      logMessage warning "${message}"
      return 1
   }

   return 0
}

verifyWindowsPackages() {
   local result=0

   while [ "${#}" -gt 0 ]
   do
      local package="${1}"
      shift 1

      verifyWindowsPackage "${package}" || result=1
   done

   return "${result}"
}

verifyMingwPackage() {
   local package="${1}"

   verifyPackage "${package}" || {
      local message="MinGW package not installed:"

      local name="$(getVariable "${package}Name")"
      [ -z "${name}" ] || message="${message} ${name}"

      logMessage warning "${message}"
      return 1
   }

   return 0
}

verifyMingwPackages() {
   local result=0

   while [ "${#}" -gt 0 ]
   do
      local package="${1}"
      shift 1

      verifyMingwPackage "${package}" || result=1
   done

   return "${result}"
}

findHostCommand() {
   local command="${1}"

   local path="$(command -v "${command}" 2>/dev/null || :)"
   setVariable "${command}Path" "${path}"
   [ -n "${path}" ] || return 1
   return 0
}

verifyWindowsCommand() {
   local command="${1}"

   findHostCommand "${command}" || {
      local message="Windows command not found: ${command}"

      local download="$(getVariable "${command}Download")"
      [ -z "${download}" ] || {
         message="${message} (install"
         local name="$(getVariable "${command}Name")"
         [ -z "${name}" ] || message="${message} ${name}"
         message="${message} from ${download})"
      }

      logMessage warning "${message}"
      return 1
   }

   return 0
}

verifyWindowsCommands() {
   local result=0

   while [ "${#}" -gt 0 ]
   do
      local command="${1}"
      shift 1

      verifyWindowsCommand "${command}" || result=1
   done

   return "${result}"
}

verifyMingwCommand() {
   local command="${1}"

   findHostCommand "${command}" || {
      local message="MinGW command not found: ${command}"

      local package="$(getVariable "${command}Package")"
      [ -z "${package}" ] || message="${message} (install package ${package})"

      logMessage warning "${message}"
      return 1
   }

   return 0
}

verifyMingwCommands() {
   local result=0

   while [ "${#}" -gt 0 ]
   do
      local command="${1}"
      shift 1

      verifyMingwCommand "${command}" || result=1
   done

   return "${result}"
}

[ "${MSYSTEM}" = "MINGW32" ] || semanticError "this script is for MinGW only"
