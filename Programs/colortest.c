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

/* Color library test program
 *
 * This program tests the color conversion and description library (color.c/color.h).
 * It includes tests for:
 * - VGA color palette round-trip conversion (RGB -> VGA -> RGB)
 * - HSV color space conversions (RGB <-> HSV)
 * - Human-readable color descriptions with HSV analysis
 * - RGB to VGA nearest-color mapping
 *
 * Test Color References:
 * The test suite uses standard CSS/HTML Named Colors from the W3C CSS Color Module
 * Level 3 specification (https://www.w3.org/TR/css-color-3/#svg-color). These are
 * well-established color standards used across web browsers, design tools, and the
 * X11 color system. Using standard colors ensures our detection algorithms work
 * with commonly recognized color names.
 *
 * Some tests include alternate acceptable descriptions for edge cases where multiple
 * valid color names exist (e.g., pure green can be described as both "vivid green"
 * and "lime" depending on the detection criteria used).
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "cmdline.h"
#include "color.h"
#include "parse.h"
#include "file.h"
#include "queue.h"

typedef enum {
   OPTQ_INFO,
   OPTQ_PASS,
   OPTQ_WARN,
   OPTQ_FAIL,
   OPTQ_RESULT,
   OPTQ_TEST,
} OptQuietness;

static int opt_quietness;
static int opt_enterInteractiveMode;
static int opt_performAllTests;

static int opt_testVGAtoRGBtoVGA;
static int opt_testVGADescriptions;
static int opt_testRGBtoHSVtoRGB;
static int opt_testColorRecognition;
static int opt_testRGBtoVGA;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "quieter",
    .letter = 'q',
    .setting.flag = &opt_quietness,
    .flags = OPT_Extend,
    .description = "reduce verbosity - this option is cumulative",
  },

  { .word = "interactive-mode",
    .letter = 'i',
    .setting.flag = &opt_enterInteractiveMode,
    .description = "enter interactive mode after performing the requested tests",
  },

  { .word = "all-tests",
    .letter = 'a',
    .setting.flag = &opt_performAllTests,
    .description = "perform all of the tests - conflicts with requesting specific tests",
  },

  { .word = "vga-roundtrip",
    .letter = 'v',
    .setting.flag = &opt_testVGAtoRGBtoVGA,
    .description = "test VGA to RGB to VGA round-trip - conflicts with requesting all tests",
  },

  { .word = "vga-descriptions",
    .letter = 'd',
    .setting.flag = &opt_testVGADescriptions,
    .description = "list VGA color descriptions - conflicts with requesting all tests",
  },

  { .word = "rgb-roundtrip",
    .letter = 'r',
    .setting.flag = &opt_testRGBtoHSVtoRGB,
    .description = "test RGB to HSV to RGB round-trip - conflicts with requesting all tests",
  },

  { .word = "color-recognition",
    .letter = 'c',
    .setting.flag = &opt_testColorRecognition,
    .description = "test recognition of common colors - conflicts with requesting all tests",
  },

  { .word = "vga-mappings",
    .letter = 'm',
    .setting.flag = &opt_testRGBtoVGA,
    .description = "show some RGB to nearest VGA mappings - conflicts with requesting all tests",
  },
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

#define VGA_NAME_FORMAT "(%13s)"
#define VGA_COLOR_FORMAT "VGA %2d"
#define RGB_COLOR_FORMAT "RGB(%3d, %3d, %3d)"
#define HSV_COLOR_FORMAT "HSV(%5.1f°, %3.0f%%, %3.0f%%)"

static void
checkForOutputError (void) {
  if (ferror(stdout)) {
    logSystemError("standard output write");
    exit(PROG_EXIT_FATAL);
  }
}

static void
flushOutput (void) {
  fflush(stdout);
  checkForOutputError();
}

static void
putf (const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  checkForOutputError();
}

static void
putNotice (const char *notice) {
  size_t dividerWidth = 50;
  unsigned int noticeIndent;

  {
    size_t noticeLength = strlen(notice);

    if (noticeLength > dividerWidth) {
      dividerWidth = noticeLength;
      noticeIndent = 0;
    } else {
      noticeIndent = (dividerWidth - noticeLength) / 2;
    }
  }

  char divider[dividerWidth + 1];
  memset(divider, '=', dividerWidth);
  divider[dividerWidth] = 0;

  char indent[noticeIndent + 1];
  memset(indent, ' ', noticeIndent);
  indent[noticeIndent] = 0;

  putf("%s\n%s%s\n%s\n", divider, indent, notice, divider);
}

static void
putTestHeader (const char *name) {
  putf("=== %s ===\n", name);
}

static int
putTestResult (const char *name, int count, int passes) {
  int hasPassed = passes == count;

  if (!hasPassed) {
    if (opt_quietness <= OPTQ_RESULT) {
      putf("%d/%d tests failed\n", (count - passes), count);
    }
  }

  if (opt_quietness <= OPTQ_TEST) {
    putf("[%s] %s\n", (hasPassed? "PASS": "FAIL"), name);
  }

  return hasPassed;
}

/* Test structure for predefined colors */
typedef struct {
  const char *name;
  unsigned char r, g, b;
  const char *expectedDescription;
  const char *alternateDescription;  /* Optional alternate acceptable description */
  const char *alternateReason;       /* Reason why alternate is acceptable */
} ColorTest;

