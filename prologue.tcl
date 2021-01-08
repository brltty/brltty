###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
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
   if {[string length $message] > 0} {
      set stream stderr
      puts $stream "[getProgramName]: $message"
      flush $stream
   }
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
   writeProgramMessage $message
   exit 2
}

proc semanticError {{message ""}} {
   writeProgramMessage $message
   exit 3
}

proc nextElement {listVariable {elementVariable ""}} {
   upvar 1 $listVariable list

   if {[llength $list] == 0} {
      return 0
   }

   if {[string length $elementVariable] > 0} {
      uplevel 1 [list set $elementVariable [lindex $list 0]]
      set list [lreplace $list 0 0]
   }

   return 1
}

proc formatColumns {rows} {
   set spacer [string repeat " " 2]
   set lines [list]

   foreach row $rows {
      set index 0

      foreach cell $row {
         if {![info exists widths($index)]} {
            set widths($index) 0
         }

         set widths($index) [max $widths($index) [string length $cell]]
         incr index
      }
   }

   foreach row $rows {
      set line ""
      set index 0

      foreach cell $row {
         if {[set width $widths($index)] > 0} {
            append line $cell
            append line [string repeat " " [expr {$width - [string length $cell]}]]
            append line $spacer
         }

         incr index
      }

      if {[string length [set line [string trimright $line]]] > 0} {
         if {[string length $lines] > 0} {
            append lines "\n"
         }

         append lines $line
      }
   }

   return $lines
}

proc processCommandOptions {valuesArray argumentsVariable definitions {optionsVariable ""}} {
   upvar 1 $valuesArray values
   upvar 1 $argumentsVariable arguments

   set prefix -
   set options [dict create]
   set optionIndex 0

   foreach definition $definitions {
      set description "option\[$optionIndex\]"

      if {![nextElement definition name]} {
         return -code error "name not specified for $description"
      }

      if {[dict exists $options $name]} {
         return -code error "duplicate name for $description: $name"
      }

      if {![nextElement definition type]} {
         return -code error "type not specified for $description"
      }

      if {[lsearch -exact {counter flag toggle} $type] >= 0} {
         set values($name) 0
      } else {
         if {[set index [string first . $type]] < 0} {
            set operand $type
         } else {
            set operand [string range $type $index+1 end]
            set type [string range $type 0 $index-1]
         }

         if {[lsearch -exact {untyped} $type] < 0} {
            if {[catch [list string is $type ""]] != 0} {
               return -code error "invalid type for $description: $type"
            }
         }
         dict set options $name operand $operand
      }
      dict set options $name type $type

      if {[nextElement definition usage]} {
         dict set options $name usage $usage
      }

      incr optionIndex
   }

   if {[string length $optionsVariable] > 0} {
      uplevel 1 [list set $optionsVariable $options]
   }

   while {[llength $arguments] > 0} {
      if {[string length [set argument [lindex $arguments 0]]] == 0} {
         break
      }

      if {![string equal [string index $argument 0] $prefix]} {
         break
      }

      set arguments [lreplace $arguments 0 0]

      if {[string equal $argument $prefix]} {
         break
      }

      if {[string equal [set name [string range $argument 1 end]] $prefix]} {
         break
      }

      if {[set count [dict size [set subset [dict filter $options key $name*]]]] == 0} {
         writeProgramMessage "unknown option: $prefix$name"
         return 0
      }

      if {$count > 1} {
         writeProgramMessage "ambiguous option: $prefix$name ([join [lsort [dict keys $subset]] ", "])"
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

proc formatCommandOptionsUsage {options} {
   set rows [list]

   foreach name [lsort [dict keys $options]] {
      set option [dict get $options $name]
      set row [list]
      lappend row "-$name"

      foreach property {operand usage} {
         if {[dict exists $option $property]} {
            lappend row [dict get $option $property]
         } else {
            lappend row ""
         }
      }

      lappend rows $row
   }

   return [formatColumns $rows]
}

proc showCommandUsage {name options positional} {
   set options [formatCommandOptionsUsage $options]
   set usage "Usage: $name"

   if {[string length $options] > 0} {
      append usage " \[-option ...\]"
   }

   if {[string length $positional] > 0} {
      append usage " $positional"
   }

   if {[string length $options] > 0} {
      append usage "\nThe following options may be specified:\n$options"
   }

   puts stdout $usage
}

proc noMorePositionalArguments {arguments} {
   if {[nextElement arguments]} {
      syntaxError "excess positional arguments: [join $arguments " "]"
   }
}

proc processProgramArguments {
   optionValuesArray optionDefinitions
   {positionalArgumentsVariable ""}
   {positionalArgumentsUsage ""}
} {
   upvar 1 $optionValuesArray optionValues
   set arguments $::argv

   lappend optionDefinitions {help flag "show this usage summary and then exit"}
   lappend optionDefinitions {quiet counter "decrease verbosity"}
   lappend optionDefinitions {verbose counter "increase verbosity"}

   if {![processCommandOptions optionValues arguments $optionDefinitions options]} {
      syntaxError
   }

   global logLevel
   incr logLevel $optionValues(quiet)
   incr logLevel -$optionValues(verbose)

   if {$optionValues(help)} {
      showCommandUsage [getProgramName] $options $positionalArgumentsUsage
      exit 0
   }

   if {[string length $positionalArgumentsVariable] > 0} {
      uplevel 1 [list set $positionalArgumentsVariable $arguments]
   } else {
      noMorePositionalArguments $arguments
   }
}

