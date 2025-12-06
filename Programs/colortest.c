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

#include <string.h>
#include <ctype.h>

#include "log.h"
#include "strfmt.h"
#include "cmdline.h"
#include "cmdlib.h"
#include "color.h"
#include "color_internal.h"
#include "parse.h"
#include "file.h"

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
static int opt_listVGAColors;
static int opt_testRGBtoHSVtoRGB;
static int opt_testColorRecognition;
static int opt_showRGBtoVGA;

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
    .description = "test the VGA to RGB to VGA round-trip - conflicts with requesting all tests",
  },

  { .word = "vga-colors",
    .letter = 'l',
    .setting.flag = &opt_listVGAColors,
    .description = "list all of the VGA colors - conflicts with requesting all tests",
  },

  { .word = "rgb-roundtrip",
    .letter = 'r',
    .setting.flag = &opt_testRGBtoHSVtoRGB,
    .description = "test the RGB to HSV to RGB round-trip - conflicts with requesting all tests",
  },

  { .word = "color-recognition",
    .letter = 'c',
    .setting.flag = &opt_testColorRecognition,
    .description = "test the recognition of common colors - conflicts with requesting all tests",
  },

  { .word = "vga-mappings",
    .letter = 'm',
    .setting.flag = &opt_showRGBtoVGA,
    .description = "show some RGB to nearest VGA mappings - conflicts with requesting all tests",
  },
END_COMMAND_LINE_OPTIONS(programOptions)

static const char *specifiedCommand;

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
  { .name = "command",
    .description = "the name of the command to execute or of the color model to evaluate",
    .setting = &specifiedCommand,
    .optional = 1,
  },
END_COMMAND_LINE_PARAMETERS(programParameters)

BEGIN_COMMAND_LINE_NOTES(programNotes)
  "The -a option may not be combined with any option that requests a specific test.",
  "Specifying a command conflicts with requesting that any tests be performed.",
  "If none of the tests are requested, and if a command isn't specified, then interactive mode is entered.",
  "",
  "The -q option is cumulative.",
  "Output verbosity is increasingly reduced, each time it's specified, as follows:",
  "  1: Informational messages and initial interactive mode help.",
  "  2: Test objectives and results that pass.",
  "  3: Test results that pass but with a qualification.",
  "  4: Test results that fail.",
  "  5: Test summaries.",
  "  6: Test headers, test status, and color model syntax (interactive mode).",
  "",
  "Command names are case-insensitive and may be abbreviated.",
  "The recognized commands are:",
  "  brightness [percent]",
  "  colors [name]",
  "  grayscale [percent]",
  "  hue [degrees]",
  "  problems",
  "  saturation [percent]",
  "",
  "Color model names are case-insensitive and may be abbreviated.",
  "The supported color models are:",
  "  ANSI  the ANSI terminal 256-color model",
  "  HLS   the Hue Lightness Saturation model",
  "  HSV   the Hue Saturation Value (brightness) model",
  "  RGB   the Red Green Blue odel",
  "  VGA   the Video Graphics Array 16-color model",
  "",
  "The return codes of this command are:",
  "  0  Successful.",
  "  2  A syntax error was encountered.",
  "  3  A test failed, a problem was found, etc.",
  "  4  A system error occurred.",
END_COMMAND_LINE_NOTES

BEGIN_COMMAND_LINE_DESCRIPTOR(programDescriptor)
  .name = "colortest",
  .purpose = "Test the color conversion and name functions.",

  .options = &programOptions,
  .parameters = &programParameters,
  .notes = COMMAND_LINE_NOTES(programNotes),

  .extraParameters = {
    .name = "arg",
    .description = "arguments for the specified command or color model",
  },
END_COMMAND_LINE_DESCRIPTOR

