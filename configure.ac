###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
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

AC_PREREQ([2.64])
AC_INIT([BRLTTY], [6.4.1], [BRLTTY@brltty.app], [brltty], [https://brltty.app/])
AC_CONFIG_SRCDIR([Programs/brltty.c])

AC_CONFIG_MACRO_DIR([m4])
m4_include([Bindings/Java/bindings.m4])
m4_include([Bindings/Lisp/bindings.m4])
m4_include([Bindings/OCaml/bindings.m4])
m4_include([Bindings/Python/bindings.m4])
m4_include([Bindings/Tcl/bindings.m4])

copyright="© 1995-2021 by The BRLTTY Developers"
AC_SUBST([PACKAGE_COPYRIGHT], ["${copyright}"])
AC_DEFINE_UNQUOTED([PACKAGE_COPYRIGHT], ["${copyright}"],
                   [Define this to be a string containing the copyright notice.])

AC_SUBST([api_major], [0])
AC_SUBST([api_minor], [8])
AC_SUBST([api_revision], [3])

AC_SUBST([api_name], [BrlAPI])
AC_SUBST([api_version], [${api_major}.${api_minor}])
AC_SUBST([api_release], [${api_version}.${api_revision}])

AC_DEFINE_UNQUOTED([BRLAPI_RELEASE], ["${api_release}"],
                   [Define this to be a string containing the API library version.])
AC_DEFINE_UNQUOTED([BRLAPI_MAJOR], [${api_major}],
                   [Define this to be the API library major version number.])
AC_DEFINE_UNQUOTED([BRLAPI_MINOR], [${api_minor}],
                   [Define this to be the API library minor version number.])
AC_DEFINE_UNQUOTED([BRLAPI_REVISION], [${api_revision}],
                   [Define this to be the API library revision number.])

BRLTTY_SUMMARY_BEGIN
brltty_build_directories=". Programs Documents Messages"
brltty_source_directory="`dirname "${0}"`"
brltty_manual_directories=""

for brltty_directory in Autostart Authorization Initramfs Android
do
   for brltty_subdirectory in `cd "${brltty_source_directory}/${brltty_directory}" && echo *`
   do
      brltty_path="${brltty_directory}/${brltty_subdirectory}"
      test -f "${brltty_source_directory}/${brltty_path}/Makefile.in" && {
         brltty_build_directories="${brltty_build_directories} ${brltty_path}"
      }
   done
done

for brltty_manual_directory in `(cd "${brltty_source_directory}" && echo Documents/Manual-*/*)`
do
   test -f "${brltty_source_directory}/${brltty_manual_directory}/Makefile.in" && {
      brltty_manual_directories="${brltty_manual_directories} ${brltty_manual_directory}"
   }
done

AC_SUBST([brltty_manual_directories])
brltty_build_directories="${brltty_build_directories} ${brltty_manual_directories}"

AC_CONFIG_HEADER([config.h])
AC_CONFIG_HEADER([Programs/brlapi.h])
AC_PREFIX_DEFAULT([])
AC_CONFIG_AUX_DIR([acdir])
AC_CANONICAL_SYSTEM

brltty_default_execute_root="/${PACKAGE_TARNAME}-${PACKAGE_VERSION}"
BRLTTY_ARG_WITH(
   [execute-root], [DIRECTORY],
   [where the package is to be anchored at run-time],
   [execute_root], [""]
)
if test "${execute_root}" = "no"
then
   execute_root=""
elif test "${execute_root}" = "yes"
then
   execute_root="${brltty_default_execute_root}"
fi
AC_SUBST([execute_root])
BRLTTY_SUMMARY_ITEM([execute-root], [execute_root])

brltty_default_install_root="/tmp/${PACKAGE_TARNAME}-${PACKAGE_VERSION}"
BRLTTY_ARG_WITH(
   [install-root], [DIRECTORY],
   [where to install the package],
   [install_root], ["${execute_root}"]
)
if test "${install_root}" = "no"
then
   install_root="${execute_root}"
elif test "${install_root}" = "yes"
then
   install_root="${brltty_default_install_root}"
fi
AC_SUBST([install_root])
BRLTTY_SUMMARY_ITEM([install-root], [install_root])

BRLTTY_ARG_WITH(
   [configuration-file], [FILE],
   [name of configuration file],
   [CONFIGURATION_FILE], ["${PACKAGE_TARNAME}.conf"]
)
AC_SUBST([CONFIGURATION_FILE])
BRLTTY_DEFINE_EXPANDED([CONFIGURATION_FILE], ["${CONFIGURATION_FILE}"],
                       [Define this to be a string containing the name of the default configuration file.])

AC_SUBST([PREFERENCES_FILE], ["${PACKAGE_TARNAME}.prefs"])
BRLTTY_DEFINE_EXPANDED([PREFERENCES_FILE], ["${PREFERENCES_FILE}"],
                       [Define this to be a string containing the name of the default preferences file.])

BRLTTY_PORTABLE_DIRECTORY([includedir], [/usr])
BRLTTY_PORTABLE_DIRECTORY([datarootdir], [/usr])
BRLTTY_PORTABLE_DIRECTORY([localstatedir], [])

BRLTTY_SUMMARY_ITEM([libdir], [libdir])
BRLTTY_SUMMARY_ITEM([sysconfdir], [sysconfdir])

BRLTTY_ARG_REQUIRED(
   [program-directory], [DIRECTORY],
   [path to directory containing executables],
   [program_directory], ["${bindir}"]
)

BRLTTY_ARG_REQUIRED(
   [drivers-directory], [DIRECTORY],
   [path to directory containing drivers],
   [drivers_directory], ["${libdir}/${PACKAGE_TARNAME}"]
)

BRLTTY_ARG_REQUIRED(
   [scripts-directory], [DIRECTORY],
   [path to directory containing helper scripts],
   [scripts_directory], ["${libexecdir}/${PACKAGE_TARNAME}"]
)

BRLTTY_ARG_WITH(
   [updatable-directory], [DIRECTORY],
   [path to directory which contains files that can be updated],
   [updatable_directory], ["yes"]
)
if test "${updatable_directory}" = "no"
then
   updatable_directory=""
elif test "${updatable_directory}" = "yes"
then
   updatable_directory="${localstatedir}/lib/${PACKAGE_TARNAME}"
fi
AC_SUBST([updatable_directory])
BRLTTY_SUMMARY_ITEM([updatable-directory], [updatable_directory])

BRLTTY_ARG_WITH(
   [writable-directory], [DIRECTORY],
   [path to directory which can be written to],
   [writable_directory], ["yes"]
)
if test "${writable_directory}" = "no"
then
   writable_directory=""
elif test "${writable_directory}" = "yes"
then
   writable_directory="/run/${PACKAGE_TARNAME}"
fi
AC_SUBST([writable_directory])
BRLTTY_SUMMARY_ITEM([writable-directory], [writable_directory])

BRLTTY_ARG_REQUIRED(
   [tables-directory], [DIRECTORY],
   [path to directory containing data files],
   [tables_directory], ["${sysconfdir}/${PACKAGE_TARNAME}"]
)

BRLTTY_DEFINE_STRING([TEXT_TABLES_SUBDIRECTORY], [Text], [the subdirectory for text tables])
BRLTTY_DEFINE_STRING([CONTRACTION_TABLES_SUBDIRECTORY], [Contraction], [the subdirectory for contraction tables])
BRLTTY_DEFINE_STRING([ATTRIBUTES_TABLES_SUBDIRECTORY], [Attributes], [the subdirectory for attributes tables])
BRLTTY_DEFINE_STRING([KEYBOARD_TABLES_SUBDIRECTORY], [Keyboard], [the subdirectory for keyboard key tables])
BRLTTY_DEFINE_STRING([INPUT_TABLES_SUBDIRECTORY], [Input], [the subdirectory for braille device key tables])

BRLTTY_DEFINE_STRING([TEXT_TABLE_EXTENSION], [.ttb], [the extension for text tables])
BRLTTY_DEFINE_STRING([TEXT_SUBTABLE_EXTENSION], [.tti], [the extension for text subtables])

BRLTTY_DEFINE_STRING([CONTRACTION_TABLE_EXTENSION], [.ctb], [the extension for contraction tables])
BRLTTY_DEFINE_STRING([CONTRACTION_SUBTABLE_EXTENSION], [.cti], [the extension for contraction subtables])

BRLTTY_DEFINE_STRING([ATTRIBUTES_TABLE_EXTENSION], [.atb], [the extension for attributes tables])
BRLTTY_DEFINE_STRING([ATTRIBUTES_SUBTABLE_EXTENSION], [.ati], [the extension for attributes subtables])

BRLTTY_DEFINE_STRING([KEY_TABLE_EXTENSION], [.ktb], [the extension for key tables])
BRLTTY_DEFINE_STRING([KEY_SUBTABLE_EXTENSION], [.kti], [the extension for key subtables])
BRLTTY_DEFINE_STRING([KEY_HELP_EXTENSION], [.txt], [the extension for key help files])

BRLTTY_DEFINE_STRING([PROFILES_SUBDIRECTORY], [Profiles], [the subdirectory for profiles])
BRLTTY_DEFINE_STRING([LANGUAGE_PROFILE_EXTENSION], [.lpf], [the extension for language profiles])

BRLTTY_ARG_REQUIRED(
   [manpage-directory], [DIRECTORY],
   [path to directory containing manual pages],
   [manpage_directory], ["${mandir}"]
)

BRLTTY_ARG_REQUIRED(
   [include-directory], [DIRECTORY],
   [path to directory containing header files],
   [include_directory], ["${includedir}/${PACKAGE_TARNAME}"]
)

brltty_default_init_path=/sbin/init_real
BRLTTY_ARG_WITH(
   [init-path], [FILE],
   [path to real init program],
   [init_path], [""]
)
if test "${init_path}" = "no"
then
   init_path=""
elif test "${init_path}" = "yes"
then
   init_path="${brltty_default_init_path}"
fi
if test -n "${init_path}"
then
   AC_DEFINE_UNQUOTED([INIT_PATH], ["${init_path}"],
                      [Define this if BRLTTY is to be run as init.])
fi
BRLTTY_SUMMARY_ITEM([init-path], [init_path])
AC_SUBST([init_path])

BRLTTY_ARG_PARAMETERS([privilege], [privilege establishment stage], [PLATFORM:], [])

brltty_default_stderr_path=/tmp/brltty.log
BRLTTY_ARG_WITH(
   [stderr-path], [FILE],
   [path to standard error log file],
   [stderr_path], [""]
)
if test "${stderr_path}" = "no"
then
   stderr_path=""
elif test "${stderr_path}" = "yes"
then
   stderr_path="${brltty_default_stderr_path}"
fi
if test -n "${stderr_path}"
then
   AC_DEFINE_UNQUOTED([STDERR_PATH], ["${stderr_path}"],
                      [Define this if standard error is to be redirected to a file.])
fi
BRLTTY_SUMMARY_ITEM([stderr-path], [stderr_path])
AC_SUBST([stderr_path])

AC_PROG_MAKE_SET

AC_PROG_CC
AC_PROG_CXX

brltty_gcc_cflags="-std=gnu99 -Wall"
brltty_gcc_cppflags=""

case "${host_os}"
in
   darwin*)
      brltty_gcc_cflags="${brltty_gcc_cflags} -Wno-initializer-overrides -Wno-unused-const-variable -Wno-pointer-bool-conversion"
      ;;
   elf*)
      brltty_gcc_cppflags="${brltty_gcc_cppflags} -ffreestanding -nostdinc -nostdlib"
      CPPFLAGS="${CPPFLAGS} -DGRUB_RUNTIME -DGRUB_FILE=__FILE__ -DNESTED_FUNC_ATTR="

      brltty_include_directory=`$CC -print-file-name=include`
      CPPFLAGS="${CPPFLAGS} -isystem $brltty_include_directory"

      brltty_grub_root="`pwd`/grub-root"
      test -d $brltty_grub_root || {
         AC_MSG_ERROR([symbolic link not found: $brltty_grub_root])
      }
      CPPFLAGS="${CPPFLAGS} -I$brltty_grub_root -I$brltty_grub_root/grub-core/lib/posix_wrap/include -I$brltty_grub_root/include"
      ;;
