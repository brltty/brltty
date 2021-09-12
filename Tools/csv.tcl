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

proc csvMakeRow {columns} {
   return [join [lmap text $columns {csvQuoteText $text}] ,]
}

proc csvMakeTable {rows} {
   set width 0
   foreach columns $rows {
      if {[set count [llength $columns]] > $width} {
         set width $count
      }
   }

   set table ""
   foreach columns $rows {
      append table [csvMakeRow [concat $columns [lrepeat [expr {$width - [llength $columns]}] ""]]]
      append table "\n"
   }

   return $table
}

