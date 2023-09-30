###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2025 by
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
# Web Page: http://brltty.app/
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

AC_PATH_PROGS([PYTHON], [python python3], [python])
if test -n "${PYTHON}"
then
   test -n "${PYTHON_VERSION}" || {
      BRLTTY_PYTHON_QUERY([PYTHON_VERSION], [version])

      test -n "${PYTHON_VERSION}" || {
         AC_MSG_WARN([Python version not defined])
         PYTHON_OK=false
      }
   }
   AC_SUBST([PYTHON_VERSION])

   test -n "${PYTHON_CPPFLAGS}" || {
      BRLTTY_PYTHON_QUERY([python_include_directory], [incdir])

      if test -n "${python_include_directory}"
      then
         PYTHON_CPPFLAGS="-I${python_include_directory}"

         test -f "${python_include_directory}/Python.h" || {
            AC_MSG_WARN([Python developer environment not installed])
            PYTHON_OK=false
         }
      else
         AC_MSG_WARN([Python include directory not found])
         PYTHON_OK=false
      fi
   }
   AC_SUBST([PYTHON_CPPFLAGS])

   test -n "${PYTHON_LIBS}" || {
      PYTHON_LIBS="-lpython${PYTHON_VERSION}"
      BRLTTY_PYTHON_QUERY([python_library_directory], [libdir])

      if test -n "${python_library_directory}"
      then
         PYTHON_LIBS="-L${python_library_directory}/config ${PYTHON_LIBS}"
      else
         AC_MSG_WARN([Python library directory not found])
         PYTHON_OK=false
      fi
   }
   AC_SUBST([PYTHON_LIBS])

   test -n "${PYTHON_EXTRA_LIBS}" || {
      BRLTTY_PYTHON_QUERY([PYTHON_EXTRA_LIBS], [libopts])
   }
   AC_SUBST([PYTHON_EXTRA_LIBS])

   test -n "${PYTHON_EXTRA_LDFLAGS}" || {
      BRLTTY_PYTHON_QUERY([PYTHON_EXTRA_LDFLAGS], [linkopts])
   }
   AC_SUBST([PYTHON_EXTRA_LDFLAGS])

   test -n "${PYTHON_SITE_PKG}" || {
      BRLTTY_PYTHON_QUERY([PYTHON_SITE_PKG], [pkgdir])

      test -n "${PYTHON_SITE_PKG}" || {
         AC_MSG_WARN([Python packages directory not found])
         PYTHON_OK=false
      }
   }
   AC_SUBST([PYTHON_SITE_PKG])
else
   AC_MSG_WARN([Python interpreter not found])
   PYTHON_OK=false
fi

AC_PATH_PROGS([CYTHON], [cython3 cython])
test -n "${CYTHON}" || {
   AC_MSG_WARN([Cython compiler not found])
   PYTHON_OK=false
}

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

AC_DEFUN([BRLTTY_PYTHON_QUERY], [dnl
   $1=`"${PYTHON}" "${srcdir}/Tools/pythoncmd" $2`
])
