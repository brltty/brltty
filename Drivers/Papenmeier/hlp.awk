BEGIN {
  OFS = ""
}
/^ *STAT_/ {
  if (x = match($0, "/.*/")) {
    print "{OFFS_STAT+", $1, ", \"", substr($0, RSTART+3, RLENGTH-6), "\"},"
  }
  next
}
