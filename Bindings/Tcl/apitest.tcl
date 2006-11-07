#!/usr/bin/env tclsh
load libbrlapi_tcl.so
package require cmdline

proc writeProgramMessage {message} {
   puts stderr "[::cmdline::getArgv0]: $message"
}

set optionList {host.arg}
while {[set optionStatus [::cmdline::getopt argv $optionList optionName optionValue]] > 0} {
   set optionValues($optionName) $optionValue
}

if {$optionStatus < 0} {
   writeProgramMessage $optionValue
   exit 2
}

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
