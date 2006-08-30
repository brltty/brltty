/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>
#include <langinfo.h>
#include <errno.h>

#include "tbl.h"
#include "tbl_internal.h"

static char *gbUserLocale;
static char *gbUtf8Locale;
static int gbInUcsBlock;

static char *
gbSetLocale (const char *locale) {
  return setlocale(LC_CTYPE, locale);
}

static int
gbBeginLocales (void) {
  const char *c;
  const char *modifier;
  int n, m = 0;
  gbUserLocale = gbSetLocale("");
  if (!gbUserLocale) return 0;
  gbUserLocale = strdup(gbUserLocale);
  if ((modifier = strchr(gbUserLocale, '@')))
    m = strlen(modifier);
  if ((c = strchr(gbUserLocale, '.')))
    n = c - gbUserLocale;
  else {
    if (modifier)
      n = modifier - gbUserLocale;
    else
      n = strlen(gbUserLocale);
  }
  gbUtf8Locale = malloc(n + 6 + m + 1);
  memcpy(gbUtf8Locale, gbUserLocale, n);
  memcpy(gbUtf8Locale + n, ".UTF-8", 6);
  if (modifier)
    memcpy(gbUtf8Locale + n + 6, modifier, m);
  gbUtf8Locale[n + 6 + m] = '\0';
  if (!gbSetLocale(gbUtf8Locale))
    return 0;
  return 1;
}

static void
gbEndLocales (void) {
  if (gbUtf8Locale) {
    free(gbUtf8Locale);
    gbUtf8Locale = NULL;
  }

  if (gbUserLocale) {
    gbSetLocale(gbUserLocale);
    free(gbUserLocale);
    gbUserLocale = NULL;
  }
}

static int
gbProcessEncodingDirective (TblInputData *input) {
  tblSkipSpace(input);
  if (!tblTestWord(input->location, tblFindSpace(input) - input->location, "UTF-8")) {
    tblReportError(input, "non-supported charset %s", input->location);
    return 0;
  }
  return 1;
}

static int
gbProcessUcsBlockDirective (TblInputData *input) {
  int length;
  tblSkipSpace(input);
  length = tblFindSpace(input) - input->location;
  if (!tblTestWord(input->location, length, "START")) {
    tblReportError(input, "expected START on line %s", input->line);
    return 0;
  } else {
    gbInUcsBlock = 1;
  }
  return 1;
}

static int
gbProcessUcsCharDirective (TblInputData *input) {
  int length;
  int n;
  wchar_t wc, wcdots;
  char c;

  tblSkipSpace(input);
  gbSetLocale(gbUtf8Locale);
  length = strlen((char *) input->location);

  n = mbtowc(&wc, (char *) input->location, length);
  if (n == -1) {
    tblReportError(input, "error %s while reading UTF-8 character %s", strerror(errno), input->location);
    return 1;
  }
  input->location += n;
  length -= n;

  if (!(*input->location)) {
    tblReportError(input, "expected dot pattern on line %s", input->line);
    return 1;
  }
  if (!isspace(*input->location)) {
    tblReportError(input, "can't handle multi-character translations");
    return 1;
  }

  tblSkipSpace(input);
  n = mbtowc(&wcdots, (char *) input->location, length);
  if (n == -1) {
    tblReportError(input, "error %s while reading UTF-8 dot pattern %s", strerror(errno), input->location);
    return 1;
  }

  if (input->location[n]) {
    tblReportError(input, "can't handle multi-dot pattern translations");
    return 1;
  }

  if ((wcdots & ~0xfful) != 0x2800) {
    tblReportError(input, "%s is not a dot pattern", input->location);
    return 1;
  }

  gbSetLocale(gbUserLocale);
  if (wctomb(&c, wc) != 1) {
    tblReportWarning(input, "unicode character U+%04lx can't be represented in current locale %s", wc, gbUserLocale);
    return 1;
  }

  input->bytes[(unsigned char) c].cell = wcdots & 0xffu;
  input->bytes[(unsigned char) c].defined = 1;
  return 1;
}

