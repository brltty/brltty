BEGIN {
  OFS = ""
}
/^ *BRL_CMD_/ {
  print "{\"", substr($1, 5), "\", KEYCODE, ", $1, "},"
  next
}
/^ *BRL_GSC_/ {
  print "{\"", substr($1, 5), "\", STATCODE, ", $1, "},"
  next
}
/#define[ \t]*BRL_BLK_PASS/ {
  next
}
/#define[ \t]*BRL_BLK_/ {
  print "{\"", substr($2, 5), "\", KEYCODE, " $2, "},"
  next
}
/^ *BRL_KEY_/ {
  gsub(",", "", $1)
  print "{\"", substr($1, 5), "\", VPK, BRL_BLK_PASSKEY+", $1, "},"
  next
}
