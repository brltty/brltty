BEGIN {
  OFS = ""
}
/^ *BRL_CMD_/ {
  if (x = match($0, "/.*/")) {
    print "{", $1, ", \"", substr($1,9), "\", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/#define[ \t]*BRL_BLK_/ {
  if (x = match($0, "/.*/")) {
    print "{", $2, ", \"", substr($2,9), "\", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/^ *BRL_KEY_/ {
  gsub(",", "", $1)
  key = tolower(substr($1, 9))
  gsub("_", "-", key)
  print "{BRL_BLK_PASSKEY+", $1, ", \"", substr($1,9), "\", \"send ", key, " key\"},"
  next
}