esac

test "${GCC}" = "yes" && {
   CFLAGS="${CFLAGS} ${brltty_gcc_cflags}"
   CPPFLAGS="${CPPFLAGS} ${brltty_gcc_cppflags}"

   extraCFlags=""
   for flag in unknown-warning-option
   do
      AX_CHECK_COMPILE_FLAG([-Werror=${flag}], [dnl
         extraCFlags="${extraCFlags} -Werror=${flag}"
      ])
   done

   # warnings that should be errors
   for flag in  format-security
   do
      AX_CHECK_COMPILE_FLAG([-Werror=${flag}], [dnl
         CFLAGS="${CFLAGS} -Werror=${flag}"
      ], [:], [${extraCFlags}])
   done

   # warnings that should be suppressed
   for flag in address-of-packed-member stringop-truncation initializer-overrides
   do
      AX_CHECK_COMPILE_FLAG([-Werror=${flag}], [dnl
         CFLAGS="${CFLAGS} -Wno-${flag}"
      ], [:], [${extraCFlags}])
   done

   AX_GCC_VAR_ATTRIBUTE([packed])
   AX_GCC_VAR_ATTRIBUTE([unused])

   AX_GCC_FUNC_ATTRIBUTE([format])
   AX_GCC_FUNC_ATTRIBUTE([format_arg])
   AX_GCC_FUNC_ATTRIBUTE([noreturn])
   AX_GCC_FUNC_ATTRIBUTE([unused])
}

test "${GXX}" = "yes" && {
   CXXFLAGS="${CXXFLAGS} ${brltty_gcc_cflags}"
}

case "${host_os}"
in
   linux*|gnu*|kfreebsd*)
      brltty_prog_cc_sysflags="-D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE"
      ;;
   solaris2.1?*) # Solaris 10 & 11 require _XOPEN_SOURCE=600 if using C99
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE=600 -D__EXTENSIONS__"
      ;;
   solaris*)
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__"
      ;;
   hpux*)
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE_EXTENDED -D_HPUX_SOURCE"
      ;;
   osf*)
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE -D_OSF_SOURCE"
      ;;
   cygwin*)
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE"
      ;;
   mingw*)
      brltty_prog_cc_sysflags="-D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE"
      ;;
   *)
      brltty_prog_cc_sysflags=""
      ;;
esac
CPPFLAGS="${CPPFLAGS} ${brltty_prog_cc_sysflags}"

if test "${GCC}" = "yes"
then
   case "${host_os}"
   in
      cygwin*|mingw*)
         LIBCFLAGS=""
         ;;
      msdos*)
         LIBCFLAGS=""
         ;;
      darwin*)
         LIBCFLAGS=""
         ;;
      *)
         LIBCFLAGS="-fPIC"
         ;;
   esac
else
   case "${host_os}"
   in
      *)
         LIBCFLAGS=""
         AC_MSG_WARN([library flags not configured for ${host_os}])
         ;;
   esac
fi
AC_SUBST([LIBCFLAGS])

if test "$GXX" = "yes"
then
   LIBCXXFLAGS="-fPIC"
else
   case "${host_os}"
   in
      *)
         LIBCXXFLAGS=""
         ;;
   esac
fi
AC_SUBST([LIBCXXFLAGS])

AC_PATH_TOOL([LD], [ld])
AC_PATH_TOOL([STRIP], [strip])
AC_PATH_TOOL([RANLIB], [ranlib])

AC_PROG_AWK
AC_PROG_LN_S

m4_ifdef([PKG_PROG_PKG_CONFIG], [dnl
   PKG_PROG_PKG_CONFIG
], [dnl
   AC_PATH_PROG([PKG_CONFIG], [pkg-config])
])

if test -z "${PKG_CONFIG}"
then
   PKG_CONFIG=false
fi

AC_PATH_PROG([MSGMERGE], [msgmerge], [false])
AC_PATH_PROG([MSGFMT], [msgfmt], [false])

AC_PATH_PROG([LINUXDOC], [linuxdoc], [false])
AC_PATH_PROG([DOXYGEN], [doxygen], [false])

case "${host_os}"
in
   cygwin*  ) LDFLAGS_DYNAMIC="--export-all-symbols --enable-auto-import";;
   darwin*  ) LDFLAGS_DYNAMIC="";;
   elf*     ) LDFLAGS_DYNAMIC="";;
   freebsd* ) LDFLAGS_DYNAMIC="-export-dynamic";;
   gnu*     ) LDFLAGS_DYNAMIC="-export-dynamic";;
   haiku*   ) LDFLAGS_DYNAMIC="-export-dynamic";;
   hpux*    ) LDFLAGS_DYNAMIC="-E +s";;
   kfreebsd*) LDFLAGS_DYNAMIC="-export-dynamic";;
   linux*   ) LDFLAGS_DYNAMIC="-export-dynamic";;
   mingw*   ) LDFLAGS_DYNAMIC="--export-all-symbols --enable-auto-import";;
   msdos*   ) LDFLAGS_DYNAMIC="";;
   netbsd*  ) LDFLAGS_DYNAMIC="-export-dynamic";;
   openbsd* ) LDFLAGS_DYNAMIC="-export-dynamic";;
   osf*     ) LDFLAGS_DYNAMIC="";;
   *qnx*    ) LDFLAGS_DYNAMIC="-export-dynamic";;
   solaris* ) LDFLAGS_DYNAMIC="";;
   *) AC_MSG_ERROR([dynamic linkage not supported for ${host_os}]);;
esac
LDFLAGS_DYNAMIC="BRLTTY_OPTIONS_LD2CC([${LDFLAGS_DYNAMIC}])"

if test "${GCC}" = "yes"
then
   LDFLAGS_STATIC="-static"
