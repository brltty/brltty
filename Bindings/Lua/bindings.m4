###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2023 by Dave Mielke <dave@mielke.cc>
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

AC_DEFUN([BRLTTY_LUA_BINDINGS], [dnl
LUA_OK=false
LUA=""
LUA_LIBRARY_DIRECTORY=""

BRLTTY_HAVE_PACKAGE([lua], [lua5.4 lua], [dnl
   AC_PATH_PROGS([LUA], [lua5.4 lua])

   if test -n "${LUA}"
   then
      AC_MSG_NOTICE([Lua shell: ${LUA}])
      BRLTTY_LUA_QUERY([LUA_LIBRARY_DIRECTORY], [libdir])

      if test -n "${LUA_LIBRARY_DIRECTORY}"
      then
         AC_MSG_NOTICE([Lua library directory: ${LUA_LIBRARY_DIRECTORY}])
         LUA_OK=true
      else
         AC_MSG_WARN([Lua library directory not found])
      fi
   else
      AC_MSG_WARN([Lua shell not found])
      LUA="LUA_SHELL_NOT_FOUND_BY_CONFIGURE"
   fi
])

AC_SUBST([LUA_OK])
AC_SUBST([LUA])
AC_SUBST([LUA_LIBRARY_DIRECTORY])
])

AC_DEFUN([BRLTTY_LUA_QUERY], [dnl
   $1=`"${srcdir}/Tools/luacmd" $2`
])