#define VGA_NAME_FORMAT "(%13s)"
#define VGA_COLOR_FORMAT "VGA %2d"
#define RGB_COLOR_FORMAT "RGB(%3d, %3d, %3d)"
#define HSV_COLOR_FORMAT "HSV(%5.1f°, %3.0f%%, %3.0f%%)"

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
      putf("%d/%d tests failed.\n", (count - passes), count);
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
  const char *expectedName;
  const char *alternateName;  /* Optional alternate acceptable name */
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
listVGAColors (const char *testName) {
  if (opt_quietness <= OPTQ_PASS) {
    for (int vga=0; vga<VGA_COLOR_COUNT; vga+=1) {
      const char *vgaName = vgaColorName(vga);

      RGBColor rgb = vgaToRgb(vga);
      ColorNameBuffer rgbName;
      rgbColorToName(rgbName, sizeof(rgbName), rgb);

      putf(VGA_COLOR_FORMAT " " VGA_NAME_FORMAT ": " RGB_COLOR_FORMAT " -> \"%s\"\n",
           vga, vgaName, rgb.r, rgb.g, rgb.b, rgbName);
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
    {"Dark Gray",        64,  64,  64,  "dark gray", "charcoal",
     "grayscale analysis is more granular"},

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

    /* Compound names */
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

    ColorNameBuffer rgbName;
    rgbToName(rgbName, sizeof(rgbName), test->r, test->g, test->b);

    /* Check if result matches expected or alternate name */
    int matchExpected = strcasecmp(rgbName, test->expectedName) == 0;
    int matchAlternate = test->alternateName &&
                         (strcasecmp(rgbName, test->alternateName) == 0);

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
           test->name, test->r, test->g, test->b, rgbName, status);

      if (!passed) {
        putf("  Expected: \"%s\"\n", test->expectedName);

        if (test->alternateName) {
          putf("  Alternate: \"%s\"\n", test->alternateName);
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
showRGBtoVGA (const char *testName) {
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
static const char brightnessName[] = "brightness percent";
static const char grayName[] = "grayscale percent";
static const char hueName[] = "hue angle";
static const char lightnessName[] = "lightness percent";
static const char saturationName[] = "saturation percent";

static void
showColor (RGBColor rgb, HSVColor hsv) {
  putf("%sRGB: (%d, %d, %d)\n", blockIndent, rgb.r, rgb.g, rgb.b);
  putf("%sHSV: (%.1f°, %.0f%%, %.0f%%)\n", blockIndent, hsv.h, hsv.s*100.0f, hsv.v*100.0f);

  {
    unsigned char wasUsingTable = useHSVColorTable;
    unsigned char wasUsingSorting = useHSVColorSorting;

    useHSVColorTable = 0;
    useHSVColorSorting = 0;

    ColorNameBuffer inlineName;
    hsvColorToName(inlineName, sizeof(inlineName), hsv);

    useHSVColorTable = 1;
    ColorNameBuffer lookupName;
    hsvColorToName(lookupName, sizeof(lookupName), hsv);

    useHSVColorSorting = 1;
    ColorNameBuffer sortedName;
    hsvColorToName(sortedName, sizeof(sortedName), hsv);

    int showLookupName = strcasecmp(inlineName, lookupName) != 0;
    int showSortedName = strcasecmp(lookupName, sortedName) != 0;

    if (showLookupName || showSortedName) {
      putf("%sInline Name: %s\n", blockIndent, inlineName);
      putf("%sLookup Name: %s\n", blockIndent, lookupName);

      if (showSortedName) {
        putf("%sSorted Name: %s\n", blockIndent, sortedName);
      }
    } else {
      putf("%sName: %s\n", blockIndent, inlineName);
    }

    useHSVColorTable = wasUsingTable;
    useHSVColorSorting = wasUsingSorting;
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

static int
parseIntensity (int *intensity, const char *argument, const char *name) {
  return parseInteger(intensity, argument, 0, UINT8_MAX, name);
}

static int
rgbHandler (CommandArguments *arguments) {
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
        if (verifyNoMoreArguments(arguments)) {
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
hsvHandler (CommandArguments *arguments) {
  const char *hueArgument;
  float hueAngle;

  const char *saturationArgument;
  float saturationLevel;

  const char *brightnessArgument;
  float brightnessLevel;

  if ((hueArgument = getNextArgument(arguments, hueName))) {
    if ((saturationArgument = getNextArgument(arguments, saturationName))) {
      if ((brightnessArgument = getNextArgument(arguments, brightnessName))) {
        if (verifyNoMoreArguments(arguments)) {
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
hlsHandler (CommandArguments *arguments) {
  const char *hueArgument;
  float hueAngle;

  const char *lightnessArgument;
  float lightnessLevel;

  const char *saturationArgument;
  float saturationLevel;

  if ((hueArgument = getNextArgument(arguments, hueName))) {
    if ((lightnessArgument = getNextArgument(arguments, lightnessName))) {
      if ((saturationArgument = getNextArgument(arguments, saturationName))) {
        if (verifyNoMoreArguments(arguments)) {
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
vgaHandler (CommandArguments *arguments) {
  const char *vgaName = "VGA color number";
  const char *vgaArgument = getNextArgument(arguments, vgaName);

  if (vgaArgument) {
    if (verifyNoMoreArguments(arguments)) {
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
ansiHandler (CommandArguments *arguments) {
  const char *ansiName = "ANSI color number";
  const char *ansiArgument = getNextArgument(arguments, ansiName);

  if (ansiArgument) {
    if (verifyNoMoreArguments(arguments)) {
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
  int (*handler) (CommandArguments *arguments);
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
cmdGrayscale (CommandArguments *arguments) {
  if (checkNoMoreArguments(arguments)) {
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

  {
    const char *argument = getNextArgument(arguments, grayName);

    if (argument) {
      if (verifyNoMoreArguments(arguments)) {
        float level;

        if (parsePercent(&level, argument, grayName)) {
          putf("%s%s\n", blockIndent, gsColorName(level));
          return 1;
        }
      }
    }
  }

  return 0;
}

static int
cmdHue (CommandArguments *arguments) {
  if (checkNoMoreArguments(arguments)) {
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

  {
    const char *argument = getNextArgument(arguments, hueName);

    if (argument) {
      if (verifyNoMoreArguments(arguments)) {
        float angle;

        if (parseDegrees(&angle, argument, hueName)) {
          putf("%s%s\n", blockIndent, hueColorName(angle));
          return 1;
        }
      }
    }
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
cmdSaturation (CommandArguments *arguments) {
  if (checkNoMoreArguments(arguments)) {
    hsvListModifiers(hsvSaturationModifier);
    return 1;
  }

  {
    const char *argument = getNextArgument(arguments, saturationName);

    if (argument) {
      if (verifyNoMoreArguments(arguments)) {
        float level;

        if (parsePercent(&level, argument, saturationName)) {
          putf("%s%s\n", blockIndent, hsvSaturationModifier(level)->name);
          return 1;
        }
      }
    }
  }

  return 0;
}

static int
cmdBrightness (CommandArguments *arguments) {
  if (checkNoMoreArguments(arguments)) {
    hsvListModifiers(hsvBrightnessModifier);
    return 1;
  }

  {
    const char *argument = getNextArgument(arguments, brightnessName);

    if (argument) {
      if (verifyNoMoreArguments(arguments)) {
        float level;

        if (parsePercent(&level, argument, brightnessName)) {
          putf("%s%s\n", blockIndent, hsvBrightnessModifier(level)->name);
          return 1;
        }
      }
    }
  }

  return 0;
}

static int
sortHSVColorEntries (const void *item1, const void *item2) {
  const HSVColorEntry *const *color1 = item1;
  const HSVColorEntry *const *color2 = item2;

  int relation = strcasecmp((*color1)->name, (*color2)->name);
  if (relation != 0) return relation;

  if ((*color1)->instance < (*color2)->instance) return -1;
  if ((*color1)->instance > (*color2)->instance) return 1;

  return 0;
}

static void
showHSVColorEntry (const HSVColorEntry *color) {
  putf(
    "%s%s: Hue:%.0f°-%.0f° Sat:%.0f%%-%.0f%% Val:%.0f%%-%.0f%%\n",
    blockIndent, color->name,
    color->hue.minimum, color->hue.maximum,
    color->saturation.minimum*100.0f, color->saturation.maximum*100.0f,
    color->value.minimum*100.0f, color->value.maximum*100.0f
  );
}

static int
cmdColors (CommandArguments *arguments) {
  const HSVColorEntry *colors[hsvColorCount];

  {
    for (int i=0; i<hsvColorCount; i+=1) {
      colors[i] = &hsvColorTable[i];
    }

    qsort(colors, hsvColorCount, sizeof(colors[0]), sortHSVColorEntries);
  }

  if (checkNoMoreArguments(arguments)) {

    for (int i=0; i<hsvColorCount; i+=1) {
      showHSVColorEntry(colors[i]);
    }

    return 1;
  }

  {
    const char *name = getNextArgument(arguments, "color name");

    if (name) {
      if (verifyNoMoreArguments(arguments)) {
        int found = 0;

        for (int i=0; i<hsvColorCount; i+=1) {
          const HSVColorEntry *color = colors[i];

          if (isAbbreviation(color->name, name)) {
            showHSVColorEntry(color);
            found = 1;
          }
        }

        if (found) return 1;
        logMessage(LOG_ERR, "color not recognized: %s", name);
        return 2;
      }
    }
  }

  return 0;
}

static int
hsvValidRange (const HSVComponentRange *range, float minimum, float maximum) {
  if (range->maximum < range->minimum) return 0;
  if (range->minimum < minimum) return 0;
  if (range->maximum > maximum) return 0;
  return 1;
}

static int
hsvValidAngleRange (const HSVComponentRange *range) {
  if ((float)(int)range->minimum != range->minimum) return 0;
  if ((float)(int)range->maximum != range->maximum) return 0;
  return hsvValidRange(range, 0.0f, 360.0f);
}

static int
hsvValidLevelRange (const HSVComponentRange *range) {
  return hsvValidRange(range, 0.0f, 1.0f);
}

static int
hsvRangesOverlap (const HSVComponentRange *range1, const HSVComponentRange *range2) {
  return (range2->minimum < range1->maximum) &&
         (range2->maximum > range1->minimum);
}

static int
hsvColorsOverlap (const HSVColorEntry *color1, const HSVColorEntry *color2) {
  return hsvRangesOverlap(&color1->hue, &color2->hue) &&
         hsvRangesOverlap(&color1->saturation, &color2->saturation) &&
         hsvRangesOverlap(&color1->value, &color2->value);
}

static int
hsvRangeContains (const HSVComponentRange *outer, const HSVComponentRange *inner) {
  return (outer->minimum <= inner->minimum) &&
         (outer->maximum >= inner->maximum);
}

static int
hsvColorContains (const HSVColorEntry *outer, const HSVColorEntry *inner) {
  return hsvRangeContains(&outer->hue, &inner->hue) &&
         hsvRangeContains(&outer->saturation, &inner->saturation) &&
         hsvRangeContains(&outer->value, &inner->value);
}

static void putColorProblem (const HSVColorEntry *color, const char *format, ...) PRINTF(2, 3);

static void
putColorProblem (const HSVColorEntry *color, const char *format, ...) {
  putf("%s%s[%u]: ", blockIndent, color->name, color->instance);

  {
    va_list args;
    va_start(args, format);
    vputf(format, args);
    va_end(args);
  }

  putf("\n");
}

static void
putColorsProblem (const HSVColorEntry *color1, const HSVColorEntry *color2, const char *relation) {
  putf(
    "%s%s[%u] %s %s[%u]\n",
    blockIndent, color1->name, color1->instance,
    relation, color2->name, color2->instance
  );
}


static int
cmdProblems (CommandArguments *arguments) {
  if (verifyNoMoreArguments(arguments)) {
    unsigned int problemCount = 0;

    unsigned char wasUsingSorting = useHSVColorSorting;
    useHSVColorSorting = 1;

    for (int i=0; i<hsvColorCount; i+=1) {
      const HSVColorEntry *color1 = &hsvColorTable[i];

      if (!hsvValidAngleRange(&color1->hue)) {
        problemCount += 1;
        putColorProblem(
          color1, "invalid hue range: %.0f-%.0f",
          color1->hue.minimum, color1->hue.maximum
        );
      }

      if (!hsvValidLevelRange(&color1->saturation)) {
        problemCount += 1;
        putColorProblem(
          color1, "invalid saturation range: %.2f-%.2f",
          color1->saturation.minimum, color1->saturation.maximum
        );
      }

      if (!hsvValidLevelRange(&color1->value)) {
        problemCount += 1;
        putColorProblem(
          color1, "invalid value range: %.2f-%.2f",
          color1->value.minimum, color1->value.maximum
        );
      }

      if (useHSVColorSorting) {
        static const float increment = 0.001;

        HSVColor hsv = {
          .h = color1->hue.minimum + increment,
          .s = color1->saturation.minimum + increment,
          .v = color1->value.minimum + increment,
        };

        const HSVColorEntry *color = hsvColorEntry(hsv);

        if (color1 != color) {
          problemCount += 1;
          putColorProblem(
            color1, "sorted lookup failed (minimum) -> %s",
            color? color->name: "none"
          );
        }
      }

      if (useHSVColorSorting) {
        static const float decrement = 0.001;

        HSVColor hsv = {
          .h = color1->hue.maximum - decrement,
          .s = color1->saturation.maximum - decrement,
          .v = color1->value.maximum - decrement,
        };

        const HSVColorEntry *color = hsvColorEntry(hsv);

        if (color1 != color) {
          problemCount += 1;
          putColorProblem(
            color1, "sorted lookup failed (maximum) -> %s",
            color? color->name: "none"
          );
        }
      }

      for (int j=i+1; j<hsvColorCount; j+=1) {
        const HSVColorEntry *color2 = &hsvColorTable[j];

        if (strcasecmp(color1->name, color2->name) == 0) {
          if (color1->instance == color2->instance) {
            problemCount += 1;
            putColorProblem(color1, "duplicate definition");
          }
        }

        if (hsvColorContains(color1, color2)) {
          problemCount += 1;
          putColorsProblem(color1, color2, "contains");
        } else if (hsvColorContains(color2, color1)) {
          problemCount += 1;
          putColorsProblem(color1, color2, "is within");
        } else if (hsvColorsOverlap(color1, color2)) {
          problemCount += 1;
          putColorsProblem(color1, color2,"overlaps");
        }
      }
    }

    useHSVColorSorting = wasUsingSorting;

    if (problemCount > 0) {
      putf(
        "%u %s found.\n",
        problemCount, ((problemCount == 1)? "problem": "problems")
      );

      return 2;
    }

    if (opt_quietness <= OPTQ_PASS) putf("No problems found.\n");
    return 1;
  }

  return 0;
}

typedef struct {
  const char *name;
  const char *help;
  const char *syntax;
  int (*handler) (CommandArguments *arguments);
} CommandEntry;

static const CommandEntry commandTable[] = {
  { .name = "brightness",
    .help = "List the HSV brightness (value) modifier names and ranges.",
    .syntax = "[percent]",
    .handler = cmdBrightness,
  },

  { .name = "colors",
    .help = "List the color definitions table.",
    .syntax = "[name]",
    .handler = cmdColors,
  },

  { .name = "grayscale",
    .help = "List the grayscale color names and brightness ranges.",
    .syntax = "[percent]",
    .handler = cmdGrayscale,
  },

  { .name = "hue",
    .help = "List the main hue color names and ranges.",
    .syntax = "[degrees]",
    .handler = cmdHue,
  },

  { .name = "problems",
    .help = "Check the color definition table for problems.",
    .syntax = "",
    .handler = cmdProblems,
  },

  { .name = "saturation",
    .help = "List the HSV saturation modifier names and ranges.",
    .syntax = "[percent]",
    .handler = cmdSaturation,
  },
};

static const CommandEntry *
getCommandEntry (const char *name) {
  for (int i=0; i<ARRAY_COUNT(commandTable); i+=1) {
    const CommandEntry *cmd = &commandTable[i];
    if (isAbbreviation(cmd->name, name)) return cmd;
  }

  return NULL;
}

static int
doCommand (const char *name, CommandArguments *arguments, const ColorModel **currentModel) {
  {
    const CommandEntry *cmd = getCommandEntry(name);

    if (cmd) {
      return cmd->handler(arguments);
    }
  }

  {
    const ColorModel *model = getColorModel(name);

    if (model) {
      if (currentModel && checkNoMoreArguments(arguments)) {
        *currentModel = model;
        putColorModelSyntax(model);
        return 1;
      }

      return model->handler(arguments);
    }
  }

  logMessage(LOG_ERR, "unrecognized command: %s", name);
  return 0;
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

    int offsets[ARRAY_COUNT(commandTable)];
    int lengths[ARRAY_COUNT(commandTable)];
    int longest = 0;

    char buffer[0X400];
    STR_BEGIN(buffer, sizeof(buffer));

    for (int i=0; i<ARRAY_COUNT(commandTable); i+=1) {
      const CommandEntry *cmd = &commandTable[i];

      int *offset = &offsets[i];
      *offset = STR_LENGTH;

      STR_PRINTF("%s", cmd->name);
      if (cmd->syntax && *cmd->syntax) STR_PRINTF(" %s", cmd->syntax);

      int *length = &lengths[i];
      *length = STR_LENGTH - *offset;

      if (*length > longest) longest = *length;
    }

    STR_END;

    for (int i=0; i<ARRAY_COUNT(commandTable); i+=1) {
      const CommandEntry *cmd = &commandTable[i];
      int *length = &lengths[i];

      putf("%s%.*s", blockIndent, *length, &buffer[offsets[i]]);

      if (cmd->help && *cmd->help) {
        putf("%*s  %s", (longest - *length), "", cmd->help);
      }

      putf("\n");
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

/* Interactive color test */
static void
doInteractiveMode (void) {
  beginInteractiveMode();
  const ColorModel *currentColorModel = defaultColorModel;

  putTestHeader("Interactive Color Test");
  if (opt_quietness <= OPTQ_INFO) showInteractiveHelp(0);
  putColorModelSyntax(currentColorModel);

  CommandArguments *arguments = newCommandArguments();
  char *line = NULL;
  size_t lineSize = 0;

  while (1) {
    putf("%s> ", currentColorModel->name);
    flushOutput();

    if (!readLine(stdin, &line, &lineSize, NULL)) {
      putf("\n");
      break;
    }

    removeArguments(arguments);
    addArgumentsFromString(arguments, line);

    if (checkNoMoreArguments(arguments)) continue;
    char *command = getNextArgument(arguments, "command");

    if (isAbbreviation(QUIT_COMMAND, command)) {
      if (verifyNoMoreArguments(arguments)) break;
    } else if (isAbbreviation(HELP_COMMAND, command)) {
      if (verifyNoMoreArguments(arguments)) {
        showInteractiveHelp(1);
        putColorModelSyntax(currentColorModel);
      }
    } else if (isdigit(command[0])) {
      restoreArgument(arguments, command);
      currentColorModel->handler(arguments);
    } else {
      doCommand(command, arguments, &currentColorModel);
    }
  }

  if (line) free(line);
  destroyCommandArguments(arguments);
  endInteractiveMode();
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

  { .name = "VGA Color Listing",
    .objective = "List the VGA number and name, as well as the HSV name, of each VGA color",
    .requested = &opt_listVGAColors,
    .perform = listVGAColors,
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
    .requested = &opt_showRGBtoVGA,
    .perform = showRGBtoVGA,
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

  if (*specifiedCommand || (argc > 0)) {
    if (testRequested) {
      logMessage(LOG_ERR, "can't request a test and specify a command");
      return PROG_EXIT_SYNTAX;
    }

    CommandArguments *arguments = newCommandArguments();
    addArgumentsFromArray(arguments, argv, argc);
    int result = doCommand(specifiedCommand, arguments, NULL);
    destroyCommandArguments(arguments);

    if (result == 2) return PROG_EXIT_SEMANTIC;
    return result? PROG_EXIT_SUCCESS: PROG_EXIT_SYNTAX;
  }

  if (!testRequested) {
    doInteractiveMode();
  } else if (!performRequestedTests()) {
    return PROG_EXIT_SEMANTIC;
  }

  return PROG_EXIT_SUCCESS;
}
