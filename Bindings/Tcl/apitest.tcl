#!/usr/bin/env tclsh
load brlapi.so

proc expandList {list args} {
   set result ""
   set delimiter ""
   foreach field $args value $list {
      append result "$delimiter$field=$value"
      set delimiter " "
   }
   return $result
}

set brlapi [brlapi connect]
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
