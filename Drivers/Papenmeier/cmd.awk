BEGIN {
  OFS = ""
}
/^ *CMD_/ {
  print "{\"", $1, "\", KEYCODE, ", $1, "},"
# print "{\"", substr($1, 5), "\", KEYCODE, ", $1, "},"
  next
}
/^ *STAT_/ {
  print "{\"", $1, "\", STATCODE, ", $1, "},"
# print "{\"", substr($1, 6), "\", STATCODE, ", $1, "},"
  next
}
/#define[ \t]*CR_/ {
  print "{\"", $2, "\", KEYCODE, " $2, "},"
# print "{\"", substr($2, 4), "\", KEYCODE, ", $2, "},"
  next
}
/^ *VPK_/ {
  gsub(",", "", $1)
  print "{\"", $1, "\", VPK, VAL_PASSKEY+", $1, "},"
# print "{\"", substr($1, 5), "\", VPK, VAL_PASSKEY+", $1, "},"
  next
}
