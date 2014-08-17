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
#include "program.h"
#include "queue.h"

typedef struct {
  ReportIdentifier identifier;
  Queue *listeners;
} ReportEntry;

static ReportEntry *reportTable = NULL;
static unsigned int reportSize = 0;
static unsigned int reportCount = 0;

static void
exitReport (void *data) {
  if (reportTable) {
    free(reportTable);
    reportTable = NULL;
  }

  reportSize = 0;
  reportCount = 0;
}

static ReportEntry *
getReportEntry (ReportIdentifier identifier) {
  int first = 0;
  int last = reportCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    ReportEntry *report = &reportTable[current];

    if (report->identifier < identifier) {
      first = current + 1;
    } else if (report->identifier > identifier) {
      last = current - 1;
    } else {
      return report;
    }
  }

  if (reportCount == reportSize) {
    unsigned int newSize = reportCount? (reportCount << 1): 1;
    ReportEntry *newTable = realloc(reportTable, ARRAY_SIZE(reportTable, newSize));

    if (!newTable) {
      logMallocError();
      return NULL;
    }

    reportTable = newTable;
    reportSize = newSize;

    if (!reportCount) {
      onProgramExit("report", exitReport, NULL);
    }
  }

  {
    ReportEntry *report = &reportTable[first];

    memmove(report+1, report, ((reportCount++ - first) * sizeof(*report)));
    memset(report, 0, sizeof(*report));

    report->identifier = identifier;
    report->listeners = NULL;

    return report;
  }
}

static int
tellListener (void *item, void *data) {
  ReportListener *listener = item;
  const ReportListenerParameters *parameters = data;

  listener(parameters);
  return 0;
}

void
report (ReportIdentifier identifier, const void *data) {
  ReportEntry *report = getReportEntry(identifier);

  if (report) {
    if (report->listeners) {
      ReportListenerParameters parameters = {
        .identifier = identifier,
        .data = data
      };

      processQueue(report->listeners, tellListener, &parameters);
    }
  }
}

int
registerReportListener (ReportIdentifier identifier, ReportListener *listener) {
  ReportEntry *report = getReportEntry(identifier);

  if (report) {
    if (!report->listeners) {
      if (!(report->listeners = newQueue(NULL, NULL))) {
        return 0;
      }
    }

    if (findElementWithItem(report->listeners, listener)) {
      logSymbol(LOG_WARNING, listener, "report listener already registered: %u", identifier);
      return 1;
    }

    if (enqueueItem(report->listeners, listener)) return 1;
  }

  return 0;
}

void
unregisterReportListener (ReportIdentifier identifier, ReportListener *listener) {
  ReportEntry *report = getReportEntry(identifier);

  if (report) {
    if (report->listeners) {
      if (deleteItem(report->listeners, listener)) {
        return;
      }
    }
  }

  logSymbol(LOG_WARNING, listener, "report listener not registered: %u", identifier);
}
