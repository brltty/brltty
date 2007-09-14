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

AC_DEFUN([BRLTTY_JAVA_BINDINGS], [dnl
if test -n "${JAVA_HOME}"
then
   BRLTTY_JAVA_COMPILER([javac], ["${JAVA_HOME}/bin"])
else
   BRLTTY_JAVA_COMPILER([javac], [], [dnl
      BRLTTY_JAVA_COMPILER([javac], [/usr/java/bin /usr/java/jdk*/bin], [dnl
         BRLTTY_JAVA_COMPILER([gcj])
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

   AC_CHECK_PROGS([JAR_NAME], [jar], [JAR_NOT_FOUND_BY_CONFIGURE], ["${JAVA_BIN}"])
   AC_SUBST([JAR], ["${JAVA_BIN}/${JAR_NAME}"])
   BRLTTY_JAVA_DIRECTORY([JAR], [/usr/share/java])

   JNIDIR="${JAVA_ROOT}/include"
   JNIHDR="jni.h"
   JNIFLAGS=""
   JAVA_OK=true
   AC_CHECK_HEADER([${JNIHDR}], [], [AC_CHECK_FILE(["${JNIDIR}/${JNIHDR}"], [JNIFLAGS="-I${JNIDIR}"], [JAVA_OK=false])])
   AC_SUBST([JNIFLAGS])
   AC_SUBST([JAVA_OK])
else
   AC_MSG_WARN([Java compiler not found])
fi
])

AC_DEFUN([BRLTTY_JAVA_COMPILER], [dnl
AC_PATH_PROG([JAVAC_PATH], [$1], [], [$2])
if test -n "${JAVAC_PATH}"
then
   JAVAC_NAME="$1"
ifelse(len([$3]), 0, [], [dnl
else
   $3
])dnl
fi])

AC_DEFUN([BRLTTY_JAVA_DIRECTORY], [dnl
JAVA_$1_DIR=""
for directory in $2
do
   test -d "${directory}" && {
      JAVA_$1_DIR="${directory}"
      break
   }
done

if test -n "${JAVA_$1_DIR}"
then
   JAVA_$1="yes"
   AC_MSG_NOTICE([Java] m4_tolower([$1]) [installation directory is ${JAVA_$1_DIR}])
else
   JAVA_$1="no"
   AC_MSG_WARN([no commonly used] m4_tolower([$1]) [installation directory])
fi

AC_SUBST([JAVA_$1])
AC_SUBST([JAVA_$1_DIR])
])
