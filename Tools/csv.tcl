###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
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

proc csvQuoteText {text} {
   return "\"[regsub -all {(")} $text {\1\1}]\""
}

proc csvMakeRow {columns {width ""}} {
   if {[string length $width] > 0} {
      set count [llength $columns]

      if {$count < $width} {
         eval lappend columns [lrepeat [expr {$width - $count}] ""]
      } elseif {$count > $width} {
         set columns [lreplace $columns $width end]
      }
   }

   return [join [lmap text $columns {csvQuoteText $text}] ,]
}

proc csvMakeTable {rows {width ""}} {
   set table ""

   if {[string length $width] == 0} {
      set width 0

      foreach columns $rows {
         if {[set count [llength $columns]] > $width} {
            set width $count
         }
      }
   }

   foreach columns $rows {
      append table [csvMakeRow $columns $width]
      append table "\n"
   }

   return $table
}

proc csvApplyHeaders {table} {
   set rows [list]
   set headers [lindex $table 0]

   foreach columns [lrange $table 1 end] {
      set row [dict create]
      set count [llength $columns]
      set index 0

      foreach header $headers {
         if {$index == $count} {
            set column ""
         } else {
            set column [lindex $columns $index]
            incr index
         }

         dict set row $header $column
      }

      lappend rows $row
   }

   return $rows
}

proc csvParseString {string} {
   set rows [list]
   set columns [list]
   set column ""

   set finishColumn {
      lappend columns $column
      set column ""
   }

   set finishRow {
      if {([llength $columns] > 0) || ([string length $column] > 0)} {
         eval $finishColumn
         lappend rows $columns
         set columns [list]
      }
   }

   set state unquoted
   set length [string length $string]
   set index 0

   while {$index < $length} {
      set character [string index $string $index]
      set isQuote [string equal $character {"}]
      incr index

      switch -exact $state {
         unquoted {
            if {$isQuote} {
               set state inQuotes
               continue
            }

            switch -exact $character {
               "," {
                  eval $finishColumn
                  continue
               }

               "\n" {
                  eval $finishRow
                  continue
               }

               "\r" {
                  eval $finishRow
                  set state wasCarriageReturn
                  continue
               }
            }
         }

         inQuotes {
            if {$isQuote} {
               set state wasQuote
               continue
            }
         }

         wasQuote {
            if {!$isQuote} {
               set state unquoted
               incr index -1
               continue
            }

            set state inQuotes
         }

         wasCarriageReturn {
            if {[string equal $character "\n"]} {
               set state unquoted
               continue
            }
         }

         default {
            return -code error "unrecognized csv parse state: $state"
         }
      }

      append column $character
   }

   eval $finishRow
   return $rows
}

proc csvLoadFile {file} {
   return [csvApplyHeaders [csvParseString [readFile $file]]]
}

