#!/usr/bin/env tclsh
###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2018 by Dave Mielke <dave@mielke.cc>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

source [file join [file dirname $argv0] prologue.tcl]

processProgramOptions optionValues {
   {auth string "which authorization schemes to use (scheme,...)"}
   {host string "where the BrlAPI server is ([{name | address}][:port])"}
   {tty string "which virtual terminal to claim ({default | number})"}
}

load libbrlapi_tcl.so

proc expandList {list args} {
   set result ""
   set delimiter ""

   foreach field $args value $list {
      append result "$delimiter$field=$value"
      set delimiter " "
   }

   return $result
}

puts "Version: [brlapi getVersionString]"

set connectionSettings [list]
if {[info exists optionValues(host)]} {
   lappend connectionSettings -host $optionValues(host)
}
if {[info exists optionValues(auth)]} {
   lappend connectionSettings -auth $optionValues(auth)
}

if {[catch [list eval brlapi openConnection $connectionSettings] session] == 0} {
   puts "Object: $session"

   set host [$session getHost]
   puts "Host: $host"

   set auth [$session getAuth]
   puts "Auth: $auth"

   set fileDescriptor [$session getFileDescriptor]
   puts "FileDescriptor: $fileDescriptor"

   set driverName [$session getDriverName]
   puts "DriverName: $driverName"

   set modelIdentifier [$session getModelIdentifier]
   puts "ModelIdentifier: $modelIdentifier"

   set displaySize [$session getDisplaySize]
   puts "DisplaySize: [expandList $displaySize width height]"

   if {[info exists optionValues(tty)]} {
      if {[catch [list $session enterTtyMode -tty $optionValues(tty)] tty] == 0} {
         puts "Tty: $tty"

         package require Tclx
         set channel [dup $fileDescriptor]

         proc setTimeout {} {
            global timeoutEvent

            set timeoutEvent [after 10000 [list set returnCode 0]]
         }

         proc resetTimeout {} {
            global timeoutEvent

            after cancel $timeoutEvent
            unset timeoutEvent

            setTimeout
         }

         proc handleKey {session} {
            set text "Key:"

            set code [$session readKey 0]
            brlapi describeKeyCode $code properties
            set properties(code) [format "0X%X" $code]

            foreach property {flags} {
               set properties($property) [join $properties($property) ","]
            }

            foreach {property name} {code code type type command cmd argument arg flags flg} {
               append text " $name=$properties($property)"
               unset properties($property)
            }

            foreach property [lsort [array names properties]] {
               append text " $property=$properties($property)"
            }

            puts $text
            $session write -text $text
            resetTimeout
         }

         $session write -text "The TCL bindings for BrlAPI seem to be working."
         fileevent $channel readable [list handleKey $session]
         setTimeout
         vwait returnCode

         $session leaveTtyMode
      } else {
         puts stderr "invalid tty: $tty"
      }
   }

   $session closeConnection; unset session
} else {
   puts stderr "connection failure: $session"
}

exit 0
