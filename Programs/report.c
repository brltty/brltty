/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include <string.h>

#include "log.h"
#include "report.h"
#include "queue.h"

typedef struct {
  ReportListener *function;
  void *data;
} ListenerEntry;

typedef struct {
  ReportIdentifier identifier;
  Queue *listeners;
} ReportEntry;

static ReportEntry **reportTable = NULL;
static unsigned int reportSize = 0;
static unsigned int reportCount = 0;

static int
findReportEntry (ReportIdentifier identifier, unsigned int *position) {
  int found = 0;
  int first = 0;
  int last = reportCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    ReportEntry *report = reportTable[current];

    if (report->identifier < identifier) {
      first = current + 1;
    } else if (report->identifier > identifier) {
      last = current - 1;
    } else {
      found = 1;
      break;
    }
  }

  *position = first;
  return found;
}

static ReportEntry *
getReportEntry (ReportIdentifier identifier, int add) {
  unsigned int position;
  int found = findReportEntry(identifier, &position);

  if (found) return reportTable[position];
  if (!add) return NULL;

  if (reportCount == reportSize) {
    unsigned int newSize = reportCount? (reportCount << 1): 1;
    ReportEntry **newTable = realloc(reportTable, ARRAY_SIZE(reportTable, newSize));

    if (!newTable) {
      logMallocError();
      return NULL;
    }

    reportTable = newTable;
    reportSize = newSize;
  }

  {
    ReportEntry **slot = &reportTable[position];
    ReportEntry *report = malloc(sizeof(*report));

    if (!report) {
      logMallocError();
      return NULL;
    }

    memset(report, 0, sizeof(*report));
    report->identifier = identifier;
    report->listeners = NULL;

    memmove(slot+1, slot, ((reportCount++ - position) * sizeof(*slot)));
    *slot = report;

    return report;
  }
}

static int
tellListener (void *item, void *data) {
  ListenerEntry *listener = item;
  ReportListenerParameters *parameters = data;

  logSymbol(LOG_DEBUG, listener->function,
            "report: %u", parameters->reportIdentifier);

  parameters->listenerData = listener->data;
  listener->function(parameters);

  return 0;
}

void
report (ReportIdentifier identifier, const void *data) {
  ReportEntry *report = getReportEntry(identifier, 0);

  if (report) {
    if (report->listeners) {
      ReportListenerParameters parameters = {
        .reportIdentifier = identifier,
        .reportData = data,
        .listenerData = NULL
      };

      processQueue(report->listeners, tellListener, &parameters);
    }
  }
}

static int
testListener (void *item, void *data) {
  ListenerEntry *listener = item;
  ReportListener *function = data;

  return listener->function == function;
}

static Element *
findListenerElement (ReportEntry *report, ReportListener *function) {
  return processQueue(report->listeners, testListener, function);
}

static void
deallocateListenerEntry (void *item, void *data) {
  ListenerEntry *listener = item;

  free(listener);
}

int
registerReportListener (ReportIdentifier identifier, ReportListener *function, void *data) {
  ReportEntry *report = getReportEntry(identifier, 1);

  if (report) {
    if (!report->listeners) {
      if (!(report->listeners = newQueue(deallocateListenerEntry, NULL))) {
        return 0;
      }
    }

    if (findListenerElement(report, function)) {
      logSymbol(LOG_WARNING, function, "report listener already registered: %u", identifier);
      return 1;
    }

    {
      ListenerEntry *listener;

      if ((listener = malloc(sizeof(*listener)))) {
        memset(listener, 0, sizeof(*listener));
        listener->function = function;
        listener->data = data;

        if (enqueueItem(report->listeners, listener)) {
          logSymbol(LOG_DEBUG, function, "report listener registered: %u", identifier);
          return 1;
        }

        free(listener);
      } else {
        logMallocError();
      }
    }
  }

  return 0;
}

int
unregisterReportListener (ReportIdentifier identifier, ReportListener *function) {
  ReportEntry *report = getReportEntry(identifier, 0);

  if (report) {
    if (report->listeners) {
      Element *element = findListenerElement(report, function);

      if (element) {
        deleteElement(element);
        logSymbol(LOG_DEBUG, function, "report listener unregistered: %u", identifier);
        return 1;
      }
    }
  }

  logSymbol(LOG_WARNING, function, "report listener not registered: %u", identifier);
  return 0;
}
