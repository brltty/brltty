#!/bin/bash

# called by dracut
check() {
   require_binaries brltty || return 1
   return 0
}

# called by dracut
depends() {
   return 0
}

# called by dracut
cmdline() {
   brlttyLoadConfigurationFile
   set -- ${BRLTTY_DRACUT_KERNEL_PARAMETERS}
   local parameter

   for parameter
   do
      echo -n " rd.brltty.${parameter}"
   done
}

# called by dracut
installkernel() {
   instmods pcspkr
   [ -d "/var/lib/bluetooth" ] && instmods =drivers/bluetooth =net/bluetooth
   [ -d "/etc/alsa" ] && instmods =sound
}

# called by dracut
install() {
   brlttyLoadConfigurationFile

   local BRLTTY_EXECUTABLE_PATH="/usr/bin/brltty"
   inst_binary "${BRLTTY_EXECUTABLE_PATH}"
   local brlttyLog="$(LC_ALL="${BRLTTY_DRACUT_LOCALE:-${LANG}}" "${BRLTTY_EXECUTABLE_PATH}" -E -v -e -ldebug 2>&1)"
   
   export BRLTTY_CONFIGURATION_FILE=/etc/brltty.conf
   inst_simple "${BRLTTY_CONFIGURATION_FILE}"

   brlttyIncludeDataFiles $(brlttyGetProperty "including data file")
   brlttyIncludeScreenDrivers lx

   brlttyIncludeBrailleDrivers $(brlttyGetDrivers braille)
   brlttyIncludeBrailleDrivers ${BRLTTY_DRACUT_BRAILLE_DRIVERS}

   brlttyIncludeSpeechDrivers $(brlttyGetDrivers speech)
   brlttyIncludeSpeechDrivers ${BRLTTY_DRACUT_SPEECH_DRIVERS}
      
   brlttyIncludeTables Text        ttb ${BRLTTY_DRACUT_TEXT_TABLES}
   brlttyIncludeTables Attributes  atb ${BRLTTY_DRACUT_ATTRIBUTES_TABLES}
   brlttyIncludeTables Contraction ctb ${BRLTTY_DRACUT_CONTRACTION_TABLES}
   brlttyIncludeTables Keyboard    ktb ${BRLTTY_DRACUT_KEYBOARD_TABLES}

   brlttyInstallPreferencesFile "/etc/brltty.prefs"
   brlttyInstallDirectory "/etc/xdg/brltty"

   [ "${BRLTTY_DRACUT_BLUETOOTH_SUPPORT}" = "yes" ] && brlttyIncludeBluetoothSupport
   [ "${BRLTTY_DRACUT_SOUND_SUPPORT}" = "yes" ] && brlttyIncludeSoundSupport

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
   brlttyIncludeDrivers b "${@}"
   local code

   for code
   do
      local directory="/etc/brltty/Input/${code}"
      brlttyIncludeDataFiles "${directory}/"*.ktb
      inst_multiple -o "${directory}/"*.txt
   done
}

brlttyIncludeSpeechDrivers() {
   brlttyIncludeDrivers s "${@}"
   local code

   for code
   do
      case "${code}"
      in
         en)
            inst_binary espeak-ng
#skip       brlttyInstallDirectory "/usr/share/espeak-ng-data"
            ;;
         es)
            inst_binary espeak
            brlttyInstallDirectory "/usr/share/espeak-data"
            ;;
      esac
   done
}

brlttyIncludeScreenDrivers() {
   brlttyIncludeDrivers x "${@}"
}

brlttyIncludeDrivers() {
   local type="${1}"
   shift 1
   local code

   for code
   do
      [ "${code}" = "no" ] && continue
      inst_libdir_file "brltty/libbrltty${type}${code}.so*"
   done
}

brlttyIncludeTables() {
   local subdirectory="$1"
   local extension="$2"
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

brlttyGetDrivers() {
   local category="${1}"

   brlttyGetProperty "checking for ${category} driver"
}

brlttyGetProperty() {
   local name="${1}"

   echo "${brlttyLog}" | awk "/: *${name} *:/ {print \$NF}"
}

brlttyLoadConfigurationFile() {
   local file="/etc/brltty/dracut.conf"
   [ -f "${file}" ] && . "${file}"
}

brlttyIncludeBluetoothSupport() {
   brlttyInstallMessageBus

   brlttyInstallDirectory /var/lib/bluetooth
   inst_multiple -o bluetoothctl hciconfig hcitool sdptool

   inst_binary /usr/libexec/bluetooth/bluetoothd
   brlttyInstallSystemdUnits bluetooth.service bluetooth.target

   inst_hook initqueue 99 "${moddir}/bluetooth-start.sh"
}

brlttyInstallMessageBus() {
   local file name

   for file in passwd group
   do
      local source="/etc/${file}"
      local target="${initdir}${source}"

      for name in dbus systemd-network systemd-resolve colord
      do
         grep -q -e "^${name}:" "${target}" || {
            local line="$(grep "^${name}:" "${source}")"
            [ -n "${line}" ] && echo >>"${target}" "${line}"
         }
      done
   done

   brlttyInstallDirectory /usr/share/dbus-1
   inst_multiple dbus-daemon dbus-send
   brlttyInstallSystemdUnits dbus.service dbus.socket
   inst_hook initqueue 99 "${moddir}/dbus-start.sh"
}

brlttyIncludeSoundSupport() {
   brlttyInstallDirectory /etc/alsa
   rm -f "${initdir}/etc/alsa/conf.d/"*

   brlttyInstallDirectory /usr/share/alsa
   brlttyInstallDirectory /usr/lib/alsa
   brlttyInstallDirectory /usr/lib64/alsa-lib

   inst_multiple -o alsactl alsaucm alsamixer amixer aplay
   inst_script alsaunmute
   inst_hook initqueue 99 "${moddir}/sound-start.sh"
}

brlttyInstallSystemdUnits() {
   local unit

   for unit
   do
      inst_simple "/usr/lib/systemd/system/${unit}"
   done
}

brlttyInstallDirectory() {
   local path="${1}"
   [ -d "${path}" ] && inst_multiple $(find "${path}" -print)
}

