###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2023 by The BRLTTY Developers.
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

set brlErrorCategory braille
set brlUnicodeRow [expr {0X2800 + 0}]

set brfDotsTable {
   0
   2346
   5
   3456
   1246
   146
   12346
   3
   12356
   23456
   16
   346
   6
   36
   46
   34
   356
   2
   23
   25
   256
   26
   235
   2356
   236
   35
   156
   56
   126
   123456
   345
   1456
   4
   1
   12
   14
   145
   15
   124
   1245
   125
   24
   245
   13
   123
   134
   1345
   135
   1234
   12345
   1235
   234
   2345
   136
   1236
   2456
   1346
   13456
   1356
   246
   1256
   12456
   45
   456
}

proc brlThrowError {code problem {value ""}} {
   set message $problem

   if {[string length $value] > 0} {
      append message ": $value"
   }

   return -code error -errorcode [list $::brlErrorCategory $code $problem $value] $message
}

proc brlHexadecimalToDecimal {hexadecimal} {
   if {![string is xdigit -strict $hexadecimal]} {
      brlThrowError not-hex "not a hexadecimal number" $hexadecimal
   }

   scan $hexadecimal "%x" decimal
   return $decimal
}

proc brlCharacterToCodepoint {character} {
   if {[string length $character] != 1} {
      brlThrowError not-char "not a single character" $character
   }

   scan $character "%c" codepoint
   return $codepoint
}

proc brlCodepointToCharacter {codepoint} {
   if {![string is integer -strict $codepoint]} {
      brlThrowError not-int "not an integer" $codepoint
   }

   return [format "%c" "$codepoint"]
}

proc brlCharacterToDots {character} {
   if {[string length $character] == 1} {
      global brlUnicodeRow

      set codepoint [brlCharacterToCodepoint $character]
      set bits [expr {$codepoint & 0XFF}]

      if {($codepoint - $bits) == $brlUnicodeRow} {
         set dots ""
         set dot 1
         set bit 0X01

         while {$bits != 0} {
            if {($bits & $bit) != 0} {
               set bits [expr {$bits & ~$bit}]
               append dots $dot
            }

            set bit [expr {$bit << 1}]
            incr dot
         }

         if {[string length $dots] > 0} {
            return $dots
         }

         return "0"
      }

      global brfDotsTable
      set index [expr {$codepoint - 0X20}]

      if {($codepoint >= 0X60) && ($codepoint <= 0X7E)} {
         incr index -0X20
      }

      if {($index >= 0) && ($index < [llength $brfDotsTable])} {
         return [lindex $brfDotsTable $index]
      }
   }

   brlThrowError not-brl "not a braille character" $character
}

proc brlDotsToCharacter {dots} {
   if {[string length $dots] == 0} {
      brlThrowError no-dots "no dot numbers"
   }

   global brlUnicodeRow
   set codepoint $brlUnicodeRow

   if {![string equal $dots "0"]} {
      foreach dot [split $dots ""] {
         if {[string first $dot "12345678"] == -1} {
            brlThrowError not-dot "Not a dot number" $dot
         }

         set bit [expr {1 << ($dot - 1)}]

         if {($codepoint & $bit) != 0} {
            brlThrowError dup-dot "duplicate dot number" $dot
         }

         incr codepoint $bit
      }
   }

   return [brlCodepointToCharacter $codepoint]
}

proc brlFormatCodepoint {codepoint} {
   set digits [format "%X" $codepoint]
   set length [string length $digits]
   set letter "z"

   foreach {count letter} {2 x 4 u 8 U} {
      if {$length <= $count} {
         set digits "[string repeat "0" [expr {$count - $length}]]$digits"
         break
      }
   }

   return "\\$letter$digits"
}

