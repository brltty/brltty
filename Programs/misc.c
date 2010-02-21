/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include "misc.h"
#include "log.h"

#ifdef __MSDOS__
#include "sys_msdos.h"
#endif /* __MSDOS__ */

char **
splitString (const char *string, char delimiter, int *count) {
  char **array = NULL;

  if (string) {
    while (1) {
      const char *start = string;
      int index = 0;

      if (*start) {
        while (1) {
          const char *end = strchr(start, delimiter);
          int length = end? end-start: strlen(start);

          if (array) {
            char *element = mallocWrapper(length+1);
            memcpy(element, start, length);
            element[length] = 0;
            array[index] = element;
          }
          index++;

          if (!end) break;
          start = end + 1;
        }
      }

      if (array) {
        array[index] = NULL;
        if (count) *count = index;
        break;
      }

      array = mallocWrapper((index + 1) * sizeof(*array));
    }
  } else if (count) {
    *count = 0;
  }

  return array;
}

void
deallocateStrings (char **array) {
  char **element = array;
  while (*element) free(*element++);
  free(array);
}

char *
joinStrings (const char *const *strings, int count) {
  char *string;
  size_t length = 0;
  size_t lengths[count];
  int index;

  for (index=0; index<count; index+=1) {
    length += lengths[index] = strlen(strings[index]);
  }

  if ((string = malloc(length+1))) {
    char *target = string;

    for (index=0; index<count; index+=1) {
      length = lengths[index];
      memcpy(target, strings[index], length);
      target += length;
    }

    *target = 0;
  }

  return string;
}

static void
noMemory (void) {
  LogPrint(LOG_CRIT, "Insufficient memory: %s", strerror(errno));
  exit(3);
}

void *
mallocWrapper (size_t size) {
  void *address = malloc(size);
  if (!address && size) noMemory();
  return address;
}

void *
reallocWrapper (void *address, size_t size) {
  if (!(address = realloc(address, size)) && size) noMemory();
  return address;
}

char *
strdupWrapper (const char *string) {
  char *address = strdup(string);
  if (!address) noMemory();
  return address;
}

void
approximateDelay (int milliseconds) {
  if (milliseconds > 0) {
#if defined(__MINGW32__)
    Sleep(milliseconds);
#elif defined(__MSDOS__)
    tsr_usleep(milliseconds*1000);
#else /* delay */
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &timeout);
#endif /* delay */
  }
}

#ifdef __MINGW32__
#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 10))
int
gettimeofday (struct timeval *tvp, void *tzp) {
  DWORD time = GetTickCount();
  /* this is not 49.7 days-proof ! */
  tvp->tv_sec = time / 1000;
  tvp->tv_usec = (time % 1000) * 1000;
  return 0;
}
#endif /* gettimeofday() */

#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 15))
void
usleep (int usec) {
  if (usec > 0) {
    approximateDelay((usec+999)/1000);
  }
}
#endif /* usleep() */
#endif /* __MINGW32__ */

ProcessIdentifier
getProcessIdentifier (void) {
#if defined(__MINGW32__)
  return GetCurrentProcessId();
#elif defined(__MSDOS__)
  return DOS_PROCESS_ID;
#else /* Unix */
  return getpid();
#endif /* getProcessIdentifier() */
}

int
testProcessIdentifier (ProcessIdentifier pid) {
#if defined(__MINGW32__)
  HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (!handle) return 0;
  CloseHandle(handle);
  return 1;
#elif defined(__MSDOS__)
  return pid == DOS_PROCESS_ID;
#else /* Unix */
  return kill(pid, 0) != -1;
#endif /* testProcessIdentifier() */
}

long int
millisecondsBetween (const struct timeval *from, const struct timeval *to) {
  return ((to->tv_sec - from->tv_sec) * 1000) + ((to->tv_usec - from->tv_usec) / 1000);
}

long int
millisecondsSince (const struct timeval *from) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return millisecondsBetween(from, &now);
}

void
accurateDelay (int milliseconds) {
  static int tickLength = 0;
  struct timeval start;
  gettimeofday(&start, NULL);
  if (!tickLength) {
#if defined(_SC_CLK_TCK)
    tickLength = 1000 / sysconf(_SC_CLK_TCK);
#elif defined(CLK_TCK)
    tickLength = 1000 / CLK_TCK;
#elif defined(HZ)
    tickLength = 1000 / HZ;
#else /* tick length */
#error cannot determine tick length
#endif /* tick length */
    if (!tickLength) tickLength = 1;
  }
  if (milliseconds >= tickLength) approximateDelay(milliseconds / tickLength * tickLength);
  while (millisecondsSince(&start) < milliseconds) {
#ifdef __MSDOS__
    /* We're executing as a system clock interrupt TSR so we need to allow
     * them to occur in order for gettimeofday() to change.
     */
    tsr_usleep(1);
#endif /* __MSDOS__ */
  }
}

