###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2008 by Dave Mielke <dave@mielke.cc>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

proc getProgramPath {} {
   return [uplevel #0 set argv0]
}

proc getProgramName {} {
   return [file tail [getProgramPath]]
}

proc writeProgramMessage {message} {
   puts stderr "[getProgramName]: $message"
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

proc processOptions {argumentsList valuesArray options} {
   upvar 1 $argumentsList arguments
   upvar 1 $valuesArray values

   set checkValue(string) {1}
   set checkValue(integer) {[string is integer -strict $value]}
   set checkValue(double) {[string is double -strict $value]}
   set checkValue(boolean) {[string is boolean -strict $value]}

   foreach option $options {
      foreach variable {name type description} value $option {
         set $variable $value
      }

      if {[info exists types($name)]} {
         return -code error "option defined more than once: $name"
      }

      if {[set defaultSpecified [expr {[set delimiter [string first : $type]] >= 0}]]} {
         set value [string range $type [expr {$delimiter + 1}] end]
         set type [string range $type 0 [expr {$delimiter - 1}]]
      }

      if {[string length $type] == 0} {
         return -code error "option type not defined: $name"
      }

      if {[string equal $type flag]} {
         if {$defaultSpecified} {
            return -code error "default value specified for $type option: $name"
         }

         set value 0
         set defaultSpecified 1
      } elseif {![info exists checkValue($type)]} {
         return -code error "invalid option type: $type"
      } elseif {$defaultSpecified && ![expr $checkValue($type)]} {
         return -code error "invalid default value for $type option: $name=$value"
      }

      if {$defaultSpecified} {
         set values($name) $value
      }

      set types($name) $type
      set descriptions($name) $description

      set abbreviation $name
      while {[string length $abbreviation] > 0} {
         if {[info exists names($abbreviation)]} {
            set name ""
         }

         set names($abbreviation) $name
         set abbreviation [string range $abbreviation 0 end-1]
      }
   }

   while {[llength $arguments] > 0} {
      if {[string length [set argument [lindex $arguments 0]]] == 0} {
         breaK
      }

      if {[string equal [set option [string trimleft $argument -]] $argument]} {
         break
      }
      set arguments [lrange $arguments 1 end]

      if {[string length $option] == 0} {
         break
      }

      if {![info exists names($option)]} {
         return -code error "unknown option: $argument"
      }

      if {[string length [set name $names($option)]] == 0} {
         return -code error "ambiguous option: $argument"
      }

      if {[string equal [set type $types($name)] flag]} {
         set values($name) 1
      } elseif {[llength $arguments] == 0} {
         return -code error "value not supplied for option: $argument"
      } else {
         set value [lindex $arguments 0]
         set arguments [lrange $arguments 1 end]

         if {![expr $checkValue($type)]} {
            return -code error "invalid value for $type option: $argument $value"
         }

         set values($name) $value
      }
   }
}

proc processProgramOptions {valuesArray options} {
   global argv
   upvar 1 $valuesArray values

   if {[catch [list processOptions argv values $options] response] != 0} {
      syntaxError $response
   }
   return $response
}
