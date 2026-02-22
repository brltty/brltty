/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <wchar.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "utf8.h"

#ifdef HAVE_REGEX_H
#include <regex.h>

/* POSIX ERE patterns tried in order.  The first match whose byte range
 * contains the cursor position wins. */
static struct {
  const char *source;
  int         matchGroup;  /* capture group holding the match bounds */
  regex_t     compiled;
} patterns[] = {
  /* 0: URL with scheme (highest priority)
   *   scheme:  letter then letters/digits/+/./- (e.g. https, ftp, git+ssh)
   *   ://      separator
   *   rest:    RFC 3986 characters only — unreserved (-._~), pct-encoded (%),
   *            sub-delims (!$&'()*+,;=), gen-delims (:/?#[]@) */
  { "[[:alpha:]][[:alnum:]+.-]*://[][[:alnum:]_.~:/?#@!$&'()*+,;=%-]*", 0 },

  /* 1: Email address
   *   local:   letters/digits and ._%+- before the @
   *   domain:  labels of letters/digits separated by dots or hyphens */
  { "[[:alnum:]._%+-]+@[[:alnum:]][[:alnum:].-]*", 0 },

  /* 2: www. hostname (no scheme required)
   *   www.:    literal prefix (case handled by REG_ICASE)
   *   rest:    labels of letters/digits/dots/hyphens (greedy, so the
   *            trailing character is always non-alnum — no boundary needed) */
  { "www\\.[[:alnum:]][[:alnum:].-]*", 0 },

  /* 3: hostname with a known multi-letter TLD
   *   labels:  explicitly structured as ([[:alnum:]][[:alnum:]-]*\\.)+ so
   *            that dots are only consumed by literal \\. — no backtracking
   *   tld:     one of the recognised suffixes
   *   group 1 captures the hostname; ([^[:alnum:]]|$) is a word boundary
   *   that prevents matching e.g. "example.io" inside "example.iox" */
  { "(([[:alnum:]][[:alnum:]-]*\\.)+(com|org|net|edu|gov|mil|int|io|dev|app|info|biz|name|pro|coop|museum))([^[:alnum:]]|$)", 1 },

  /* 4: hostname with a two-letter country-code TLD
   *   labels:  same structure as pattern 3
   *   tld:     exactly two letters
   *   group 1 captures the hostname; ([^[:alnum:]]|$) is a word boundary
   *   that prevents matching e.g. "example.xy" inside "example.xyz" */
  { "(([[:alnum:]][[:alnum:]-]*\\.)+[[:alpha:]][[:alpha:]])([^[:alnum:]]|$)", 1 },
};

#define PATTERN_COUNT (sizeof(patterns) / sizeof(patterns[0]))

static int patternsCompiled = 0;

static int
ensurePatterns (void) {
  if (patternsCompiled) return 1;

  for (size_t i = 0; i < PATTERN_COUNT; i += 1) {
    if (regcomp(&patterns[i].compiled, patterns[i].source, REG_EXTENDED | REG_ICASE) != 0) {
      for (size_t j = 0; j < i; j += 1) {
        regfree(&patterns[j].compiled);
      }

      return 0;
    }
  }

  patternsCompiled = 1;
  return 1;
}

/* Strip trailing .,; and unmatched ) from a byte (ASCII) string.
 * Returns the new byte count. */
static int
stripTrailingPunct (const char *text, int count) {
  while (count > 0) {
    char last = text[count - 1];

    if (last == '.' || last == ',' || last == ';') {
      count -= 1;
    } else if (last == ')' && !memchr(text, '(', (size_t)count)) {
      count -= 1;
    } else {
      break;
    }
  }

  return count;
}

/* Return the byte length of the UTF-8 codepoint whose lead byte is `c`. */
static inline size_t
utf8CharLen (unsigned char c) {
  if (c >= 0xF0) return 4;
  if (c >= 0xE0) return 3;
  if (c >= 0xC0) return 2;
  return 1;
}

/* Return the byte offset of the `n`-th codepoint in a UTF-8 string. */
static size_t
utf8ByteOffset (const char *utf8, int n) {
  const char *p = utf8;
  for (int i = 0; i < n; i += 1) p += utf8CharLen((unsigned char)*p);
  return (size_t)(p - utf8);
}

/* Return the number of codepoints in the first `bytes` bytes of a UTF-8 string. */
static int
utf8WcharCount (const char *utf8, size_t bytes) {
  const char *end = utf8 + bytes;
  int count = 0;

  while (utf8 < end) {
    utf8 += utf8CharLen((unsigned char)*utf8);
    count += 1;
  }

  return count;
}

int
matchSmart (const wchar_t *buf, int len, int target,
               int *matchOffset, int *matchLength) {
  if (len <= 0 || target < 0 || target >= len) return 0;

  size_t utf8Len;
  char *utf8 = getUtf8FromWchars(buf, (unsigned int)len, &utf8Len);
  if (!utf8) return 0;

  size_t targetByte = utf8ByteOffset(utf8, target);

  if (!ensurePatterns()) {
    free(utf8);
    return 0;
  }

  int found = 0;

  for (size_t p = 0; p < PATTERN_COUNT && !found; p += 1) {
    size_t offset = 0;
    int g = patterns[p].matchGroup;
    regmatch_t pmatch[5];

    while (regexec(&patterns[p].compiled, utf8 + offset, 5, pmatch,
                   (offset > 0) ? REG_NOTBOL : 0) == 0) {
      if (pmatch[0].rm_so < 0 || pmatch[0].rm_eo <= pmatch[0].rm_so) break;

      size_t mStart = offset + (size_t)pmatch[g].rm_so;
      size_t mEnd   = offset + (size_t)pmatch[g].rm_eo;

      /* Past the cursor — no later match can contain it. */
      if (mStart > targetByte) break;

      if (targetByte < mEnd) {
        /* This match contains the cursor — strip trailing punctuation. */
        int byteCount = stripTrailingPunct(utf8 + mStart,
                                           (int)(mEnd - mStart));

        if (byteCount > 0) {
          *matchOffset = utf8WcharCount(utf8, mStart);
          *matchLength = utf8WcharCount(utf8 + mStart, (size_t)byteCount);
          found = 1;
        }

        break;
      }

      offset += (size_t)pmatch[0].rm_eo;
    }
  }

  free(utf8);
  return found;
}

#else /* !HAVE_REGEX_H */

int
matchSmart (const wchar_t *buf, int len, int target,
               int *matchOffset, int *matchLength) {
  return 0;
}

#endif /* HAVE_REGEX_H */