else
   case "${host_os}"
   in
      cygwin*  ) LDFLAGS_STATIC="-static";;
      elf*     ) LDFLAGS_STATIC="-static";;
      freebsd* ) LDFLAGS_STATIC="-static";;
      gnu*     ) LDFLAGS_STATIC="-static";;
      hpux*    ) LDFLAGS_STATIC="-Bstatic";;
      kfreebsd*) LDFLAGS_STATIC="-static";;
      linux*   ) LDFLAGS_STATIC="-static";;
      mingw*   ) LDFLAGS_STATIC="-static";;
      netbsd*  ) LDFLAGS_STATIC="-Bstatic";;
      openbsd* ) LDFLAGS_STATIC="-static";;
      osf*     ) LDFLAGS_STATIC="-non_shared";;
      *qnx*    ) LDFLAGS_STATIC="-static";;
      solaris* ) LDFLAGS_STATIC="-dn";;
      *) AC_MSG_ERROR([static linkage not supported for ${host_os}]);;
   esac
   LDFLAGS_STATIC="BRLTTY_OPTIONS_LD2CC([${LDFLAGS_STATIC}])"
fi

pkgconfig_flags_libs="--libs"

BRLTTY_ARG_ENABLE(
   [standalone-programs],
   [statically linked executables],
   [],
[dnl
   LDFLAGS="${LDFLAGS} ${LDFLAGS_STATIC}"
   brltty_standalone_programs="yes"
   pkgconfig_flags_libs="${pkgconfig_flags_libs} --static"
], [dnl
   LDFLAGS="${LDFLAGS} ${LDFLAGS_DYNAMIC}"

   AC_DEFINE([ENABLE_SHARED_OBJECTS], [1],
             [Define this if shared object support is to be included.])
])

AC_PROG_INSTALL
BRLTTY_EXECUTABLE_PATH([INSTALL])

test "${LINUXDOC}" = "false" && can_make_manual=no || can_make_manual=yes
AC_SUBST([can_make_manual])

test "${DOXYGEN}" = "false" && can_make_BrlAPIref=no || can_make_BrlAPIref=yes
AC_SUBST([can_make_BrlAPIref])

AC_CACHE_CHECK([for shared object creation command], [brltty_cv_prog_make_object_shared], [dnl
   case "${host_os}"
   in
      linux*|gnu*|openbsd*|freebsd*|kfreebsd*|netbsd*|*qnx*|cygwin*|mingw*|msdos*|elf*|haiku*)
         brltty_mkshr_ld_make="-shared"
         ;;
      solaris*)
         brltty_mkshr_ld_make="-G"
         ;;
      hpux*)
         brltty_mkshr_ld_make="-b"
         ;;
      osf*)
         brltty_mkshr_ld_make="-shared"
         ;;
      darwin*)
         brltty_mkshr_ld_make="-bundle"
         brltty_mkshr_ld_options="-flat_namespace -undefined suppress"
         brltty_mkshr_gcc_make="-bundle"
         ;;
      *)
         AC_MSG_ERROR([loadable module creation command not configured for ${host_os}])
         brltty_mkshr_ld_make="NOT_SUPPORTED"
         brltty_mkshr_ld_options="NOT_SUPPORTED"
         brltty_mkshr_gcc_make="NOT_SUPPORTED"
         ;;
   esac

   if test "${GCC}" = "yes"
   then
      brltty_cv_prog_make_object_shared="\$(CC) ${brltty_mkshr_gcc_make=-shared} BRLTTY_OPTIONS_LD2CC([${brltty_mkshr_ld_options}])"
   else
      brltty_cv_prog_make_object_shared="\$(LD) ${brltty_mkshr_ld_make} ${brltty_mkshr_ld_options}"
   fi

   brltty_cv_prog_make_object_shared="${brltty_cv_prog_make_object_shared} ${LDFLAGS} -o"
])
AC_SUBST([MKSHR], ["${brltty_cv_prog_make_object_shared}"])

AC_CACHE_CHECK([for make relocatable object command], [brltty_cv_prog_make_object_relocatable], [dnl
case "${host_os}"
in
   *)
      brltty_cv_prog_make_object_relocatable="\$(LD) -r -o"
      ;;
esac])
MKREL="${brltty_cv_prog_make_object_relocatable}"
AC_SUBST([MKREL])

AC_CACHE_CHECK([for dynamic library creation command], [brltty_cv_prog_mklib], [dnl
case "${host_os}"
in
   linux*|gnu*|openbsd*|freebsd*|kfreebsd*|netbsd*|*qnx*|elf*|haiku*)
      brltty_mklib_ld_make="-shared"
      brltty_mklib_ld_options="-soname"
      ;;
   solaris*)
      brltty_mklib_ld_make="-G"
      brltty_mklib_ld_options="-h"
      ;;
   hpux*)
      brltty_mklib_ld_make="-b"
      brltty_mklib_ld_options="+h"
      ;;
   osf*)
      brltty_mklib_ld_make="-shared"
      brltty_mklib_ld_options="-expect_unresolved '*' -soname"
      ;;
   cygwin*|mingw*)
      brltty_mklib_ld_make="-shared"
      brltty_mklib_ld_options="--out-implib implib.a --output-def lib.def -soname"
      ;;
   msdos*)
      brltty_mklib_ld_make="not_supported_on_msdos"
      brltty_mklib_ld_options="not_supported_on_msdos"
      brltty_mklib_gcc_make="not_supported_on_msdos"
      ;;
   darwin*)
      dylibFound=false
      dylibPath=/usr/lib/dylib1.o
      set -- "${dylibPath}"

      sdkRootPrefix="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX"
      sdkRootSuffix=".sdk/${dylibPath}"
      sdkVersion=`sw_vers -productVersion`

      while true
      do
         set -- "${@}" "${sdkRootPrefix}${sdkVersion}${sdkRootSuffix}"
         sdkNext="${sdkVersion%.*}"
         test "${sdkNext}" != "${sdkVersion}" || break
         sdkVersion="${sdkNext}"
      done

      set -- "${@}" "${sdkRootPrefix}"*"${sdkRootSuffix}"
      for sdkPath
      do
         test ! -f "${sdkPath}" || {
            dylibPath="${sdkPath}"
            dylibFound=true
            break
         }
      done
      "${dylibFound}" || AC_MSG_WARN([cannot find ${dylibPath}])

      brltty_mklib_ld_make="-dylib"
      brltty_mklib_ld_options="${dylibPath} -single_module -install_name"
      brltty_mklib_gcc_make="-dynamiclib"
      ;;
   *)
      AC_MSG_ERROR([dynamic library creation command not configured for ${host_os}])
      ;;
esac
if test "${GCC}" = "yes"
then
   brltty_cv_prog_mklib="\$(CC) \$(LDFLAGS) ${brltty_mklib_gcc_make=-shared} BRLTTY_OPTIONS_LD2CC([${brltty_mklib_ld_options+${brltty_mklib_ld_options} <name>}]) -o"
else
   brltty_cv_prog_mklib="\$(LD) \$(LDFLAGS) ${brltty_mklib_ld_make} ${brltty_mklib_ld_options+${brltty_mklib_ld_options} <name>} -o"
fi])
AC_SUBST([MKLIB], ["${brltty_cv_prog_mklib}"])

AC_CACHE_CHECK([for configure shared object directory command], [brltty_cv_prog_conflibdir], [dnl
case "${host_os}"
in
   linux*|gnu*|kfreebsd*|elf*)
      brltty_cv_prog_conflibdir="/sbin/ldconfig -n"
      ;;
   *)
      brltty_cv_prog_conflibdir=":"
      ;;
esac])
AC_SUBST([CONFLIBDIR], ["${brltty_cv_prog_conflibdir}"])

case "${host_os}"
in
   mingw*)
      BRLTTY_HAVE_WINDOWS_LIBRARY([ws2_32])
      BRLTTY_HAVE_WINDOWS_LIBRARY([gdi32])
      BRLTTY_HAVE_WINDOWS_LIBRARY([winmm])
      ;;
   cygwin*)
      BRLTTY_HAVE_WINDOWS_LIBRARY([gdi32])
      ;;
   *)
      AC_SEARCH_LIBS([socket], [socket network])
      AC_SEARCH_LIBS([inet_ntoa], [nsl])
      ;;
esac

install_messages=
BRLTTY_ARG_DISABLE(
   [i18n],
   [support for internationalization],
   [],
[dnl
   test "${MSGMERGE}" = "false" || {
      install_messages=install-messages

      AC_CHECK_HEADER([libintl.h], [dnl
         AC_HAVE_LIBRARY([intl])
         AC_DEFINE([ENABLE_I18N_SUPPORT], [1],
                   [Define this if internationalization support is to be included.])
      ])
   }
])
AC_SUBST([install_messages])

AC_CHECK_FUNC([wcslen], [AC_CHECK_HEADERS([wchar.h], [dnl
   AC_CHECK_FUNCS([wcwidth])
])])

AC_CHECK_HEADERS([glob.h], [dnl
   AC_CHECK_FUNCS([glob])
])

AC_CHECK_HEADERS([langinfo.h], [dnl
   AC_CHECK_FUNCS([nl_langinfo])
])

