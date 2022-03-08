#!/bin/bash

installTree="$(dirname "${BASH_SOURCE[0]}")"
installTree="$(realpath --no-symlinks -- "${installTree}")"
readonly installTree="$(dirname "${installTree}")"
. "${installTree}/bin/brltty-prologue.sh"

canuteSetServer() {
   export BRLAPI_HOST=":canute"
}

readonly ldConfigurationFile="/etc/ld.so.conf.d/brlapi.conf"
readonly apiLinksDirectory="${installTree}/lib"
readonly canuteSystemdUnit="brltty@canute.path"