static int
gbProcessUcsBlockLine (TblInputData *input) {
  int length;

  tblSkipSpace(input);
  length = tblFindSpace(input) - input->location;
  if (tblTestWord(input->location, length, "UCS-BLOCK")) {
    input->location += length;
    tblSkipSpace(input);
    if (!tblTestWord(input->location, tblFindSpace(input) - input->location, "END"))
      tblReportError(input, "expected END on line %s", input->location);
    gbInUcsBlock = 0;
    return 0;
  }
  if (tblIsEndOfLine(input)) return 1;

  gbProcessUcsCharDirective(input);
  return 1;
}

static int
gbProcessUnicodeCharDirective (TblInputData *input) {
  unsigned char *err;
  wchar_t wc, wcdots;
  char c;

  tblSkipSpace(input);
  if (input->location[0] != 'U' || input->location[1] != '+') {
    tblReportError(input, "expected 'U+' at %s", input->location);
    return 1;
  }
  input->location += 2;
  wc = strtol((char*) input->location, (char**) &err, 16);
  if (err == input->location) {
    tblReportError(input, "expected character number at %s", input->location);
    return 1;
  }
  input->location = err;

  tblSkipSpace(input);
  if (input->location[0] != 'U' || input->location[1] != '+') {
    tblReportError(input, "expected 'U+' at %s", input->location);
    return 1;
  }
  input->location += 2;
  wcdots = strtol((char*) input->location, (char**) &err, 16);
  if (err == input->location) {
    tblReportError(input, "expected dot pattern number at %s", input->location);
    return 1;
  }

  if ((wcdots & ~0xfful) != 0x2800) {
    tblReportError(input, "%s is not a dot pattern", input->location);
    return 1;
  }

  gbSetLocale(gbUserLocale);
  if (wctomb(&c, wc) != 1) {
    tblReportError(input, "unicode character U+%04lx can't be represented in current locale %s", wc, gbUserLocale);
    return 1;
  }

  input->bytes[(unsigned char) c].cell = wcdots & 0xffu;
  input->bytes[(unsigned char) c].defined = 1;
  return 1;
}

static int
gbSkipDirectiveWarn (TblInputData *input) {
  tblReportError(input, "skipping directive %s", input->location);
  return 1;
}

static int
gbSkipDirective (TblInputData *input) {
  return 1;
}

static int
gbProcessLine (TblInputData *input) {
  tblSkipSpace(input);
  if (tblIsEndOfLine(input)) return 1;
  if (gbInUcsBlock) {
    gbProcessUcsBlockLine(input);
  } else {
    typedef struct {
      const char *name;
      int (*handler) (TblInputData *input);
    } DirectiveEntry;
    static const DirectiveEntry directiveTable[] = {
      {"encoding",     gbProcessEncodingDirective},
      {"name",         gbSkipDirective},
      {"locales",      gbSkipDirective},
      {"ucs-suffix",   gbSkipDirective},
      {"delegate",     gbSkipDirective},
      {"utf8-string",  gbSkipDirectiveWarn},
      {"ucs-block",    gbProcessUcsBlockDirective},
      {"ucs-char",     gbProcessUcsCharDirective},
      {"unicode-char", gbProcessUnicodeCharDirective},
      {"unknown-char", gbSkipDirective},
    };
    const DirectiveEntry *directive = directiveTable;
    int length = tblFindSpace(input) - input->location;

    while (directive->name) {
      if (tblTestWord(input->location, length, directive->name)) {
        input->location += length;
        break;
      }
      ++directive;
    }

    if (!directive->handler(input)) return 0;
  }
  return 1;
}

int
tblLoad_Gnome (const char *path, FILE *file, TranslationTable table, int options) {
  int ok = 0;
  if (gbBeginLocales()) {
    gbInUcsBlock = 0;
    if (tblProcessLines(path, file, table, gbProcessLine, options)) ok = 1;
    gbEndLocales();
  }
  return ok;
}
