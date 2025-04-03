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

#ifndef BRLTTY_INCLUDED_OPTIONS
#define BRLTTY_INCLUDED_OPTIONS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *opt_configurationFile;
extern int opt_environmentVariables;
extern int opt_bootParameters;

extern char *opt_preferencesFile;
extern char *opt_overridePreferences;

extern char *opt_logFile;
extern char *opt_logLevel;
extern int opt_logToStandardError;

extern char *opt_localeDirectory;
extern char *opt_tablesDirectory;
extern char *opt_updatableDirectory;
extern char *opt_writableDirectory;
extern char *opt_driversDirectory;
extern char *opt_helpersDirectory;

extern char *opt_textTable;
extern char *opt_contractionTable;
extern char *opt_attributesTable;
extern char *opt_keyboardTable;
extern char *opt_keyboardProperties;

extern char *opt_brailleDriver;
extern char *opt_brailleParameters;
extern char *opt_brailleDevice;
extern int opt_releaseDevice;

extern char *opt_speechDriver;
extern char *opt_speechParameters;
extern int opt_quietIfNoBraille;
extern char *opt_speechInput;

extern char *opt_screenDriver;
extern char *opt_screenParameters;

extern char *opt_pcmDevice;
extern char *opt_midiDevice;

extern int opt_noDaemon;
extern char *opt_pidFile;

extern char *opt_privilegeParameters;
extern int opt_stayPrivileged;

extern char *opt_startMessage;
extern char *opt_stopMessage;

extern int opt_noApi;
extern char *opt_apiParameters;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_OPTIONS */
