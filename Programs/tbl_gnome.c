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
#include <errno.h>

#include "misc.h"
#include "charset.h"
#include "tbl.h"
#include "tbl_internal.h"

#ifdef HAVE_ICONV_H
static int gbInUcsBlock;

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
    tblReportError(input, "expected START in %s", input->location);
    return 0;
  } else {
    gbInUcsBlock = 1;
  }
  return 1;
}

static int
gbProcessUcsCharDirective (TblInputData *input) {
  size_t length;
  int c;
  wint_t wcdots;
  const unsigned char *pos;

  tblSkipSpace(input);

  pos = input->location;
  length = strlen((const char *) input->location);
  if ((c = convertUtf8ToChar((char **) &input->location, &length)) == EOF) {
    tblReportWarning(input, "while converting UTF-8 character %s into %s", pos, getCharset());
    return 1;
  }

  if (!(*input->location)) {
    tblReportError(input, "expected dot pattern in %s", input->location);
    return 1;
  }
  if (!isspace(*input->location)) {
    tblReportError(input, "can't handle multi-character translations: %s", input->location);
    return 1;
  }

  pos = input->location;
  tblSkipSpace(input);
  length -= input->location - pos;
  if ((wcdots = convertUtf8ToWchar((char **) &input->location, &length)) == EOF
    || (wcdots & ~0xffu) != BRL_UC_ROW) {
    tblReportError(input, "expected dot pattern in %s", input->location);
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
      tblReportError(input, "expected END in %s", input->location);
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
  wchar_t wcdots;
  wint_t wc;
  int c;

  tblSkipSpace(input);
  if (input->location[0] != 'U' || input->location[1] != '+') {
    tblReportError(input, "expected 'U+' at %s", input->location);
    return 1;
  }
  input->location += 2;
  wc = strtol((char*) input->location, (void*) &err, 16);
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
  wcdots = strtol((char*) input->location, (void*) &err, 16);
  if (err == input->location) {
    tblReportError(input, "expected dot pattern number at %s", input->location);
    return 1;
  }

  if ((wcdots & ~0xfful) != BRL_UC_ROW) {
    tblReportError(input, "%s is not a dot pattern", input->location);
    return 1;
  }

  if ((c = convertWcharToChar(wc)) == EOF)
    return 1;

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
  gbInUcsBlock = 0;
  if (tblProcessLines(path, file, table, gbProcessLine, options)) ok = 1;
  return ok;
}
#else
int
tblLoad_Gnome (const char *path, FILE *file, TranslationTable table, int options) {
  return 0;
}
#endif
