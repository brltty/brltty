function normalizePath(path) {
  if (length(path) == 0) path = "."
  path = path pathDelimiter
  gsub((pathDelimiter "+"), pathDelimiter, path)
  return path
}

BEGIN {
  pathDelimiter = "/"

  pathLength = length(path = normalizePath(path))
  referenceLength = length(reference = normalizePath(reference))

  minimumLength = pathLength
  if (referenceLength < minimumLength) minimumLength = referenceLength

  prefixLength = 0
  for (i=1; i<=minimumLength; ++i) {
    character = substr(reference, i, 1)
    if (substr(path, i, 1) != character) break
    if (character == pathDelimiter) prefixLength = i
  }

  if ((prefixLength > 0) || ((substr(path, 1, 1) == pathDelimiter) == (substr(reference, 1, 1) == pathDelimiter))) {
    suffix = substr(path, prefixLength+1)
    prefix = substr(reference, prefixLength+1)
    gsub(("[^" pathDelimiter "]+"), "..", prefix)
    path = normalizePath(prefix suffix)
  }

  path = normalizePath(path)
  sub((pathDelimiter "$"), "", path)

  printf "%s", path
  exit 0
}