int
hasTimedOut (int milliseconds) {
  static struct timeval start = {0, 0};

  if (milliseconds) return millisecondsSince(&start) >= milliseconds;

  gettimeofday(&start, NULL);
  return 1;
}

int
rescaleInteger (int value, int from, int to) {
  return (to * (value + (from / (to * 2)))) / from;
}

int
isInteger (int *value, const char *string) {
  if (*string) {
    char *end;
    long l = strtol(string, &end, 0);
    if (!*end) {
      *value = l;
      return 1;
    }
  }
  return 0;
}

int
isFloat (float *value, const char *string) {
  if (*string) {
    char *end;
    double d = strtod(string, &end);
    if (!*end) {
      *value = d;
      return 1;
    }
  }
  return 0;
}

int
validateInteger (int *value, const char *string, const int *minimum, const int *maximum) {
  if (*string) {
    int i;
    if (!isInteger(&i, string)) return 0;
    if (minimum && (i < *minimum)) return 0;
    if (maximum && (i > *maximum)) return 0;
    *value = i;
  }
  return 1;
}

int
validateFloat (float *value, const char *string, const float *minimum, const float *maximum) {
  if (*string) {
    float f;
    if (!isFloat(&f, string)) return 0;
    if (minimum && (f < *minimum)) return 0;
    if (maximum && (f > *maximum)) return 0;
    *value = f;
  }
  return 1;
}

int
validateChoice (unsigned int *value, const char *string, const char *const *choices) {
  int length = strlen(string);
  *value = 0;
  if (!length) return 1;

  {
    int index = 0;
    while (choices[index]) {
      if (strncasecmp(string, choices[index], length) == 0) {
        *value = index;
        return 1;
      }
      ++index;
    }
  }

  return 0;
}

int
validateFlag (unsigned int *value, const char *string, const char *on, const char *off) {
  const char *choices[] = {off, on, NULL};
  return validateChoice(value, string, choices);
}

int
validateOnOff (unsigned int *value, const char *string) {
  return validateFlag(value, string, "on", "off");
}

int
validateYesNo (unsigned int *value, const char *string) {
  return validateFlag(value, string, "yes", "no");
}

static void
parseParameters (
  char **values,
  const char *const *names,
  const char *qualifier,
  const char *parameters
) {
  if (parameters && *parameters) {
    char *copy = strdupWrapper(parameters);
    char *name = copy;

    while (1) {
      char *end = strchr(name, ',');
      int done = end == NULL;
      if (!done) *end = 0;

      if (*name) {
        char *value = strchr(name, '=');
        if (!value) {
          LogPrint(LOG_ERR, "%s: %s", gettext("missing parameter value"), name);
        } else if (value == name) {
        noName:
          LogPrint(LOG_ERR, "%s: %s", gettext("missing parameter name"), name);
        } else {
          int nameLength = value++ - name;
          int eligible = 1;

          if (qualifier) {
            char *colon = memchr(name, ':', nameLength);
            if (colon) {
              int qualifierLength = colon - name;
              int nameAdjustment = qualifierLength + 1;
              eligible = 0;
              if (!qualifierLength) {
                LogPrint(LOG_ERR, "%s: %s", gettext("missing parameter qualifier"), name);
              } else if (!(nameLength -= nameAdjustment)) {
                goto noName;
              } else if ((qualifierLength == strlen(qualifier)) &&
                         (memcmp(name, qualifier, qualifierLength) == 0)) {
                name += nameAdjustment;
                eligible = 1;
              }
            }
          }

          if (eligible) {
            unsigned int index = 0;
            while (names[index]) {
              if (strncasecmp(name, names[index], nameLength) == 0) {
                free(values[index]);
                values[index] = strdupWrapper(value);
                break;
              }
              ++index;
            }

            if (!names[index]) {
              LogPrint(LOG_ERR, "%s: %s", gettext("unsupported parameter"), name);
            }
          }
        }
      }

      if (done) break;
      name = end + 1;
    }

    free(copy);
  }
}

char **
getParameters (const char *const *names, const char *qualifier, const char *parameters) {
  char **values;

  if (!names) {
    static const char *const noNames[] = {NULL};
    names = noNames;
  }

  {
    unsigned int count = 0;
    while (names[count]) ++count;
    values = mallocWrapper((count + 1) * sizeof(*values));
    values[count] = NULL;
    while (count--) values[count] = strdupWrapper("");
  }

  parseParameters(values, names, qualifier, parameters);
  return values;
}

void
logParameters (const char *const *names, char **values, char *description) {
  if (names && values) {
    while (*names) {
      LogPrint(LOG_INFO, "%s: %s=%s", description, *names, *values);
      ++names;
      ++values;
    }
  }
}
