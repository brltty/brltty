###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 1995-2021 by Dave Mielke <dave@mielke.cc>
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

AC_DEFUN([BRLTTY_UPPERCASE_TRANSLATE], [translit([$1], [a-z], [A-Z])])

AC_DEFUN([BRLTTY_TRANSLATE_ASSIGN], [$1=`echo "$2" | sed -e 'y%$3%$4%'`])
AC_DEFUN([BRLTTY_UPPERCASE_ASSIGN], BRLTTY_TRANSLATE_ASSIGN([$1], [$2], [abcdefghijklmnopqrstuvwxyz$3], [ABCDEFGHIJKLMNOPQRSTUVWXYZ$4]))
AC_DEFUN([BRLTTY_LOWERCASE_ASSIGN], [BRLTTY_TRANSLATE_ASSIGN([$1], [$2], [ABCDEFGHIJKLMNOPQRSTUVWXYZ$3], [abcdefghijklmnopqrstuvwxyz$4])])

AC_DEFUN([BRLTTY_INCLUDE_HEADERS], [dnl
ifelse(len([$1]), 0, [], [
[#]include <$1>
])ifelse($#, 1, [], [BRLTTY_INCLUDE_HEADERS(m4_shift($@))])])

AC_DEFUN([BRLTTY_CHECK_FUNCTION], [dnl
   AC_CHECK_DECL([$1], [dnl
      AC_SEARCH_LIBS([$1], [$3], [dnl
         BRLTTY_UPPERCASE_ASSIGN([brltty_define], [have_$1])
         AC_DEFINE_UNQUOTED(${brltty_define}, [1], [Define this if the function $1 is available.])
      ])
   ], [], [BRLTTY_INCLUDE_HEADERS(m4_translit([$2], [ ], [,]))])
])

AC_DEFUN([BRLTTY_SEARCH_LIBS], [dnl
BRLTTY_UPPERCASE_ASSIGN([brltty_uc], [$1])
AC_SEARCH_LIBS([$1], [$2], [AC_DEFINE_UNQUOTED(HAVE_${brltty_uc}, [1], [Define this if the function $1 is available.])])])

AC_DEFUN([BRLTTY_VAR_TRIM], [dnl
$1="`echo "${$1}" | sed -e 's/^ *//' -e 's/ *$//'`"
])

AC_DEFUN([BRLTTY_VAR_EXPAND], [dnl
eval '$1="'"$2"'"'])

AC_DEFUN([BRLTTY_SUBST_EXPANDED], [dnl
BRLTTY_VAR_EXPAND([brltty_expanded], [$2])
AC_SUBST([$1], ["${brltty_expanded}"])])

AC_DEFUN([BRLTTY_DEFINE_STRING], [dnl
   AC_SUBST([$1], [$2])
   AC_DEFINE([$1], ["$2"], [Define this to be a string containing $3.])
])

AC_DEFUN([BRLTTY_DEFINE_EXPANDED], [dnl
BRLTTY_VAR_EXPAND([brltty_expanded], [$2])
AC_DEFINE_UNQUOTED([$1], ["${brltty_expanded}"], [$3])])

AC_DEFUN([BRLTTY_RELATIVE_PATH], [dnl
AC_REQUIRE([AC_PROG_AWK])
eval '$1="`"${AWK}" -v path="'"$2"'" -v reference="'"$3"'" -f "${srcdir}/relpath.awk"`"'])

AC_DEFUN([BRLTTY_DEFINE_DIRECTORY], [dnl
BRLTTY_VAR_EXPAND([$1], [$2])
AC_SUBST([$1])
if test "${brltty_enabled_relocatable_install}" = "yes"
then
   BRLTTY_RELATIVE_PATH([brltty_path], [${$1}], [${brltty_reference_directory}])
else
   brltty_path="${$1}"
fi
BRLTTY_DEFINE_EXPANDED([$1], [${brltty_path}], [$3])])

AC_DEFUN([BRLTTY_ARG_WITH], [dnl
AC_ARG_WITH([$1], BRLTTY_HELP_STRING([--with-$1=$2], [$3]), [$4="${withval}"], [$4=$5])])

AC_DEFUN([BRLTTY_ARG_REQUIRED], [dnl
BRLTTY_ARG_WITH([$1], [$2], [$3], [$4], ["yes"])
if test "${$4}" = "no"
then
   AC_MSG_ERROR([$1 not specified])
elif test "${$4}" = "yes"
then
   $4=$5
fi
AC_SUBST([$4])
BRLTTY_SUMMARY_ITEM([$1], [$4])])

AC_DEFUN([BRLTTY_ARG_TABLE], [dnl
brltty_default_table="$2"
BRLTTY_ARG_WITH(
   [$1-table], [FILE],
   [built-in (fallback) $1 table]brltty_tables_$1,
   [$1_table], ["${brltty_default_table}"]
)
install_$1_tables=install-$1-tables
if test "${$1_table}" = "no"
then
   install_$1_tables=
   $1_table="${brltty_default_table}"
elif test "${$1_table}" = "yes"
then
   $1_table="${brltty_default_table}"
fi
AC_SUBST([install_$1_tables])
BRLTTY_SUMMARY_ITEM([$1-table], [$1_table])
AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([$1_table]), ["${$1_table}"],
                   [Define this to be a string containing the path to the default $1 table.])
AC_SUBST([$1_table])])

AC_DEFUN([BRLTTY_ARG_PARAMETERS], [dnl
BRLTTY_ARG_WITH(
   [$1-parameters], [$3NAME=VALUE... (comma-separated)],
   [default parameters for the $2],
   [$1_parameters], ["yes"]
)
if test "${$1_parameters}" = "no"
then
   $1_parameters=""
elif test "${$1_parameters}" = "yes"
then
   $1_parameters="$4"
fi
AC_SUBST([$1_parameters])
AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([$1_parameters]), ["${$1_parameters}"],
                   [Define this to be a string containing the default parameters for the $2.])
BRLTTY_SUMMARY_ITEM([$1-parameters], [$1_parameters])])

AC_DEFUN([BRLTTY_ARG_PACKAGE], [dnl
$1_package=""
$1_includes=""
$1_libs=""

BRLTTY_ARG_WITH(
   [$1-package], [PACKAGE],
   [which $2 package to use],
   [packages], ["yes"]
)

if test "${packages}" = "no"
then
   $1_package="none"
elif test "${packages}" = "yes"
then
   packages="$3"
   ifelse(len([$4]), 0, [], [dnl
      case "${host_os}"
      in
         $4
         *);;
      esac
   ])
else
   packages=`echo "${packages}" | sed -e 'y/,/ /'`
fi

test -z "${$1_package}" && {
   test -n "${packages}" && {
      for package in ${packages}
      do
         BRLTTY_HAVE_PACKAGE([$1], [${package}], [], [:])
         test -n "${$1_package}" && break

         ifelse(len([$5]), 0, [], [dnl
            case "${package}"
            in
               $5
               *);;
            esac

            test -n "${$1_package}" && break
         ])

         AC_MSG_NOTICE([$2 package not available: ${package}])
      done
   }
}

test -z "${$1_package}" && {
   $1_package="none"
   AC_MSG_WARN([$2 support not available on this platform])
}

AC_SUBST([$1_package])
AC_SUBST([$1_includes])
AC_SUBST([$1_libs])
AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([$1_package]), [${$1_package}], [Define this to the name of the selected $2 package.])
BRLTTY_UPPERCASE_ASSIGN([brltty_uc], [use_pkg_$1_${$1_package}], [-.], [__])
AC_DEFINE_UNQUOTED([${brltty_uc}])
BRLTTY_SUMMARY_ITEM([$1-package], [$1_package])])