icu_includes=""
icu_libs=""
BRLTTY_ARG_DISABLE(
   [icu],
   [support for Unicode-based internationalization],
   [],
[dnl
   BRLTTY_HAVE_PACKAGE([icu], [icu-uc], [dnl
      AC_CHECK_HEADERS([unicode/unorm2.h])
      AC_DEFINE_UNQUOTED([HAVE_ICU], [1],
                         [Define this if Unicode-based internationalization support is to be included.])

      if test "${brltty_standalone_programs}" = "yes"
      then
         icu_libs="`echo ${icu_libs} | sed -e 's/\(-l\)\(icu\)/\1s\2/g'`"
      fi
   ])
])
AC_SUBST([icu_includes])
AC_SUBST([icu_libs])

BRLTTY_ARG_DISABLE(
   [iconv],
   [support for character encoding conversion],
   [],
[dnl
   AC_CHECK_HEADERS([iconv.h], [
      AC_HAVE_LIBRARY([iconv])
   ])
])

BRLTTY_ARG_DISABLE(
   [polkit],
   [support for the Polkit authorization manager],
   [],
[dnl
   BRLTTY_HAVE_PACKAGE([polkit], [polkit-gobject-1], [dnl
      AC_DEFINE(HAVE_POLKIT, 1, [if the Polkit authorization manager should be used])
   ])
])

BRLTTY_ARG_DISABLE(
   [api],
   [the application programming interface],
   [],
[dnl
   BRLTTY_IF_PTHREADS([dnl
      AC_DEFINE([ENABLE_API], [1],
                [Define this if the application programming interface is to be included.])
      BRLTTY_ARG_PARAMETERS([api], [application programming interface])

      api_server_objects='brlapi_server.$O brlapi_keyranges.$O auth.$O'
      api_libraries="${LIBS}"

      all_api="all-api"
      build_api="api"
      install_api="install-api"
      api_ref='$(API_LIBS)'

      api_dynamic_library="api-dynamic-library"
      install_api_libraries="install-api-libraries"
      uninstall_api_libraries="uninstall-api-libraries"
      default_api_socket_path="${localstatedir}/lib/BrlAPI"

      case "${host_os}"
      in
         cygwin*|mingw*)
            AC_DEFINE([BRLAPI_WIN32], [1],
                      [Define this if the Win32 programming interface is available.])
            default_api_socket_path='\\\\.\\pipe\\BrlAPI'
            api_dynamic_library="api-dynamic-library-windows"
            install_api_libraries="install-api-libraries-windows"
            uninstall_api_libraries="uninstall-api-libraries-windows"
            ;;
      esac

      BRLTTY_ARG_WITH(
          [api-socket-path], [PATH],
          [where local sockets for teh api are stored],
          [api_socket_path], ["${default_api_socket_path}"]
      )

      BRLTTY_BINDINGS([Java])
      BRLTTY_BINDINGS([Lisp])
      BRLTTY_BINDINGS([OCaml])
      BRLTTY_BINDINGS([Python])
      BRLTTY_BINDINGS([Tcl])

      test -n "${api_bindings}" && {
         for brltty_bindings_directory in ${api_bindings}
         do
            brltty_build_directories="${brltty_build_directories} Bindings/${brltty_bindings_directory}"
         done

         all_api_bindings="all-api-bindings"
         install_api_bindings="install-api-bindings"
      }
   ])
])
AC_SUBST([api_server_objects])
AC_SUBST([api_libraries])
AC_SUBST([api_bindings])
AC_SUBST([all_api_bindings])
AC_SUBST([install_api_bindings])
AC_SUBST([all_api])
AC_SUBST([build_api])
AC_SUBST([install_api])
AC_SUBST([api_ref])
AC_SUBST([api_dynamic_library])
AC_SUBST([install_api_libraries])
AC_SUBST([uninstall_api_libraries])
AC_SUBST([api_authkeyfile], [brlapi.key])
AC_SUBST([api_socket_path])
BRLTTY_SUMMARY_ITEM([api-socket-path], [api_socket_path])
BRLTTY_DEFINE_EXPANDED([BRLAPI_AUTHKEYFILE], ["${api_authkeyfile}"],
                       [Define this to be a string containing the name of BrlAPI's authorization key file.])

brltty_curses_libs_save="${LIBS}"
BRLTTY_PACKAGE_CHOOSE([curses],
   [ncursesw ncursesw/ncurses.h],
   [pdcursesw curses.h],
   [ncurses ncurses.h],
   [pdcursesu curses.h],
   [curses curses.h],
   [pdcurses curses.h])
if test -n "${curses_package}"
then
   AC_CHECK_LIB([tinfo], [intrflush])
fi
curses_libs="${LIBS%${brltty_curses_libs_save}}"
LIBS="${brltty_curses_libs_save}"
AC_SUBST([curses_libs])

xkb_libs=""
xtk_libs=""
xsel_object=""
all_xbrlapi=""
install_xbrlapi=""
BRLTTY_ARG_DISABLE(
   [x],
   [support for X11],
   [],
[dnl
   BRLTTY_HAVE_PACKAGE([x11], [x11], [dnl
      all_xbrlapi="all-xbrlapi"
      install_xbrlapi="install-xbrlapi"
      xsel_object='xsel.$O'

      AC_CHECK_HEADERS([X11/keysym.h])

      BRLTTY_HAVE_PACKAGE([xext], [xext], [dnl
         xkb_libs="${xext_libs} ${xkb_libs}"

         BRLTTY_HAVE_PACKAGE([xtst], [xtst], [dnl
            xkb_libs="${xtst_libs} ${xkb_libs}"
             
            AC_CHECK_HEADERS([X11/extensions/XTest.h])
            AC_CHECK_HEADERS([X11/extensions/XKB.h])
         ])

         BRLTTY_HAVE_PACKAGE([xfixes], [xfixes], [dnl
            AC_CHECK_HEADERS([X11/extensions/Xfixes.h])
         ])
      ])

      BRLTTY_HAVE_PACKAGE([xt], [xt], [dnl
         xtk_libs="${xt_libs} ${xtk_libs}"

         brltty_cppflags_save="${CPPFLAGS}"
         CPPFLAGS="${CPPFLAGS} ${x11_includes}"

         brltty_libs_save="${LIBS}"
         LIBS="${xtk_libs} ${x11_libs} ${LIBS}"

         BRLTTY_PACKAGE_CHOOSE([gui_toolkit],
            [Xaw X11/Xaw/Form.h X11/Xaw/Paned.h X11/Xaw/Label.h X11/Xaw/Command.h X11/Xaw/Repeater.h],
            [Xaw3d X11/Xaw3d/Form.h X11/Xaw3d/Paned.h X11/Xaw3d/Label.h X11/Xaw3d/Command.h X11/Xaw3d/Repeater.h],
            [neXtaw X11/neXtaw/Form.h X11/neXtaw/Paned.h X11/neXtaw/Label.h X11/neXtaw/Command.h X11/neXtaw/Repeater.h],
            [XawPlus X11/XawPlus/Form.h X11/XawPlus/Paned.h X11/XawPlus/Label.h X11/XawPlus/Command.h X11/XawPlus/Repeater.h],
            [Xm Xm/Xm.h Xm/Form.h Xm/PanedW.h Xm/Label.h Xm/PushB.h])

         xtk_libs="${LIBS%${x11_libs} ${brltty_libs_save}}"

         CPPFLAGS="${brltty_cppflags_save}"
         LIBS="${brltty_libs_save}"
      ])
   ])
])
AC_SUBST([xkb_libs])
AC_SUBST([xtk_libs])
AC_SUBST([xsel_object])
AC_SUBST([all_xbrlapi])
AC_SUBST([install_xbrlapi])

test -z "$gui_toolkit_package" && {
   case "${host_os}"
   in
      cygwin*|mingw*)
         gui_toolkit_package="windows"
         ;;
   esac
}

AC_CHECK_FUNCS([time stime gettimeofday settimeofday nanosleep])
BRLTTY_CHECK_FUNCTION([clock_gettime], [time.h], [rt])
BRLTTY_CHECK_FUNCTION([clock_settime], [time.h], [rt])

BRLTTY_CHECK_FUNCTION([pthread_getname_np], [pthread.h], [pthread])
BRLTTY_CHECK_FUNCTION([pthread_atfork], [pthread.h], [pthread])

AC_CHECK_DECLS([localtime_r], [], [], [dnl
#include <time.h>
])

AC_CHECK_HEADERS([sys/poll.h sys/select.h sys/wait.h])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([poll])

AC_CHECK_HEADERS([sys/capability.h sys/prctl.h sched.h])
AC_CHECK_HEADERS([linux/seccomp.h linux/filter.h linux/audit.h])

AC_CHECK_HEADERS([signal.h sys/signalfd.h])
AC_CHECK_FUNCS([sigaction])

AC_CHECK_HEADERS([alloca.h getopt.h regex.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([sys/file.h sys/socket.h])
AC_CHECK_HEADERS([pwd.h grp.h])
AC_CHECK_HEADERS([sys/io.h sys/modem.h machine/speaker.h dev/speaker/speaker.h linux/vt.h])
AC_CHECK_HEADERS([sdkddkver.h])

AC_CHECK_HEADERS([execinfo.h], [AC_HAVE_LIBRARY([execinfo])])

AC_CHECK_HEADERS([linux/input.h], [
   AC_CHECK_HEADERS([linux/uinput.h], [], [], [
#include <linux/input.h>
])])

AC_CHECK_FUNCS([getopt_long hstrerror vsyslog])
AC_CHECK_FUNCS([realpath readlink])
AC_CHECK_FUNCS([pause])
AC_CHECK_FUNCS([fchdir fchmod])
AC_CHECK_FUNCS([shmget shm_open])
AC_CHECK_FUNCS([getpeereid getpeerucred getzoneid])
AC_CHECK_FUNCS([mempcpy wmempcpy])

case "${host_os}"
in
   cygwin*|mingw*)
      if test "${brltty_cv_dll_ws2_32}" = "yes"
      then
         BRLTTY_HAVE_WINDOWS_FUNCTION([getaddrinfo], [ws2_32])
         BRLTTY_HAVE_WINDOWS_FUNCTION([getnameinfo], [ws2_32])
         AC_CHECK_FUNCS([gai_strerror])
      fi
      AC_CHECK_TYPES([PROCESS_INFORMATION_CLASS], [], [], [
#include <windows.h>
])
      AC_CHECK_DECLS([ProcessUserModeIOPL], [], [], [
#include <windows.h>
])
      ;;
   *)
      AC_CHECK_FUNCS([gai_strerror getaddrinfo getnameinfo])
      ;;
esac

AC_CHECK_SIZEOF([wchar_t], [], [
#include <wchar.h>
])
AC_DEFINE_UNQUOTED([SIZEOF_WCHAR_T_STR], ["${ac_cv_sizeof_wchar_t}"],
                   [Define this to be a string containing the size of the wchar_t type.])

BRLTTY_IF_PTHREADS([dnl
   AC_DEFINE([HAVE_POSIX_THREADS], [1],
             [Define this if posix threads are supported.])
])

AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_OBJEXT
AC_EXEEXT

AC_CACHE_CHECK(
   [if the compiler understands the __alignof__ operator],
   [brltty_cv_c_operator_alignof],
   [AC_TRY_COMPILE([
int a;
int b = __alignof__(a);
],
[],
[brltty_cv_c_operator_alignof=yes],
[brltty_cv_c_operator_alignof=no])])
if test "${brltty_cv_c_operator_alignof}" = "yes"
then
   AC_DEFINE([HAVE_OPERATOR_ALIGNOF],  [1],
             [Define this if the __alignof__ operator is supported.])
fi

AC_CACHE_CHECK(
   [if the compiler supports thread-local variables],
   [brltty_cv_c_thread_local],
   [
      brltty_cv_c_thread_local=""
      for thread_local_qualifier in "_Thread_local" "__thread" "__declspec(thread)"
      do
         AC_TRY_COMPILE([
static ${thread_local_qualifier} int i;
],
            [],
            [brltty_cv_c_thread_local="${thread_local_qualifier}"; break])
      done
   ]
)
if test -n "${brltty_cv_c_thread_local}"
then
   AC_DEFINE_UNQUOTED([THREAD_LOCAL], [${brltty_cv_c_thread_local}],
                      [Define this if thread-local variables are supported.])
fi

AC_SUBST([archive_prefix], [lib])
AC_SUBST([archive_extension], [a])

case "${host_os}"
in
   cygwin*)
      library_prefix="cyg"
      ;;
   mingw*)
      library_prefix=""
      ;;
   *)
      library_prefix="${archive_prefix}"
      ;;
