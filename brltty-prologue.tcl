###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2026 by The BRLTTY Developers.
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

set initialDirectory [pwd]
set scriptDirectory [file normalize [file dirname $::argv0]]
set prologueDirectory [file normalize [file dirname [info script]]]

try {
   package require Tclx
} trap {TCL PACKAGE UNFOUND} {} {
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

   proc lcontain {list element} {
      return [expr {[lsearch -exact $list $element] >= 0}]
   }

   proc lempty {list} {
      return [expr {[llength $list] == 0}]
   }

   proc lrmdups {list} {
      return [lsort -unique $list]
   }

   proc lvarcat {listVariable first args} {
      upvar 1 $listVariable list
      eval lappend list $first

      foreach arg $args {
         eval lappend list $arg
      }

      return $list
   }

   proc lvarpop {listVariable {index 0}} {
      upvar 1 $listVariable list
      set result [lindex $list $index]
      set list [lreplace $list $index $index]
      return $result
   }

   proc lvarpush {listVariable string {index 0}} {
      upvar 1 $listVariable list
      set list [linsert $list $index $string]
      return ""
   }

   proc loop {indexVariable from to command} {
      upvar 1 $indexVariable index
      set index $from

      while {$index < $to} {
         try {
            uplevel 1 $command
         } on return {result} {
            return -code return $result
         }

         incr index
      }
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

makeEnumeration logLevels {error warning notice task note detail}
set logLevel $logLevels(task)

proc logMessage {level message} {
   global logLevels logLevel

   if {$logLevels($level) <= $logLevel} {
      writeProgramMessage $message
   }
}

proc logError {{message ""}} {
   logMessage error $message
}

proc logWarning {{message ""}} {
   logMessage warning $message
}

proc logNotice {{message ""}} {
   logMessage notice $message
}

proc logTask {{message ""}} {
   logMessage task $message
}

proc logNote {{message ""}} {
   logMessage note $message
}

proc logDetail {{message ""}} {
   logMessage detail $message
}

proc syntaxError {{message ""}} {
   logError $message
   exit 2
}

proc semanticError {{message ""}} {
   logError $message
   exit 3
}

proc internalError {{message ""}} {
   logError $message
   exit 4
}

proc getScreenDimension {default variable attribute} {
   global env
   set pattern {^[1-9][0-9]*$}

   if {[info exists env($variable)]} {
      if {[regexp $pattern [set value $env($variable)]]} {
         return $value
      }
   }

   try {
      set value [exec << "" tput $attribute 2>@ stderr]

      if {[regexp $pattern $value]} {
         return $value
      }
   } trap {POSIX ENOENT} {} {
   }

   return $default
}

proc getScreenColumns {} {
   return [getScreenDimension 80 COLUMNS cols]
}

proc getScreenLines {} {
   return [getScreenDimension 25 LINES lines]
}

proc toRelativePath {to {from .}} {
   set variables {from to}

   foreach variable $variables {
      set $variable [file split [file normalize [set $variable]]]
   }

   set count [expr {min([llength $from], [llength $to])}]

   while {$count > 0} {
      if {![string equal [lindex $from 0] [lindex $to 0]]} {
         break
      }

      foreach variable $variables {
         lvarpop $variable
      }

      incr count -1
   }

   if {[llength [set to [concat [lrepeat [llength $from] ..] $to]]] == 0} {
      set to {.}
   }

   return [eval file join $to]
}

proc testContainingDirectory {directory names} {
   foreach name $names {
      if {![file exists [file join $directory $name]]} {
         return 0
      }
   }

   return 1
}

proc findContainingDirectory {variable directory names} {
   if {[info exists $variable]} {
      if {[string length [set $variable]] > 0} {
         return 1
      }
   }

   set directory [file normalize $directory]

   while {1} {
      if {[testContainingDirectory $directory $names]} {
         break
      }

      if {[string equal [set parent [file dirname $directory]] $directory]} {
         return 0
      }

      set directory $parent
   }

   set $variable $directory
   return 1
}

proc withChannel {variable channel body} {
   uplevel 1 [list set $variable $channel]

   try {
      try {
         uplevel 1 $body
      } on return {result} {
         return -level 2 $result
      }
   } finally {
      close $channel
      uplevel 1 [list unset $variable]
   }
}

proc readFile {file {binary 0}} {
   withChannel channel [open $file {RDONLY}] {
      if {$binary} {
         fconfigure $channel -translation binary
      }

      return [read $channel]
   }
}

proc compareFiles {file1 file2} {
   return [string equal [readFile $file1 1] [readFile $file2 1]]
}

proc refreshFile {newFile oldFile {executable ""}} {
   if {[string length $executable] == 0} {
      set preferredPermissions ""
   } elseif {$executable} {
      set preferredPermissions 0o755
   } else {
      set preferredPermissions 0o644
   }

   if {[file exists $oldFile]} {
      set actualPermissions [file attributes $oldFile -permissions]

      if {[string length $preferredPermissions] > 0} {
         if {$actualPermissions != $preferredPermissions} {
            logWarning "unexpected file permissions: $actualPermissions != $preferredPermissions: $oldFile"
         }
      }

      if {[compareFiles $newFile $oldFile]} {
         logDetail "file not changed: $oldFile"
         file delete $newFile
         return 0
      }

      set action "refreshing"
   } else {
      set actualPermissions $preferredPermissions
      set action "adding"
   }

   if {[string length $actualPermissions] > 0} {
      file attributes $newFile -permissions $actualPermissions
   }

   logNote "$action file: $oldFile"
   file rename -force $newFile $oldFile
   return 1
}

proc writeFile {file data} {
   withChannel channel [open $file {WRONLY CREAT TRUNC}] {
      puts -nonewline $channel $data
   }
}

proc replaceFile {file data} {
   set newFile "$file.new"
   writeFile $newFile $data
   return [refreshFile $newFile $file]
}

proc forEachLine {lineVariable file body} {
   upvar 1 $lineVariable line

   try {
      withChannel channel [open $file {RDONLY}] {
         while {[gets $channel line] >= 0} {
            uplevel 1 $body
         }
      }
   } on return {result} {
      return -level 2 $result
   }
}

proc readLines {file} {
   set lines [list]

   forEachLine line $file {
      lappend lines $line
   }

   return $lines
}

proc isWhitespace {string} {
   return [string is space $string]
}

proc wrapLine {line width} {
   set lines [list]

   while {[set length [string length [set line [string trimleft $line]]]] > 0} {
      if {$width < $length} {
         set index $width

         while {$index >= 0} {
            if {[isWhitespace [string index $line $index]]} {
               break
            }

            incr index -1
         }

         if {$index < 0} {
            set index $width

            while {[incr index] < $length} {
               if {[isWhitespace [string index $line $index]]} {
                  break
               }
            }
         }
      } else {
         set index $length
      }

      lappend lines [string trimright [string range $line 0 $index-1]]
      set line [string range $line $index end]
   }

   return $lines
}

proc formatLines {lines {width ""}} {
   if {[string length $width] == 0} {
      set width [getScreenColumns]
   }

   set result [list]
   set paragraph ""

   set finishParagraph {
      if {[string length $paragraph] > 0} {
         lvarcat result [wrapLine $paragraph $width]
         set paragraph ""
      }
   }

   foreach line $lines {
      if {[set length [string length [string trimright $line]]] > 0} {
         if {![string equal [string index $line 0] " "]} {
            if {[string length $paragraph] > 0} {
               append paragraph " "
            }

            append paragraph $line
            continue
         }
      }

      eval $finishParagraph
      lappend result $line
   }

   eval $finishParagraph
   return [join $result \n]
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

proc formatChoicesPhrase {choices} {
   set separator " "

   if {[set count [llength $choices]] > 2} {
      set separator ",$separator"
   }

   if {$count > 1} {
      lappend choices "or [lvarpop choices end]"
   }

   return [join $choices $separator]
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

proc addProgramOption {definitionsVariable name type usage {default ""}} {
   upvar 1 $definitionsVariable definitions

   if {[string length $default] > 0} {
      append usage " - defaults to $default"
   }

   lappend definitions [list $name $type $usage]
}

proc processProgramOptions {valuesArray argumentsVariable definitions {optionsVariable ""}} {
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

      if {[lcontain {counter flag toggle} $type]} {
         set values($name) 0
      } else {
         if {[set index [string first . $type]] < 0} {
            set operand $type
         } else {
            set operand [string range $type $index+1 end]
            set type [string range $type 0 $index-1]
         }

         if {![lcontain {string integer wideinteger double boolean} $type]} {
            return -code error "invalid type for $description: $type"
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
         logError "unknown option: $prefix$name"
         return 0
      }

      if {$count > 1} {
         logError "ambiguous option: $prefix$name ([join [lsort [dict keys $subset]] ", "])"
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
               logError "missing operand: $prefix$name"
               return 0
            }

            set value [lindex $arguments 0]
            set arguments [lreplace $arguments 0 0]

            if {![string equal $type string]} {
               try {
                  set result [string is $type -strict $value]
               } trap {TCL LOOKUP INDEX} {} {
                  return -code error "unimplemented option type: $type"
               }

               if {!$result} {
                  logError "operand not $type: $prefix$name $value"
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

proc showCommandUsage {name optionsDescriptor argumentsUsage getArgumentsUsageSummary} {
   set optionsUsage [formatCommandOptionsUsage $optionsDescriptor]
   set usage "Syntax: $name"

   if {[string length $optionsUsage] > 0} {
      append usage " \[-option ...\]"
   }

   if {[string length $argumentsUsage] > 0} {
      append usage " $argumentsUsage"
   }

   if {[string length $getArgumentsUsageSummary] > 0} {
      if {[string length [set lines [formatLines [$getArgumentsUsageSummary]]]] > 0} {
         append usage "\n\n"
         append usage $lines
         append usage "\n"
      }
   }

   if {[string length $optionsUsage] > 0} {
      append usage \n
      append usage "The following options may be specified:\n$optionsUsage"
   }

   puts stdout $usage
}

proc noMorePositionalArguments {arguments} {
   if {[nextElement arguments]} {
      syntaxError "too many positional arguments: [join $arguments " "]"
   }
}

proc processProgramArguments {
   optionValuesArray optionDefinitions
   {argumentValuesList ""}
   {argumentsUsage ""}
   {getArgumentsUsageSummary ""}
} {
   upvar 1 $optionValuesArray optionValues
   set argumentValues $::argv

   addProgramOption optionDefinitions help flag "show this usage summary and then exit"
   addProgramOption optionDefinitions quiet counter "decrease verbosity"
   addProgramOption optionDefinitions verbose counter "increase verbosity"

   if {![processProgramOptions optionValues argumentValues $optionDefinitions optionsDescriptor]} {
      syntaxError
   }

   global logLevel
   incr logLevel $optionValues(verbose)
   incr logLevel -$optionValues(quiet)

   if {$optionValues(help)} {
      showCommandUsage [getProgramName] $optionsDescriptor $argumentsUsage $getArgumentsUsageSummary
      exit 0
   }

   if {[string length $argumentValuesList] > 0} {
      uplevel 1 [list set $argumentValuesList $argumentValues]
   } else {
      noMorePositionalArguments $argumentValues
   }
}

proc addKeywordOption {definitionsVariable option operand usage keywords} {
   upvar 1 $definitionsVariable definitions

   if {![lempty $keywords]} {
      set choices [lmap choice $keywords {set choice "\"$choice\""}]
      append usage " - [formatChoicesPhrase $choices]"
   }

   lappend definitions [list $option string.$operand $usage]
}

proc testKeyword {valueVariable keywordList} {
   upvar 1 $valueVariable value

   if {![info exists value]} {
      set value [lindex $keywordList [set index 0]]
      return $index
   }

   set valueLength [string length $value]

   loop index 0 [llength $keywordList] {
      set keyword [lindex $keywordList $index]

      if {[string equal -nocase -length $valueLength $value $keyword]} {
         set value $keyword
         return $index
      }
   }

   return -1
}

proc verifyKeyword {valueVariable description keywordList} {
   upvar 1 $valueVariable value

   if {[testKeyword value $keywordList] < 0} {
      syntaxError "unrecognized $description: $value"
   }
}

proc addIntegerOption {definitionsVariable option operand usage {minimum ""} {maximum ""}} {
   upvar 1 $definitionsVariable definitions
   set bounds [list]

   if {[string length $minimum] > 0} {
      lappend bounds ">=$minimum"
   }

   if {[string length $maximum] > 0} {
      lappend bounds "<=$maximum"
   }

   if {![lempty $bounds]} {
      append usage " - must be [join $bounds " and "]"
   }

   lappend definitions [list $option integer.$operand $usage]
}

proc testInteger {value {minimum ""} {maximum ""}} {
   if {![string is integer -strict $value]} {
      return 0
   }

   if {([string length $value] > 1) && [string equal [string index $value 0] 0]} {
      return 0
   }

   if {[string length $minimum] > 0} {
      if {$value < $minimum} {
         return 0
      }
   }

   if {[string length $maximum] > 0} {
      if {$value > $maximum} {
         return 0
      }
   }

   return 1
}

proc verifyInteger {value description {minimum ""} {maximum ""}} {
   if {![testInteger $value]} {
      syntaxError "invalid $description: not an integer"
   }

   if {[string length $minimum] > 0} {
      if {$value < $minimum} {
         syntaxError "invalid $description: $value < $minimum"
      }
   }

   if {[string length $maximum] > 0} {
      if {$value > $maximum} {
         syntaxError "invalid $description: $value > $maximum"
      }
   }
}

proc verifyDirectory {path} {
   if {![file exists $path]} {
      semanticError "directory not found: $path"
   }

   if {![file isdirectory $path]} {
      semanticError "not a directory: $path"
   }
}

proc verifyOutputDirectory {path} {
   verifyDirectory $path

   if {![file writable $path]} {
      semanticError "directory not writable: $path"
   }
}

proc verifyInputFile {path} {
   if {![file exists $path]} {
      semanticError "file not found: $path"
   }

   if {![file isfile $path]} {
      semanticError "not a file: $path"
   }

   if {![file readable $path]} {
      semanticError "file not readable: $path"
   }
}

proc verifyOutputFile {path} {
   if {[file exists $path]} {
      if {![file isfile $path]} {
         semanticError "not a file: $path"
      }

      if {![file writable $path]} {
         semanticError "file not writable: $path"
      }
   } else {
      set directory [file dirname [file normalize $path]]
      verifyOutputDirectory $directory
   }
}

proc makeDictionary {initializer {namesPath {}}} {
   set result [dict create]

   set namesDelimiter " -> "
   set namesString [join $namesPath $namesDelimiter]

   if {[string length $namesString] == 0} {
      set namesString "top"
   }

   append namesString " [string trim $namesDelimiter]"
   lappend namesPath ""

   if {([llength $initializer] % 3) != 0} {
      return -code error "incorrectly formatted initializer: $namesString"
   }

   foreach {name type value} $initializer {
      set namesString [join [lset namesPath end $name] $namesDelimiter]

      if {[dict exists $result $name]} {
         return -code error "duplicate name: $namesString"
      }

      switch -exact -- $type {
         dict {
            dict set result $name [uplevel 1 [list makeDictionary $value $namesPath]]
         }

         string {
            dict set result $name $value
         }

         integer -
         wideinteger -
         double -
         boolean -
         list {
            if {![string is $type -strict $value]} {
               return -code error "invalid $type: $value: $namesString"
            }

            dict set result $name $value
         }

         expr -
         subst {
            dict set result $name [uplevel 1 [list $type $value]]
         }

         default {
            return -code error "unrecognized type: $type: $namesString"
         }
      }
   }

   return $result
}