/* Test VGA palette round-trip conversion */
static int
testVGAtoRGBtoVGA (const char *testName) {
  const int testCount = VGA_COLOR_COUNT;
  int passCount = 0;

  for (int vga=0; vga<testCount; vga+=1) {
    const char *name = vgaColorName(vga);

    RGBColor rgb = vgaToRgb(vga);
    int vgaBack = rgbColorToVga(rgb);

    int passed = vga == vgaBack;
    if (passed) passCount += 1;

    if (opt_quietness <= (passed? OPTQ_PASS: OPTQ_FAIL)) {
      putf(VGA_COLOR_FORMAT " " VGA_NAME_FORMAT ": " RGB_COLOR_FORMAT " -> " VGA_COLOR_FORMAT " [%s]\n",
           vga, name, rgb.r, rgb.g, rgb.b, vgaBack,
           (passed? "OK": "FAIL"));
    }
  }

  return putTestResult(testName, testCount, passCount);
}

/* List VGA colors */
static int
testVGADescriptions (const char *testName) {
  if (opt_quietness <= OPTQ_PASS) {
    for (int vga=0; vga<VGA_COLOR_COUNT; vga+=1) {
      const char *name = vgaColorName(vga);

      RGBColor rgb = vgaToRgb(vga);
      char description[64];
      rgbColorToDescription(description, sizeof(description), rgb);

      putf(VGA_COLOR_FORMAT " " VGA_NAME_FORMAT ": " RGB_COLOR_FORMAT " -> \"%s\"\n",
           vga, name, rgb.r, rgb.g, rgb.b, description);
    }
  }

  return 1;
}

