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
#include <string.h>

#include "log.h"
#include "cmdline.h"
#include "color.h"

static int opt_quiet;
static int opt_enterInteractiveMode;
static int opt_performAllTests;

static int opt_testVGAtoRGBtoVGA;
static int opt_describeVGAColors;
static int opt_testRGBtoHSVtoRGB;
static int opt_testRGBtoHSV;
static int opt_testRGBtoVGA;

BEGIN_COMMAND_LINE_OPTIONS(programOptions)
  { .word = "quiet",
    .letter = 'q',
    .setting.flag = &opt_quiet,
    .description = "quiet mode - only show test results",
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

  { .word = "vga-rgb-vga",
    .letter = 'v',
    .setting.flag = &opt_testVGAtoRGBtoVGA,
    .description = "test VGA to RGB to VGA round-trip - conflicts with requesting all tests",
  },

  { .word = "describe-vga-colors",
    .letter = 'd',
    .setting.flag = &opt_describeVGAColors,
    .description = "describe VGA colors - conflicts with requesting all tests",
  },

  { .word = "rgb-hsv-rgb",
    .letter = 'r',
    .setting.flag = &opt_testRGBtoHSVtoRGB,
    .description = "test RGB to HSV to RGB round-trip - conflicts with requesting all tests",
  },

  { .word = "common-colors",
    .letter = 'c',
    .setting.flag = &opt_testRGBtoHSV,
    .description = "test common RGB colors - conflicts with requesting all tests",
  },

  { .word = "mapping",
    .letter = 'm',
    .setting.flag = &opt_testRGBtoVGA,
    .description = "test RGB to VGA color mapping - conflicts with requesting all tests",
  },
END_COMMAND_LINE_OPTIONS(programOptions)

BEGIN_COMMAND_LINE_PARAMETERS(programParameters)
END_COMMAND_LINE_PARAMETERS(programParameters)

/* Test structure for predefined colors */
typedef struct {
  const char *name;
  unsigned char r, g, b;
  const char *expectedDescription;
  const char *alternateDescription;  /* Optional alternate acceptable description */
  const char *alternateReason;       /* Reason why alternate is acceptable */
} ColorTest;

/* Print a test result */
static void
printTestResult (const char *testName, int passed) {
  printf("[%s] %s\n", passed ? "PASS" : "FAIL", testName);
}

/* Test VGA palette round-trip conversion */
static void
testVGAtoRGBtoVGA (void) {
  printf("\n=== VGA Color Round-Trip Test ===\n");
  printf("Testing that VGA colors convert to themselves...\n\n");

  int allPassed = 1;
  for (int vga=0; vga<VGA_COLOR_COUNT; vga+=1) {
    RGBColor rgb = vgaToRgb(vga);
    int vgaBack = rgbToVga(rgb.r, rgb.g, rgb.b);
    const char *name = vgaColorName(vga);

    int passed = (vga == vgaBack);
    allPassed = allPassed && passed;

    printf("VGA %2d (%15s): RGB(%3d,%3d,%3d) -> VGA %2d [%s]\n",
           vga, name, rgb.r, rgb.g, rgb.b, vgaBack,
           passed ? "OK" : "FAIL");
  }

  printf("\n");
  printTestResult("VGA Round-Trip Conversion", allPassed);
}

/* Test VGA color descriptions */
static void
describeVGAColors (void) {
  printf("\n=== VGA Color Descriptions ===\n");
  printf("Describing each VGA color...\n\n");

  for (int vga=0; vga<VGA_COLOR_COUNT; vga+=1) {
    RGBColor rgb = vgaToRgb(vga);
    const char *name = vgaColorName(vga);
    char description[64];

    rgbColorToDescription(description, sizeof(description), rgb);

    printf("VGA %2d (%15s): RGB(%3d,%3d,%3d) -> \"%s\"\n",
           vga, name, rgb.r, rgb.g, rgb.b, description);
  }
}

/* Test HSV conversion round-trip */
static void
testRGBtoHSVtoRGB (void) {
  printf("\n=== HSV Round-Trip Test ===\n");
  printf("Testing RGB -> HSV -> RGB conversion...\n\n");

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

  int allPassed = 1;
  for (int i=0; i<ARRAY_COUNT(tests); i+=1) {
    const ColorTest *test = &tests[i];

    /* RGB -> HSV -> RGB */
    HSVColor hsv = rgbToHsv(test->r, test->g, test->b);
    RGBColor rgb = hsvToRgb(hsv.h, hsv.s, hsv.v);

    /* Allow small rounding errors (±1) */
    int rDiff = abs((int)rgb.r - (int)test->r);
    int gDiff = abs((int)rgb.g - (int)test->g);
    int bDiff = abs((int)rgb.b - (int)test->b);

    int passed = (rDiff <= 1 && gDiff <= 1 && bDiff <= 1);
    allPassed = allPassed && passed;

    printf("%-12s: RGB(%3d,%3d,%3d) -> HSV(%6.1f°,%4.2f,%4.2f) -> RGB(%3d,%3d,%3d) [%s]\n",
           test->name, test->r, test->g, test->b,
           hsv.h, hsv.s, hsv.v,
           rgb.r, rgb.g, rgb.b,
           passed ? "OK" : "FAIL");
  }

  printf("\n");
  printTestResult("HSV Round-Trip Conversion", allPassed);
}

/* Test common color descriptions
 *
 * Color Reference: Most test colors are taken from the CSS/HTML Named Colors
 * specification (W3C CSS Color Module Level 3):
 * https://www.w3.org/TR/css-color-3/#svg-color
 *
 * These are standard web colors used across browsers, design tools, and the X11
 * color system. Using standard colors ensures our detection algorithms work with
 * commonly recognized color names.
 */
static void
testRGBtoHSV (void) {
  printf("\n=== Common Color Descriptions Test ===\n");
  printf("Testing recognition of common color names...\n\n");

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

  const int total = ARRAY_COUNT(tests);
  int passed = 0;

  for (int i=0; i<total; i+=1) {
    const ColorTest *test = &tests[i];
    char description[64];

    rgbToDescription(description, sizeof(description), test->r, test->g, test->b);

    /* Check if result matches expected or alternate description */
    int matchExpected = (strcmp(description, test->expectedDescription) == 0);
    int matchAlternate = (test->alternateDescription != NULL &&
                          strcmp(description, test->alternateDescription) == 0);
    int match = matchExpected || matchAlternate;

    if (match) passed++;

    /* Determine status display */
    const char *status;
    if (matchExpected) {
      status = "OK";
    } else if (matchAlternate) {
      status = "OK (alternate)";
    } else {
      status = "FAIL";
    }

    printf("%-20s RGB(%3d,%3d,%3d) -> %-20s [%s]\n",
           test->name, test->r, test->g, test->b, description, status);

    if (!match) {
      printf("  Expected: \"%s\"\n", test->expectedDescription);
      if (test->alternateDescription) {
        printf("  Alternate: \"%s\"\n", test->alternateDescription);
      }
    } else if (matchAlternate) {
      /* Show why alternate was accepted */
      printf("  Note: %s\n", test->alternateReason);
    }
  }

  printf("\n");
  printf("Passed %d/%d tests\n", passed, total);
  printTestResult("Common Color Descriptions", passed == total);
}

/* Test RGB to VGA conversion with various colors */
static void
testRGBtoVGA (void) {
  printf("\n=== RGB to VGA Conversion Test ===\n");
  printf("Testing RGB colors map to nearest VGA color...\n\n");

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

  for (int i=0; i<ARRAY_COUNT(tests); i+=1) {
    const ColorTest *test = &tests[i];
    int vga = rgbToVga(test->r, test->g, test->b);
    RGBColor vgaRgb = vgaToRgb(vga);
    const char *vgaName = vgaColorName(vga);

    printf("%-15s RGB(%3d,%3d,%3d) -> VGA %2d (%s) RGB(%3d,%3d,%3d)\n",
           test->name, test->r, test->g, test->b,
           vga, vgaName, vgaRgb.r, vgaRgb.g, vgaRgb.b);
  }
}

/* Interactive color description test */
static void
enterInteractiveMode (void) {
  printf("\n=== Interactive Color Test ===\n");
  printf("Enter RGB values to test color descriptions.\n");
  printf("Format: R G B (0-255 for each)\n");
  printf("Enter 'q' to quit.\n\n");

  char line[256];
  while (1) {
    printf("RGB> ");
    fflush(stdout);

    if (!fgets(line, sizeof(line), stdin)) break;

    if (line[0] == 'q' || line[0] == 'Q') break;

    int r, g, b;
    if (sscanf(line, "%d %d %d", &r, &g, &b) == 3) {
      if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        printf("Error: RGB values must be 0-255\n");
        continue;
      }

      char description[64];
      HSVColor hsv = rgbToHsv(r, g, b);
      int vga = rgbToVga(r, g, b);
      const char *vgaName = vgaColorName(vga);

      rgbToDescription(description, sizeof(description), r, g, b);

      printf("\n");
      printf("  RGB: (%d, %d, %d)\n", r, g, b);
      printf("  HSV: (%.1f°, %.2f, %.2f)\n", hsv.h, hsv.s, hsv.v);
      printf("  Description: %s\n", description);
      printf("  Nearest VGA: %d (%s)\n", vga, vgaName);
      printf("\n");
    } else {
      printf("Error: Invalid format. Use: R G B\n");
    }
  }
}

