#!/bin/bash

readonly latitudeSetting="latitude"
readonly longitudeSetting="longitude"
readonly locationDescriptorSetting="location-descriptor"
readonly locationPhraseSetting="location-phrase"
readonly distanceUnitSetting="distance-unit"
readonly pressureUnitSetting="pressure-unit"
readonly speedUnitSetting="speed-unit"
readonly temperatureUnitSetting="temperature-unit"
readonly timeFormatSetting="time-format"

readonly -A distanceUnits=(
   ["ft"]="feet"
   ["km"]="kilometers"
   ["m"]="meters"
   ["mi"]="miles"
)

readonly -A pressureUnits=(
   ["hpa"]="hectopascals"
   ["inHg"]="inches of mercury"
   ["kpa"]="kilopascals"
   ["mb"]="millibars"
)

readonly -A speedUnits=(
   ["km/hr"]="kilometers per hour"
   ["m/s"]="meters per second"
   ["mph"]="miles per hour"
)

readonly -A temperatureUnits=(
   ["C"]="celsius"
   ["F"]="fahrenheit"
   ["K"]="kelvin"
   ["R"]="rankine"
)

readonly -A timeFormats=(
  ["24-hours"]="%H:%M"
  ["12-hours"]="%l:%M%p"
)

readonly -A metricUnits=(
   ["${distanceUnitSetting}"]="km"
   ["${pressureUnitSetting}"]="kpa"
   ["${speedUnitSetting}"]="km/hr"
   ["${temperatureUnitSetting}"]="C"
   ["${timeFormatSetting}"]="24-hours"
)

readonly -A imperialUnits=(
   ["${distanceUnitSetting}"]="mi"
   ["${pressureUnitSetting}"]="inHg"
   ["${speedUnitSetting}"]="mph"
   ["${temperatureUnitSetting}"]="F"
   ["${timeFormatSetting}"]="12-hours"
)

readonly unitsTypeNames=(metric imperial)
readonly windDirections=(N NNE NE ENE E ESE SE SSE S SSW SW WSW W WNW NW NNW)

toWindDirection() {
   local directionVariable="${1}"
   local degrees="${2}"

   local direction=$(( ((((degrees * 4) + 45) % 1440) / 90) ))
   direction="${windDirections[direction]}"
   setVariable "${directionVariable}" "${direction}"
}

parseCoordinate() {
   local valueVariable="${1}"
   local maximumValue="${2}"
   local positiveLetter="${3}"
   local negativeLetter="${4}"

   local value="${!valueVariable}"
   local pattern='^([-+])?(0|[1-9][0-9]*)?(\.[0-9]+)?([A-Za-z])?$'

   [[ "${value}" =~ ${pattern} ]] || return 1
   local sign="${BASH_REMATCH[1]}"
   local integer="${BASH_REMATCH[2]}"
   local fraction="${BASH_REMATCH[3]}"
   local letter="${BASH_REMATCH[4]}"

   if [ "${integer}" -eq "${maximumValue}" ]
   then
      fraction="${fraction##.*(0)}"
      [ -n "${fraction}" ] && return 1
   elif [ "${integer}" -gt "${maximumValue}" ]
   then
      return 1
   fi

   local negativeSign=false
   [ "${sign}" = "-" ] && negativeSign=true

   [ -n "${letter}" ] && {
      if [ "${letter}" = "${negativeLetter}" ]
      then
         "${negativeSign}" && negativeSign=false || negativeSign=true
      elif [ "${letter}" != "${positiveLetter}" ]
      then
         return 1
      fi
   }

   value="${integer:-0}${fraction}"
   "${negativeSign}" && value="-${value}"
   setVariable "${valueVariable}" "${value}"
   return 0
}

readonly positiveLatitudeLetter="N"
readonly negativeLatitudeLetter="S"

parseLatitude() {
   local latitudeVariable="${1}"

   parseCoordinate "${latitudeVariable}" 90 "${positiveLatitudeLetter}" "${negativeLatitudeLetter}" || return 1
   return 0
}

readonly positiveLongitudeLetter="E"
readonly negativeLongitudeLetter="W"

parseLongitude() {
   local longitudeVariable="${1}"

   parseCoordinate "${longitudeVariable}" 180 "${positiveLongitudeLetter}" "${negativeLongitudeLetter}" || return 1
   return 0
}

