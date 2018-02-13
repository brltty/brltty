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
load libbrlapi_tcl.so
package require Tclx

set optionDefinitions {
   {auth untyped}
   {host untyped}
   {tty untyped}
}

if {![processProgramOptions optionValues $optionDefinitions]} {
   syntaxError
}

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
   putProperties "File Descriptor" [set fileDescriptor [$session getFileDescriptor]]
   putProperties "Driver Name" [$session getDriverName]
   putProperties "Model Identifier" [$session getModelIdentifier]
   putProperties "Display Size" [formatValues [$session getDisplaySize] width height]

   if {[info exists optionValues(tty)]} {
      if {[catch [list $session enterTtyMode -tty $optionValues(tty)] tty] == 0} {
         putProperties "TTY Number" $tty

         proc ttySetTimeout {} {
            global ttyTimeoutEvent
            set ttyTimeoutEvent [after 10000 [list set ttyReturnCode 0]]
         }

         proc ttyCancelTimeout {} {
            global ttyTimeoutEvent
            after cancel $ttyTimeoutEvent
            unset ttyTimeoutEvent
         }

         proc ttyResetTimeout {} {
            ttyCancelTimeout
            ttySetTimeout
         }

         proc ttyShowKey {session} {
            set properties [list]

            set code [$session readKey 0]
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

            foreach name [lsort [array names description]] {
               lappend properties [formatProperty $name $description($property)]
            }

            set text [join $properties " "]
            putProperties Key $text
            $session write -text $text

            ttyResetTimeout
         }

         proc ttyShowKeys {} {
            global session fileDescriptor

            set channel [dup $fileDescriptor]
            fileevent $channel readable [list ttyShowKey $session]

            $session write -text "The TCL bindings for BrlAPI seem to be working."
            ttySetTimeout

            vwait ttyReturnCode
            fileevent $channel readable ""
         }

         ttyShowKeys
         $session leaveTtyMode
      } else {
         writeProgramMessage "invalid tty: $tty"
      }
   }

   $session closeConnection; unset session
} else {
   writeProgramMessage "connection failure: $session"
}

exit 0
