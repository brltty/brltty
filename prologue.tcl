###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2023 by The BRLTTY Developers.
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

source [file join [file dirname [info script]] "brltty-prologue.tcl"]
set sourceRoot [file normalize [file dirname [info script]]]

set documentsSubdirectory Documents
set programsSubdirectory Programs
set toolsSubdirectory Tools

set tablesSubdirectory Tables
set textTablesSubdirectory [file join $tablesSubdirectory Text]
set contractionTablesSubdirectory [file join $tablesSubdirectory Contraction]
set attributesTablesSubdirectory [file join $tablesSubdirectory Attributes]
set keyboardTablesSubdirectory [file join $tablesSubdirectory Keyboard]
set inputTablesSubdirectory [file join $tablesSubdirectory Input]

set driversSubdirectory Drivers
set brailleDriversSubdirectory [file join $driversSubdirectory Braille]
set speechDriversSubdirectory [file join $driversSubdirectory Speech]
set screenDriversSubdirectory [file join $driversSubdirectory Screen]

proc setSourceRoot {} {
   set variable ::env(BRLTTY_SOURCE_ROOT)

   if {![findContainingDirectory $variable [pwd] [list brltty.pc.in]]} {
      semanticError "source tree not found"
   }

   return [set ::sourceRoot [set $variable]]
}

proc setBuildRoot {} {
   global initialDirectory scriptDirectory
   set variable ::env(BRLTTY_BUILD_ROOT)

   foreach directory [list $initialDirectory $scriptDirectory] {
      if {[findContainingDirectory $variable $directory [list brltty.pc]]} {
         return [set ::buildRoot [set $variable]]
      }
   }

   semanticError "build tree not found"
}

proc getMakeFileProperty {file property {variable ""}} {
   set pattern {^\s*(\w+)\s*=\s*(.*?)\s*$}

   forEachLine line $file {
      if {[regexp $pattern $line x name value]} {
         if {[string equal $name $property]} {
            if {[string length $variable] == 0} {
               return $value
            }

            uplevel 1 [list set $variable $value]
            return 1
         }
      }
   }

   if {[string length $variable] == 0} {
      return -code error "property not found: $property"
   }

   return 0
}

proc getBrailleDriverProperty {driver property {variable ""}} {
   return [uplevel 1 [list getMakeFileProperty [file join $::sourceRoot $::brailleDriversSubdirectory $driver Makefile.in] $property $variable]]
}

proc getBrailleDriverCode {driver {variable ""}} {
   return [uplevel 1 [list getBrailleDriverProperty $driver DRIVER_CODE $variable]]
}

proc getBrailleDriverComment {driver {variable ""}} {
   return [uplevel 1 [list getBrailleDriverProperty $driver DRIVER_COMMENT $variable]]
}