AC_DEFUN([BRLTTY_ARG_ENABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [enable], [no], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_DISABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [disable], [yes], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_FEATURE], [dnl
AC_ARG_ENABLE([$1], BRLTTY_HELP_STRING([--$3-$1], [$2]), [], [enableval="$4"])

pushdef([var], brltty_enabled_[]translit([$1], [-], [_]))dnl
var="${enableval}"
BRLTTY_SUMMARY_ITEM([$1], var)dnl
popdef([var])

if test "${enableval}" = "no"
then
   ifelse(len([$7]), 0, [:], [$7])
else
ifelse(len([$5]), 0, [], [dnl
   set -- [$5]
])dnl
   if test "${enableval}" = "yes"
   then
      brltty_ok=true
ifelse(len([$5]), 0, [], [dnl
      test "${#}" -gt 0 && enableval="${1}"
])dnl
   else
      brltty_ok=false
ifelse(len([$5]), 0, [], [dnl
      test "${#}" -gt 0 && {
         for brltty_selection
         do
            test "${brltty_selection}" = "${enableval}" && {
               brltty_ok=true
               break
            }
         done
      }
])dnl
   fi

   if "${brltty_ok}"
   then
ifelse(len([$5]), 0, [], [dnl
      test "${#}" -gt 0 && {
         BRLTTY_UPPERCASE_ASSIGN([brltty_uc], [use_$1_${enableval}], [-], [_])
         AC_DEFINE_UNQUOTED([${brltty_uc}])
      }
])dnl
      ifelse(len([$6]), 0, [:], [$6])
   else
      AC_MSG_ERROR([invalid selection: --enable-$1=${enableval}])
   fi
fi])

