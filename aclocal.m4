AC_DEFUN([BRLTTY_SEARCH_LIBS], [dnl
brltty_uc="`echo "$1" | sed -e 'y%abcdefghijklmnopqrstuvwxyz%ABCDEFGHIJKLMNOPQRSTUVWXYZ%'`"
AC_SEARCH_LIBS([$1], [$2], [AC_DEFINE_UNQUOTED(HAVE_FUNC_${brltty_uc})])])

AC_DEFUN([BRLTTY_VAR_TRIM], [dnl
changequote(, )dnl
$1="`expr "${$1}" : ' *\(.*[^ ]\) *$'`"
changequote([, ])])

AC_DEFUN([BRLTTY_VAR_EXPAND], [dnl
eval '$1="'"$2"'"'])

AC_DEFUN([BRLTTY_DEFINE_EXPANDED], [dnl
BRLTTY_VAR_EXPAND([brltty_expanded], [$2])
AC_DEFINE_UNQUOTED([$1], ["${brltty_expanded}"])])

AC_DEFUN([BRLTTY_ARG_WITH], [dnl
AC_ARG_WITH([$1], BRLTTY_HELP_STRING([--with-$1=$2], [$3]), [$4="${withval}"], [$4=$5])])

AC_DEFUN([BRLTTY_ARG_ENABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [enable], [no], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_DISABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [disable], [yes], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_FEATURE], [dnl
AC_ARG_ENABLE([$1], BRLTTY_HELP_STRING([--$3-$1], [$2]), [], [enableval="$4"])
case "${enableval}" in
   yes)
      $5
      ;;
   no)
      $6
      ;;
   *)
      ifelse($7, [], [AC_MSG_ERROR([unexpected value for feature $1: ${enableval}])], [$7])
      ;;
esac
pushdef([var], brltty_enabled_[]translit([$1], [-], [_]))dnl
var="${enableval}"
BRLTTY_SUMMARY_ITEM([$1], var)dnl
popdef([var])])

