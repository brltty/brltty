BEGIN {
  OFS = ""
}
/^ *BRL_CMD_/ {
  print "{\"", substr($1, 5), "\", KEYCODE, ", $1, "},"
  next
}
/^ *STAT_/ {
  print "{\"", $1, "\", STATCODE, ", $1, "},"
# print "{\"", substr($1, 6), "\", STATCODE, ", $1, "},"
  next
}
/#define[ \t]*BRL_BLK_/ {
  print "{\"", substr($2, 5), "\", KEYCODE, " $2, "},"
  next
}
/^ *VPK_/ {
  gsub(",", "", $1)
  print "{\"", $1, "\", VPK, BRL_BLK_PASSKEY+", $1, "},"
  next
}
