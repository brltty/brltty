#!/bin/bash

# called by dracut
check() {
   require_binaries brltty brltty-lsinc || return 1
   return 0
}

# called by dracut
depends() {
   return 0
}

# called by dracut
installkernel() {
   instmods pcspkr uinput
   [ -d "${initdir}/etc/bluetooth" ] && instmods =drivers/bluetooth =net/bluetooth
   [ -d "${initdir}/etc/alsa" ] && instmods =sound
   return 0
}

# called by dracut
install() {
   brlttyImportInstallOptions
   local -A includedDrivers

   local BRLTTY_EXECUTABLE_PATH="/usr/bin/brltty"
   inst_binary "${BRLTTY_EXECUTABLE_PATH}"
   local brlttyLog="$(LC_ALL="${BRLTTY_DRACUT_LOCALE:-${LANG}}" "${BRLTTY_EXECUTABLE_PATH}" -E -v -e -ldebug 2>&1)"
   
   export BRLTTY_CONFIGURATION_FILE="/etc/brltty.conf"
   brlttyIncludeDataFiles "${BRLTTY_CONFIGURATION_FILE}"

   brlttyIncludeDataFiles $(brlttyGetProperty "including data file")
   brlttyIncludeScreenDrivers lx

   brlttyIncludeBrailleDrivers $(brlttyGetConfiguredDrivers braille)
   brlttyIncludeBrailleDrivers ${BRLTTY_DRACUT_BRAILLE_DRIVERS}

   brlttyIncludeSpeechDrivers $(brlttyGetConfiguredDrivers speech)
   brlttyIncludeSpeechDrivers ${BRLTTY_DRACUT_SPEECH_DRIVERS}
      
   brlttyIncludeTables Text        ttb ${BRLTTY_DRACUT_TEXT_TABLES}
   brlttyIncludeTables Contraction ctb ${BRLTTY_DRACUT_CONTRACTION_TABLES}
   brlttyIncludeTables Attributes  atb ${BRLTTY_DRACUT_ATTRIBUTES_TABLES}
   brlttyIncludeTables Keyboard    ktb ${BRLTTY_DRACUT_KEYBOARD_TABLES}

   brlttyInstallPreferencesFile "/etc/brltty.prefs"
   brlttyInstallDirectories "/etc/xdg/brltty"
   inst_simple "/etc/brltty/Initramfs/cmdline"

   if [ "${BRLTTY_DRACUT_BLUETOOTH_SUPPORT}" = "yes" ]
   then
      brlttyIncludeBluetoothSupport
   fi

   inst_hook cmdline 05 "${moddir}/brltty-start.sh"
   inst_hook cleanup 95 "${moddir}/brltty-stop.sh"

   dracut_need_initqueue
}

brlttyInstallPreferencesFile() {
   local path="${1}"
   local file=$(brlttyGetProperty "Preferences File")

   if [ -n "${file}" ]
   then
      if [ "${file}" = "${file#/}" ]
      then
         local directory=$(brlttyGetProperty "Updatable Directory")

         if [ -n "${directory}" ]
         then
            file="${directory}/${file}"
         fi
      fi

      if [ -f "${file}" ]
      then
         inst_simple "${file}" "${path}"
      fi
   fi
}

brlttyIncludeBrailleDrivers() {
   local code

   for code
   do
      brlttyIncludeDriver b "${code}" || continue

      local directory="/etc/brltty/Input/${code}"
      brlttyIncludeDataFiles "${directory}/"*.ktb
      inst_multiple -o "${directory}/"*.txt
   done
}

brlttyIncludeSpeechDrivers() {
   local code

   for code
   do
      brlttyIncludeDriver s "${code}" || continue

      case "${code}"
      in
         en)
            inst_binary espeak-ng
            brlttyInstallDirectories "/usr/share/espeak-ng-data"
            ;;

         es)
            inst_binary espeak
            brlttyInstallDirectories "/usr/share/espeak-data"
            brlttyIncludePulseAudioSupport
            ;;

         fl)
            inst_binary flite
            ;;

         fv)
            inst_binary festival
            brlttyInstallDirectories /etc/festival
            brlttyInstallDirectories /usr/lib*/festival
            brlttyInstallDirectories /usr/share/festival/lib
            ;;

         sd)
            inst_binary speech-dispatcher
            brlttyInstallDirectories /etc/speech-dispatcher
            brlttyInstallDirectories /usr/lib*/speech-dispatcher
            brlttyInstallDirectories /usr/lib*/speech-dispatcher-modules
            brlttyInstallDirectories /usr/share/speech-dispatcher
            brlttyInstallDirectories /usr/share/sounds/speech-dispatcher
            brlttyInstallSystemdUnits speech-dispatcherd.service
            inst_hook initqueue 98 "${moddir}/speechd-start.sh"
            ;;
      esac

      brlttyIncludeAlsaSupport
   done
}

brlttyIncludeScreenDrivers() {
   local code

   for code
   do
      brlttyIncludeDriver x "${code}" || continue
   done
}

