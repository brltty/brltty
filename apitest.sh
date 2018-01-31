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

programName="$(basename "${0}")"
cd "$(dirname "${0}")"
programDirectory="$(pwd)"

[ -z "${LD_LIBRARY_PATH}" ] || LD_LIBRARY_PATH=":${LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="${programDirectory}${LD_LIBRARY_PATH}"
export LD_PRELOAD="${programDirectory}/../../Programs/libbrlapi.so"