esac
AC_SUBST([library_prefix])

case "${host_os}"
in
   hpux*)
      library_extension="sl"
      ;;
   cygwin*|mingw*)
      library_extension="dll"
      ;;
   darwin*)
      library_extension="dylib"
      ;;
   *)
      library_extension="so"
      ;;
esac
AC_SUBST([library_extension])
AC_DEFINE_UNQUOTED([LIBRARY_EXTENSION], ["${library_extension}"],
                   [Define this to be a string containing the library extension.])

AC_SUBST([module_name], ["${library_prefix}${PACKAGE_TARNAME}"])
AC_DEFINE_UNQUOTED([MODULE_NAME], ["${module_name}"],
                   [Define this to be a string containing the base module name.])

case "${host_os}"
in
   darwin*)
      module_extension="bundle"
      ;;
   *)
      module_extension="${library_extension}"
      ;;
esac
AC_SUBST([module_extension])
AC_DEFINE_UNQUOTED([MODULE_EXTENSION], ["${module_extension}"],
                   [Define this to be a string containing the module extension.])

AC_CACHE_CHECK([for static archive extension], [brltty_cv_ext_arc], [dnl
case "${host_os}"
in
   *)
      brltty_cv_ext_arc="a"
      ;;
esac])
archive_extension="${brltty_cv_ext_arc}"
AC_SUBST([archive_extension])

AC_CACHE_CHECK([for shared object run-time search path environment variable], [brltty_cv_env_libsearch], [dnl
case "${host_os}"
in
   hpux*)
      brltty_cv_env_libsearch="SHLIB_PATH"
      ;;
   darwin*)
      brltty_cv_env_libsearch="DYLD_LIBRARY_PATH"
      ;;
   *)
      brltty_cv_env_libsearch="LD_LIBRARY_PATH"
      ;;
esac])
libsearch_variable="${brltty_cv_env_libsearch}"
AC_SUBST([libsearch_variable])

BRLTTY_PKGCONFIG_VARIABLE([POLKIT_POLICY_DIR], [polkit-gobject-1], [policydir], [/usr/share/polkit-1/actions])
BRLTTY_PKGCONFIG_VARIABLE([POLKIT_RULE_DIR], [polkit-gobject-1], [ruledir], [/usr/share/polkit-1/rules.d])
BRLTTY_PKGCONFIG_VARIABLE([SYSTEMD_SYSUSERS_DIR], [systemd], [sysusersdir], [/usr/lib/sysusers.d])
BRLTTY_PKGCONFIG_VARIABLE([SYSTEMD_TMPFILES_DIR], [systemd], [tmpfilesdir], [/usr/lib/tmpfiles.d])
BRLTTY_PKGCONFIG_VARIABLE([SYSTEMD_UNIT_DIR], [systemd], [systemdsystemunitdir], [/usr/lib/systemd/system])
BRLTTY_PKGCONFIG_VARIABLE([UDEV_DIR], [udev], [udevdir], [/usr/lib/udev])

BRLTTY_ARG_PACKAGE([system], [base system], [], [dnl
   *android*)
      system_package="java"
      system_libs="-llog"
      ;;
   *qnx*)
      AC_CHECK_LIB([asound], [snd_pcm_open])
      ;;
   cygwin*)
      system_package="windows"
      ;;
   darwin*)
      system_package="darwin"
      system_includes="-x objective-c"
      system_libs="-framework IOKit -framework Foundation -framework CoreFoundation -lobjc"
      ;;
   hpux*)
      if test -d /opt/audio/include
      then
         CPPFLAGS="${CPPFLAGS} -I/opt/audio/include"
         system_libs="-L/opt/audio/lib -lAlib"
         AC_DEFINE([HAVE_HPUX_AUDIO], [1],
                   [Define this if HP-UX audio support is available.])
      fi
      ;;
   linux*)
      system_package="linux"
      ;;
   mingw*)
      system_package="windows"
      ;;
   msdos*)
      system_package="msdos"
      system_libs="-ldbg"
      ;;
])

expat_includes=""
expat_libs=""
BRLTTY_ARG_DISABLE(
   [expat],
   [support for XML parsing (CLDR)],
   [],
[dnl
   BRLTTY_HAVE_PACKAGE([expat], [expat], [dnl
      AC_DEFINE_UNQUOTED([HAVE_EXPAT], [1],
                         [Define this if XML parsing support is to be included.])
   ])
])
AC_SUBST([expat_includes])
AC_SUBST([expat_libs])

contracted_braille_objects='ctb_compile.$O cldr.$O ctb_translate.$O ctb_native.$O ctb_external.$O'
louis_includes=""
louis_libs=""
BRLTTY_ARG_DISABLE(
   [liblouis],
   [support for LibLouis-based contracted braille],
   [],
[dnl
   BRLTTY_HAVE_PACKAGE([louis], [liblouis], [dnl
      louisTablesDirectory=`${PKG_CONFIG} --variable=tablesdir ${louis_package}`
      if test -n "${louisTablesDirectory}"
      then
         AC_SUBST([louis_tables_directory], ["${louisTablesDirectory}"])
         AC_DEFINE_UNQUOTED([LOUIS_TABLES_DIRECTORY], ["${louisTablesDirectory}"],
                            [Define this to be a string containing the path to the LibLouis tables directory.])
         contracted_braille_objects="${contracted_braille_objects} ctb_louis.\$O"
      fi
   ])
])
AC_SUBST([louis_includes])
AC_SUBST([louis_libs])
AC_SUBST([contracted_braille_objects])

