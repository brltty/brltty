###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2021 by
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
#   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
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

BEGIN {
}

END {
}

function brlCommand(name, symbol, value, help) {
}

function brlBlock(name, symbol, value, help) {
}

function brlKey(name, symbol, value, help) {
}

function brlFlag(name, symbol, value, help) {
}

function brlDot(number, symbol, value, help) {
}

function apiConstant(name, symbol, value, help) {
  if (gsub("^PARAM_", "", name)) {
    if (name == "COUNT") return
    if (gsub("^TYPE_", "", name)) return

    gsub("_", "-", name)
    name = tolower(name)
    print "{ \"" name "\", " symbol " }," >parametersHeader
  }
}

function apiMask(name, symbol, value, help) {
}

function apiShift(name, symbol, value, help) {
}

function apiType(name, symbol, value, help) {
}

function apiKey(name, symbol, value, help) {
}

function apiRangeType(name, symbol, value, help) {
}

