BEGIN {
  OFS = ""
}
/^ *BRL_CMD_/ {
  if (x = match($0, "/.*/")) {
    print "{", $1, ", \"", substr($1,9), "\", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/#define[ \t]*CR_/ {
  if (x = match($0, "/.*/")) {
    print "{", $2, ", \"", substr($2,4), "\", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/^ *VPK_/ {
  gsub(",", "", $1)
  key = tolower(substr($1, 5))
  gsub("_", "-", key)
  print "{VAL_PASSKEY+", $1, ", \"", substr($1,5), "\", \"send ", key, " key\"},"
  next
}
/#define[ \t]*VAL_PASS/ {
  if (x = match($0, "/.*/")) {
    print "{", $2, ", \"", substr($2,5), "\", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