/* Test RGB conversion round-trip */
static int
testRGBtoHSVtoRGB (const char *testName) {
  static const ColorTest tests[] = {
    {"Pure Red",     255, 0,   0,   NULL, NULL, NULL},
    {"Pure Green",   0,   255, 0,   NULL, NULL, NULL},
    {"Pure Blue",    0,   0,   255, NULL, NULL, NULL},
    {"White",        255, 255, 255, NULL, NULL, NULL},
    {"Black",        0,   0,   0,   NULL, NULL, NULL},
    {"Grey",         128, 128, 128, NULL, NULL, NULL},
    {"Yellow",       255, 255, 0,   NULL, NULL, NULL},
    {"Cyan",         0,   255, 255, NULL, NULL, NULL},
    {"Magenta",      255, 0,   255, NULL, NULL, NULL},
    {"Orange",       255, 165, 0,   NULL, NULL, NULL},
  };

  const int testCount = ARRAY_COUNT(tests);
  int passCount = 0;

  for (int i=0; i<testCount; i+=1) {
    const ColorTest *test = &tests[i];

    /* RGB -> HSV -> RGB */
    HSVColor hsv = rgbToHsv(test->r, test->g, test->b);
    RGBColor rgb = hsvColorToRgb(hsv);

    /* Allow small rounding errors (±1) */
    int rDiff = abs((int)rgb.r - (int)test->r);
    int gDiff = abs((int)rgb.g - (int)test->g);
    int bDiff = abs((int)rgb.b - (int)test->b);

    int passed = (rDiff <= 1) && (gDiff <= 1) && (bDiff <= 1);
    if (passed) passCount += 1;

    if (opt_quietness <= (passed? OPTQ_PASS: OPTQ_FAIL)) {
      putf("%-12s: " RGB_COLOR_FORMAT " -> " HSV_COLOR_FORMAT " -> " RGB_COLOR_FORMAT " [%s]\n",
           test->name, test->r, test->g, test->b,
           hsv.h, hsv.s*100.0, hsv.v*100.0,
           rgb.r, rgb.g, rgb.b,
           (passed? "OK": "FAIL"));
    }
  }

  return putTestResult(testName, testCount, passCount);
}

/* Test color recognition
 *
 * Color Reference: Most test colors are taken from the CSS/HTML Named Colors
 * specification (W3C CSS Color Module Level 3):
 * https://www.w3.org/TR/css-color-3/#svg-color
 *
 * These are standard web colors used across browsers, design tools, and the X11
 * color system. Using standard colors ensures our detection algorithms work with
 * commonly recognized color names.
 */
