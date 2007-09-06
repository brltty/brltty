###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2007 by
#   Alexis Robert <alexissoft@free.fr>
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
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

AC_DEFUN([AC_PYTHON], [dnl
python_ok=true

AC_PATH_PROG([PYTHON], [python])
if test -z "${PYTHON}"
then
   AC_MSG_WARN([Python interpreter not found])
   python_ok=false
else
   for python_module in distutils
   do
      if test -n "`${PYTHON} -c "import ${python_module};" 2>&1`"
      then
         AC_MSG_WARN([Python module not found: ${python_module}])
         python_ok=false
      fi
   done

   if ${python_ok}
   then
      PYTHON_VERSION="`${PYTHON} -c "from distutils.sysconfig import get_config_vars; print get_config_vars('VERSION')[[0]];"`"
      if test -z "${PYTHON_VERSION}"
      then
         AC_MSG_WARN([Python version not defined])
         python_ok=false
      fi
   fi
   AC_SUBST([PYTHON_VERSION])

   if ${python_ok}
   then
      python_include_directory="`${PYTHON} -c "from distutils.sysconfig import get_python_inc; print get_python_inc();"`"
      if test -z "${python_include_directory}"
      then
         AC_MSG_WARN([Python include directory not found])
         python_ok=false
      elif test ! -f "${python_include_directory}/Python.h"
      then
         AC_MSG_WARN([Python development package not installed])
         python_ok=false
      else
         PYTHON_CPPFLAGS="-I${python_include_directory}"
      fi
   fi
   AC_SUBST([PYTHON_CPPFLAGS])

   if ${python_ok}
   then
      python_library_directory="`${PYTHON} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(0,1);"`"
      if test -z "${python_library_directory}"
      then
         AC_MSG_WARN([Python library directory not found])
         python_ok=false
      else
         PYTHON_LDFLAGS="-L${python_library_directory}/config -lpython${PYTHON_VERSION}"
      fi
   fi
   AC_SUBST([PYTHON_LDFLAGS])

   if ${python_ok}
   then
      python_package_directory="`${PYTHON} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(0,0);"`"
      if test -z "${python_package_directory}"
      then
         AC_MSG_WARN([Python package directory not found])
         python_ok=false
      else
         PYTHON_SITE_PKG="${python_package_directory}"
      fi
   fi
   AC_SUBST([PYTHON_SITE_PKG])

   if ${python_ok}
   then
      PYTHON_EXTRA_LIBS="`${PYTHON} -c "from distutils.sysconfig import get_config_var; print get_config_var('LOCALMODLIBS'), get_config_var('LIBS');"`"
   fi
   AC_SUBST([PYTHON_EXTRA_LIBS])

   if ${python_ok}
   then
      PYTHON_EXTRA_LDFLAGS="`${PYTHON} -c "from distutils.sysconfig import get_config_var; print get_config_var('LINKFORSHARED');"`"
   fi
   AC_SUBST([PYTHON_EXTRA_LDFLAGS])

   if ${python_ok}
   then
      AC_PATH_PROG([PYREXC], [pyrexc])
      if test -z "${PYREXC}"
      then
         AC_MSG_WARN([Pyrex compiler not found])
         python_ok=false
      fi
   fi
fi
])
