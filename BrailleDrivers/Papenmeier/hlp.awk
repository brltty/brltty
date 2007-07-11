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
/^ *BRL_GSC_/ {
  if (x = match($0, "/.*/")) {
    print "{KEYS_STATUS+", $1, ", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
