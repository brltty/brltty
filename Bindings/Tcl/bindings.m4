###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2006-2021 by Dave Mielke <dave@mielke.cc>
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

AC_DEFUN([BRLTTY_TCL_BINDINGS], [dnl
AC_PATH_PROGS([TCLSH], [tclsh tclsh8.5 tclsh8.6 tclsh8.7], [])

TCL_OK=false
TCL_CPPFLAGS=""
TCL_LIBS=""
TCL_DIR=""

if test -n "${TCLSH}"
then
   AC_MSG_NOTICE([Tcl shell: ${TCLSH}])
   tcl_config_name="tclConfig.sh"

m4_changequote([(((], [)))])
tcl_config_script=`
${TCLSH} <<END_OF_TCL_SCRIPT
foreach directory [concat \\$tcl_library [lindex [::tcl::pkgconfig get libdir,install] 0] \\$auto_path] {
   set file [file join \\$directory ${tcl_config_name}]
   if {[file isfile \\\$file]} {
      puts \\$file
      break
   }
}
END_OF_TCL_SCRIPT
`
m4_changequote([, ])

   if test -n "${tcl_config_script}"
   then
      AC_MSG_NOTICE([Tcl configuration script: ${tcl_config_script}])

      if test ! -r "${tcl_config_script}"
      then
         AC_MSG_WARN([Tcl configuration script not readable: ${tcl_config_script}])
      elif . "${tcl_config_script}"
      then
         TCL_OK=true
         TCL_CPPFLAGS="${TCL_INCLUDE_SPEC}"
         TCL_LIBS="${TCL_LIB_SPEC}"
      fi
   else
      AC_MSG_WARN([Tcl configuration script not found: ${tcl_config_name}])
   fi
else
   AC_MSG_WARN([Tcl shell not found])
   TCLSH="TCL_SHELL_NOT_FOUND_BY_CONFIGURE"
fi

${TCL_OK} && {
   test -n "${TCL_PACKAGE_PATH}" && {
      for directory in ${TCL_PACKAGE_PATH}
      do
         test `expr "${directory}" : '.*/lib'` -eq 0 || {
            TCL_DIR="${directory}"
            break
         }
      done
   }

   if test -n "${TCL_DIR}"
   then
      AC_MSG_NOTICE([Tcl packages directory: ${TCL_DIR}])
   else
      AC_MSG_WARN([Tcl packages directory not found])
      TCL_DIR="TCL_PACKAGES_DIRECTORY_NOT_FOUND_BY_CONFIGURE"
   fi
}

AC_SUBST([TCL_OK])
AC_SUBST([TCL_CPPFLAGS])
AC_SUBST([TCL_LIBS])
AC_SUBST([TCL_DIR])
])