static int
testColorRecognition (const char *testName) {
  static const ColorTest tests[] = {
    /* Basic colors - Note: NULL in last two fields means no alternate */
    {"Pure Red",         255, 0,   0,   "vivid red", NULL, NULL},

    /* Pure green (0,255,0) is at H=120° S=1.0 V=1.0, which matches our lime detection
     * criteria (H 90-135°, V>0.75, S>0.75). Both names are technically correct.
     * Note: CSS "Lime" is rgb(0,255,0), this is also tested here. */
    {"Pure Green/Lime",  0,   255, 0,   "vivid green", "lime",
     "H=120° matches lime criteria (bright saturated yellow-green)"},

    {"Pure Blue",        0,   0,   255, "vivid blue", NULL, NULL},
    {"White",            255, 255, 255, "white", NULL, NULL},
    {"Black",            0,   0,   0,   "black", NULL, NULL},

    /* Greys */
    {"Light Grey",       200, 200, 200, "light grey", NULL, NULL},
    {"Medium Grey",      128, 128, 128, "grey", NULL, NULL},
    {"Dark Grey",        64,  64,  64,  "dark grey", NULL, NULL},

    /* Named colors - CSS/HTML color standard */
    {"Brown",            170, 85,  0,   "brown", NULL, NULL},  /* VGA Brown RGB values */
    {"Dark Brown",       101, 67,  33,  "dark brown", NULL, NULL},
    {"Pink",             255, 192, 203, "pink", NULL, NULL},     /* CSS Pink */
    {"Light Pink",       255, 182, 193, "light pink", NULL, NULL}, /* CSS LightPink */
    {"Coral",            255, 127, 80,  "coral", NULL, NULL},    /* CSS Coral */
    {"Olive",            128, 128, 0,   "olive", NULL, NULL},    /* CSS Olive */
    {"LimeGreen",        50,  205, 50,  "lime", NULL, NULL},     /* CSS LimeGreen */
    {"Teal",             0,   128, 128, "teal", NULL, NULL},     /* CSS Teal */
    {"Turquoise",        64,  224, 208, "turquoise", NULL, NULL}, /* CSS Turquoise */
    {"Maroon",           128, 0,   0,   "maroon", NULL, NULL},   /* CSS Maroon */
    {"Navy",             0,   0,   128, "navy", NULL, NULL},     /* CSS Navy */
    {"Indigo",           75,  0,   130, "indigo", NULL, NULL},   /* CSS Indigo */
    {"Lavender",         230, 230, 250, "lavender", NULL, NULL}, /* CSS Lavender */
    {"Gold",             255, 215, 0,   "gold", NULL, NULL},     /* CSS Gold */
    {"Tan",              210, 180, 140, "tan", NULL, NULL},      /* CSS Tan */
    {"Beige",            245, 245, 220, "beige", NULL, NULL},    /* CSS Beige */

    /* Compound descriptions */
    {"Dark Blue",        0,   0,   139, "navy", NULL, NULL},

    /* Light blue (173,216,230) has H=197° which is in the cyan range (180-210°).
     * "light cyan" is actually more accurate than "light blue" based on HSV analysis. */
    {"Light Blue",       173, 216, 230, "light blue", "light cyan",
     "H=197° is in cyan range; HSV analysis gives more accurate result"},

    {"Dark Green",       0,   100, 0,   "dark green", NULL, NULL},
    {"Light Green",      144, 238, 144, "light green", NULL, NULL},
  };

  const int testCount = ARRAY_COUNT(tests);
  int passCount = 0;

  for (int i=0; i<testCount; i+=1) {
    const ColorTest *test = &tests[i];

    char description[64];
    rgbToDescription(description, sizeof(description), test->r, test->g, test->b);

    /* Check if result matches expected or alternate description */
    int matchExpected = strcmp(description, test->expectedDescription) == 0;
    int matchAlternate = test->alternateDescription && (strcmp(description, test->alternateDescription) == 0);

    int passed = matchExpected || matchAlternate;
    if (passed) passCount += 1;

    /* Determine status display */
    int quietnessLevel;
    const char *status;

    if (matchExpected) {
      quietnessLevel = OPTQ_PASS;
      status = "OK";
    } else if (matchAlternate) {
      quietnessLevel = OPTQ_WARN;
      status = "OK (alternate)";
    } else {
      quietnessLevel = OPTQ_FAIL;
      status = "FAIL";
    }

    if (opt_quietness <= quietnessLevel) {
      putf("%-20s " RGB_COLOR_FORMAT " -> %-20s [%s]\n",
           test->name, test->r, test->g, test->b, description, status);

      if (!passed) {
        putf("  Expected: \"%s\"\n", test->expectedDescription);

        if (test->alternateDescription) {
          putf("  Alternate: \"%s\"\n", test->alternateDescription);
        }
      } else if (matchAlternate) {
        /* Show why alternate was accepted */
        putf("  Note: %s\n", test->alternateReason);
      }
    }
  }

  return putTestResult(testName, testCount, passCount);
}

/* Test RGB to VGA mapping with various colors */
static int
testRGBtoVGA (const char *testName) {
  static const ColorTest tests[] = {
    {"Bright Red",       255, 0,   0,   NULL, NULL, NULL},  /* Should be 12 (Light Red) */
    {"Dark Red",         128, 0,   0,   NULL, NULL, NULL},  /* Should be 4 (Red) */
    {"Bright Green",     0,   255, 0,   NULL, NULL, NULL},  /* Should be 10 (Light Green) */
    {"Dark Green",       0,   128, 0,   NULL, NULL, NULL},  /* Should be 2 (Green) */
    {"Bright Blue",      0,   0,   255, NULL, NULL, NULL},  /* Should be 9 (Light Blue) */
    {"Dark Blue",        0,   0,   128, NULL, NULL, NULL},  /* Should be 1 (Blue) */
    {"Orange",           255, 165, 0,   NULL, NULL, NULL},  /* Should be 6 (Brown) or 11 (Yellow) */
    {"Purple",           128, 0,   128, NULL, NULL, NULL},  /* Should be 5 (Magenta) */
  };

  if (opt_quietness <= OPTQ_PASS) {
    for (int i=0; i<ARRAY_COUNT(tests); i+=1) {
      const ColorTest *test = &tests[i];
      int vga = rgbToVga(test->r, test->g, test->b);
      RGBColor rgb = vgaToRgb(vga);
      const char *color = vgaColorName(vga);

      putf("%-15s " RGB_COLOR_FORMAT " -> " VGA_COLOR_FORMAT " (%s) " RGB_COLOR_FORMAT "\n",
           test->name, test->r, test->g, test->b,
           vga, color, rgb.r, rgb.g, rgb.b);
    }
  }

  return 1;
}

