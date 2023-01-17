###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2023 by
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

AC_DEFUN([BRLTTY_JAVA_BINDINGS], [dnl
AC_SUBST([JAVA_OK], [false])
AC_SUBST([JAVAC], [JAVA_COMPILER_NOT_FOUND_BY_CONFIGURE])
AC_SUBST([JAVADOC], [JAVA_DOCUMENT_COMMAND_NOT_FOUND_BY_CONFIGURE])
AC_SUBST([JAR], [JAVA_ARCHIVE_COMMAND_NOT_FOUND_BY_CONFIGURE])

java_compiler_name="javac"
java_compiler_path=""

if test -n "${JAVA_HOME}"
then
   BRLTTY_CHECK_JAVA_HOME(["${JAVA_HOME}"])
else
   BRLTTY_CHECK_JAVA_COMPILER([${java_compiler_name}])
   BRLTTY_CHECK_JAVA_HOME(["/usr/java"])
   BRLTTY_CHECK_JAVA_HOME(["/usr/lib/jvm/default-java"])
   BRLTTY_CHECK_JAVA_HOME(["/usr/lib/jvm/java"])
   BRLTTY_CHECK_JAVA_COMPILER([gcj])
fi

test -n "${java_compiler_path}" && {
   brltty_path=`realpath -- "${java_compiler_path}"` && java_compiler_path="${brltty_path}"
   test "${java_compiler_path##*/}" = "javavm" && java_compiler_path=""
}

BRLTTY_CHECK_JAVA_PATH([dnl
   test -n "${JAVA_VERSION}" || export JAVA_VERSION=11+
   AC_MSG_NOTICE([Java version: ${JAVA_VERSION}])

   AC_MSG_CHECKING([JVM path])
   BRLTTY_JAVA_QUERY([brltty_path], [jvmpath])

   if test -n "${brltty_path}"
   then
      AC_MSG_RESULT([${brltty_path}])
      java_compiler_path="${brltty_path%/*}/${java_compiler_name}"
   else
      AC_MSG_RESULT([not found])
   fi
])

if test -n "${java_compiler_path}"
then
   JAVA_OK=true
   AC_MSG_NOTICE([Java compiler: ${java_compiler_path}])

   java_compiler_name="${java_compiler_path##*/}"
   java_source_encoding="UTF-8"

   case "${java_compiler_name}"
   in
      javac) java_compiler_options="-encoding ${java_source_encoding}";;
      gcj)   java_compiler_options="-C --encoding=${java_source_encoding}";;

      *)     java_compiler_options=""
             AC_MSG_WARN([Java compiler name not handled: ${java_compiler_name}])
             ;;
   esac

   JAVAC="'${java_compiler_path}' ${java_compiler_options}"
   java_commands_directory=`AS_DIRNAME("${java_compiler_path}")`
   java_home_directory=`AS_DIRNAME("${java_commands_directory}")`

   BRLTTY_CHECK_JAVA_COMMAND([java_document_command], [javadoc gjdoc])
   test -n "${java_document_command}" || java_document_command=":"
   JAVADOC="'${java_document_command}' -encoding ${java_source_encoding}"

   BRLTTY_CHECK_JAVA_COMMAND([java_archive_command], [jar gjar])
   JAR="'${java_archive_command}'"

   BRLTTY_JAVA_DIRECTORY([JAR], [/usr/share/java /mingw])
   BRLTTY_JAVA_DIRECTORY([JNI], [/usr/lib*/java /usr/lib*/jni /usr/lib/*/jni /mingw])

   JAVA_JNI_INC="${java_home_directory}/include"
   JAVA_JNI_HDR="jni.h"
   JAVA_JNI_FLAGS=""
   AC_CHECK_HEADER([${JAVA_JNI_HDR}], [], [AC_CHECK_FILE(["${JAVA_JNI_INC}/${JAVA_JNI_HDR}"], [JAVA_JNI_FLAGS="-I${JAVA_JNI_INC}"], [JAVA_OK=false])])

   "${JAVA_OK}" && {
      set -- "${JAVA_JNI_INC}"/*/jni_md.h

      if test ${#} -eq 1
      then
         brltty_directory=`AS_DIRNAME("${1}")`
         JAVA_JNI_FLAGS="${JAVA_JNI_FLAGS} -I${brltty_directory}"
      elif test ${#} -gt 0
      then
         AC_MSG_WARN([more than one machine-dependent Java header found: ${*}])
         JAVA_OK=false
      fi
   }

   AC_SUBST([JAVA_JNI_HDR])
   AC_SUBST([JAVA_JNI_INC], ["'${JAVA_JNI_INC}'"])
   AC_SUBST([JAVA_JNI_FLAGS])
else
   AC_MSG_WARN([Java compiler not found])
fi
])

AC_DEFUN([BRLTTY_CHECK_JAVA_PATH], [dnl
test -n "${java_compiler_path}" || {
   $1
}
])

AC_DEFUN([BRLTTY_CHECK_JAVA_COMPILER], [dnl
   BRLTTY_CHECK_JAVA_PATH([dnl
      AC_PATH_PROG([java_compiler_path], [$1], [])
   ])
])

AC_DEFUN([BRLTTY_CHECK_JAVA_HOME], [dnl
test -n "${java_compiler_path}" || {
   AC_MSG_CHECKING([if $1 is a JDK])
   brltty_path="$1/bin/${java_compiler_name}"

   if test -f "${brltty_path}" -a -x "${brltty_path}"
   then
      java_compiler_path="${brltty_path}"
      AC_MSG_RESULT([yes])
   else
      AC_MSG_RESULT([no])
   fi
}
])

AC_DEFUN([BRLTTY_CHECK_JAVA_COMMAND], [dnl
   AC_PATH_PROGS([$1], [$2], [], ["${java_commands_directory}"])
])

AC_DEFUN([BRLTTY_JAVA_DIRECTORY], [dnl
test -n "${JAVA_$1_DIR}" || {
   for directory in $2
   do
      test -d "${directory}" && {
	 JAVA_$1_DIR="${directory}"
	 break
      }
   done
}

if test -n "${JAVA_$1_DIR}"
then
   JAVA_$1="yes"
   AC_MSG_NOTICE([Java] m4_tolower([$1]) [installation directory: ${JAVA_$1_DIR}])
else
   JAVA_$1="no"
   AC_MSG_WARN([no commonly used] m4_tolower([$1]) [installation directory])
fi

AC_SUBST([JAVA_$1])
AC_SUBST([JAVA_$1_DIR])
])

AC_DEFUN([BRLTTY_JAVA_QUERY], [dnl
   $1=`"${srcdir}/Tools/javacmd" $2`
])