AC_DEFUN([BRLTTY_HELP_STRING], [dnl
AC_HELP_STRING([$1], patsubst([$2], [
.*$]), [brltty_help_prefix])dnl
changequote(<, >)dnl
patsubst(patsubst(<$2>, <\`[^
]*>), <
>, <\&brltty_help_prefix>)<>dnl
changequote([, ])dnl
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
$1_libraries_$2='$4'
if test "${brltty_standalone_programs}" != "yes"
then
   case "${host_os}"
   in
      linux*)
         brltty_ld_runpath="-rpath"
         ;;
      *)
         brltty_ld_runpath=""
         ;;
   esac
changequote(, )dnl
   if test -n "${brltty_ld_runpath}"
   then
      if test "${GCC}" = "yes"
      then
         $1_libraries_$2="`echo "${$1_libraries_$2}" | sed -e 's%\(-L *\)\([^ ]*\)%-Wl,'"${brltty_ld_runpath}"',\2 \1\2%g'`"
      else
         $1_libraries_$2="`echo "${$1_libraries_$2}" | sed -e 's%\(-L *\)\([^ ]*\)%'"${brltty_ld_runpath}"' \2 \1\2%g'`"
      fi
   fi
changequote([, ])
fi
AC_SUBST([$1_libraries_$2])])

AC_DEFUN([BRLTTY_BRAILLE_DRIVER], [dnl
BRLTTY_ITEM([braille], [$1], [$2], [$3])])

AC_DEFUN([BRLTTY_SPEECH_DRIVER], [dnl
BRLTTY_ITEM([speech], [$1], [$2], [$3])])

AC_DEFUN([BRLTTY_ARG_ITEM], [dnl
BRLTTY_VAR_TRIM([brltty_item_codes_$1])
AC_SUBST([brltty_item_codes_$1])

BRLTTY_VAR_TRIM([brltty_item_names_$1])
AC_SUBST([brltty_item_names_$1])

BRLTTY_ARG_WITH(
   [$1-$2], translit([$2], [a-z], [A-Z]),
   [$1 $2(s) to build in]brltty_item_list_$1,
   [brltty_items], ["yes"]
)
if test "${brltty_items}" = "no"
then
   brltty_external_codes_$1=""
   brltty_external_names_$1=""
   brltty_internal_codes_$1=""
   brltty_internal_names_$1=""
else
   brltty_external_codes_$1=" ${brltty_item_codes_$1} "
   brltty_external_names_$1=" ${brltty_item_names_$1} "
   brltty_internal_codes_$1=""
   brltty_internal_names_$1=""
   if test "${brltty_items}" != "yes"
   then
      while :
      do
changequote(, )dnl
         brltty_delimiter="`expr "${brltty_items}" : '[^,]*,'`"
         if test "${brltty_delimiter}" -eq 0
         then
            brltty_item="${brltty_items}"
            brltty_items=""
            if test "${brltty_item}" = "all"
            then
               brltty_internal_codes_$1="${brltty_internal_codes_$1}${brltty_external_codes_$1}"
               brltty_internal_names_$1="${brltty_internal_names_$1}${brltty_external_names_$1}"
               brltty_external_codes_$1=""
               brltty_external_names_$1=""
               break
            fi
         else
            brltty_item="`expr "${brltty_items}" : '\([^,]*\)'`"
            brltty_items="`expr "${brltty_items}" : '[^,]*,\(.*\)'`"
         fi
         brltty_item_unknown=true
         if test -n "${brltty_item}"
         then
            brltty_item_entry="`expr "${brltty_item_entries_$1}" : '.* \('"${brltty_item}"'-[^ ]*\)'`"
            if test -n "${brltty_item_entry}"
            then
               brltty_item_code="${brltty_item}"
               brltty_item_name="`expr "${brltty_item_entry}" : '[^-]*-\(.*\)$'`"
               brltty_item_unknown=false
            else
               brltty_item_entry="`expr "${brltty_item_entries_$1}" : '.* \([^- ]*-'"${brltty_item}"'[^ ]*\)'`"
               if test -z "${brltty_item_entry}"
               then
                  brltty_lowercase="`echo "${brltty_item_entries_$1}" | sed 'y%ABCDEFGHIJKLMNOPQRSTUVWXYZ%abcdefghijklmnopqrstuvwxyz%'`"
                  brltty_item_code="`expr "${brltty_lowercase}" : '.* \([^- ]*\)-'"${brltty_item}"`"
                  if test -n "${brltty_item_code}"
                  then
                     brltty_item_entry="`expr "${brltty_item_entries_$1}" : '.* \('"${brltty_item_code}"'-[^ ]*\)'`"
                  fi
               fi
               if test -n "${brltty_item_entry}"
               then
                  brltty_item_code="`expr "${brltty_item_entry}" : '\([^-]*\)'`"
                  brltty_item_name="`expr "${brltty_item_entry}" : '[^-]*-\(.*\)$'`"
                  brltty_item_unknown=false
               fi
            fi
         fi
changequote([, ])dnl
         if "${brltty_item_unknown}"
         then
            AC_MSG_ERROR([unknown $1 $2: ${brltty_item}])
         fi
         brltty_item_found="`expr "${brltty_external_codes_$1}" : ".* ${brltty_item_code} "`"
         if test "${brltty_item_found}" -eq 0
         then
            AC_MSG_ERROR([duplicate $1 $2: ${brltty_item}])
         fi
         brltty_external_codes_$1="`echo "${brltty_external_codes_$1}" | sed -e "s% ${brltty_item_code} % %"`"
         brltty_external_names_$1="`echo "${brltty_external_names_$1}" | sed -e "s% ${brltty_item_name} % %"`"
         brltty_internal_codes_$1="${brltty_internal_codes_$1} ${brltty_item_code}"
         brltty_internal_names_$1="${brltty_internal_names_$1} ${brltty_item_name}"
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
AC_DEFINE_UNQUOTED(translit([$1_$2_codes], [a-z], [A-Z]), ["${brltty_item_codes_$1}"])

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

AC_DEFUN([BRLTTY_ARG_DRIVER], [dnl
BRLTTY_ARG_ITEM([$1], [driver])
if test "${brltty_enabled_$1_support}" != "no"
then
   if test -n "${brltty_internal_codes_$1}"
   then
changequote(, )dnl
      $1_driver_objects="`echo "${brltty_internal_names_$1}" | sed -e 's%\([^ ][^ ]*\)%$(BLD_TOP)Drivers/\1/$1.$O%g'`"
changequote([, ])dnl
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
fi
AC_SUBST([$1_driver_objects])
AC_SUBST([$1_help])
AC_SUBST([$1_drivers])])

AC_DEFUN([BRLTTY_DEVICE_PATH], [dnl
if test `expr "${$1}" : '/'` -eq 0
then
   $1="/dev/${$1}"
fi])

AC_DEFUN([BRLTTY_FILE_PATH], [dnl
ifelse(len([$3]), 0, [], [dnl
if test `expr "${$1}" : '.*/'` -eq 0
then
   if test `expr "${$1}" : '$3\.'` -eq 0
   then
      $1="$3.${$1}"
   fi
fi
])dnl
ifelse(len([$2]), 0, [], [dnl
if test `expr "${$1}" : '.*\.$2$'` -eq 0
then
   $1="${$1}.$2"
fi])])

AC_DEFUN([BRLTTY_TABLE_PATH], [BRLTTY_FILE_PATH([$1], [tbl], [$2])])

AC_DEFUN([BRLTTY_TEXT_TABLE], [dnl
define([brltty_tables_text], ifdef([brltty_tables_text], [brltty_tables_text])[
m4_text_wrap([$2], [           ], [- m4_format([%-8s ], [$1])], brltty_help_width)])])

AC_DEFUN([BRLTTY_ATTRIBUTES_TABLE], [dnl
define([brltty_tables_attributes], ifdef([brltty_tables_attributes], [brltty_tables_attributes])[
m4_text_wrap([$2], [             ], [- m4_format([%-10s ], [$1])], brltty_help_width)])])

AC_DEFUN([BRLTTY_SUMMARY_BEGIN], [dnl
brltty_summary_lines="
Options Summary:"])

AC_DEFUN([BRLTTY_SUMMARY_END], [dnl
AC_OUTPUT_COMMANDS([echo "${brltty_summary_lines}"], [brltty_summary_lines="${brltty_summary_lines}"])])

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
changequote()dnl
      if test `expr "${$1} " : '\${$3}/[^/]*$'` -gt 0
changequote([, ])dnl
      then
         $1="`echo ${$1} | sed -e 's%/%$2/%'`"
      fi
   fi
fi])

AC_DEFUN([BRLTTY_EXECUTABLE_PATH], [dnl
changequote()dnl
if test `expr "${$1} " : '[^/ ][^/ ]*/'` -gt 0
changequote([, ])dnl
then
   $1="`pwd`/${$1}"
fi])

AC_DEFUN([BRLTTY_IF_PACKAGE], [dnl
BRLTTY_ARG_WITH(
   [$2], [DIRECTORY],
   [where the $1 package is installed],
   [$2_root], ["yes"]
)
if test "${$2_root}" = "no"
then
   $2_root=""
elif test "${$2_root}" = "yes"
then
   $2_root=""
   roots="/usr /usr/local /opt/$2"
   for root in ${roots}
   do
      if test -f "${root}/$3"
      then
         $2_root="${root}"
         break
      fi
   done
   if test -z "${$2_root}"
   then
      AC_MSG_WARN([$1 package not found: ${roots}])
   fi
fi
AC_SUBST([$2_root])
BRLTTY_SUMMARY_ITEM([$2-root], [$2_root])
if test -n "${$2_root}"
then
   AC_DEFINE_UNQUOTED(translit([$2_root], [a-z], [A-Z]), ["${$2_root}"])
   $4
fi])

