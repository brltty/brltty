/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>

#include "msg_locale.h"
#include "log.h"
#include "file.h"

static char *localeSpecifier = NULL;
static char *localeDomain = NULL;
static char *localeDirectory = NULL;

const char *
getMessageLocaleSpecifier (void) {
  return localeSpecifier;
}

const char *
getMessageLocaleDomain (void) {
  return localeDomain;
}

const char *
getMessageLocaleDirectory (void) {
  return localeDirectory;
}

#ifdef ENABLE_I18N_SUPPORT
static void
releaseLocaleData (void) {
}

static int
setDomain (const char *domain) {
  const char *result = textdomain(domain);
  if (result) return 1;

  logSystemError("textdomain");
  return 0;
}

static int
bindDomain (const char *directory) {
  const char *result = bindtextdomain(localeDomain, directory);
  if (result) return 1;

  logSystemError("bindtextdomain");
  return 0;
}
#else /* ENABLE_I18N_SUPPORT */
static const uint32_t magicNumber = UINT32_C(0X950412DE);
typedef uint32_t GetIntegerFunction (uint32_t value);

typedef struct {
  uint32_t magicNumber;
  uint32_t versionNumber;
  uint32_t stringCount;
  uint32_t originalStrings;
  uint32_t translatedStrings;
} LocaleDataHeader;

typedef struct {
  union {
    void *area;
    const unsigned char *bytes;
    const LocaleDataHeader *header;
  } view;

  size_t areaSize;
  GetIntegerFunction *getInteger;
} LocaleData;

static LocaleData localeData = {
  .view = {
    .area = NULL
  },

  .areaSize = 0,
  .getInteger = NULL,
};

static void
releaseLocaleData (void) {
  if (localeData.view.area) {
    free(localeData.view.area);
    localeData.view.area = NULL;
  }

  localeData.areaSize = 0;
  localeData.getInteger = NULL;
}

static int
setDomain (const char *domain) {
  return 1;
}

static int
bindDomain (const char *directory) {
  return 1;
}

static char *
makeLocaleRootPath (void) {
  if (localeSpecifier && localeDomain && localeDirectory) {
    size_t length = strlen(localeSpecifier);

    char dialect[length + 1];
    strcpy(dialect, localeSpecifier);
    length = strcspn(dialect, ".@");
    dialect[length] = 0;

    char language[length + 1];
    strcpy(language, dialect);
    length = strcspn(language, "_");
    language[length] = 0;

    char *names[] = {dialect, language, NULL};
    char **name = names;

    while (*name) {
      char *path = makePath(localeDirectory, *name);

      if (path) {
        if (testDirectoryPath(path)) return path;
        free(path);
      }

      name += 1;
    }
  }

  return NULL;
}

static char *
makeLocaleDataPath (void) {
  char *directory = makeLocaleRootPath();

  if (directory) {
    char *subdirectory = makePath(directory, "LC_MESSAGES");

    free(directory);
    directory = NULL;

    if (subdirectory) {
      char *file = makeFilePath(subdirectory, localeDomain, ".mo");

      free(subdirectory);
      subdirectory = NULL;

      if (file) return file;
    }
  }

  return NULL;
}

static uint32_t
getNativeInteger (uint32_t value) {
  return value;
}

static uint32_t
getFlippedInteger (uint32_t value) {
  uint32_t result = 0;

  while (value) {
    result <<= 8;
    result |= value & UINT8_MAX;
    value >>= 8;
  }

  return result;
}

static int
verifyLocaleData (LocaleData *data) {
  const LocaleDataHeader *header = data->view.header;

  {
    static GetIntegerFunction *const functions[] = {getNativeInteger, getFlippedInteger, NULL};
    GetIntegerFunction *const *function = functions;

    while (*function) {
      if ((*function)(header->magicNumber) == magicNumber) {
        data->getInteger = *function;
        return 1;
      }

      function += 1;
    }
  }

  return 0;
}

static int
loadLocaleData (void) {
  if (localeData.view.area) return 1;

  int loaded = 0;
  char *path = makeLocaleDataPath();

  if (path) {
    int fd = open(path, O_RDONLY);

    if (fd != -1) {
      struct stat info;

      if (fstat(fd, &info) != -1) {
        size_t size = info.st_size;
        void *area = NULL;

        if (size) {
          if ((area = malloc(size))) {
            ssize_t count = read(fd, area, size);

            if (count == -1) {
              logMessage(LOG_DEBUG, "locale data read error: %s: %s", path, strerror(errno));
            } else if (count < size) {
              logMessage(LOG_DEBUG, "locale data truncated: %zu < %zu: %s", count, size, path);
            } else {
              LocaleData data = {
                .view.area = area,
                .areaSize = size
              };

              if (verifyLocaleData(&data)) {
                localeData = data;
                loaded = 1;
              }
            }

            if (!loaded) free(area);
          } else {
            logMallocError();
          }
        }
      } else {
        logMessage(LOG_DEBUG, "locale data stat error: %s: %s", path, strerror(errno));
      }

      close(fd);
    } else {
      logMessage(LOG_DEBUG, "locale data open error: %s: %s", path, strerror(errno));
    }

    free(path);
  }

  return loaded;
}