static void
showColor (RGBColor rgb, HSVColor hsv) {
  const char *indent = "  ";

  putf("%sRGB: (%d, %d, %d)\n", indent, rgb.r, rgb.g, rgb.b);
  putf("%sHSV: (%.1f°, %.0f%%, %.0f%%)\n", indent, hsv.h, hsv.s*100.0, hsv.v*100.0);

  char description[64];
  rgbColorToDescription(description, sizeof(description), rgb);
  putf("%sDescription: %s\n", indent, description);

  int vga = rgbToVga(rgb.r, rgb.g, rgb.b);
  const char *name = vgaColorName(vga);
  putf("%sNearest VGA: %d \"%s\"\n", indent, vga, name);
}

static void
showRGB (unsigned char r, unsigned char g, unsigned char b) {
  RGBColor rgb = {.r=r, .g=g, .b=b};
  showColor(rgb, rgbColorToHsv(rgb));
}

static void
showHSV (float h, float s, float v) {
  HSVColor hsv = {.h=h, .s=s, .v=v};
  showColor(hsvColorToRgb(hsv), hsv);
}

static const char *
getArgument (Queue *arguments, const char *name) {
  const char *argument = dequeueItem(arguments);

  if (!argument) {
    putf("missing %s\n", name);
  }

  return argument;
}

static int
noMoreArguments (Queue *arguments) {
  const char *argument = dequeueItem(arguments);
  if (!argument) return 1;

  putf("too many arguments: %s\n", argument);
  return 0;
}

static int
validateColorComponent (int *component, const char *argument, const char *name) {
  static const int minimum = 0;
  static const int maximum = UINT8_MAX;
  if (validateInteger(component, argument, &minimum, &maximum)) return 1;

  putf(
    "invalid %s component: %s (must be an integer >= %d and <= %d)\n",
    name, argument, minimum, maximum
  );

  return 0;
}

static int
validateAngle (float *angle, const char *argument, const char *name) {
  static const float minimum = 0.0;
  static const float maximum = 360.0;

  if (validateFloat(angle, argument, &minimum, &maximum)) {
    if (*angle != maximum) {
      return 1;
    }
  }

  putf(
    "invalid %s angle: %s (must be a floating-point number >= %g and < %g)\n",
    name, argument, minimum, maximum
  );

  return 0;
}

static int
validatePercentage (float *value, const char *argument, const char *name) {
  static const float minimum = 0.0;
  static const float maximum = 100.0;

  if (validateFloat(value, argument, &minimum, &maximum)) {
    *value /= 100.0;
    return 1;
  }

  putf(
    "invalid %s percentage: %s (must be a floating-point number >= %g and <= %g)\n",
    name, argument, minimum, maximum
  );

  return 0;
}

static void
rgbHandler (Queue *arguments) {
  const char *red, *green, *blue;
  int r, g, b;

  if ((red = getArgument(arguments, "red component"))) {
    if ((green = getArgument(arguments, "green component"))) {
      if ((blue = getArgument(arguments, "blue component"))) {
        if (noMoreArguments(arguments)) {
          if (validateColorComponent(&r, red, "red")) {
            if (validateColorComponent(&g, green, "green")) {
              if (validateColorComponent(&b, blue, "blue")) {
                showRGB(r, g, b);
              }
            }
          }
        }
      }
    }
  }
}