/* perform the requested tests */
static ProgramExitStatus
performRequestedTests (void) {
  typedef struct {
    int *requested;
    void (*perform) (void);
  } TestEntry;

  static const TestEntry testTable[] = {
    { .requested = &opt_testVGAtoRGBtoVGA,
      .perform = testVGAtoRGBtoVGA
    },

    { .requested = &opt_describeVGAColors,
      .perform = describeVGAColors
    },

    { .requested = &opt_testRGBtoHSVtoRGB,
      .perform = testRGBtoHSVtoRGB
    },

    { .requested = &opt_testRGBtoHSV,
      .perform = testRGBtoHSV
    },

    { .requested = &opt_testRGBtoVGA,
      .perform = testRGBtoVGA
    },
  };

  const size_t testCount = ARRAY_COUNT(testTable);
  int testRequested = 0;

  for (int i=0; i<testCount; i+=1) {
    const TestEntry *test = &testTable[i];

    if (*test->requested) {
      if (opt_performAllTests) {
        logMessage(LOG_ERR, "conflicting test options");
        return PROG_EXIT_SYNTAX;
      }

      testRequested = 1;
      break;
    }
  }

  if (!testRequested) {
    if (opt_performAllTests || !opt_enterInteractiveMode) {
      for (int i=0; i<testCount; i+=1) {
        const TestEntry *test = &testTable[i];
        *test->requested = 1;
      }
    }
  }

  for (int i=0; i<testCount; i+=1) {
    const TestEntry *test = &testTable[i];
    if (*test->requested) test->perform();
  }

  return PROG_EXIT_SUCCESS;
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

  if (!opt_quiet) {
    printf("==================================================\n");
    printf("       BRLTTY Color Conversion Test Suite        \n");
    printf("==================================================\n");
  }

  /* Perform the requested tests */
  {
    ProgramExitStatus status = performRequestedTests();
    if (status != PROG_EXIT_SUCCESS) return status;
  }

  /* Interactive mode if requested */
  if (opt_enterInteractiveMode) {
    enterInteractiveMode();
  } else if (!opt_quiet) {
    printf("\n");
    printf("Run with the -i flag for interactive mode.\n");
  }

  if (!opt_quiet) {
    printf("\n");
    printf("==================================================\n");
    printf("                Tests Complete                    \n");
    printf("==================================================\n");
  }

  return PROG_EXIT_SUCCESS;
}
