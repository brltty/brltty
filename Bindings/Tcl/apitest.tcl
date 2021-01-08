#!/usr/bin/env tclsh
###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2021 by Dave Mielke <dave@mielke.cc>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

source [file join [file dirname $argv0] prologue.tcl]

proc putProperties {label properties} {
   puts stdout "$label: $properties"
}

proc formatProperty {name value} {
   return "$name=$value"
}

proc formatValues {values args} {
   set result ""
   set delimiter ""

   foreach name $args value $values {
      append result "$delimiter[formatProperty $name $value]"
      set delimiter " "
   }

   return $result
}

proc ttyShowKeyCode {session code} {
   set properties [list]

   brlapi describeKeyCode $code description
   set description(code) [format "0X%X" $code]

   foreach property {flags} {
      set description($property) [join $description($property) ","]
   }

   foreach {property name} {
      code     code
      type     type
      command  cmd
      argument arg
      flags    flg
   } {
      lappend properties [formatProperty $name $description($property)]
      unset description($property)
   }

   foreach property [lsort [array names description]] {
      lappend properties [formatProperty $property $description($property)]
   }

   set text [join $properties " "]
   $session write -text $text
   putProperties Key $text
}

proc ttyShowKeys {session timeout} {
   $session write -text "press keys (timeout is $timeout seconds)"

   while {[string length [set code [$session readKeyWithTimeout $timeout]]] > 0} {
      ttyShowKeyCode $session $code
   }
}

proc mainProgram {} {
   set optionDefinitions {
      {host untyped.server  "which server to connect to"}
      {auth untyped.schemes "which authorization schemes to use"}
      {tty  untyped.number  "which virtual terminal to claim"}
   }

   processProgramArguments optionValues $optionDefinitions

   putProperties "BrlAPI Version" [brlapi getVersionString]

   set connectionSettings [list]
   if {[info exists optionValues(host)]} {
      lappend connectionSettings -host $optionValues(host)
   }
   if {[info exists optionValues(auth)]} {
      lappend connectionSettings -auth $optionValues(auth)
   }

   if {[catch [list eval brlapi openConnection $connectionSettings] session] == 0} {
      putProperties "Session Command" $session
      putProperties "Server Host" [$session getHost]
      putProperties "Authorization Schemes" [$session getAuth]
      putProperties "File Descriptor" [$session getFileDescriptor]
      putProperties "Driver Name" [$session getDriverName]
      putProperties "Model Identifier" [$session getModelIdentifier]
      putProperties "Display Size" [formatValues [$session getDisplaySize] width height]

      if {[info exists optionValues(tty)]} {
         if {[catch [list $session enterTtyMode -tty $optionValues(tty)] tty] == 0} {
            putProperties "TTY Number" $tty
            ttyShowKeys $session 10
            $session leaveTtyMode
         } else {
            writeProgramMessage "invalid tty: $tty"
         }
         unset tty
      }

      $session closeConnection
   } else {
      writeProgramMessage "connection failure: $session"
   }
   unset session
}

mainProgram
exit 0
