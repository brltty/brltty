/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_STRFMT
#define BRLTTY_INCLUDED_STRFMT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STR_BEGIN(buffer, size) { \
  char *strNext = (buffer); \
  char *strStart = strNext; \
  char *strEnd = strStart + (size); \
  *strNext = 0;

#define STR_END }

#define STR_LENGTH (size_t)(strNext - strStart)

#define STR_NEXT strNext

#define STR_LEFT (size_t)(strEnd - strNext)

#define STR_ADJUST(length) if ((strNext += (length)) > strEnd) strNext = strEnd

#define STR_PRINTF(format, ...) { \
  size_t strLength = snprintf(STR_NEXT, STR_LEFT, format, ## __VA_ARGS__); \
  STR_ADJUST(strLength); \
}

#define STR_VPRINTF(format, arguments) { \
  size_t strLength = vsnprintf(STR_NEXT, STR_LEFT, format, arguments); \
  STR_ADJUST(strLength); \
}

#define STR_DEFINE_FORMATTER(name, ...) \
  size_t name (char *strFormatterBuffer, size_t strFormatterSize, __VA_ARGS__)

#define STR_BEGIN_FORMATTER(name, ...) \
STR_DEFINE_FORMATTER(name, __VA_ARGS__) { \
  size_t strFormatterResult; \
  STR_BEGIN(strFormatterBuffer, strFormatterSize);

#define STR_END_FORMATTER \
  strFormatterResult = STR_LENGTH; \
  STR_END; \
  return strFormatterResult; \
}

#define STR_FORMAT(formatter, ...) STR_ADJUST(formatter(STR_NEXT, STR_LEFT, __VA_ARGS__))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_STRFMT */
