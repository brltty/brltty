###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2012 by The BRLTTY Developers.
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

set programDirectory [file normalize [file dirname [info script]]]

if {[catch [list package require Tclx] response] != 0} {
   proc lvarcat {list elements} {
      uplevel 1 [list lappend $list] $elements
   }

   proc intersect3 {list1 list2} {
      foreach number {1 2} {
         set length$number [llength [set list$number [lsort [set list$number]]]]
         set index$number 0
         set only$number [list]
      }
      set both [list]

      while {1} {
         if {$index1 == $length1} {
            if {$index2 < $length2} {
               lvarcat only2 [lrange $list2 $index2 end]
            }

            break
         }

         if {$index2 == $length2} {
            lvarcat only1 [lrange $list1 $index1 end]
            break
         }

         switch -exact -- [string compare [lindex $list1 $index1] [lindex $list2 $index2]] {
           -1 {
               lappend only1 [lindex $list1 $index1]
               incr index1
            }

            1 {
               lappend only2 [lindex $list2 $index2]
               incr index2
            }

            0 {
               lappend both [lindex $list1 $index1]
               incr index1
               incr index2
            }
         }
      }

      return [list $only1 $both $only2]
   }

   proc lrmdups {list} {
      return [lsort -unique $list]
   }

   proc lempty {list} {
      return [expr {[llength $list] == 0}]
   }

   if {[package vcompare $tcl_version 8.4] < 0} {
      proc readdir {directory} {
         set workingDirectory [pwd]
         cd $directory
         set names [glob -nocomplain *]
         cd $workingDirectory
         return $names
      }
   } else {
      proc readdir {directory} {
         return [glob -directory $directory -tails -nocomplain *]
      }
   }
}

proc getProgramName {} {
   return [file tail [info script]]
}

proc writeProgramMessage {message} {
   set stream stderr
   puts $stream "[getProgramName]: $message"
   flush $stream
}

proc syntaxError {{message ""}} {
   if {[string length $message] > 0} {
      writeProgramMessage $message
   }

   exit 2
}

proc semanticError {{message ""}} {
   if {[string length $message] > 0} {
      writeProgramMessage $message
   }

   exit 3
}

proc nextOperand {operandsVariable {operandVariable ""}} {
   upvar 1 $operandsVariable operands

   if {[llength $operands] == 0} {
      return 0
   }

   if {[string length $operandVariable] > 0} {
      uplevel 1 [list set $operandVariable [lindex $operands 0]]
      set operands [lreplace $operands 0 0]
   }

   return 1
}

proc processOptions {valuesArray argumentsVariable definitions} {
   package require cmdline

   upvar 1 $valuesArray values
   upvar 1 $argumentsVariable arguments

   set options [list]
   array set values [list]
   set index 0

   foreach definition $definitions {
      if {[set count [llength $definition]] < 1} {
         return -code error "name not specified for option\[$index\]"
      }

      if {[set delimiter [string first . [set option [lindex $definition 0]]]] < 0} {
         set name $option
         set isFlag($name) 1
         set values($name) 0
      } else {
         set name [string range $option 0 $delimiter-1]
         set isFlag($name) 0
      }

      if {![string is alpha -strict $name]} {
         return -code error "invalid name for option\[$index\]: $name"
      }

      lappend options $option
      incr index
   }

   while {[set result [::cmdline::typedGetopt arguments $options name value]] > 0} {
      if {$isFlag($name)} {
         set values($name) 1
      } else {
         set values($name) $value
      }
   }

   if {$result < 0} {
      writeProgramMessage $value
      return 0
   }

   return 1
}
