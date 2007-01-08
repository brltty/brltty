###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2007 by The BRLTTY Developers.
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

BEGIN {
  OFS = ""
}
/^ *BRL_CMD_/ {
  print "{\"", substr($1, 5), "\", KEYCODE, ", $1, "},"
  next
}
/^ *BRL_GSC_/ {
  print "{\"", substr($1, 5), "\", STATCODE, ", $1, "},"
  next
}
/#define[ \t]*BRL_BLK_PASS/ {
  next
}
/#define[ \t]*BRL_BLK_/ {
  print "{\"", substr($2, 5), "\", KEYCODE, " $2, "},"
  next
}
/^ *BRL_KEY_/ {
  gsub(",", "", $1)
  print "{\"", substr($1, 5), "\", VPK, BRL_BLK_PASSKEY+", $1, "},"
  next
}
