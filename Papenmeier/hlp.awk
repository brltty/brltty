BEGIN {
  OFS = ""
  zyx
}
/^ *CMD_/ {
  if (x = match($0, "/.*/")) {
    print "{", $1, ", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/^ *STAT_/ {
  if (x = match($0, "/.*/")) {
    print "{OFFS_STAT+", $1, ", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/#define[ \t]*CR_/ {
  if (x = match($0, "/.*/")) {
    print "{", $2, ", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
/^ *VPK_/ {
  gsub(",", "", $1)
  print "{VAL_PASSKEY+", $1, ", \"send ", substr($1, 5), " key\"},"
  next
}