BRLTTY_ARG_DISABLE(
   [gpm],
   [mouse tracking via GPM],
   [],
[dnl
   AC_CHECK_LIB([gpm], [Gpm_Open])
])

case "${host_os}"
in
   hpux*)
      BRLTTY_SEARCH_LIBS([shl_load], [dld])
      ;;
   *)
      ;;
esac

BRLTTY_ARG_PACKAGE([pgmpath], [program path], [], [dnl
   cygwin*)
      pgmpath_package="windows"
      ;;
   freebsd*)
      pgmpath_package="freebsd"
      ;;
   linux*)
      pgmpath_package="linux"
      ;;
   gnu*)
      pgmpath_package="hurd"
      ;;
   mingw*)
      pgmpath_package="windows"
      ;;
   solaris*)
      pgmpath_package="solaris"
      ;;
])

BRLTTY_ARG_PACKAGE([pgmprivs], [program privileges], [], [dnl
   linux*)
      pgmprivs_package="linux"
      AC_HAVE_LIBRARY([cap])
      ;;
])

BRLTTY_ARG_PACKAGE([service], [system service], [libsystemd], [dnl
   mingw*)
      service_package="windows"
      ;;
])

AC_SUBST([SYSTEMD_SERVICE_TYPE], ["forking"])
AC_SUBST([SYSTEMD_SERVICE_ARGUMENTS], [""])

case "${service_package}"
in
   libsystemd)
      SYSTEMD_SERVICE_TYPE="notify"
      SYSTEMD_SERVICE_ARGUMENTS="-n"
      ;;
esac

BRLTTY_HAVE_PACKAGE([systemd], [libsystemd], [
   brltty_libs_save="${LIBS}"
   LIBS="${LIBS} ${systemd_libs}"
   AC_CHECK_FUNC([sd_session_get_vt], [dnl
      AC_DEFINE([HAVE_SD_SESSION_GET_VT], [1],
                [Define this if the function sd_session_get_vt is available in libsystemd.])
      api_libraries="${api_libraries} ${systemd_libs}"
   ], [dnl
      AC_MSG_WARN([function not available: systemd:sd_session_get_vt()])
   ])
   LIBS="${brltty_libs_save}"
], [:])

BRLTTY_ARG_PACKAGE([params], [boot parameters], [], [dnl
   linux*)
      params_package="linux"
      ;;
])

BRLTTY_ARG_PACKAGE([dynld], [dynamic loading], [dlfcn dyld], [dnl
   elf*)
      dynld_package="grub"
      ;;
   hpux*)
      dynld_package="shl"
      ;;
   mingw*)
      dynld_package="windows"
      ;;
], [dnl
   dlfcn)
      AC_CHECK_HEADERS([dlfcn.h], [
         AC_SEARCH_LIBS([dlopen], [dl], [dnl
            dynld_package="dlfcn"
         ])
      ])
      ;;
   dyld)
      AC_CHECK_HEADERS([mach-o/dyld.h], [
         dynld_package="dyld"
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([rgx], [regular expression], [libpcre2-32 libpcre32], [dnl
])

BRLTTY_ARG_PACKAGE([charset], [character set], [iconv], [dnl
   elf*)
      charset_package=grub
      ;;
   mingw*)
      charset_package=windows
      ;;
   msdos*)
      charset_package=msdos
      ;;
], [dnl
   iconv)
      if test "${brltty_enabled_iconv}" = "yes"
      then
         AC_CHECK_HEADER([iconv.h], [dnl
            charset_package=iconv
         ])
      fi
      ;;
])

BRLTTY_ARG_PACKAGE([hostcmd], [host command], [unix], [dnl
   mingw*)
      hostcmd_package=windows
      ;;
], [dnl
   unix)
      AC_CHECK_DECL([fork], [dnl
         AC_CHECK_HEADER([signal.h], [dnl
            AC_CHECK_DECL([SIGCHLD], [dnl
               hostcmd_package=unix
            ], [], [dnl
#include <signal.h>
            ])
         ])
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([mntpt], [mount point], [mntent mnttab], [dnl
], [dnl
   mntent)
      AC_CHECK_HEADER([mntent.h], [dnl
         AC_CHECK_FUNC([setmntent], [dnl
            mntpt_package=mntent
            AC_CHECK_FUNCS([addmntent])
         ])
      ])
      ;;
   mnttab)
      AC_CHECK_HEADER([sys/mnttab.h], [dnl
         mntpt_package=mnttab
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([mntfs], [mount file system], [], [dnl
   linux*)
      mntfs_package="linux"
      ;;
])

BRLTTY_ARG_PACKAGE([kbd], [keyboard], [], [dnl
   *android*) kbd_package="android";;
   linux*) kbd_package="linux";;
])

BRLTTY_ARG_PACKAGE([bell], [console bell], [], [dnl
   linux*)
      bell_package="linux"
      ;;
])

BRLTTY_ARG_PACKAGE([leds], [keyboard LEDs], [], [dnl
   linux*)
      leds_package="linux"
      ;;
])

BRLTTY_ARG_PACKAGE([beep], [beeper], [], [dnl
   *android*)
      beep_package="none"
      ;;
   cygwin*)
      beep_package="windows"
      ;;
   freebsd*)
      beep_package="spkr"
      ;;
   kfreebsd*)
      beep_package="spkr"
      ;;
   linux*)
      beep_package="linux"
      ;;
   mingw*)
      beep_package="windows"
      ;;
   msdos*)
      beep_package="msdos"
      ;;
   netbsd*)
      beep_package="wskbd"
      ;;
   openbsd*)
      beep_package="wskbd"
      ;;
   solaris*)
      beep_package="solaris"
      ;;
])

BRLTTY_ARG_PACKAGE([pcm], [PCm], [alsa oss], [dnl
   *android*)
      pcm_package="android"
      ;;
   *qnx*)
      pcm_package="qsa"
      ;;
   hpux*)
      pcm_package="hpux"
      ;;
   mingw*)
      pcm_package="windows"
      ;;
   msdos*)
      pcm_package="none"
      ;;
   netbsd*)
      pcm_package="audio"
      ;;
   openbsd*)
      pcm_package="audio"
      ;;
   solaris*)
      pcm_package="audio"
      ;;
], [dnl
   oss)
      AC_CHECK_HEADER([sys/soundcard.h], [dnl
         pcm_package="oss"
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([midi], [MIDI], [alsa oss], [dnl
   darwin*)
      midi_package="darwin"
      midi_libs="-framework AudioUnit -framework AudioToolbox"
      ;;
   mingw*)
      midi_package="windows"
      ;;
   msdos*)
      midi_package="none"
      ;;
], [dnl
   oss)
      AC_CHECK_HEADER([sys/soundcard.h], [dnl
         midi_package="oss"
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([fm], [FM], [adlib], [dnl
], [dnl
   adlib)
      fm_package="adlib"
      ;;
])

BRLTTY_ARG_PACKAGE([serial], [serial I/O], [termios], [dnl
   elf*)
      serial_package="grub"
      ;;
   mingw*)
      serial_package="windows"
      ;;
   msdos*)
      serial_package="msdos"
      ;;
], [dnl
   termios)
      AC_CHECK_HEADER([termios.h], [dnl
         serial_package="termios"
         AC_CHECK_FUNCS([tcdrain])
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([usb], [USB I/O], [winusb libusb-1.0 libusb], [dnl
   *android*)
      usb_package="android"
      ;;
   darwin*)
      usb_package="darwin"
      usb_includes="-x objective-c"
      ;;
   elf*)
      usb_package="grub"
      ;;
   freebsd*)
      usb_package="freebsd"
      ;;
   kfreebsd*)
      usb_package="kfreebsd"
      AC_CHECK_HEADERS([legacy/dev/usb/usb.h])
      ;;
   linux*)
      usb_package="linux"
      ;;
   netbsd*)
      usb_package="netbsd"
      ;;
   openbsd*)
      usb_package="openbsd"
      ;;
   solaris*)
      usb_package="solaris"
      AC_HAVE_LIBRARY([aio])
      AC_HAVE_LIBRARY([rt])
      ;;
], [dnl
   winusb)
      AC_CHECK_HEADER([winusb.h], [dnl
         usb_package="winusb"
         usb_libs="-lwinusb"
      ], [], [-])
      ;;
   libusb)
      AC_CHECK_HEADER([usb.h], [dnl
         usb_package="libusb"
         usb_libs="-lusb"
      ], [], [-])
      ;;
])

BRLTTY_ARG_PACKAGE([bluetooth], [Bluetooth I/O], [], [dnl
   *android*)
      bluetooth_package="android"
      ;;
   darwin*)
      bluetooth_package="darwin"
      bluetooth_includes="-x objective-c"
      bluetooth_libs="-framework IOBluetooth"
      ;;
   cygwin*|mingw*)
      AC_CHECK_HEADER([ws2bth.h], [dnl
         bluetooth_package="windows"
         bluetooth_libs="-lbthprops -lws2_32"
      ], , [[#include <windows.h>]])
      ;;
   linux*)
      AC_CHECK_HEADER([bluetooth/bluetooth.h], [dnl
         bluetooth_package="linux"
         AC_HAVE_LIBRARY([bluetooth])

         BRLTTY_IF_DBUS([dnl
            bluetooth_includes="${dbus_includes}"
            bluetooth_libs="${dbus_libs}"
         ])
      ])
      ;;
])