static void
hsvHandler (Queue *arguments) {
  const char *hue, *saturation, *value;
  float h, s, v;

  if ((hue = getArgument(arguments, "hue angle"))) {
    if ((saturation = getArgument(arguments, "saturation percentage"))) {
      if ((value = getArgument(arguments, "value percentage"))) {
        if (noMoreArguments(arguments)) {
          if (validateAngle(&h, hue, "hue")) {
            if (validatePercentage(&s, saturation, "saturation")) {
              if (validatePercentage(&v, value, "value")) {
                showHSV(h, s, v);
              }
            }
          }
        }
      }
    }
  }
}

static void
vgaHandler (Queue *arguments) {
  const char *argument = getArgument(arguments, "VGA color number");

  if (argument) {
    if (noMoreArguments(arguments)) {
      static const int minimum = 0;
      static const int maximum = VGA_COLOR_COUNT - 1;
      int vga;

      if (validateInteger(&vga, argument, &minimum, &maximum)) {
        RGBColor rgb = vgaToRgb(vga);
        showRGB(rgb.r, rgb.g, rgb.b);
      } else {
        putf(
          "invalid VGA color number: %s (must be an integer >= %d and <= %d)\n",
          argument, minimum, maximum
        );
      }
    }
  }
}

static void
ansiHandler (Queue *arguments) {
  const char *argument = getArgument(arguments, "ANSI color number");

  if (argument) {
    if (noMoreArguments(arguments)) {
      static const int minimum = 0;
      static const int maximum = UINT8_MAX;
      int ansi;

      if (validateInteger(&ansi, argument, &minimum, &maximum)) {
        RGBColor rgb = ansiToRgb(ansi);
        showRGB(rgb.r, rgb.g, rgb.b);
      } else {
        putf(
          "invalid ANSI color number: %s (must be an integer >= %d and <= %d)\n",
          argument, minimum, maximum
        );
      }
    }
  }
}

typedef struct {
  const char *name;
  void (*handler) (Queue *arguments);
} ColorSpace;

static const ColorSpace colorSpaces[] = {
  { .name = "rgb",
    .handler = rgbHandler,
  },

  { .name = "hsv",
    .handler = hsvHandler,
  },

  { .name = "vga",
    .handler = vgaHandler,
  },

  { .name = "ansi",
    .handler = ansiHandler,
  },
};

/* Interactive color description test */
static void
enterInteractiveMode (void) {
  const ColorSpace *currentColorSpace = colorSpaces;

  putTestHeader("Interactive Color Test");
  putf("Enter RGB values to test color descriptions.\n");
  putf("Format: R G B (0-255 for each)\n");
  putf("Enter 'q' to quit.\n\n");

  Queue *arguments = newQueue(NULL, NULL);
  char *line = NULL;
  size_t lineSize = 0;

  while (1) {
    putf("%s> ", currentColorSpace->name);
    flushOutput();

    if (!readLine(stdin, &line, &lineSize, NULL)) {
      putf("\n");
      break;
    }

    {
      deleteElements(arguments);

      const char *delimiters = " ";
      char *string = line;
      char *argument;

      while ((argument = strtok(string, delimiters))) {
        enqueueItem(arguments, argument);
        string = NULL;
      }
    }

    if (getQueueSize(arguments) > 0) {
      char *command = dequeueItem(arguments);
      if (isAbbreviation("quit", command)) break;

      for (int i=0; i<ARRAY_COUNT(colorSpaces); i+=1) {
        const ColorSpace *cs = &colorSpaces[i];

        if (isAbbreviation(cs->name, command)) {
          if (getQueueSize(arguments) == 0) {
            currentColorSpace = cs;
          } else {
            cs->handler(arguments);
          }

          goto COMMAND_DONE;
        }
      }

      if (isdigit(command[0])) {
        prequeueItem(arguments, command);
        currentColorSpace->handler(arguments);
      } else {
        putf("unrecognized command\n");
      }
    }
  COMMAND_DONE:
  }

  if (line) free(line);
  deallocateQueue(arguments);
}

