#!/bin/bash

makeSymbolicLink() {
   local linkPath="${1}"
   local linkTarget="${2}"

   if [ -L "${linkPath}" ]
   then
      [ "$(readlink "${linkPath}")" != "${linkTarget}" ] || {
         logNote "link already correct: ${linkPath} -> ${linkTarget}"
         return 0
      }

      executeHostCommand sudo rm -- "${linkPath}"
   elif [ -e "${linkPath}" ]
   then
      semanticError "not a symbolic link: ${linkPath}"
   fi

   executeHostCommand sudo ln --symbolic -- "${linkTarget}" "${linkPath}"
}