BRLTTY_ARG_PACKAGE([ports], [I/O ports], [], [dnl
   cygwin*)
      ports_package="windows"
      ;;
   elf*)
      ports_package="grub"
      ;;
   gnu*)
      ports_package="glibc"
      ;;
   kfreebsd*)
      ports_package="kfreebsd"
      ;;
   linux*)
      AC_CHECK_FUNC([ioperm], [
         ports_package="glibc"
      ])
      ;;
   mingw*)
      ports_package="windows"
      ;;
   msdos*)
      ports_package="msdos"
      ;;
])

BRLTTY_BRAILLE_DRIVER([al], [Alva])
BRLTTY_BRAILLE_DRIVER([at], [Albatross])
test -n "${api_server_objects}" && {
   BRLTTY_BRAILLE_DRIVER([ba], [BrlAPI], [$(API_LIBS)])
}
BRLTTY_BRAILLE_DRIVER([bc], [BrailComm])
BRLTTY_BRAILLE_DRIVER([bd], [Braudi])
BRLTTY_BRAILLE_DRIVER([bl], [BrailleLite])
BRLTTY_BRAILLE_DRIVER([bm], [Baum])
BRLTTY_BRAILLE_DRIVER([bn], [BrailleNote])
BRLTTY_BRAILLE_DRIVER([cb], [CombiBraille])
BRLTTY_BRAILLE_DRIVER([ce], [Cebra])
BRLTTY_BRAILLE_DRIVER([cn], [Canute])
BRLTTY_BRAILLE_DRIVER([ec], [EcoBraille])
BRLTTY_BRAILLE_DRIVER([eu], [EuroBraille])
BRLTTY_BRAILLE_DRIVER([fa], [FrankAudiodata])
BRLTTY_BRAILLE_DRIVER([fs], [FreedomScientific])
BRLTTY_BRAILLE_DRIVER([hd], [Hedo])
BRLTTY_BRAILLE_DRIVER([hm], [HIMS])
BRLTTY_BRAILLE_DRIVER([ht], [HandyTech])
BRLTTY_BRAILLE_DRIVER([hw], [HumanWare])
BRLTTY_BRAILLE_DRIVER([ir], [Iris])
BRLTTY_BRAILLE_DRIVER([ic], [Inceptor])
BRLTTY_IF_PACKAGE([Libbraille], [libbraille], [include/braille.h], [dnl
   BRLTTY_BRAILLE_DRIVER([lb], [Libbraille], [-L$(LIBBRAILLE_ROOT)/lib -lbraille])
])
BRLTTY_BRAILLE_DRIVER([lt], [LogText])
BRLTTY_BRAILLE_DRIVER([mb], [MultiBraille])
BRLTTY_BRAILLE_DRIVER([md], [MDV])
BRLTTY_BRAILLE_DRIVER([mm], [BrailleMemo])
BRLTTY_BRAILLE_DRIVER([mn], [MiniBraille])
BRLTTY_BRAILLE_DRIVER([mt], [Metec])
BRLTTY_BRAILLE_DRIVER([np], [NinePoint])
BRLTTY_BRAILLE_DRIVER([pg], [Pegasus])
BRLTTY_BRAILLE_DRIVER([pm], [Papenmeier])
BRLTTY_BRAILLE_DRIVER([sk], [Seika])
BRLTTY_BRAILLE_DRIVER([tn], [TechniBraille])
BRLTTY_BRAILLE_DRIVER([ts], [TSI])
BRLTTY_BRAILLE_DRIVER([tt], [TTY], [$(CURSES_LIBS)])
BRLTTY_BRAILLE_DRIVER([vd], [VideoBraille])
BRLTTY_BRAILLE_DRIVER([vo], [Voyager])
BRLTTY_BRAILLE_DRIVER([vr], [Virtual])
BRLTTY_BRAILLE_DRIVER([vs], [VisioBraille])
test -n "${gui_toolkit_package}" && {
   BRLTTY_BRAILLE_DRIVER([xw], [XWindow], [$(XTK_LIBS) $(X11_LIBS)])
}
case "${host_os}"
in
   linux*)
      BRLTTY_BRAILLE_DRIVER([bg], [B2G])
      ;;
   *) ;;
esac
BRLTTY_ARG_DRIVER([braille], [Braille])

AC_CACHE_CHECK([for device directory], [brltty_cv_device_directory], [dnl
case "${host_os}"
in
   mingw*)
      brltty_cv_device_directory="//."
      ;;
   elf*|msdos*)
      brltty_cv_device_directory=""
      ;;
   *)
      brltty_cv_device_directory="/dev"
      ;;
esac])
AC_DEFINE_UNQUOTED([DEVICE_DIRECTORY], ["${brltty_cv_device_directory}"],
                   [Define this to be a string containing the path to the directory containing the devices.])

AC_CACHE_CHECK([for first serial device], [brltty_cv_device_serial_first], [dnl
case "${host_os}"
in
   linux*)
      brltty_cv_device_serial_first="ttyS0"
      ;;
   gnu*)
      brltty_cv_device_serial_first="com0"
      ;;
   solaris*)
      brltty_cv_device_serial_first="ttya"
      ;;
   haiku*)
      brltty_cv_device_serial_first="ports/pc_serial0"
      ;;
   hpux*)
      brltty_cv_device_serial_first="tty0p0"
      ;;
   openbsd*)
      brltty_cv_device_serial_first="cua00"
      ;;
   freebsd*)
      brltty_cv_device_serial_first="cuaa0"
      ;;
   kfreebsd*)
      brltty_cv_device_serial_first="cuaa0"
      ;;
   netbsd*)
      brltty_cv_device_serial_first="dty00"
      ;;
   osf*)
      brltty_cv_device_serial_first="tty00"
      ;;
   *qnx*)
      brltty_cv_device_serial_first="ser1"
      ;;
   cygwin*)
      brltty_cv_device_serial_first="ttyS0"
      ;;
   mingw*|msdos*)
      brltty_cv_device_serial_first="COM1"
      ;;
   elf*)
      brltty_cv_device_serial_first="com0"
      ;;
   *)
      brltty_cv_device_serial_first=""
      AC_MSG_WARN([primary serial device not configured for ${host_os}])
      ;;
esac])
AC_SUBST([serial_first_device], [${brltty_cv_device_serial_first}])
AC_DEFINE_UNQUOTED([SERIAL_FIRST_DEVICE], ["${serial_first_device}"],
                   [Define this to be a string containing the path to the first serial device.])

BRLTTY_ARG_WITH(
   [braille-device], [DEVICE],
   [default braille device],
   [braille_device], ["yes"]
)
if test "${braille_device}" = "no"
then
   braille_device=""
elif test "${braille_device}" = "yes"
then
   braille_device=""
   test "${usb_package}" != "none" && braille_device="${braille_device},usb:"
   test "${bluetooth_package}" != "none" && braille_device="${braille_device},bluetooth:"

   if test -n "${braille_device}"
   then
      braille_device="${braille_device#,}"
   else
      braille_device="serial:${serial_first_device}"
   fi
fi
AC_SUBST([braille_device])
AC_DEFINE_UNQUOTED([BRAILLE_DEVICE], ["${braille_device}"],
                   [Define this to be a string containing the path to the default braille device.])
BRLTTY_SUMMARY_ITEM([braille-device], [braille_device])

BRLTTY_ARG_TABLE([text], [en-nabcc], [ttb])
BRLTTY_ARG_TABLE([contraction], [none], [ctb])
BRLTTY_ARG_TABLE([attributes], [left_right], [atb])

