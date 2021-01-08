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

/* FestivalLite/speech.c - Speech library
 * For the Festival Lite text to speech package
 * Maintained by Mario Lang <mlang@delysid.org>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "log.h"
#include "parse.h"

typedef enum {
  PARM_pitch
} DriverParameter;
#define SPKPARMS "pitch"

#include "spk_driver.h"
#include <flite.h>
#include <flite_version.h>

extern	cst_voice *	REGISTER_VOX	(const char *voxdir);
extern	void		UNREGISTER_VOX	(cst_voice *voice);

static	cst_voice	*voice		= NULL;
static	const char	*outtype	= "play";

static	pid_t		child		= -1;

static	int		fds[2];
static	int		*const readfd	= &fds[0];
static	int		*const writefd	= &fds[1];

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting)
{
  feat_set_float(voice->features, "duration_stretch", 1.0/getFloatSpeechRate(setting));
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters)
{
  spk->setRate = spk_setRate;

  child = -1;
  flite_init();
  voice = REGISTER_VOX(NULL);

  {
    int pitch = 100, minpitch = 50, maxpitch = 200;
    if (*parameters[PARM_pitch])
      if (!validateInteger(&pitch, parameters[PARM_pitch], &minpitch, &maxpitch))
        logMessage(LOG_WARNING, "%s: %s", "invalid pitch specification", parameters[PARM_pitch]);
    feat_set_int(voice->features, "int_f0_target_mean", pitch);
  }

  logMessage(LOG_INFO, "Festival Lite Engine: version %s-%s, %s",
	     FLITE_PROJECT_VERSION, FLITE_PROJECT_STATE,
	     FLITE_PROJECT_DATE);
  return 1;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk)
{
  spk_mute(spk);

  UNREGISTER_VOX(voice);
  voice = NULL;
}

static int
doChild (void) {
  FILE *stream = fdopen(*readfd, "r");

  if (stream) {
    char buffer[0X400];
    char *line;

    while ((line = fgets(buffer, sizeof(buffer), stream))) {
      flite_text_to_speech(line, voice, outtype);
    }

    fclose(stream);
  }

  return 0;
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes)
{
  if (child != -1) goto ready;

  if (pipe(fds) != -1) {
    if ((child = fork()) == -1) {
      logSystemError("fork");
    } else if (child == 0) {
      _exit(doChild());
    } else
    ready: {
      unsigned char text[length + 1];
      memcpy(text, buffer, length);
      text[length] = '\n';
      write(*writefd, text, sizeof(text));
      return;
    }

    close(*readfd);
    close(*writefd);
  } else {
    logSystemError("pipe");
  }
}

static void
spk_mute (volatile SpeechSynthesizer *spk)
{
  if (child != -1) {
    close(*readfd);
    close(*writefd);

    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    child = -1;
  }
}