typedef struct {
  const char *name;
  const char *summary;
  int *requested;
  int (*perform) (const char *testName);
} RequestableTest;

static const RequestableTest requetableTestTable[] = {
  { .name = "VGA to RGB to VGA Round-Trip Test",
    .summary = "Verify the successful conversion of each VGA color to RGB and then back to VGA",
    .requested = &opt_testVGAtoRGBtoVGA,
    .perform = testVGAtoRGBtoVGA,
  },

  { .name = "VGA Color Descriptions",
    .summary = "List the name and HSV description of each VGA color",
    .requested = &opt_testVGADescriptions,
    .perform = testVGADescriptions,
  },

  { .name = "RGB to HSV to RGB Round-Trip Test",
    .summary = "Verify the successful conversion of some RGB colors to HSV and then back to RGB",
    .requested = &opt_testRGBtoHSVtoRGB,
    .perform = testRGBtoHSVtoRGB,
  },

  { .name = "Color Recognition Test",
    .summary = "Verify the recognition of some common colors",
    .requested = &opt_testColorRecognition,
    .perform = testColorRecognition,
  },

  { .name = "RGB to Nearest VGA Mappings",
    .summary = "Show how some common colors are mapped to their nearest VGA colors",
    .requested = &opt_testRGBtoVGA,
    .perform = testRGBtoVGA,
  },
};

static const size_t requetableTestCount = ARRAY_COUNT(requetableTestTable);

/* Audit the test options */
static int
auditTestOptions (void) {
  int testRequested = 0;

  for (int i=0; i<requetableTestCount; i+=1) {
    const RequestableTest *test = &requetableTestTable[i];

    if (*test->requested) {
      if (opt_performAllTests) {
        logMessage(LOG_ERR, "conflicting test options");
        return 0;
      }

      testRequested = 1;
      break;
    }
  }

  if (!testRequested) {
    if (opt_performAllTests || !opt_enterInteractiveMode) {
      for (int i=0; i<requetableTestCount; i+=1) {
        const RequestableTest *test = &requetableTestTable[i];
        *test->requested = 1;
      }
    }
  }

  return 1;
}

/* perform the requested tests */
static int
performRequestedTests (void) {
  int allPassed = 1;

  for (int i=0; i<requetableTestCount; i+=1) {
    const RequestableTest *test = &requetableTestTable[i];

    if (*test->requested) {
      if (opt_quietness <= OPTQ_TEST) {
        putTestHeader(test->name);
      }

      if (test->summary) {
        if (opt_quietness <= OPTQ_INFO) {
          putf("%s...\n\n", test->summary);
        }
      }

      if (!test->perform(test->name)) {
        allPassed = 0;
      }

      if (opt_quietness <= OPTQ_TEST) {
        putf("\n");
      }
    }
  }

  return allPassed;
}

/* Main test program */
int
main (int argc, char *argv[]) {
  {
    const CommandLineDescriptor descriptor = {
      .options = &programOptions,
      .parameters = &programParameters,
      .applicationName = "colortest",

      .usage = {
        .purpose = "Test the color conversion and description functions.",
      }
    };

    PROCESS_COMMAND_LINE(descriptor, argc, argv);
  }

  if (!auditTestOptions()) {
    return PROG_EXIT_SYNTAX;
  }

  if (opt_quietness <= OPTQ_INFO) {
    putNotice("BRLTTY Color Conversion Test Suite");
    putf("\n");
  }

  int allTestsPassed = performRequestedTests();

  /* Interactive mode if requested */
  if (opt_enterInteractiveMode) {
    enterInteractiveMode();
  } else if (opt_quietness <= OPTQ_INFO) {
    putf("Run with the -i flag for interactive mode.\n\n");
  }

  if (opt_quietness <= OPTQ_INFO) {
    putNotice("Tests Complete");
  }

  return allTestsPassed? PROG_EXIT_SUCCESS: PROG_EXIT_SEMANTIC;
}
