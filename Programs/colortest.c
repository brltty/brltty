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
#include "color_internal.h"
#include "parse.h"
#include "file.h"
#include "queue.h"

typedef enum {
   OPTQ_INFO,
   OPTQ_PASS,
   OPTQ_WARN,
   OPTQ_FAIL,
   OPTQ_SUMMARY,
   OPTQ_TEST,
} OptQuietness;

static int opt_quietness;
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

BEGIN_COMMAND_LINE_NOTES(programNotes)
  "The -a option may not be combined with any option that requests a specific test.",
  "If none of the tests are requested then interactive mode is entered.",
  "",
  "The -q option is cumulative.",
  "Output verbosity is increasingly reduced, each time it's specified, as follows:",
  "  1: Informational messages and initial interactive mode help.",
  "  2: Test objectives and results that pass.",
  "  3: Test results that pass but with a qualification.",
  "  4: Test results that fail.",
  "  5: Test summaries.",
  "  6: Test headers, test status, and color model syntax (interactive mode).",
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "colortest",
  .purpose = "Test the color conversion and description functions.",

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),
END_COMMAND_LINE_DESCRIPTOR

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

static void putf (const char *format, ...) PRINTF(1, 2);
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
  if (opt_quietness <= OPTQ_TEST) {
    putf("=== %s ===\n", name);
  }
}

static int
putTestResult (const char *name, int count, int passes) {
  int hasPassed = passes == count;

  if (!hasPassed) {
    if (opt_quietness <= OPTQ_SUMMARY) {
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
    int vgaBack = rgbColorToVga(rgb, 0);

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
      ColorDescriptionBuffer description;
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
    {"Gray",         128, 128, 128, NULL, NULL, NULL},
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
           hsv.h, hsv.s*100.0f, hsv.v*100.0f,
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
    {"Pure Red",         255, 0,   0,   "bright pure red", NULL, NULL},

    /* Pure green (0,255,0) is at H=120° S=1.0 V=1.0, which matches our lime detection
     * criteria (H 90-135°, V>0.75, S>0.75). Both names are technically correct.
     * Note: CSS "Lime" is rgb(0,255,0), this is also tested here. */
    {"Pure Green/Lime",  0,   255, 0,   "vivid green", "lime",
     "H=120° matches lime criteria (bright saturated yellow-green)"},

    {"Pure Blue",        0,   0,   255, "bright pure blue", NULL, NULL},
    {"White",            255, 255, 255, "white", NULL, NULL},
    {"Black",            0,   0,   0,   "black", NULL, NULL},

    /* Grays */
    {"Light Gray",       200, 200, 200, "light gray", NULL, NULL},
    {"Medium Gray",      128, 128, 128, "natural gray", NULL, NULL},
    {"Dark Gray",        64,  64,  64,  "dark gray", NULL, NULL},

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
    {"Light Blue",       173, 216, 230, "light blue", "bright pale cyan",
     "H=197° is in cyan range; HSV analysis gives more accurate result"},

    {"Dark Green",       0,   100, 0,   "dark pure green", NULL, NULL},
    {"Light Green",      144, 238, 144, "bright weak green", NULL, NULL},
  };

  const int testCount = ARRAY_COUNT(tests);
  int passCount = 0;

  for (int i=0; i<testCount; i+=1) {
    const ColorTest *test = &tests[i];

    ColorDescriptionBuffer description;
    rgbToDescription(description, sizeof(description), test->r, test->g, test->b);

    /* Check if result matches expected or alternate description */
    int matchExpected = strcasecmp(description, test->expectedDescription) == 0;
    int matchAlternate = test->alternateDescription &&
                         (strcasecmp(description, test->alternateDescription) == 0);

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
      int vga = rgbToVga(test->r, test->g, test->b, 0);
      RGBColor rgb = vgaToRgb(vga);
      const char *color = vgaColorName(vga);

      putf("%-15s " RGB_COLOR_FORMAT " -> " VGA_COLOR_FORMAT " (%s) " RGB_COLOR_FORMAT "\n",
           test->name, test->r, test->g, test->b,
           vga, color, rgb.r, rgb.g, rgb.b);
    }
  }

  return 1;
}

static const char blockIndent[] = "  ";

