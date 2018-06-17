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
cmdline() {
   brlttyImportInstallOptions
   set -- ${BRLTTY_DRACUT_KERNEL_PARAMETERS}
   local parameter

   for parameter
   do
      echo -n " rd.brltty.${parameter}"
   done
}

# called by dracut
installkernel() {
   instmods pcspkr uinput
   [ ! -d "${initdir}/var/lib/bluetooth" ] || instmods =drivers/bluetooth =net/bluetooth
   [ ! -d "${initdir}/etc/alsa" ] || instmods =sound
}

# called by dracut
install() {
   brlttyImportInstallOptions

   local BRLTTY_EXECUTABLE_PATH="/usr/bin/brltty"
   inst_binary "${BRLTTY_EXECUTABLE_PATH}"
   local brlttyLog="$(LC_ALL="${BRLTTY_DRACUT_LOCALE:-${LANG}}" "${BRLTTY_EXECUTABLE_PATH}" -E -v -e -ldebug 2>&1)"
   
   export BRLTTY_CONFIGURATION_FILE="/etc/brltty.conf"
   inst_simple "${BRLTTY_CONFIGURATION_FILE}"

   brlttyIncludeDataFiles $(brlttyGetProperty "including data file")
   brlttyIncludeScreenDrivers lx

   brlttyIncludeBrailleDrivers $(brlttyGetConfiguredDrivers braille)
   brlttyIncludeBrailleDrivers ${BRLTTY_DRACUT_BRAILLE_DRIVERS}

   local includingSpeechDrivers=false
   brlttyIncludeSpeechDrivers $(brlttyGetConfiguredDrivers speech)
   brlttyIncludeSpeechDrivers ${BRLTTY_DRACUT_SPEECH_DRIVERS}
   "${includingSpeechDrivers}" && brlttyIncludeSoundSupport
      
   brlttyIncludeTables Text        ttb ${BRLTTY_DRACUT_TEXT_TABLES}
   brlttyIncludeTables Attributes  atb ${BRLTTY_DRACUT_ATTRIBUTES_TABLES}
   brlttyIncludeTables Contraction ctb ${BRLTTY_DRACUT_CONTRACTION_TABLES}
   brlttyIncludeTables Keyboard    ktb ${BRLTTY_DRACUT_KEYBOARD_TABLES}

   brlttyInstallPreferencesFile "/etc/brltty.prefs"
   brlttyInstallDirectories "/etc/xdg/brltty"

   if [ "${BRLTTY_DRACUT_BLUETOOTH_SUPPORT}" = "yes" ]
   then
      brlttyIncludeBluetoothSupport
   fi

   inst_hook cmdline 99 "${moddir}/brltty-start.sh"
   inst_hook cleanup 99 "${moddir}/brltty-stop.sh"

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
      includingSpeechDrivers=true

      case "${code}"
      in
         en)
            inst_binary espeak-ng
            brlttyInstallDirectories "/usr/share/espeak-ng-data"
            ;;

         es)
            inst_binary espeak
            brlttyInstallDirectories "/usr/share/espeak-data"
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
      esac
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
   inst_libdir_file "brltty/libbrltty${type}${code}.so*"
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
   local file="/etc/brltty/dracut.conf"
   [ -f "${file}" ] && . "${file}"
}

brlttyIncludeBluetoothSupport() {
   brlttyInstallMessageBus

   brlttyInstallDirectories /var/lib/bluetooth
   brlttyInstallDirectories /etc/bluetooth

   inst_multiple -o bluetoothctl hciconfig hcitool sdptool
   inst_binary /usr/libexec/bluetooth/bluetoothd
   brlttyInstallSystemdUnits bluetooth.service bluetooth.target

   inst_hook initqueue 99 "${moddir}/bluetooth-start.sh"
}

brlttyInstallMessageBus() {
   set -- dbus $(sed -n -r -e 's/^.* user="([^"]*)".*$/\1/p' /usr/share/dbus-1/system.d/* | sort -u)
   local file name

   for file in passwd group
   do
      local source="/etc/${file}"
      local target="${initdir}${source}"

      for name
      do
         grep -q -e "^${name}:" "${target}" || {
            local line="$(grep "^${name}:" "${source}")"
            [ -n "${line}" ] && echo >>"${target}" "${line}"
         }
      done
   done

   brlttyInstallDirectories /etc/dbus-1
   brlttyInstallDirectories /usr/share/dbus-1
   brlttyInstallDirectories /usr/libexec/dbus-1

   inst_multiple dbus-daemon dbus-send dbus-cleanup-sockets dbus-monitor
   brlttyInstallSystemdUnits dbus.service dbus.socket

   inst_hook initqueue 99 "${moddir}/dbus-start.sh"
}

brlttyIncludeSoundSupport() {
   brlttyIncludeAlsaSupport
   brlttyIncludePulseAudioSupport
}

brlttyIncludeAlsaSupport() {
   brlttyInstallDirectories /etc/alsa
   rm -f "${initdir}/etc/alsa/conf.d/"*

   brlttyInstallDirectories /usr/share/alsa
   brlttyInstallDirectories /usr/lib/alsa
   brlttyInstallDirectories /usr/lib*/alsa-lib

   inst_multiple -o alsactl alsaucm alsamixer amixer aplay
   inst_script alsaunmute

   inst_hook initqueue 99 "${moddir}/alsa-start.sh"
}

brlttyIncludePulseAudioSupport() {
   inst_multiple -o pulseaudio pactl pacmd
   inst_multiple -o pamon paplay parec parecord
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

