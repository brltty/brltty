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

#include <string.h>
#include <wchar.h>

#include "cmdline.h"
#include "utf8.h"
#include "unicode.h"

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "utf8test",
  .purpose = strtext("Test the incremental (streaming) UTF-8 decoder."),

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

#define REPL UNICODE_REPLACEMENT_CHARACTER

static unsigned int testsRun = 0;
static unsigned int testsFailed = 0;

/* Run a byte stream through the decoder, collecting every emitted character
 * (both successfully decoded code points and replacement characters), honoring
 * the reprocess protocol exactly as the terminal parser does.
 */
static unsigned int
decodeBytes (const unsigned char *bytes, size_t count, wchar_t *out, unsigned int max) {
  Utf8StreamDecoder decoder;
  memset(&decoder, 0, sizeof(decoder));

  unsigned int emitted = 0;
  size_t index = 0;

  while (index < count) {
    wchar_t character;
    int reprocess;
    Utf8DecodeResult result = putUtf8StreamByte(&decoder, bytes[index], &character, &reprocess);

    if (result == UTF8_DECODE_PENDING) {
      index += 1;
      continue;
    }

    if (emitted < max) out[emitted++] = character;

    if ((result == UTF8_DECODE_INVALID) && reprocess) {
      /* Feed the same byte again from a fresh state. */
      continue;
    }

    index += 1;
  }

  return emitted;
}

static void
checkCase (
  const char *name,
  const unsigned char *bytes, size_t byteCount,
  const wchar_t *expected, unsigned int expectedCount
) {
  wchar_t got[64];
  testsRun += 1;

  unsigned int gotCount = decodeBytes(bytes, byteCount, got, ARRAY_COUNT(got));
  int ok = gotCount == expectedCount;

  if (ok) {
    for (unsigned int i=0; i<gotCount; i+=1) {
      if (got[i] != expected[i]) {
        ok = 0;
        break;
      }
    }
  }

  if (ok) {
    fprintf(stderr, "ok   %s\n", name);
    return;
  }

  testsFailed += 1;
  fprintf(stderr, "FAIL %s\n", name);

  fprintf(stderr, "       got (%u):", gotCount);
  for (unsigned int i=0; i<gotCount; i+=1) fprintf(stderr, " U+%04X", (unsigned int)got[i]);
  fprintf(stderr, "\n");

  fprintf(stderr, "  expected (%u):", expectedCount);
  for (unsigned int i=0; i<expectedCount; i+=1) fprintf(stderr, " U+%04X", (unsigned int)expected[i]);
  fprintf(stderr, "\n");
}

#define BYTES(...) ((const unsigned char []){__VA_ARGS__})
#define CHARS(...) ((const wchar_t []){__VA_ARGS__})
#define CHECK(name, bytes, chars) \
  checkCase((name), (bytes), sizeof(bytes), (chars), ARRAY_COUNT(chars))

static void
runTests (void) {
  /* plain ASCII */
  CHECK("ascii letters", BYTES('H','i','!'), CHARS('H','i','!'));
  CHECK("ascii control (tab)", BYTES(0X09), CHARS(0X09));

  /* well-formed multi-byte sequences */
  CHECK("2-byte e-acute", BYTES(0XC3,0XA9), CHARS(0X00E9));
  CHECK("3-byte box hline", BYTES(0XE2,0X94,0X80), CHARS(0X2500));
  CHECK("3-byte braille spinner", BYTES(0XE2,0XA0,0X8B), CHARS(0X280B));

  /* Claude Code's rounded input-box corners and edge */
  CHECK("rounded box top",
    BYTES(0XE2,0X95,0XAD, 0XE2,0X94,0X80, 0XE2,0X95,0XAE),
    CHARS(0X256D, 0X2500, 0X256E));

  /* mixed ascii and multi-byte */
  CHECK("mixed a-hline-b", BYTES('a', 0XE2,0X94,0X80, 'b'), CHARS('a', 0X2500, 'b'));

#if WCHAR_MAX >= 0X10FFFF
  CHECK("4-byte emoji", BYTES(0XF0,0X9F,0X98,0X80), CHARS(0X1F600));
  CHECK("max code point", BYTES(0XF4,0X8F,0XBF,0XBF), CHARS(0X10FFFF));
  CHECK("beyond unicode", BYTES(0XF4,0X90,0X80,0X80), CHARS(REPL));
#endif /* WCHAR_MAX */

  /* malformed input */
  CHECK("lone continuation", BYTES(0X80), CHARS(REPL));
  CHECK("invalid lead 0xFF", BYTES(0XFF), CHARS(REPL));
  CHECK("invalid lead 0xF8", BYTES(0XF8), CHARS(REPL));

  /* truncation: the partial sequence becomes U+FFFD and the interrupting
   * byte is reprocessed as a fresh character */
  CHECK("truncated then ascii", BYTES(0XE2,0X94,'X'), CHARS(REPL, 'X'));
  CHECK("truncated then new lead",
    BYTES(0XE2, 0XC3,0XA9), CHARS(REPL, 0X00E9));
  CHECK("complete then stray continuation",
    BYTES(0XC3,0XA9, 0XA9), CHARS(0X00E9, REPL));

  /* overlong encodings must be rejected */
  CHECK("overlong 2-byte slash", BYTES(0XC0,0XAF), CHARS(REPL));
  CHECK("overlong 3-byte nul", BYTES(0XE0,0X80,0X80), CHARS(REPL));

  /* surrogates are not valid UTF-8 */
  CHECK("utf-16 surrogate", BYTES(0XED,0XA0,0X80), CHARS(REPL));
}

int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  runTests();

  fprintf(stderr, "\n%u test(s) run, %u failed\n", testsRun, testsFailed);
  return testsFailed? PROG_EXIT_FATAL: PROG_EXIT_SUCCESS;
}