static inline const LocaleDataHeader *
getHeader (void) {
  return localeData.view.header;
}

static inline const void *
getItem (uint32_t offset) {
  return &localeData.view.bytes[localeData.getInteger(offset)];
}

static inline uint32_t
getStringCount (void) {
  return localeData.getInteger(getHeader()->stringCount);
}

typedef struct {
  uint32_t length;
  uint32_t offset;
} LocaleDataString;

static inline uint32_t
getStringLength (const LocaleDataString *string) {
  return localeData.getInteger(string->length);
}

static inline const char *
getStringText (const LocaleDataString *string) {
  return getItem(string->offset);
}

static inline const LocaleDataString *
getOriginalStrings (void) {
  return getItem(getHeader()->originalStrings);
}

static inline const LocaleDataString *
getOriginalString (unsigned int index) {
  return &getOriginalStrings()[index];
}

static inline const LocaleDataString *
getTranslatedStrings (void) {
  return getItem(getHeader()->translatedStrings);
}

static inline const LocaleDataString *
getTranslatedString (unsigned int index) {
  return &getTranslatedStrings()[index];
}

static int
findOriginalString (const char *text, size_t textLength, unsigned int *index) {
  const LocaleDataString *strings = getOriginalStrings();
  int from = 0;
  int to = getStringCount();

  while (from < to) {
    int current = (from + to) / 2;
    const LocaleDataString *string = &strings[current];

    uint32_t stringLength = getStringLength(string);
    int relation = memcmp(text, getStringText(string), MIN(textLength, stringLength));

    if (relation == 0) {
      if (textLength == stringLength) {
        *index = current;
        return 1;
      }

      relation = (textLength < stringLength)? -1: 1;
    }

    if (relation < 0) {
      to = current;
    } else {
      from = current + 1;
    }
  }

  return 0;
}

static const char *
findTranslation (const char *text, size_t length) {
  if (!text) return NULL;
  if (!length) return NULL;

  if (loadLocaleData()) {
    unsigned int index;

    if (findOriginalString(text, length, &index)) {
      const LocaleDataString *string = getTranslatedString(index);
      return getStringText(string);
    }
  }

  return NULL;
}

char *
gettext (const char *text) {
  const char *translation = findTranslation(text, strlen(text));
  if (!translation) translation = text;
  return (char *)translation;
}

static const char *
getTranslation (unsigned int index, const char *const *strings) {
  unsigned int count = 0;
  while (strings[count]) count += 1;

  size_t size = 0;
  size_t lengths[count];

  for (unsigned int index=0; index<count; index+=1) {
    size_t length = strlen(strings[index]);
    lengths[index] = length;
    size += length + 1;
  }

  char text[size];
  char *byte = text;

  for (unsigned int index=0; index<count; index+=1) {
    byte = mempcpy(byte, strings[index], (lengths[index] + 1));
  }

  byte -= 1; // the length mustn't include the final NUL
  const char *translation = findTranslation(text, (byte - text));
  if (!translation) return strings[index];

  while (index > 0) {
    translation += strlen(translation) + 1;
    index -= 1;
  }

  return translation;
}

char *
ngettext (const char *singular, const char *plural, unsigned long int count) {
  unsigned int index = 0;
  if (count != 1) index += 1;

  const char *const strings[] = {singular, plural, NULL};
  return (char *)getTranslation(index, strings);
}
#endif /* ENABLE_I18N_SUPPORT */

static int
updateProperty (char **property, const char *value, int (*updater) (const char *value)) {
  char *copy = strdup(value);

  if (copy) {
    if (!updater || updater(value)) {
      if (*property) free(*property);
      *property = copy;
      return 1;
    }

    free(copy);
  } else {
    logMallocError();
  }

  return 0;
}

int
setMessageLocaleSpecifier (const char *specifier) {
  releaseLocaleData();
  return updateProperty(&localeSpecifier, specifier, NULL);
}

int
setMessageLocaleDomain (const char *domain) {
  releaseLocaleData();
  return updateProperty(&localeDomain, domain, setDomain);
}

int
setMessageLocaleDirectory (const char *directory) {
  releaseLocaleData();
  return updateProperty(&localeDirectory, directory, bindDomain);
}

void
setMessageLocale (void) {
  if (!localeSpecifier) {
    const char *specifier = setlocale(LC_MESSAGES, "");
    if (!specifier) specifier = "C.UTF-8";
    setMessageLocaleSpecifier(specifier);
  }

  setMessageLocaleDomain(PACKAGE_TARNAME);
  setMessageLocaleDirectory(LOCALE_DIRECTORY);
}
