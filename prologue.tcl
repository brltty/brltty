###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2013 by The BRLTTY Developers.
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

set sourceDirectory [file normalize [file dirname [info script]]]

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

proc makeEnumeration {arrayVariable elements} {
   upvar 1 $arrayVariable array
   set value 0

   foreach element $elements {
      set array($element) $value
      incr value
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

makeEnumeration logLevels {
   debug
   information
   notice
   warning
   error
   alert
   critical
   emergency
}
set logLevel $logLevels(notice)

proc logMessage {level message} {
   global logLevels logLevel

   if {$logLevels($level) >= $logLevel} {
      writeProgramMessage $message
   }
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
   upvar 1 $valuesArray values
   upvar 1 $argumentsVariable arguments

   set prefix -
   set options [dict create]
   set index 0

   foreach definition $definitions {
      set description "option\[$index\]"

      if {![nextOperand definition name]} {
         return -code error "name not specified for $description"
      }

      if {[dict exists $options $name]} {
         return -code error "duplicate name for $description: $name"
      }

      if {![nextOperand definition type]} {
         return -code error "type not specified for $description"
      }

      if {[lsearch -exact {counter flag toggle} $type] >= 0} {
         set values($name) 0
      } elseif {[lsearch -exact {untyped} $type] < 0} {
         if {[catch [list string is $type ""]] != 0} {
            return -code error "invalid type for $description: $type"
         }
      }

      dict set options $name type $type
      incr index
   }

   while {[llength $arguments] > 0} {
      if {[string length [set argument [lindex $arguments 0]]] == 0} {
         break
      }

      if {![string equal [string index $argument 0] $prefix]} {
         break
      }

      if {[string length [set name [string range $argument 1 end]]] == 0} {
         break
      }

      set arguments [lreplace $arguments 0 0]

      if {[string equal $name $prefix]} {
         break
      }

      if {[set count [dict size [set subset [dict filter $options key $name*]]]] == 0} {
         writeProgramMessage "unknown option: $prefix$name"
         return 0
      }

      if {$count > 1} {
         writeProgramMessage "ambiguous option: $prefix$name"
         return 0
      }

      set name [lindex [dict keys $subset] 0]
      set option [dict get $subset $name]

      switch -exact [set type [dict get $option type]] {
         counter {
            set value [expr {$values($name) + 1}]
         }

         flag {
            set value 1
         }

         toggle {
            set value [expr {!$values($name)}]
         }

         default {
            if {[llength $arguments] == 0} {
               writeProgramMessage "missing operand: $prefix$name"
               return 0
            }

            set value [lindex $arguments 0]
            set arguments [lreplace $arguments 0 0]

            if {![string equal $type untyped]} {
               if {[catch [list string is $type -strict $value] result] != 0} {
                  return -code error "unimplemented option type: $type"
               }

               if {!$result} {
                  writeProgramMessage "operand not $type: $prefix$name $value"
                  return 0
               }
            }
         }
      }

      set values($name) $value
   }

   return 1
}
