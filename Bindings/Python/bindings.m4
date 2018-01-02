###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2018 by
#   Alexis Robert <alexissoft@free.fr>
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
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

AC_DEFUN([BRLTTY_PYTHON_BINDINGS], [dnl
PYTHON_OK=true

# Suppress a new warning introduced in Python 3.6:
#
#    Python runtime initialized with LC_CTYPE=C
#    (a locale with default ASCII encoding),
#    which may cause Unicode compatibility problems.
#    Using C.UTF-8, C.utf8, or UTF-8 (if available)
#    as alternative Unicode-compatible locales is recommended.
#
export PYTHONCOERCECLOCALE=0

AC_PATH_PROG([PYTHON], [python])
if test -z "${PYTHON}"
then
   AC_MSG_WARN([Python interpreter not found])
   PYTHON_OK=false
else
   PYTHON_PROLOGUE=""
   for python_module in sys distutils.sysconfig
   do
      if test -n "`${PYTHON} -c "import ${python_module};" 2>&1`"
      then
         AC_MSG_WARN([Python module not found: ${python_module}])
         PYTHON_OK=false
      else
         PYTHON_PROLOGUE="${PYTHON_PROLOGUE}import ${python_module}; "
      fi
   done
   AC_SUBST([PYTHON_PROLOGUE])

   if "${PYTHON_OK}"
   then
      if test -z "${PYTHON_VERSION}"
      then
         [PYTHON_VERSION="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_config_vars('VERSION')[0]);"`"]
         if test -z "${PYTHON_VERSION}"
         then
            [PYTHON_VERSION="`${PYTHON} -c "${PYTHON_PROLOGUE} print('.'.join(sys.version.split()[0].split('.')[:2]));"`"]
            if test -z "${PYTHON_VERSION}"
            then
               AC_MSG_WARN([Python version not defined])
            fi
         fi
      fi
      AC_SUBST([PYTHON_VERSION])

      if test -z "${PYTHON_CPPFLAGS}"
      then
         [python_include_directory="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_python_inc());"`"]
         if test -z "${python_include_directory}"
         then
            AC_MSG_WARN([Python include directory not found])
            PYTHON_OK=false
         else
            PYTHON_CPPFLAGS="-I${python_include_directory}"

            if test ! -f "${python_include_directory}/Python.h"
            then
               AC_MSG_WARN([Python developer environment not installed])
               PYTHON_OK=false
            fi
         fi
      fi
      AC_SUBST([PYTHON_CPPFLAGS])

      if test -z "${PYTHON_LIBS}"
      then
         PYTHON_LIBS="-lpython${PYTHON_VERSION}"

         [python_library_directory="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_python_lib(0,1));"`"]
         if test -z "${python_library_directory}"
         then
            AC_MSG_WARN([Python library directory not found])
         else
            PYTHON_LIBS="-L${python_library_directory}/config ${PYTHON_LIBS}"
         fi
      fi
      AC_SUBST([PYTHON_LIBS])

      if test -z "${PYTHON_EXTRA_LIBS}"
      then
         [PYTHON_EXTRA_LIBS="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_config_var('LOCALMODLIBS'), distutils.sysconfig.get_config_var('LIBS'));"`"]
      fi
      AC_SUBST([PYTHON_EXTRA_LIBS])

      if test -z "${PYTHON_EXTRA_LDFLAGS}"
      then
         [PYTHON_EXTRA_LDFLAGS="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_config_var('LINKFORSHARED'));"`"]
      fi
      AC_SUBST([PYTHON_EXTRA_LDFLAGS])

      if test -z "${PYTHON_SITE_PKG}"
      then
         [PYTHON_SITE_PKG="`${PYTHON} -c "${PYTHON_PROLOGUE} print(distutils.sysconfig.get_python_lib(1,0));"`"]
         if test -z "${PYTHON_SITE_PKG}"
         then
            AC_MSG_WARN([Python package directory not found])
         fi
      fi
      AC_SUBST([PYTHON_SITE_PKG])
   fi
fi

AC_PATH_PROGS([CYTHON], [cython])
if test -z "${CYTHON}"
then
   AC_MSG_WARN([Cython compiler not found])
   PYTHON_OK=false
fi

if test "${GCC}" = "yes"
then
   CYTHON_CFLAGS="-Wno-parentheses -Wno-unused -fno-strict-aliasing -U_POSIX_C_SOURCE -U_XOPEN_SOURCE"
else
   case "${host_os}"
   in
      *)
         CYTHON_CFLAGS=""
         ;;
   esac
fi
AC_SUBST([CYTHON_CFLAGS])

AC_SUBST([PYTHON_OK])
])
