###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2006 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

package require cmdline

proc writeProgramMessage {message} {
   puts stderr "[::cmdline::getArgv0]: $message"
}

proc syntaxError {message} {
   writeProgramMessage $message
   exit 2
}

proc getArgument {resultVariable} {
   global argv
   if {[llength $argv] == 0} {
      return 0
   }
   uplevel 1 [list set $resultVariable [lindex $argv 0]]
   set argv [lrange $argv 1 end]
   return 1
}

proc nextArgument {description} {
   if {![getArgument result]} {
      syntaxError "missing $description."
   }
   return $result
}

proc noMoreArguments {} {
   if {[getArgument argument]} {
      syntaxError "excess parameter: $argument"
   }
}

proc processOptions {valuesArray options} {
   global argv
   upvar 1 $valuesArray values

   while {[set status [::cmdline::getopt argv $options option value]] > 0} {
      set values($option) $value
   }

   if {$status < 0} {
      syntaxError $value
   }

   foreach option $options {
      if {[set index [string first . $option]] < 0} {
         set default 0
      } else {
         set option [string range $option 0 [expr {$index - 1}]]
         set default ""
      }

      if {![info exists values($option)]} {
         set values($option) $default
      }
   }
}