BRLTTY_ARG_DISABLE(
   [speech-support],
   [support for speech synthesizers and text-to-speech engines],
   [],
[dnl
   AC_DEFINE([ENABLE_SPEECH_SUPPORT], [1],
             [Define this to include speech synthesizer and text-to-speech engine support.])
   speech_support_object='spk.$O'

   BRLTTY_SPEECH_DRIVER([al], [Alva])
   BRLTTY_SPEECH_DRIVER([bl], [BrailleLite])
   BRLTTY_SPEECH_DRIVER([cb], [CombiBraille])

   BRLTTY_IF_PACKAGE([eSpeak-NG], [espeak_ng], [include/espeak-ng/speak_lib.h], [dnl
      BRLTTY_SPEECH_DRIVER([en], [eSpeak-NG], [-L$(ESPEAK_NG_ROOT)/lib -lespeak-ng])
   ])

   BRLTTY_IF_PACKAGE([eSpeak], [espeak], [include/espeak/speak_lib.h], [dnl
      BRLTTY_SPEECH_DRIVER([es], [eSpeak], [-L$(ESPEAK_ROOT)/lib -lespeak])
   ])

   BRLTTY_IF_PACKAGE([FestivalLite], [flite], [include/flite/flite.h], [dnl
      BRLTTY_ARG_REQUIRED(
         [flite-language], [LANGUAGE],
         [the FestivalLite language to use],
         [flite_language], ["usenglish"]
      )

      BRLTTY_ARG_REQUIRED(
         [flite-lexicon], [LEXICON],
         [the FestivalLite lexicon to use],
         [flite_lexicon], ["cmulex"]
      )

      BRLTTY_ARG_REQUIRED(
         [flite-voice], [VOICE],
         [the FestivalLite voice to use],
         [flite_voice], ["cmu_us_kal"]
      )

      BRLTTY_SPEECH_DRIVER([fl], [FestivalLite], [-L$(FLITE_ROOT)/lib -lflite_$(FLITE_VOICE) -lflite_$(FLITE_LEXICON) -lflite_$(FLITE_LANGUAGE) -lflite -lm])
   ])

   BRLTTY_SPEECH_DRIVER([fv], [Festival])
   BRLTTY_SPEECH_DRIVER([gs], [GenericSay])

   BRLTTY_IF_PACKAGE([Mikropuhe], [mikropuhe], [mpwrfile.h], [dnl
      BRLTTY_IF_PTHREADS([dnl
         BRLTTY_SPEECH_DRIVER([mp], [Mikropuhe], [-L$(MPLINUX_ROOT) -lmplinux])
      ])
   ])

   BRLTTY_IF_PACKAGE([speech-dispatcher], [speechd], [include/libspeechd.h], [dnl
      BRLTTY_SPEECH_DRIVER([sd], [SpeechDispatcher])
   ], [include], [lib], [speechd])

   BRLTTY_IF_PACKAGE([Swift], [swift], [include/swift.h], [dnl
      BRLTTY_SPEECH_DRIVER([sw], [Swift], [-L$(SWIFT_ROOT)/lib -lswift -lm])
   ])

   BRLTTY_IF_PACKAGE([Theta], [theta], [include/theta.h], [dnl
      BRLTTY_SPEECH_DRIVER([th], [Theta], [-L$(THETA_ROOT)/lib -ltheta])
   ])

   AC_CHECK_HEADER([eci.h], [dnl
      BRLTTY_SPEECH_DRIVER([vv], [ViaVoice], [-libmeci])
   ])

   BRLTTY_SPEECH_DRIVER([xs], [ExternalSpeech])

   case "${host_os}"
   in
      *android*)
         BRLTTY_SPEECH_DRIVER([an], [Android])
         ;;
      *) ;;
   esac
])
AC_SUBST([speech_support_object])
BRLTTY_ARG_DRIVER([speech], [Speech])

case "${host_os}"
in
   *android*)
      BRLTTY_SCREEN_DRIVER([an], [Android])
      ;;
   linux*)
      BRLTTY_SCREEN_DRIVER([lx], [Linux])
      ;;
   gnu*)
      BRLTTY_SCREEN_DRIVER([hd], [Hurd])
      ;;
   cygwin*|mingw*)
      BRLTTY_SCREEN_DRIVER([wn], [Windows])
      brltty_build_directories="${brltty_build_directories} Drivers/BrlAPI/WindowEyes"
      ;;
   msdos*)
      BRLTTY_SCREEN_DRIVER([pb], [PcBios])
      ;;
   elf*)
      BRLTTY_SCREEN_DRIVER([gb], [Grub])
      ;;
   *)
      AC_MSG_WARN([no native screen driver for ${host_os}])
      ;;
esac

AC_CHECK_HEADER([sys/shm.h], [dnl
   BRLTTY_SCREEN_DRIVER([sc], [Screen])
])

if test "${brltty_enabled_x}" = "yes"
then
   BRLTTY_HAVE_PACKAGE([cspi], [cspi-1.0], [dnl
      AC_CHECK_HEADER([X11/keysym.h], [dnl
         AC_CHECK_HEADER([X11/Xlib.h], [dnl
            BRLTTY_SCREEN_DRIVER([as], [AtSpi], [$(X11_LIBS) $(CSPI_LIBS)])
         ])
      ])
   ])
fi

BRLTTY_IF_DBUS([dnl
   AC_CHECK_HEADER([X11/keysym.h], [dnl
      BRLTTY_HAVE_PACKAGE([atspi2], [atspi-2], [dnl
         brltty_cppflags_save="${CPPFLAGS}"
         CPPFLAGS="${CPPFLAGS} ${atspi2_cflags}"
         BRLTTY_SCREEN_DRIVER([a2], [AtSpi2], [$(DBUS_LIBS) $(ATSPI2_LIBS) $(GLIB2_LIBS) $(X11_LIBS) $(XFIXES_LIBS)])

         brltty_libs_save="${LIBS}"
         LIBS="${atspi2_libs} ${LIBS}"

         AC_CHECK_FUNCS([atspi_get_a11y_bus])

         CPPFLAGS="${brltty_cppflags_save}"
         LIBS="${brltty_libs_save}"

         BRLTTY_HAVE_PACKAGE([glib2], [glib-2.0])
      ])
   ])
])

BRLTTY_ARG_DRIVER([screen], [Screen])
BRLTTY_SUMMARY_ITEM([screen-driver], [default_screen_driver])

BRLTTY_ARG_ENABLE(
   [relocatable-install],
   [installation using paths relative to the program directory])

case "${host_os}"
in
   darwin*)
      INSTALL_OPTION_STRIP=""
      ;;
   *)
      INSTALL_OPTION_STRIP="-s"
      ;;
esac

if test "${cross_compiling}" != "no"
then
   if test -n "${INSTALL_OPTION_STRIP}"
   then
      INSTALL_OPTION_STRIP="${INSTALL_OPTION_STRIP} --strip-program ${STRIP}"
   fi
fi

BRLTTY_ARG_DISABLE(
   [stripping],
   [stripping of executables and shared objects during installation],
   [],
   [],
   [INSTALL_OPTION_STRIP=""]
)
AC_SUBST([INSTALL_OPTION_STRIP])

original_prefix="${prefix}"
test "${prefix}" = "NONE" && prefix=""
original_exec_prefix="${exec_prefix}"
test "${exec_prefix}" = "NONE" && exec_prefix="${prefix}"
BRLTTY_VAR_EXPAND([brltty_reference_directory], [${execute_root}${program_directory}])
BRLTTY_DEFINE_DIRECTORY([LOCALE_DIRECTORY], [${execute_root}${localedir}],
                        [Define this to be a string containing the path to the locale directory.])
if test -n "${updatable_directory}"
then
   BRLTTY_DEFINE_DIRECTORY([UPDATABLE_DIRECTORY], [${updatable_directory}],
                           [Define this to be a string containing the path to a directory which contains files that can be updated.])
fi

if test -n "${writable_directory}"
then
   BRLTTY_DEFINE_DIRECTORY([WRITABLE_DIRECTORY], [${writable_directory}],
                           [Define this to be a string containing the path to a writable directory.])
fi

BRLTTY_DEFINE_DIRECTORY([TABLES_DIRECTORY], [${execute_root}${tables_directory}],
                        [Define this to be a string containing the path to the data files directory.])

BRLTTY_DEFINE_DIRECTORY([DRIVERS_DIRECTORY], [${execute_root}${drivers_directory}],
                        [Define this to be a string containing the path to the drivers directory.])

BRLTTY_DEFINE_DIRECTORY([SCRIPTS_DIRECTORY], [${execute_root}${scripts_directory}],
                        [Define this to be a string containing the path to the scripts directory.])

BRLTTY_DEFINE_DIRECTORY([CONFIGURATION_DIRECTORY], [${sysconfdir}],
                        [Define this to be a string containing the path to the configuration directory.])

BRLTTY_DEFINE_DIRECTORY([BRLAPI_ETCDIR], [${CONFIGURATION_DIRECTORY}],
                        [Define this to be a string containing the path to BrlAPI's data files directory.])

BRLTTY_DEFINE_DIRECTORY([BRLAPI_SOCKETPATH], [${api_socket_path}],
                        [Define this to be a string containing the path to BrlAPI's local sockets directory.])

prefix="${original_prefix}"
exec_prefix="${original_exec_prefix}"

AC_SUBST([install_drivers])
BRLTTY_SUMMARY_END

AC_CONFIG_COMMANDS([build-configure], [dnl
   "${srcdir}/mk4build"
])

[brltty_make_files="`echo "${brltty_build_directories}" | sed -e '
s%\([^ ][^ ]*\)%\1/Makefile:prologue.mk:\1/Makefile.in%g
'`"]

AC_OUTPUT([
   config.mk
   brltty.spec
   brltty-config.sh
   brltty.pc
   Documents/brltty.conf
   Documents/brltty.1
   Documents/xbrlapi.1
   Documents/BrlAPIref.doxy
   Bindings/Lisp/brlapi.asd
   Bindings/Lisp/brlapi_config.lisp
   Bindings/Python/setup.py
   Autostart/Systemd/brltty@.service
   Autostart/Systemd/tmpfiles
   Autostart/Udev/device.rules
   Autostart/X11/90xbrlapi
   Android/Gradle/config.properties
   ${brltty_make_files}
])