static void
showColor (RGBColor rgb, HSVColor hsv) {
  putf("%sRGB: (%d, %d, %d)\n", blockIndent, rgb.r, rgb.g, rgb.b);
  putf("%sHSV: (%.1f°, %.0f%%, %.0f%%)\n", blockIndent, hsv.h, hsv.s*100.0f, hsv.v*100.0f);

  {
    ColorDescriptionBuffer description;
    hsvColorToDescription(description, sizeof(description), hsv);
    putf("%sColor: %s\n", blockIndent, description);
  }

  {
    HLSColor hls = rgbColorToHls(rgb);
    putf("%sHLS: (%.1f°, %.0f%%, %.0f%%)\n", blockIndent, hls.h, hls.l*100.0f, hls.s*100.0f);
  }

  {
    int vga = rgbToVga(rgb.r, rgb.g, rgb.b, 0);
    int fastVga = rgbToVgaFast(rgb.r, rgb.g, rgb.b, 0);

    {
      const char *name = vgaColorName(vga);
      putf("%sNearest VGA: %d \"%s\"\n", blockIndent, vga, name);
    }

    if (vga != fastVga) {
      const char *name = vgaColorName(fastVga);
      putf("%sFast VGA: %d \"%s\"\n", blockIndent, fastVga, name);
    }
  }
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

static void
showHLS (float h, float l, float s) {
  RGBColor rgb = hlsToRgb(h, l, s);
  showRGB(rgb.r, rgb.g, rgb.b);
}

static void
showVGA (int vga) {
  putf("%sVGA: %d\n", blockIndent, vga);
  RGBColor rgb = vgaToRgb(vga);
  showRGB(rgb.r, rgb.g, rgb.b);
}

static void
showANSI (int ansi) {
  putf("%sANSI: %d\n", blockIndent, ansi);
  RGBColor rgb = ansiToRgb(ansi);
  showRGB(rgb.r, rgb.g, rgb.b);
}

static const char *
getNextArgument (Queue *arguments, const char *name) {
  const char *argument = dequeueItem(arguments);

  if (!argument) {
    logMessage(LOG_WARNING, "missing %s", name);
  }

  return argument;
}

static int
noMoreArguments (Queue *arguments) {
  const char *argument = dequeueItem(arguments);
  if (!argument) return 1;

  logMessage(LOG_WARNING, "too many arguments: %s", argument);
  return 0;
}

static int
parseInteger (int *value, const char *argument, int minimum, int maximum, const char *name) {
  if (validateInteger(value, argument, &minimum, &maximum)) return 1;

  logMessage(LOG_WARNING,
    "invalid %s: %s (must be an integer >= %d and <= %d)",
    name, argument, minimum, maximum
  );

  return 0;
}

static int
parseFloat (float *value, const char *argument, float minimum, float maximum, int inclusive, const char *name) {
  if (validateFloat(value, argument, &minimum, &maximum)) {
    if (inclusive || (*value < maximum)) {
      return 1;
    }
  }

  logMessage(LOG_WARNING,
    "invalid %s: %s (must be a real number >= %g and %s %g)",
    name, argument, minimum, (inclusive? "<=": "<"), maximum
  );

  return 0;
}

static int
parseIntensity (int *intensity, const char *argument, const char *name) {
  static const int minimum = 0;
  static const int maximum = UINT8_MAX;
  if (validateInteger(intensity, argument, &minimum, &maximum)) return 1;

  logMessage(LOG_WARNING,
    "invalid %s: %s (must be an integer >= %d and <= %d)",
    name, argument, minimum, maximum
  );

  return 0;
}

static int
parseDegrees (float *degrees, const char *argument, const char *name) {
  return parseFloat(degrees, argument, 0.0f, 360.0f, 0, name);
}

static int
parsePercent (float *value, const char *argument, const char *name) {
  const float maximum = 100.0f;
  if (!parseFloat(value, argument, 0.0f, maximum, 1, name)) return 0;

  *value /= maximum;
  return 1;
}

static int
rgbHandler (Queue *arguments) {
  const char *redName = "red intensity";
  const char *redArgument;
  int redIntensity;

  const char *greenName = "green intensity";
  const char *greenArgument;
  int greenIntensity;

  const char *blueName = "blue intensity";
  const char *blueArgument;
  int blueIntensity;

  if ((redArgument = getNextArgument(arguments, redName))) {
    if ((greenArgument = getNextArgument(arguments, greenName))) {
      if ((blueArgument = getNextArgument(arguments, blueName))) {
        if (noMoreArguments(arguments)) {
          if (parseIntensity(&redIntensity, redArgument, redName)) {
            if (parseIntensity(&greenIntensity, greenArgument, greenName)) {
              if (parseIntensity(&blueIntensity, blueArgument, blueName)) {
                showRGB(redIntensity, greenIntensity, blueIntensity);
                return 1;
              }
            }
          }
        }
      }
    }
  }

  return 0;
}

static int
hsvHandler (Queue *arguments) {
  const char *hueName = "hue angle";
  const char *hueArgument;
  float hueAngle;

  const char *saturationName = "saturation percent";
  const char *saturationArgument;
  float saturationLevel;

  const char *brightnessName = "brightness percent";
  const char *brightnessArgument;
  float brightnessLevel;

  if ((hueArgument = getNextArgument(arguments, hueName))) {
    if ((saturationArgument = getNextArgument(arguments, saturationName))) {
      if ((brightnessArgument = getNextArgument(arguments, brightnessName))) {
        if (noMoreArguments(arguments)) {
          if (parseDegrees(&hueAngle, hueArgument, hueName)) {
            if (parsePercent(&saturationLevel, saturationArgument, saturationName)) {
              if (parsePercent(&brightnessLevel, brightnessArgument, brightnessName)) {
                showHSV(hueAngle, saturationLevel, brightnessLevel);
                return 1;
              }
            }
          }
        }
      }
    }
  }

  return 0;
}

static int
hlsHandler (Queue *arguments) {
  const char *hueName = "hue angle";
  const char *hueArgument;
  float hueAngle;

  const char *lightnessName = "lightness percent";
  const char *lightnessArgument;
  float lightnessLevel;

  const char *saturationName = "saturation percent";
  const char *saturationArgument;
  float saturationLevel;

  if ((hueArgument = getNextArgument(arguments, hueName))) {
    if ((lightnessArgument = getNextArgument(arguments, lightnessName))) {
      if ((saturationArgument = getNextArgument(arguments, saturationName))) {
        if (noMoreArguments(arguments)) {
          if (parseDegrees(&hueAngle, hueArgument, hueName)) {
            if (parsePercent(&lightnessLevel, lightnessArgument, lightnessName)) {
              if (parsePercent(&saturationLevel, saturationArgument, saturationName)) {
                showHLS(hueAngle, lightnessLevel, saturationLevel);
                return 1;
              }
            }
          }
        }
      }
    }
  }

  return 0;
}

static int
vgaHandler (Queue *arguments) {
  const char *vgaName = "VGA color number";
  const char *vgaArgument = getNextArgument(arguments, vgaName);

  if (vgaArgument) {
    if (noMoreArguments(arguments)) {
      int vgaColor;

      if (parseInteger(&vgaColor, vgaArgument, 0, (VGA_COLOR_COUNT - 1), vgaName)) {
        showVGA(vgaColor);
        return 1;
      }
    }
  }

  return 0;
}

static int
ansiHandler (Queue *arguments) {
  const char *ansiName = "ANSI color number";
  const char *ansiArgument = getNextArgument(arguments, ansiName);

  if (ansiArgument) {
    if (noMoreArguments(arguments)) {
      int ansiColor;

      if (parseInteger(&ansiColor, ansiArgument, 0, UINT8_MAX, ansiName)) {
        showANSI(ansiColor);
        return 1;
      }
    }
  }

  return 0;
}

typedef struct {
  const char *name;
  const char *syntax;
  int (*handler) (Queue *arguments);
} ColorModel;

static void
putColorModelSyntax (const ColorModel *model) {
  if (opt_quietness <= OPTQ_TEST) {
    putf("\n%s color model syntax: %s\n", model->name, model->syntax);
  }
}

static const ColorModel colorModels[] = {
  { .name = "RGB",
    .syntax = "red green blue (all integers within the range 0-255)",
    .handler = rgbHandler,
  },

  { .name = "HSV",
    .syntax = "hue (degrees) saturation (percent) brightness (percent)",
    .handler = hsvHandler,
  },

  { .name = "HLS",
    .syntax = "hue (degrees) lightness (percent) saturation (percent)",
    .handler = hlsHandler,
  },

  { .name = "VGA",
    .syntax = "an integer within the range 0-15",
    .handler = vgaHandler,
  },

  { .name = "ANSI",
    .syntax = "an integer within the range 0-255",
    .handler = ansiHandler,
  },
};

static const ColorModel *
getColorModel (const char *name) {
  for (int i=0; i<ARRAY_COUNT(colorModels); i+=1) {
    const ColorModel *model = &colorModels[i];
    if (isAbbreviation(model->name, name)) return model;
  }

  return NULL;
}

static int
cmdGrayscale (Queue *arguments) {
  if (noMoreArguments(arguments)) {
    typedef struct {
      const char *color;
      unsigned char from;
      unsigned char to;
    } Range;

    Range ranges[20];
    unsigned int count = 0;

    for (unsigned int percent=0; percent<=100; percent+=1) {
      const char *color = gsColorName((float)percent / 100.0f);

      if (count > 0) {
        if (strcmp(color, ranges[count-1].color) == 0) {
          goto next;
        }
      }

      if (count == ARRAY_COUNT(ranges)) break;
      ranges[count].color = color;
      ranges[count].from = percent;
      count += 1;

    next:
      ranges[count-1].to = percent;
    }

    for (unsigned int i=0; i<count; i+=1) {
      const Range *range = &ranges[i];

      putf(
        "%s%s: %u%%-%u%%\n",
        blockIndent, range->color,
        range->from, range->to
      );
    }

    return 1;
  }

  return 0;
}

static int
cmdHues (Queue *arguments) {
  if (noMoreArguments(arguments)) {
    typedef struct {
      const char *color;
      unsigned int from;
      unsigned int to;
    } Range;

    Range ranges[20];
    unsigned int count = 0;

    for (unsigned int angle=0; angle<=360; angle+=1) {
      const char *color = hueColorName((float)angle);

      if (count > 0) {
        if (strcmp(color, ranges[count-1].color) == 0) {
          goto next;
        }
      }

      if (count == ARRAY_COUNT(ranges)) break;
      ranges[count].color = color;
      ranges[count].from = angle;
      count += 1;

    next:
      ranges[count-1].to = angle;
    }

    for (unsigned int i=0; i<count; i+=1) {
      const Range *range = &ranges[i];

      putf(
        "%s%s: %u°-%u°\n",
        blockIndent, range->color,
        range->from, range->to
      );
    }

    return 1;
  }

  return 0;
}

static int
sortHSVColorEntries (const void *item1, const void *item2) {
  const HSVColorEntry *const *color1 = item1;
  const HSVColorEntry *const *color2 = item2;
  return strcasecmp((*color1)->name, (*color2)->name);
}

static void
showHSVColorEntry (const HSVColorEntry *color) {
  putf(
    "%s%s: Hue:%.0f°-%.0f° Saturatinn:%.0f%%-%.0f%% Value:%.0f%%-%.0f%%\n",
    blockIndent, color->name,
    color->hue.minimum, color->hue.maximum,
    color->saturation.minimum*100.0f, color->saturation.maximum*100.0f,
    color->value.minimum*100.0f, color->value.maximum*100.0f
  );
}

static int
cmdColors (Queue *arguments) {
  const HSVColorEntry *sorted[hsvColorCount];

  {
    for (int i=0; i<hsvColorCount; i+=1) {
      sorted[i] = &hsvColorTable[i];
    }

    qsort(sorted, hsvColorCount, sizeof(sorted[0]), sortHSVColorEntries);
  }

  if (isEmptyQueue(arguments)) {

    for (int i=0; i<hsvColorCount; i+=1) {
      showHSVColorEntry(sorted[i]);
    }

    return 1;
  }

  {
    const char *name = getNextArgument(arguments, "color name");

    if (name) {
      if (noMoreArguments(arguments)) {
        int found = 0;

        for (int i=0; i<hsvColorCount; i+=1) {
          const HSVColorEntry *color = sorted[i];

          if (isAbbreviation(color->name, name)) {
            showHSVColorEntry(color);
            found = 1;
          }
        }

        if (found) return 1;
        logMessage(LOG_WARNING, "color not found");
      }
    }
  }

  return 0;
}

static inline int
hsvRangesOverlap (const HSVComponentRange *range1, const HSVComponentRange *range2) {
  return (range2->minimum <= range1->maximum) &&
         (range2->maximum >= range1->minimum);
}

static int
cmdOverlaps (Queue *arguments) {
  if (noMoreArguments(arguments)) {
    for (int i=0; i<hsvColorCount; i+=1) {
      const HSVColorEntry *color1 = &hsvColorTable[i];

      for (int j=i+1; j<hsvColorCount; j+=1) {
        const HSVColorEntry *color2 = &hsvColorTable[j];
        int huesOverlap = hsvRangesOverlap(&color1->hue, &color2->hue);
        int saturationsOverlap = hsvRangesOverlap(&color1->saturation, &color2->saturation);
        int valuesOverlap = hsvRangesOverlap(&color1->value, &color2->value);

        if (huesOverlap && saturationsOverlap && valuesOverlap) {
          putf(
            "%s%s & %s\n",
            blockIndent, color1->name, color2->name
          );
        }
      }
    }

    return 1;
  }

  return 0;
}

static void
hsvListModifiers (const HSVModifier *(*getModifier) (float level)) {
  typedef struct {
    const HSVModifier *modifier;
    unsigned char from;
    unsigned char to;
  } Range;

  Range ranges[20];
  unsigned int count = 0;

  for (unsigned int percent=0; percent<=100; percent+=1) {
    const HSVModifier *modifier = getModifier((float)percent / 100.0f);

    if (count > 0) {
      if (strcmp(modifier->name, ranges[count-1].modifier->name) == 0) {
        goto next;
      }
    }

    if (count == ARRAY_COUNT(ranges)) break;
    ranges[count].modifier = modifier;
    ranges[count].from = percent;
    count += 1;

  next:
    ranges[count-1].to = percent;
  }

  for (unsigned int i=0; i<count; i+=1) {
    const Range *range = &ranges[i];
    const HSVModifier *modifier = range->modifier;

    putf(
      "%s%s: %u%%-%u%%: %s\n",
      blockIndent, modifier->name,
      range->from, range->to,
      modifier->comment
    );
  }
}

static int
cmdSaturation (Queue *arguments) {
  if (noMoreArguments(arguments)) {
    hsvListModifiers(hsvSaturationModifier);
    return 1;
  }

  return 0;
}

static int
cmdBrightness (Queue *arguments) {
  if (noMoreArguments(arguments)) {
    hsvListModifiers(hsvBrightnessModifier);
    return 1;
  }

  return 0;
}

typedef struct {
  const char *name;
  const char *help;
  int (*handler) (Queue *arguments);
} CommandEntry;

static const CommandEntry commandTable[] = {
  { .name = "brightness",
    .help = "List the HSV brightness (value) modifiers.",
    .handler = cmdBrightness,
  },

  { .name = "colors",
    .help = "List the color table.",
    .handler = cmdColors,
  },

  { .name = "grayscale",
    .help = "List the grayscale colors.",
    .handler = cmdGrayscale,
  },

  { .name = "hues",
    .help = "List the hue colors.",
    .handler = cmdHues,
  },

  { .name = "overlaps",
    .help = "List the HSV color definition overlaps.",
    .handler = cmdOverlaps,
  },

  { .name = "saturation",
    .help = "List the HSV saturation modifiers.",
    .handler = cmdSaturation,
  },
};

static const CommandEntry *
getCommand (const char *name) {
  for (int i=0; i<ARRAY_COUNT(commandTable); i+=1) {
    const CommandEntry *cmd = &commandTable[i];
    if (isAbbreviation(cmd->name, name)) return cmd;
  }

  return NULL;
}

#define QUIT_COMMAND "quit"
#define HELP_COMMAND "help"
static const ColorModel *const defaultColorModel = colorModels;

static void
showInteractiveHelp (int includeCommands) {
  putf("Commands are not case-sensitive and may be abbreviated.\n");
  putf("Use the \"" QUIT_COMMAND "\" command to exit this mode.\n");
  putf("Use the \"" HELP_COMMAND "\" command to discover the rest of them.\n");
  putf("\n");

  if (includeCommands) {
    putf("The rest of the commands are:\n");

    for (int i=0; i<ARRAY_COUNT(commandTable); i+=1) {
      const CommandEntry *cmd = &commandTable[i];

      putf(
        "%s%-10s  %s\n",
        blockIndent, cmd->name, cmd->help
      );
    }

    putf("\n");
  }

  {
    putf("The supported color models are:");
    int last = ARRAY_COUNT(colorModels) - 1;

    for (int i=0; i<=last; i+=1) {
      const ColorModel *model = &colorModels[i];

      if (i > 0) putf(",");
      if (i == last) putf(" and");
      putf(" %s", model->name);
      if (model == defaultColorModel) putf(" (the default)");
    }

    putf("\n");
  }

  putf("To switch to another color model, enter its name with no additional arguments.\n");
  putf("If additional arguments follow the name then that color model is used.\n");
  putf("If only numeric arguments are specified then the current color model is used.\n");
}

/* Interactive color description test */
static void
doInteractiveMode (void) {
  pushLogPrefix("ERROR");
  const ColorModel *currentColorModel = defaultColorModel;

  putTestHeader("Interactive Color Test");
  if (opt_quietness <= OPTQ_INFO) showInteractiveHelp(0);
  putColorModelSyntax(currentColorModel);

  Queue *arguments = newQueue(NULL, NULL);
  char *line = NULL;
  size_t lineSize = 0;

  while (1) {
    putf("%s> ", currentColorModel->name);
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

    if (isEmptyQueue(arguments)) continue;
    char *command = dequeueItem(arguments);

    if (isAbbreviation(QUIT_COMMAND, command)) {
      if (noMoreArguments(arguments)) break;
    } else if (isAbbreviation(HELP_COMMAND, command)) {
      if (noMoreArguments(arguments)) {
        showInteractiveHelp(1);
        putColorModelSyntax(currentColorModel);
      }
    } else {
      const CommandEntry *cmd = getCommand(command);

      if (cmd) {
        cmd->handler(arguments);
      } else {
        const ColorModel *model = getColorModel(command);

        if (model) {
          if (isEmptyQueue(arguments)) {
            currentColorModel = model;
            putColorModelSyntax(currentColorModel);
          } else {
            model->handler(arguments);
          }
        } else if (isdigit(command[0])) {
          prequeueItem(arguments, command);
          currentColorModel->handler(arguments);
        } else {
          logMessage(LOG_WARNING, "unrecognized command");
        }
      }
    }
  }

  if (line) free(line);
  deallocateQueue(arguments);
  popLogPrefix();
}

typedef struct {
  const char *name;
  const char *objective;
  int *requested;
  int (*perform) (const char *testName);
} RequestableTest;

static const RequestableTest requetableTestTable[] = {
  { .name = "VGA to RGB to VGA Round-Trip Test",
    .objective = "Verify the successful conversion of each VGA color to RGB and then back to VGA",
    .requested = &opt_testVGAtoRGBtoVGA,
    .perform = testVGAtoRGBtoVGA,
  },

  { .name = "VGA Color Descriptions",
    .objective = "List the name and HSV description of each VGA color",
    .requested = &opt_testVGADescriptions,
    .perform = testVGADescriptions,
  },

  { .name = "RGB to HSV to RGB Round-Trip Test",
    .objective = "Verify the successful conversion of some RGB colors to HSV and then back to RGB",
    .requested = &opt_testRGBtoHSVtoRGB,
    .perform = testRGBtoHSVtoRGB,
  },

  { .name = "Color Recognition Test",
    .objective = "Verify the recognition of some common colors",
    .requested = &opt_testColorRecognition,
    .perform = testColorRecognition,
  },

  { .name = "RGB to Nearest VGA Mappings",
    .objective = "Show how some common colors are mapped to their nearest VGA colors",
    .requested = &opt_testRGBtoVGA,
    .perform = testRGBtoVGA,
  },
};

static const size_t requetableTestCount = ARRAY_COUNT(requetableTestTable);

/* perform the requested tests */
static int
performRequestedTests (void) {
  int allPassed = 1;

  if (opt_quietness <= OPTQ_INFO) {
    putNotice("BRLTTY Color Conversion Test Suite");
    putf("\n");
  }

  for (int i=0; i<requetableTestCount; i+=1) {
    const RequestableTest *test = &requetableTestTable[i];

    if (*test->requested) {
      putTestHeader(test->name);

      if (test->objective) {
        if (opt_quietness <= OPTQ_PASS) {
          putf("%s...\n\n", test->objective);
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

  if (opt_quietness <= OPTQ_INFO) {
    putNotice("Tests Complete");
  }

  return allPassed;
}

/* Main test program */
int
main (int argc, char *argv[]) {
  PROCESS_COMMAND_LINE(programDescriptor, argc, argv);

  int testRequested = opt_performAllTests;

  for (int i=0; i<requetableTestCount; i+=1) {
    const RequestableTest *test = &requetableTestTable[i];

    if (*test->requested) {
      if (opt_performAllTests) {
        logMessage(LOG_ERR, "conflicting test options");
        return PROG_EXIT_SYNTAX;
      }

      testRequested = 1;
    } else if (opt_performAllTests) {
      *test->requested = 1;
    }
  }

  if (!testRequested) {
    doInteractiveMode();
  } else if (!performRequestedTests()) {
    return PROG_EXIT_SEMANTIC;
  }

  return PROG_EXIT_SUCCESS;
}
