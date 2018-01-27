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
   {host string "where the BrlAPI server is [name][:port]"}
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

set connectionSettings [list]
if {[info exists optionValues(host)]} {
   lappend connectionSettings -host $optionValues(host)
}
set session [eval brlapi openConnection $connectionSettings]
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

set tty [$session enterTtyMode]
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

proc handleCommand {session} {
   set code [$session readKey 0]
   set text "code:$code"
   brlapi describeKeyCode $code properties

   foreach {property name} {type type command cmd argument arg flags flg} {
      if {[info exists properties($property)]} {
         append text " $name:$properties($property)"
         unset properties($property)
      }
   }

   foreach property [lsort [array names properties]] {
      append text " $property:$properties($property)"
   }

   $session write -text $text
   resetTimeout
}

$session write -text "The TCL bindings for BrlAPI seem to be working."
fileevent $channel readable [list handleCommand $session]
setTimeout
vwait returnCode

$session leaveTtyMode
$session closeConnection; unset session
exit 0
