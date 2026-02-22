/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "cmdline.h"
#include "cmdput.h"
#include "copysmart.h"
#include "parse.h"

static char *opt_text    = NULL;
static char *opt_offset  = NULL;
static int   opt_verbose = 0;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "text",
    .letter = 't',
    .argument = "string",
    .setting.string = &opt_text,
    .description = "UTF-8 text to test"
  },

  { .word = "offset",
    .letter = 'o',
    .argument = "offset",
    .setting.string = &opt_offset,
    .description = "zero-based character offset of the cursor"
  },

  { .word = "verbose",
    .letter = 'v',
    .setting.flag = &opt_verbose,
    .description = "print source buffer and match result for every test"
  },
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
  "Without options, runs all built-in test cases.",
  "With --text and --target, tests a single string and prints the result.",
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "copysmarttest",
  .purpose = strtext("Test COPY_SMART URL/email/hostname detection."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

/* ============================================================
 * Test harness
 * ============================================================ */

static int testCount  = 0;
static int passCount  = 0;
static int failCount  = 0;

static void
runTest (const char *desc, const wchar_t *input, int target,
      const wchar_t *expected) {
  testCount += 1;

  int len = (int)wcslen(input);
  int matchOffset, matchLength;
  int matched = cpbSmartMatch(input, len, target, &matchOffset, &matchLength);

  int pass;
  if (expected == NULL) {
    pass = !matched;
  } else {
    int expectedLen = (int)wcslen(expected);
    pass = matched && matchLength == expectedLen &&
           wcsncmp(input + matchOffset, expected, (size_t)expectedLen) == 0;
  }

  if (pass) {
    passCount += 1;
    putf("  PASS: %s\n", desc);
  } else {
    failCount += 1;
    putf("  FAIL: %s\n", desc);

    if (expected == NULL) {
      putf("    expected: no match\n");
      if (matched) putf("    got:      \"%.*ls\"\n", matchLength, input + matchOffset);
    } else {
      putf("    expected: \"%ls\"\n", expected);

      if (matched) {
        putf("    got:      \"%.*ls\"\n", matchLength, input + matchOffset);
      } else {
        putf("    got:      no match\n");
      }
    }
  }

  if (opt_verbose) {
    putf("    buffer:   \"%ls\" (target=%d)\n", input, target);

    if (matched) {
      putf("    result:   \"%.*ls\"\n", matchLength, input + matchOffset);
    } else {
      putf("    result:   no match\n");
    }
  }
}

/* ============================================================
 * Test cases
 * ============================================================
 * Rows with input==NULL are section headers (desc is printed as-is).
 */

typedef struct {
  const char    *desc;
  const wchar_t *input;
  int            target;
  const wchar_t *expected;
} TestDescriptor;

static const TestDescriptor tests[] = {
  /* URL detection */
  { "\nURL detection:" },
  { "cursor on scheme letter",                  L"https://example.com/path?q=1#frag",     2,  L"https://example.com/path?q=1#frag" },
  { "cursor on path character",                 L"https://example.com/path?q=1#frag",     22, L"https://example.com/path?q=1#frag" },
  { "URL embedded in text",                     L"visit https://foo.bar/baz for details",  10, L"https://foo.bar/baz"               },
  { "trailing period stripped",                 L"see https://foo.bar.",                   10, L"https://foo.bar"                   },
  { "trailing comma stripped",                  L"see https://foo.bar,",                   10, L"https://foo.bar"                   },
  { "trailing semicolon stripped",              L"see https://foo.bar;",                   10, L"https://foo.bar"                   },
  { "unmatched ) stripped",                     L"(see https://foo.bar/baz)",               9, L"https://foo.bar/baz"               },
  { "matched parens preserved",                 L"https://foo.bar/f(x)",                    0, L"https://foo.bar/f(x)"              },
  { "ftp:// scheme",                            L"ftp://files.example.org/pub/",            0, L"ftp://files.example.org/pub/"      },
  { "percent-encoded character",                L"https://example.com/a%20b",               0, L"https://example.com/a%20b"         },
  { "non-ASCII character in path",             L"http://foo.bar/\u00e9",                   0, L"http://foo.bar/\u00e9"             },
  { "no-scheme hostname matched by hostname detector", L"www.example.com",                  0, L"www.example.com"                   },
  { "email matched by email detector",          L"user@example.com",                        0, L"user@example.com"                  },
  { "URL stops at double-quote (HTML attribute)", L"href=\"https://example.com\"",          10, L"https://example.com"               },
  { "URL stops at > (angle-bracketed URL)",     L"<https://example.com>",                   5, L"https://example.com"               },

  /* Email detection */
  { "\nEmail detection:" },
  { "cursor on @",                              L"user@example.com",                        4, L"user@example.com"                  },
  { "cursor on first char of local part",       L"user@example.com",                        0, L"user@example.com"                  },
  { "cursor in middle of local part",           L"user@example.com",                        2, L"user@example.com"                  },
  { "cursor on first char of domain",           L"user@example.com",                        5, L"user@example.com"                  },
  { "cursor on TLD",                            L"user@example.com",                       13, L"user@example.com"                  },
  { "cursor on _ in local part",                L"user_name@example.com",                   4, L"user_name@example.com"             },
  { "plus in local part",                       L"user+tag@example.com",                    4, L"user+tag@example.com"              },
  { "single-label domain (root@localhost)",     L"root@localhost",                           0, L"root@localhost"                    },
  { "single-label short hostname (nico@xanadu)", L"nico@xanadu",                            0, L"nico@xanadu"                       },
  { "subdomain in email",                       L"user@mail.example.co.uk",                 0, L"user@mail.example.co.uk"           },
  { "email embedded in text (cursor on local)", L"contact nico@example.com please",         9, L"nico@example.com"                  },
  { "email embedded in text (cursor on domain)", L"contact nico@fluxnic.net please",       16, L"nico@fluxnic.net"                  },
  { "no local part rejected",                   L"@example.com",                            0, NULL                                },
  { "no domain rejected",                       L"user@",                                   0, NULL                                },
  { "bare word not matched",                    L"hello",                                   0, NULL                                },

  /* Hostname detection */
  { "\nHostname detection:" },
  { "www prefix accepted",                      L"www.example.com",                         0, L"www.example.com"                   },
  { "www prefix, cursor on TLD",                L"www.example.com",                        12, L"www.example.com"                   },
  { "known TLD (com) without www",              L"example.com",                             0, L"example.com"                       },
  { "known TLD (org)",                          L"example.org",                             0, L"example.org"                       },
  { "known TLD (io)",                           L"example.io",                              0, L"example.io"                        },
  { "country code TLD (fr)",                    L"example.fr",                              0, L"example.fr"                        },
  { "country code TLD (uk)",                    L"bbc.co.uk",                               0, L"bbc.co.uk"                         },
  { "unknown TLD (xyz) without www rejected",   L"example.xyz",                             0, NULL                                },
  { "no dot (bare word) rejected",              L"localhost",                                0, NULL                                },
  { "www with unknown TLD accepted",            L"www.foo.xyz",                             0, L"www.foo.xyz"                       },
  { "hostname embedded in text",                L"visit www.example.com for info",           6, L"www.example.com"                   },
  { "URL takes priority over hostname",         L"https://example.com",                    10, L"https://example.com"               },

  /* Negative cases */
  { "\nNegative cases:" },
  { "space character",                          L"hello world",                             5, NULL                                },
  { "plain word",                               L"hello",                                   2, NULL                                },
  { "number alone",                             L"12345",                                   2, NULL                                },
  { "punctuation only",                         L"...",                                     1, NULL                                },

  /* Multiple targets — cursor selects the right one;
   * surrounding non-target characters must not bleed into the result. */
  { "\nMultiple targets / surrounding characters:" },
  { "two URLs, cursor on first",                L"https://first.com and https://second.org",  8,  L"https://first.com"              },
  { "two URLs, cursor on second",               L"https://first.com and https://second.org",  30, L"https://second.org"             },
  { "two emails, cursor on first",              L"foo@a.com and bar@b.com",                   4,  L"foo@a.com"                      },
  { "two emails, cursor on second",             L"foo@a.com and bar@b.com",                   17, L"bar@b.com"                      },
  { "two hostnames, cursor on first",           L"example.com and example.org",               5,  L"example.com"                    },
  { "two hostnames, cursor on second",          L"example.com and example.org",               22, L"example.org"                    },
  { "email before URL, cursor on email",        L"user@example.com or https://example.com",   8,  L"user@example.com"               },
  { "email before URL, cursor on URL",          L"user@example.com or https://example.com",   24, L"https://example.com"            },
  { "URL before email, cursor on URL",          L"https://example.com or user@example.org",   8,  L"https://example.com"            },
  { "URL before email, cursor on email",        L"https://example.com or user@example.org",   26, L"user@example.org"               },
  { "email in angle brackets",                  L"<user@example.com>",                         8,  L"user@example.com"               },
  { "hostname in square brackets",              L"[example.com]",                              5,  L"example.com"                    },
  { "URL in parens preceded by URL, cursor on first",  L"https://first.com (https://second.org)",  8,  L"https://first.com"          },
  { "URL in parens preceded by URL, cursor on second", L"https://first.com (https://second.org)", 27, L"https://second.org"          },
};

static void
runBuiltinTests (void) {
  putf("COPY_SMART detection tests\n");
  putf("==========================\n");

  for (size_t i = 0; i < ARRAY_COUNT(tests); i += 1) {
    const TestDescriptor *test = &tests[i];

    if (!test->input) {
      putf("%s\n", test->desc);
    } else {
      runTest(test->desc, test->input, test->target, test->expected);
    }
  }

  putf("\n==========================\n");
}

/* ============================================================
 * Main
 * ============================================================ */

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int haveOffset = opt_offset && *opt_offset;
  int targetOffset;

  if (haveOffset) {
    static int minimum = 0;

    if (!validateInteger(&targetOffset, opt_offset, &minimum, NULL)) {
      logMessage(LOG_ERR, "invalid offset: %s", opt_offset);
      return PROG_EXIT_SYNTAX;
    }
  }

  if (opt_text && *opt_text) {
    size_t textLen = strlen(opt_text);
    if (!haveOffset) targetOffset = 0;

    if (targetOffset > textLen) {
      logMessage(LOG_ERR,
        "offset greater than text length: %d > %" PRIsize,
        targetOffset, textLen
      );

      return PROG_EXIT_SEMANTIC;
    }

    wchar_t *wtext = malloc((textLen + 1) * sizeof(wchar_t));

    if (!wtext) {
      logMallocError();
      return PROG_EXIT_FATAL;
    }

    size_t wlen = mbstowcs(wtext, opt_text, textLen);

    if (wlen == (size_t)-1) {
      free(wtext);
      logMessage(LOG_ERR, "invalid UTF-8 byte in text");
      return PROG_EXIT_SYNTAX;
    }

    int matchOffset, matchLength;
    int matched = cpbSmartMatch(wtext, (int)wlen, targetOffset, &matchOffset, &matchLength);

    if (matched) {
      putf("%.*ls\n", matchLength, wtext + matchOffset);
    } else {
      logMessage(LOG_WARNING, "no match");
    }

    free(wtext);
    if (!matched) return PROG_EXIT_SEMANTIC;
  } else {
    if (haveOffset) {
      logMessage(LOG_ERR, "offset without text");
      return PROG_EXIT_SYNTAX;
    }

    runBuiltinTests();
    int failed = !!failCount;

    putf("Results: %d/%d passed", passCount, testCount);
    if (failed) putf(", %d FAILED", failCount);
    putf("\n");

    if (failed) return PROG_EXIT_SEMANTIC;
  }

  return PROG_EXIT_SUCCESS;
}