brlttyIncludeDriver() {
   local type="${1}"
   local code="${2}"

   [ "${code}" = "no" ] && return 1
   local driver="${type}${code}"

   [ -n "${includedDrivers[${driver}]}" ] && return 2
   includedDrivers[${driver}]=1

   inst_libdir_file "brltty/libbrltty${driver}.so*"
   return 0
}

brlttyIncludeTables() {
   local subdirectory="${1}"
   local extension="${2}"
   shift 2
   local name

   for name
   do
      brlttyIncludeDataFiles "/etc/brltty/${subdirectory}/${name}.${extension}"
   done
}

brlttyIncludeDataFiles() {
   local file

   while read -r file
   do
      inst_simple "${file}"
   done < <(brltty-lsinc "${@}")
}

brlttyGetConfiguredDrivers() {
   local category="${1}"
   brlttyGetProperty "checking for ${category} driver"
}

brlttyGetProperty() {
   local name="${1}"
   echo "${brlttyLog}" | awk "/: *${name} *:/ {print \$NF}"
}

brlttyImportInstallOptions() {
   local file="/etc/brltty/Initramfs/dracut.conf"
   [ -f "${file}" ] && [ -r "${file}" ] && . "${file}"
}

brlttyIncludePulseAudioSupport() {
   [ -d "${initdir}/etc/pulse" ] && return 0

   brlttyInstallDirectories /etc/pulse
   brlttyInstallDirectories /usr/share/pulseaudio
   brlttyInstallDirectories /usr/lib*/pulseaudio
   brlttyInstallDirectories /usr/lib*/pulse-*
   brlttyInstallDirectories /usr/libexec/pulse

   inst_multiple -o pulseaudio pactl pacmd
   inst_multiple -o pamon paplay parec parecord

   brlttyAddUserEntries pulse
   brlttyAddGroupEntries pulse pulse-access pulse-rt

   brlttyIncludeAlsaSupport
   brlttyIncludeMessageBusSupport
   inst_simple /etc/dbus-1/system.d/pulseaudio-system.conf

   inst_binary chmod
   inst_hook initqueue 97 "${moddir}/pulse-start.sh"
   inst_hook cleanup 98 "${moddir}/pulse-stop.sh"
}

brlttyIncludeAlsaSupport() {
   [ -d "${initdir}/etc/alsa" ] && return 0;

   brlttyInstallDirectories /etc/alsa
   rm -f "${initdir}/etc/alsa/conf.d/"*

   brlttyInstallDirectories /usr/share/alsa
   brlttyInstallDirectories /usr/lib/alsa
   brlttyInstallDirectories /usr/lib*/alsa-lib

   inst_multiple -o alsactl alsaucm alsamixer amixer aplay
   inst_script alsaunmute

   inst_hook initqueue 96 "${moddir}/alsa-start.sh"
}

brlttyIncludeBluetoothSupport() {
   [ -d "${initdir}/etc/bluetooth" ] && return 0

   brlttyInstallDirectories /etc/bluetooth
   brlttyInstallDirectories /var/lib/bluetooth

   inst_multiple -o bluetoothctl hciconfig hcitool sdptool
   inst_binary /usr/libexec/bluetooth/bluetoothd
   brlttyInstallSystemdUnits bluetooth.service bluetooth.target

   inst_hook initqueue 97 "${moddir}/bluetooth-start.sh"
   brlttyIncludeMessageBusSupport
}

brlttyIncludeMessageBusSupport() {
   [ -d "${initdir}/etc/dbus-1" ] && return 0

   brlttyAddMessageBusUsers /usr/share/dbus-1/system.d/*
   brlttyAddMessageBusUsers /etc/dbus-1/system.d/*

   brlttyInstallDirectories /etc/dbus-1
   brlttyInstallDirectories /usr/share/dbus-1
   brlttyInstallDirectories /usr/libexec/dbus-1

   inst_multiple dbus-daemon dbus-send dbus-cleanup-sockets dbus-monitor
   brlttyInstallSystemdUnits dbus.service dbus.socket

   inst_hook initqueue 96 "${moddir}/dbus-start.sh"
}

brlttyAddMessageBusUsers() {
   set -- dbus $(sed -n -r -e 's/^.* user="([^"]*)".*$/\1/p' "${@}" | sort -u)
   brlttyAddUserEntries "${@}"
   brlttyAddGroupEntries "${@}"
}

brlttyAddUserEntries() {
   brlttyAddEntries passwd "${@}"
}

brlttyAddGroupEntries() {
   brlttyAddEntries group "${@}"
}

brlttyAddEntries() {
   local file="${1}"
   shift 1

   local source="/etc/${file}"
   local target="${initdir}${source}"
   local name

   for name
   do
      grep -q -e "^${name}:" "${target}" || {
         local line="$(grep "^${name}:" "${source}")"
         [ -n "${line}" ] && echo >>"${target}" "${line}"
      }
   done
}

brlttyInstallSystemdUnits() {
   local unit

   for unit
   do
      inst_simple "/usr/lib/systemd/system/${unit}"
   done
}

brlttyInstallDirectories() {
   local directory

   for directory
   do
      [ -d "${directory}" ] && {
         eval set -- $(find "${directory}" -printf "'%p'\n")
         inst_multiple "${@}"
      }
   done
}

