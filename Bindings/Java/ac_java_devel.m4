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
   AC_JAVA_COMPILER([javac], ["${JAVA_HOME}/bin"])
else
   AC_JAVA_COMPILER([javac], [], [dnl
      AC_JAVA_COMPILER([javac], [/usr/java/bin /usr/java/jdk*/bin], [dnl
         AC_JAVA_COMPILER([gcj])
      ])
   ])
fi

if test -n "${JAVAC_PATH}"
then
   AC_MSG_NOTICE([Java compiler is ${JAVAC_PATH}])

   JAVA_ENCODING="UTF-8"
   case "${JAVAC_NAME}"
   in
      javac) JAVAC_OPTIONS="-encoding ${JAVA_ENCODING}";;
      gcj)   JAVAC_OPTIONS="-C --encoding=${JAVA_ENCODING}";;
      *)     JAVAC_OPTIONS="";;
   esac
   AC_SUBST([JAVAC], ["${JAVAC_PATH} ${JAVAC_OPTIONS}"])

   JAVA_BIN=`AS_DIRNAME("${JAVAC_PATH}")`
   JAVA_ROOT=`AS_DIRNAME("${JAVA_BIN}")`

   AC_CHECK_PROGS([JAVADOC_NAME], [javadoc gjdoc], [], ["${JAVA_BIN}"])
   if test -n "${JAVADOC_NAME}"
   then
      JAVADOC_PATH="${JAVA_BIN}/${JAVADOC_NAME}"
   else
      JAVADOC_PATH=":"
   fi
   AC_SUBST([JAVADOC], ["${JAVADOC_PATH} -encoding ${JAVA_ENCODING}"])

   AC_CHECK_PROGS([JAR_NAME], [jar], [], ["${JAVA_BIN}"])
   AC_SUBST([JAR], ["${JAVA_BIN}/${JAR_NAME}"])

   JNIDIR="${JAVA_ROOT}/include"
   JNIHDR="jni.h"
   JNIFLAGS=""
   JAVA=yes
   AC_CHECK_HEADER([${JNIHDR}], [], [AC_CHECK_FILE(["${JNIDIR}/${JNIHDR}"], [JNIFLAGS="-I${JNIDIR}"], [JAVA=no])])
   AC_SUBST([JNIFLAGS])
   AC_SUBST([JAVA])
else
   AC_MSG_WARN([Java compiler not found])
fi
])

AC_DEFUN([AC_JAVA_COMPILER], [dnl
AC_PATH_PROG([JAVAC_PATH], [$1], [], [$2])
if test -n "${JAVAC_PATH}"
then
   JAVAC_NAME="$1"
ifelse(len([$3]), 0, [], [dnl
else
   $3
])dnl
fi])
