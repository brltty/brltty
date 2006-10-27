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

/#define[ \t]*BRLAPI_KEY_TYPE_/ {
  if ($2 ~ /_(MASK|SHIFT)$/) next
  apiType(substr($2, 17), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_SYM_/ {
  apiKey(substr($2, 16), $2, getDefineValue(), "")
  next
}

