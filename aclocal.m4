AC_DEFUN([BRLTTY_SEARCH_LIBS], [dnl
brltty_uc="`echo "$1" | sed -e 'y%abcdefghijklmnopqrstuvwxyz%ABCDEFGHIJKLMNOPQRSTUVWXYZ%'`"
AC_SEARCH_LIBS([$1], [$2], [AC_DEFINE_UNQUOTED(HAVE_FUNC_${brltty_uc})])])

AC_DEFUN([BRLTTY_VAR_TRIM], [dnl
changequote(, )dnl
$1="`expr "${$1}" : ' *\(.*[^ ]\) *$'`"
changequote([, ])])

AC_DEFUN([BRLTTY_DEFINE_EXPANDED], [dnl
eval 'brltty_expanded="'$2'"'
AC_DEFINE_UNQUOTED([$1], ["${brltty_expanded}"])])

AC_DEFUN([BRLTTY_ARG_WITH], [dnl
AC_ARG_WITH([$1], BRLTTY_STRING_HELP([--with-$1=$2], [$3]), [$4="${withval}"], [$4=$5])])

AC_DEFUN([BRLTTY_ARG_ENABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [enable], [no], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_DISABLE], [dnl
BRLTTY_ARG_FEATURE([$1], [$2], [disable], [yes], [$3], [$4], [$5])])

AC_DEFUN([BRLTTY_ARG_FEATURE], [dnl
AC_ARG_ENABLE([$1], BRLTTY_STRING_HELP([--$3-$1], [$2]), [], [enableval="$4"])
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
esac])

AC_DEFUN([BRLTTY_STRING_HELP], [dnl
pushdef([brltty_column], ifelse([$3], [], [32], [$3]))dnl
builtin([format], [  %-]builtin([eval], brltty_column-3)[s ], [$1])dnl
patsubst([$2], [
], [\&builtin([format], [%]brltty_column[s], [])])[]dnl
popdef([brltty_column])])dnl

AC_DEFUN([BRLTTY_ITEM], [dnl
define([brltty_item_list_$1], ifdef([brltty_item_list_$1], [brltty_item_list_$1])[
- $2  $3])dnl
brltty_item_entries_$1="${brltty_item_entries_$1} $2-$3"
brltty_item_codes_$1="${brltty_item_codes_$1} $2"
brltty_item_names_$1="${brltty_item_names_$1} $3"])

AC_DEFUN([BRLTTY_BRAILLE_DRIVER], [dnl
BRLTTY_ITEM([braille], [$1], [$2])])

AC_DEFUN([BRLTTY_SPEECH_DRIVER], [dnl
BRLTTY_ITEM([speech], [$1], [$2])])

AC_DEFUN([BRLTTY_ARG_ITEM], [dnl
BRLTTY_ITEM([$1], [no], [])
brltty_item_entries_$1=" ${brltty_item_entries_$1} "
BRLTTY_VAR_TRIM([brltty_item_codes_$1])
BRLTTY_VAR_TRIM([brltty_item_names_$1])
BRLTTY_ARG_WITH(
   [$1-$2], translit([$2], [a-z], [A-Z]),
   [$1 $2 to build in]brltty_item_list_$1,
   [brltty_item], ["no"]
)
changequote(, )dnl
brltty_item_unknown=true
brltty_item_length="`expr "${brltty_item}" : '[a-zA-Z0-9]*$'`"
if test ${brltty_item_length} -eq 2
then
   brltty_item_entry="`expr "${brltty_item_entries_$1}" : '.* \('"${brltty_item}"'-[^ ]*\)'`"
   if test -n "${brltty_item_entry}"
   then
      brltty_item_code_$1="${brltty_item}"
      brltty_item_name_$1="`expr "${brltty_item_entry}" : '[^-]*-\(.*\)$'`"
      brltty_item_unknown=false
   fi
elif test ${brltty_item_length} -gt 2
then
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
      brltty_item_code_$1="`expr "${brltty_item_entry}" : '\([^-]*\)'`"
      brltty_item_name_$1="`expr "${brltty_item_entry}" : '[^-]*-\(.*\)$'`"
      brltty_item_unknown=false
   fi
fi
changequote([, ])dnl
if "${brltty_item_unknown}"
then
   AC_MSG_ERROR([unknown $1 $2: ${brltty_item}])
fi
if test "${brltty_item_code_$1}" = "no"
then
   brltty_item_name_$1=""
fi
AC_SUBST([brltty_item_code_$1])
AC_SUBST([brltty_item_name_$1])
AC_SUBST([brltty_item_codes_$1])
AC_SUBST([brltty_item_names_$1])
AC_DEFINE_UNQUOTED(translit([$1_$2s], [a-z], [A-Z]), ["${brltty_item_codes_$1}"])])

AC_DEFUN([BRLTTY_ARG_DRIVER], [dnl
BRLTTY_ARG_ITEM([$1], [driver])
if test -z "${brltty_item_name_$1}"
then
   brltty_driver_targets_$1="dynamic-$1"
   brltty_driver_objects_$1=""
   install_drivers="install-drivers"
else
   brltty_driver_targets_$1="static-$1"
   brltty_driver_objects_$1='$(TOP_DIR)/Drivers/'"${brltty_item_name_$1}/$1.o"
   AC_DEFINE(translit([$1_builtin], [a-z], [A-Z]))
fi
AC_SUBST([brltty_driver_targets_$1])
AC_SUBST([brltty_driver_objects_$1])])
