#!/bin/bash

installTree="$(dirname "${BASH_SOURCE[0]}")"
installTree="$(realpath --no-symlinks -- "${installTree}")"
readonly installTree="$(dirname "${installTree}")"
. "${installTree}/bin/brltty-prologue.sh"

readonly ldConfigurationFile="/etc/ld.so.conf.d/brlapi.conf"
readonly apiLinksDirectory="${installTree}"
readonly canuteSystemdUnit="brltty@canute.path"

brlttyPackageProperty() {
   local resultVariable="${1}"
   local name="${2}"

   local result
   result="$(PKG_CONFIG_PATH="${installTree}/lib/pkgconfig" pkg-config "--variable=${name}" -- brltty)" || return 1

   setVariable "${resultVariable}" "${result}"
   return 0
}

canuteSetSession() {
   local session="${1}"

   [ -n "${session}" ] || {
      if [ -n "${DISPLAY}" ]
      then
         session=startx
      else
         session=canute
      fi
   }

   export BRLAPI_HOST=":${session}"
}

