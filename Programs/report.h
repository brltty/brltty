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

#ifndef BRLTTY_INCLUDED_REPORT
#define BRLTTY_INCLUDED_REPORT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  REPORT_BRAILLE_OFF,
} ReportIdentifier;

extern void report (ReportIdentifier identiier, const void *data);

typedef struct {
  ReportIdentifier identifier;
  const void *data;
} ReportListenerParameters;

#define REPORT_LISTENER(name) void name (const ReportListenerParameters *parameters)
typedef REPORT_LISTENER(ReportListener);

extern int registerReportListener (ReportIdentifier identifier, ReportListener *listener);
extern void unregisterReportListener (ReportIdentifier identifier, ReportListener *listener);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_REPORT */
