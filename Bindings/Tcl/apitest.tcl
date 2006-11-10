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
processOptions optionValues {host.arg}

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
set brlapi [eval brlapi openConnection $connectionSettings]
puts "Object: $brlapi"

set host [$brlapi getHost]
puts "Host: $host ([expandList [brlapi expandHost $host] name port family])"

set keyFile [$brlapi getKeyFile]
puts "KeyFile: $keyFile"

set fileDescriptor [$brlapi getFileDescriptor]
puts "FileDescriptor: $fileDescriptor"

set driverId [$brlapi getDriverId]
puts "DriverId: $driverId"

set driverName [$brlapi getDriverName]
puts "DriverName: $driverName"

set displaySize [$brlapi getDisplaySize]
puts "DisplaySize: [expandList $displaySize width height]"

$brlapi closeConnection
exit 0
