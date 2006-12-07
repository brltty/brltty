#!/usr/bin/env tclsh
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
puts "Host: $host ([expandList [brlapi expandHost $host] name port family])"

set auth [$session getAuth]
puts "Auth: $auth"

set fileDescriptor [$session getFileDescriptor]
puts "FileDescriptor: $fileDescriptor"

set driverId [$session getDriverId]
puts "DriverId: $driverId"

set driverName [$session getDriverName]
puts "DriverName: $driverName"

set displaySize [$session getDisplaySize]
puts "DisplaySize: [expandList $displaySize width height]"

$session closeConnection; unset session
exit 0
