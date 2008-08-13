#!/usr/bin/env tclsh
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

set displaySize [$session getDisplaySize]
puts "DisplaySize: [expandList $displaySize width height]"

set tty [$session enterTtyMode]
puts "Tty: $tty"

$session leaveTtyMode
$session closeConnection; unset session
exit 0
