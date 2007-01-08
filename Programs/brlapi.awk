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

/#define[ \t]*BRLAPI_(CURSOR|DISPLAY|TTY)_/ {
  apiConstant(substr($2, 8), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_MAX/ {
  apiMask(substr($2, 12), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_[A-Z_]+_MASK/ {
  apiMask(substr($2, 12), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_[A-Z_]+_SHIFT/ {
  apiShift(substr($2, 12), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_TYPE_/ {
  apiType(substr($2, 17), $2, getDefineValue(), "")
  next
}

/#define[ \t]*BRLAPI_KEY_SYM_/ {
  apiKey(substr($2, 16), $2, getDefineValue(), "")
  next
}

