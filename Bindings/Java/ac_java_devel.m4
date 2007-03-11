###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2007 by
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
#   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License,
# or (at your option) any later version.
# Please see the file COPYING-API for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

AC_DEFUN([AC_JAVA_DEVEL], [dnl
if test -n "${JAVA_HOME}"
then
   AC_PATH_PROG([JAVAC], [javac], [], ["${JAVA_HOME}/bin"])
else
   AC_PATH_PROG([JAVAC], [javac])
   if test -z "${JAVAC}"
   then
      AC_PATH_PROG([JAVAC], [javac], [], [/usr/java/bin /usr/java/jdk*/bin])
      if test -z "${JAVAC}"
      then
         AC_PATH_PROG([JAVAC], [gcj])
      fi
   fi
fi

if test -n "${JAVAC}"
then
   AC_MSG_NOTICE([javac is ${JAVAC}])

   JAVA_BIN=`AS_DIRNAME("${JAVAC}")`
   JAVA_ROOT=`AS_DIRNAME("${JAVA_BIN}")`

   AC_SUBST([JAVADOC], ["${JAVA_BIN}/javadoc"])

   JNIDIR="${JAVA_ROOT}/include"
   JNIHDR="jni.h"
   JNIFLAGS=""
   JAVA=yes
   AC_CHECK_HEADER([${JNIHDR}], [], [AC_CHECK_FILE(["${JNIDIR}/${JNIHDR}"], [JNIFLAGS="-I${JNIDIR}"], [JAVA=no])])
   AC_SUBST([JNIFLAGS])
   AC_SUBST([JAVA])
else
   AC_MSG_WARN([javac not found])
fi
])