AC_DEFUN([BRLTTY_HELP_STRING], [dnl
AC_HELP_STRING([$1], patsubst([$2], [
.*$]), m4_defn([brltty_help_prefix]))dnl
patsubst(patsubst([$2], [\`[^
]*]), [
], [\&brltty_help_prefix])[]dnl
])
m4_define([brltty_help_indent], 32)
m4_define([brltty_help_prefix], m4_format([%]brltty_help_indent[s], []))
m4_define([brltty_help_width], m4_eval(79-brltty_help_indent))

AC_DEFUN([BRLTTY_ITEM], [dnl
define([brltty_item_list_$1], ifdef([brltty_item_list_$1], [brltty_item_list_$1])[
m4_text_wrap([$3], [      ], [- $2  ], brltty_help_width)])dnl
brltty_item_entries_$1="${brltty_item_entries_$1} $2-$3"
brltty_item_codes_$1="${brltty_item_codes_$1} $2"
brltty_item_names_$1="${brltty_item_names_$1} $3"
AC_SUBST([$1_libraries_$2], ['$4'])])

AC_DEFUN([BRLTTY_BRAILLE_DRIVER], [dnl
BRLTTY_ITEM([braille], [$1], [$2], [$3])])

AC_DEFUN([BRLTTY_SPEECH_DRIVER], [dnl
BRLTTY_ITEM([speech], [$1], [$2], [$3])])

AC_DEFUN([BRLTTY_SCREEN_DRIVER], [dnl
BRLTTY_ITEM([screen], [$1], [$2], [$3])])

AC_DEFUN([BRLTTY_ARG_ITEM], [dnl
BRLTTY_VAR_TRIM([brltty_item_codes_$1])
AC_SUBST([brltty_item_codes_$1])

BRLTTY_VAR_TRIM([brltty_item_names_$1])
AC_SUBST([brltty_item_names_$1])

case "${host_os}"
in
   cygwin*|mingw*)
      brltty_default="all"
      ;;
   *)
      brltty_default="yes"
      ;;
esac

BRLTTY_ARG_WITH(
   [$1-$2], BRLTTY_UPPERCASE_TRANSLATE([$2]),
   [$1 $2(s) to build in]brltty_item_list_$1,
   [brltty_items], ["${brltty_default}"]
)

if test "${brltty_items}" = "no"
then
   brltty_external_codes_$1=""
   brltty_external_names_$1=""
   brltty_internal_codes_$1=""
   brltty_internal_names_$1=""
else
   brltty_items_left_$1=" ${brltty_item_entries_$1} "
   brltty_external_codes_$1=" ${brltty_item_codes_$1} "
   brltty_external_names_$1=" ${brltty_item_names_$1} "
   brltty_internal_codes_$1=""
   brltty_internal_names_$1=""

   if test "${brltty_items}" != "yes"
   then
      while :
      do
         [brltty_delimiter="`expr "${brltty_items}" : '[^,]*,'`"]
         if test "${brltty_delimiter}" -eq 0
         then
            brltty_item="${brltty_items}"
            brltty_items=""
            if test "${brltty_item}" = "all"
            then
               brltty_item_all=true
               brltty_item_include=true
            elif test "${brltty_item}" = "-all"
            then
               brltty_item_all=true
               brltty_item_include=false
            else
               brltty_item_all=false
            fi
            if "${brltty_item_all}"
            then
               if "${brltty_item_include}"
               then
                  brltty_internal_codes_$1="${brltty_internal_codes_$1}${brltty_external_codes_$1}"
                  brltty_internal_names_$1="${brltty_internal_names_$1}${brltty_external_names_$1}"
               fi
               brltty_external_codes_$1=""
               brltty_external_names_$1=""
               break
            fi
         else
            [brltty_item="`expr "${brltty_items}" : '\([^,]*\)'`"]
            [brltty_items="`expr "${brltty_items}" : '[^,]*,\(.*\)'`"]
         fi
         brltty_item_suffix="${brltty_item#-}"
         if test "${brltty_item}" = "${brltty_item_suffix}"
         then
            brltty_item_include=true
         else
            brltty_item_include=false
            brltty_item="${brltty_item_suffix}"
         fi

         BRLTTY_ITEM_RESOLVE([$1])
         "${brltty_item_unknown}" && {
            AC_MSG_ERROR([unknown $1 $2: ${brltty_item}])
         }

         brltty_item_found="`expr "${brltty_external_codes_$1}" : ".* ${brltty_item_code} "`"
         test "${brltty_item_found}" -eq 0 && {
            AC_MSG_ERROR([duplicate $1 $2: ${brltty_item}])
         }

         brltty_items_left_$1="`echo "${brltty_items_left_$1}" | sed -e "s% ${brltty_item_entry} % %"`"
         brltty_external_codes_$1="`echo "${brltty_external_codes_$1}" | sed -e "s% ${brltty_item_code} % %"`"
         brltty_external_names_$1="`echo "${brltty_external_names_$1}" | sed -e "s% ${brltty_item_name} % %"`"

         "${brltty_item_include}" && {
            brltty_internal_codes_$1="${brltty_internal_codes_$1} ${brltty_item_code}"
            brltty_internal_names_$1="${brltty_internal_names_$1} ${brltty_item_name}"
         }

         BRLTTY_ITEM_RESOLVE([$1])
         "${brltty_item_unknown}" || {
            AC_MSG_ERROR([ambiguous $1 $2: ${brltty_item}])
         }

         test "${brltty_delimiter}" -eq 0 && break
      done
   fi

   BRLTTY_VAR_TRIM([brltty_external_codes_$1])
   BRLTTY_VAR_TRIM([brltty_external_names_$1])
   BRLTTY_VAR_TRIM([brltty_internal_codes_$1])
   BRLTTY_VAR_TRIM([brltty_internal_names_$1])
fi

AC_SUBST([brltty_external_codes_$1])
AC_SUBST([brltty_external_names_$1])
AC_SUBST([brltty_internal_codes_$1])
AC_SUBST([brltty_internal_names_$1])

set -- ${brltty_internal_codes_$1} ${brltty_external_codes_$1}
AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([$1_$2_codes]), ["${*}"], 
                   [Define this to be a string containing a list of the $1 $2 codes.])

AC_SUBST([default_$1_$2], ["${1}"])
AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([default_$1_$2]), ["${1}"], 
                   [Define this to be a string containing the default $1 $2 code.])

$1_driver_libraries=""
if test -n "${brltty_internal_codes_$1}"
then
   for brltty_driver in ${brltty_internal_codes_$1}
   do
      eval 'brltty_libraries="${$1_libraries_'"${brltty_driver}"'}"'
      if test -n "${brltty_libraries}"
      then
         $1_driver_libraries="${$1_driver_libraries} ${brltty_libraries}"
      fi
   done
fi
BRLTTY_VAR_TRIM([$1_driver_libraries])
AC_SUBST([$1_driver_libraries])
])

AC_DEFUN([BRLTTY_ITEM_RESOLVE], [dnl
brltty_item_unknown=true
brltty_item_length=`expr length "${brltty_item}"`

if test "${brltty_item_length}" -eq 2
then
   [brltty_item_entry=`expr "${brltty_items_left_$1}" : '.* \('"${brltty_item}"'-[^ ]*\)'`]
   if test -n "${brltty_item_entry}"
   then
      brltty_item_code="${brltty_item}"
      [brltty_item_name=`expr "${brltty_item_entry}" : '[^[.-.]]*-\(.*\)$'`]
      brltty_item_unknown=false
   fi
elif test "${brltty_item_length}" -gt 2
then
   [brltty_item_entry=`expr "${brltty_items_left_$1}" : '.* \([^- ]*-'"${brltty_item}"'[^ ]*\)'`]
   if test -z "${brltty_item_entry}"
   then
      BRLTTY_LOWERCASE_ASSIGN([brltty_lowercase], [${brltty_items_left_$1}])
      [brltty_item_code=`expr "${brltty_lowercase}" : '.* \([^- ]*\)-'"${brltty_item}"`]
      if test -n "${brltty_item_code}"
      then
         [brltty_item_entry=`expr "${brltty_items_left_$1}" : '.* \('"${brltty_item_code}"'-[^ ]*\)'`]
      fi
   fi

   if test -n "${brltty_item_entry}"
   then
      [brltty_item_code=`expr "${brltty_item_entry}" : '\([^[.-.]]*\)'`]
      [brltty_item_name=`expr "${brltty_item_entry}" : '[^[.-.]]*-\(.*\)$'`]
      brltty_item_unknown=false
   fi
fi
])

AC_DEFUN([BRLTTY_ARG_DRIVER], [dnl
BRLTTY_ARG_ITEM([$1], [driver])
if test "${brltty_enabled_$1_support}" != "no"
then
   if test -n "${brltty_internal_codes_$1}"
   then
      [$1_driver_objects="`echo "${brltty_internal_names_$1}" | sed -e 's%\([^ ][^ ]*\)%$(BLD_TOP)Drivers/$2/\1/$1.$O%g'`"]
      $1_help="$1-help"
   fi

   if test "${brltty_standalone_programs}" != "yes"
   then
      if test -n "${brltty_external_codes_$1}"
      then
         $1_drivers="$1-drivers"
         install_drivers="install-drivers"
      fi
      BRLTTY_SUMMARY_ITEM([external-$1-drivers], [brltty_external_codes_$1])
   fi

   BRLTTY_SUMMARY_ITEM([internal-$1-drivers], [brltty_internal_codes_$1])
   BRLTTY_ARG_PARAMETERS([$1], [$1 driver(s)], [DRIVER:])
fi

for brltty_driver in ${brltty_item_names_$1}
do
   brltty_build_directories="${brltty_build_directories} Drivers/$2/${brltty_driver}"
done

AC_SUBST([$1_driver_objects])
AC_SUBST([$1_drivers])
AC_SUBST([$1_help])
])

AC_DEFUN([BRLTTY_TEXT_TABLE], [dnl
define([brltty_tables_text], ifdef([brltty_tables_text], [brltty_tables_text])[
m4_text_wrap([$2], [           ], [- ]m4_format([%-8s ], [$1]), brltty_help_width)])])

AC_DEFUN([BRLTTY_ATTRIBUTES_TABLE], [dnl
define([brltty_tables_attributes], ifdef([brltty_tables_attributes], [brltty_tables_attributes])[
m4_text_wrap([$2], [             ], [- ]m4_format([%-10s ], [$1]), brltty_help_width)])])

AC_DEFUN([BRLTTY_SUMMARY_BEGIN], [dnl
brltty_summary_lines="Options Summary:"
])

AC_DEFUN([BRLTTY_SUMMARY_END], [dnl
AC_CONFIG_COMMANDS([item-summary],
   [AC_MSG_NOTICE([${brltty_summary_lines}])],
   [brltty_summary_lines='${brltty_summary_lines}']
)])

AC_DEFUN([BRLTTY_SUMMARY_ITEM], [dnl
brltty_summary_lines="${brltty_summary_lines}
   $1: ${$2}"])

AC_DEFUN([BRLTTY_PORTABLE_DIRECTORY], [dnl
   BRLTTY_TOPLEVEL_DIRECTORY([$1], [$2], [prefix])])

AC_DEFUN([BRLTTY_ARCHITECTURE_DIRECTORY], [dnl
if test "${exec_prefix}" = "NONE"
then
   BRLTTY_TOPLEVEL_DIRECTORY([$1], [$2], [exec_prefix])
fi])

AC_DEFUN([BRLTTY_TOPLEVEL_DIRECTORY], [dnl
if test "${prefix}" = "NONE"
then
   if test -z "${execute_root}"
   then
      [if test `expr "${$1} " : '\${$3}/[^/]*$'` -gt 0]
      then
         $1="`echo ${$1} | sed -e 's%/%$2/%'`"
      fi
   fi
fi])

AC_DEFUN([BRLTTY_EXECUTABLE_PATH], [dnl
[if test `expr "${$1} " : '[^/ ][^/ ]*/'` -gt 0]
then
   $1="`pwd`/${$1}"
fi])

AC_DEFUN([BRLTTY_IF_PACKAGE], [dnl
BRLTTY_ARG_WITH(
   [$2], [DIRECTORY],
   [where the $1 package is installed],
   [$2_root], ["yes"]
)

$2_found=false
m4_define([$2_find], ifelse(m4_eval($# > 4), 1, [true], [false]))
ifelse($2_find, [true], [BRLTTY_HAVE_PACKAGE([$2], [$1], [$2_found=true], [])])

if test "${$2_root}" = "no"
then
   $2_root=""
elif test "${$2_root}" = "yes"
then
   "${$2_found}" || {
      $2_root=""
      roots="/usr /usr/local /usr/local/$1 /usr/local/$2 /opt/$1 /opt/$2 /mingw /mingw/$1 /mingw/$2"

      for root in ${roots}
      do
         test -f "${root}/$3" && {
            $2_root="${root}"
            AC_MSG_NOTICE([$1 root: ${$2_root}])
            break
         }
      done

      if test -z "${$2_root}"
      then
         AC_MSG_WARN([$1 package not found: ${roots}])
      fi
   }
fi

AC_SUBST([$2_root])
BRLTTY_SUMMARY_ITEM([$2-root], [$2_root])

test -n "${$2_root}" && {
   AC_DEFINE_UNQUOTED(BRLTTY_UPPERCASE_TRANSLATE([$2_root]), ["${$2_root}"],
                      [Define this to be a string containing the path to the root of the $1 package.])

   ifelse($2_find, [true], [dnl
      test "${$2_root}" = "yes" || {
         $2_includes="BRLTTY_WORDS_PREPEND([$5], [-I${$2_root}/])"
         $2_libs="BRLTTY_WORDS_PREPEND([$6], [-L${$2_root}/]) BRLTTY_WORDS_PREPEND([$7], [-l])"
      }
   ])

   $4
}
])

AC_DEFUN([BRLTTY_WORDS_PREPEND], [dnl
patsubst([$1], [\(\S+\)], [$2\1])])

AC_DEFUN([BRLTTY_HAVE_PACKAGE], [dnl
$1_package=""
$1_includes=""
$1_libs=""

for package_specification in $2
do
   ${PKG_CONFIG} --exists "${package_specification}" && {
      AC_DEFINE(BRLTTY_UPPERCASE_TRANSLATE([HAVE_PKG_$1]))

      $1_package="${package_specification%% *}"
      AC_MSG_NOTICE([$1 package: ${$1_package}])

      $1_includes=`${PKG_CONFIG} --cflags-only-I "${$1_package}"`
      AC_MSG_NOTICE([$1 includes: ${$1_includes}])

      $1_libs=`${PKG_CONFIG} ${pkgconfig_flags_libs} "${$1_package}"`
      AC_MSG_NOTICE([$1 libs: ${$1_libs}])

      $3
      break
   }
done

test -n "${$1_package}" || {
   ifelse(len([$4]), 0, [AC_MSG_WARN([$1 support not available])], [$4])
}

AC_SUBST([$1_package])
AC_SUBST([$1_includes])
AC_SUBST([$1_libs])
])

AC_DEFUN([BRLTTY_HAVE_PTHREADS], [dnl
   AC_CACHE_CHECK([if pthreads are available], [brltty_cv_have_pthreads], [dnl
      SYSCFLAGS="${SYSCFLAGS} -D_REENTRANT"
      case "${host_os}"
      in
         mingw32*)
            brltty_cv_have_pthreads=yes
            ;;
         *)
            brltty_cv_have_pthreads=no
            AC_CHECK_HEADER([pthread.h], [dnl
               case "${host_os}"
               in
                  solaris*) AC_SEARCH_LIBS([_getfp], [pthread], [brltty_cv_have_pthreads=yes]);;
                  hpux*) AC_SEARCH_LIBS([__pthread_cancel_stack], [pthread], [brltty_cv_have_pthreads=yes]);;
                  *) AC_SEARCH_LIBS([pthread_create], [pthread c_r], [brltty_cv_have_pthreads=yes]);;
               esac
            ])
            ;;
      esac
   ])
])

AC_DEFUN([BRLTTY_IF_PTHREADS], [dnl
AC_REQUIRE([BRLTTY_HAVE_PTHREADS])
test "${brltty_cv_have_pthreads}" = "yes" && {
   $1
}])

AC_DEFUN([BRLTTY_PACKAGE_CHOOSE], [dnl
BRLTTY_ARG_WITH(
   [translit([$1], [_], [-])], [PACKAGE],
   [which translit([$1], [_], [ ]) package to use (BRLTTY_PACKAGE_LIST(m4_shift($@)))],
   [$1_package], ["yes"]
)
if test "${$1_package}" = "no"
then
   $1_package=""
elif test "${$1_package}" = "yes"
then
AC_CACHE_CHECK([which translit([$1], [_], [ ]) package to use], [brltty_cv_package_$1], [dnl
   brltty_cv_package_$1=""
   brltty_packages=""
   BRLTTY_PACKAGE_DEFINE(m4_shift($@))

   for brltty_package in ${brltty_packages}
   do
      eval 'brltty_headers="${brltty_headers_'"${brltty_package}"'}"'
      test -n "${brltty_headers}" && {
         brltty_found=true
         for brltty_header in ${brltty_headers}
         do
            AC_CHECK_HEADER([${brltty_header}], [], [brltty_found=false], [-])
            "${brltty_found}" || break
         done
         "${brltty_found}" || continue
      }

      AC_CHECK_LIB([${brltty_package}], [main], [], [continue])
      brltty_cv_package_$1="${brltty_package}"
      break
   done
])
   $1_package="${brltty_cv_package_$1}"
else
   AC_HAVE_LIBRARY([${$1_package}], [], [$1_package=""])
fi
AC_SUBST([$1_package])
test -n "${$1_package}" && {
   BRLTTY_UPPERCASE_ASSIGN([brltty_uc], [${$1_package}])
   AC_DEFINE_UNQUOTED([HAVE_PKG_${brltty_uc}])
   BRLTTY_SUMMARY_ITEM([translit([$1], [_], [-])-package], [$1_package])
}])
AC_DEFUN([BRLTTY_PACKAGE_DEFINE], [dnl
ifelse($#, 0, [], [dnl

set -- [$1]
brltty_package="${1}"
shift 1
eval "brltty_headers_${brltty_package}"'="${*}"'
brltty_packages="${brltty_packages} ${brltty_package}"
ifelse($#, 1, [], [BRLTTY_PACKAGE_DEFINE(m4_shift($@))])])])
AC_DEFUN([BRLTTY_PACKAGE_LIST], [dnl
ifelse($#, 0, [], $#, 1, [BRLTTY_PACKAGE_NAME([$1])], [BRLTTY_PACKAGE_NAME([$1]) BRLTTY_PACKAGE_LIST(m4_shift($@))])])
AC_DEFUN([BRLTTY_PACKAGE_NAME], [patsubst([$1], [^ *\(\w+\).*], [\1])])

AC_DEFUN([BRLTTY_OPTIONS_LD2CC], [dnl
`echo "$1" | sed -e '
/^$/d
s/^ */-Wl /
s/ *$//
s/  */,/g
'`])

AC_DEFUN([BRLTTY_HAVE_WINDOWS_LIBRARY], [dnl
AC_CACHE_CHECK(
   [if DLL $1 can be loaded],
   [brltty_cv_dll_$1],
   [AC_TRY_RUN([
#include <windows.h>
int main () {
   return !LoadLibrary("$1.DLL");
}
],
[brltty_cv_dll_$1=yes],
[brltty_cv_dll_$1=no])])
if test "${brltty_cv_dll_$1}" = "yes"
then
   AC_HAVE_LIBRARY([$1])
   $2
else
   :
   $3
fi])

AC_DEFUN([BRLTTY_HAVE_WINDOWS_FUNCTION], [dnl
AC_CACHE_CHECK(
   [if function $1 in DLL $2 exists],
   [brltty_cv_function_$1],
   [AC_TRY_RUN([
#include <windows.h>
#include <stdio.h>
#include <errno.h>

int
main (void) {
  HMODULE module;
  HINSTANCE instance;
  if (!(instance = LoadLibrary("$2.dll"))) return 1;
  if (!(module = GetModuleHandle("$2.dll"))) return 2;
  if (!(GetProcAddress(module, "$1"))) return 3;
  return 0;
}
],
   [brltty_cv_function_$1=yes],
   [brltty_cv_function_$1=no])])
if test "${brltty_cv_function_$1}" = "yes"
then
   AC_DEFINE(BRLTTY_UPPERCASE_TRANSLATE([HAVE_$1]), [1],
             [Define this if the function $1 is available.])
   $3
else
   :
   $4
fi
])

AC_DEFUN([BRLTTY_BINDINGS], [dnl
ifelse($#, 1, [dnl
BRLTTY_BINDINGS([$1], m4_tolower([$1]), m4_toupper([$1]))dnl
], [dnl
BRLTTY_ARG_DISABLE(
   [$2-bindings],
   [$1 bindings for BrlAPI],
   [],
   [
      BRLTTY_$3_BINDINGS
      if "${$3_OK}"
      then
         api_bindings="${api_bindings} $1"
      else
         AC_MSG_WARN([$1 BrlAPI bindings not included])
      fi
   ]
)dnl
])])
