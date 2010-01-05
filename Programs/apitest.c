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

/* apitest provides a small test utility for BRLTTY's API */
 
#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "options.h"
#include "brldefs.h"
#include "cmd.h"

#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"

static brlapi_connectionSettings_t settings;

static int opt_learnMode;
static int opt_showDots;
static int opt_showName;
static int opt_showSize;
static int opt_showKeyCodes;
static int opt_suspendMode;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'n',
    .word = "name",
    .setting.flag = &opt_showName,
    .description = "Show the name of the braille driver."
  },

  { .letter = 'w',
    .word = "window",
    .setting.flag = &opt_showSize,
    .description = "Show the dimensions of the braille window."
  },

  { .letter = 'd',
    .word = "dots",
    .setting.flag = &opt_showDots,
    .description = "Show dot pattern."
  },

  { .letter = 'l',
    .word = "learn",
    .setting.flag = &opt_learnMode,
    .description = "Enter interactive command learn mode."
  },

  { .letter = 'k',
    .word = "keycodes",
    .setting.flag = &opt_showKeyCodes,
    .description = "Enter interactive keycode learn mode."
  },

  { .letter = 's',
    .word = "suspend",
    .setting.flag = &opt_suspendMode,
    .description = "Suspend the braille driver (press ^C or send SIGUSR1 to resume)."
  },

  { .letter = 'b',
    .word = "brlapi",
    .argument = "[host][:port]",
    .setting.string = &settings.host,
    .description = "BrlAPIa host and/or port to connect to."
  },

  { .letter = 'a',
    .word = "auth",
    .argument = "file",
    .setting.string = &settings.auth,
    .description = "BrlAPI authorization/authentication string."
  },
END_OPTION_TABLE

void showDisplaySize(void)
{
  unsigned int x, y;
  fprintf(stderr,"Getting display size: ");
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%dX%d\n", x, y);
}

void showDriverName(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
}

#define DOTS_TEXT "dots: "
#define DOTS_TEXTLEN (strlen(DOTS_TEXT))
#define DOTS_LEN 8
#define DOTS_TOTALLEN (DOTS_TEXTLEN+DOTS_LEN)
void showDots(void)
{
  unsigned int x, y;
  brlapi_keyCode_t k;
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  if (brlapi_enterTtyMode(-1, NULL)<0) {
    brlapi_perror("enterTtyMode");
    exit(1);
  }
  if (x*y<DOTS_TOTALLEN) {
    fprintf(stderr,"can't show dots with a braille display with less than %d cells\n",(int)DOTS_TOTALLEN);
    exit(1);
  }
  {
    char text[x*y];
    unsigned char or[x*y];
    brlapi_writeArguments_t wa = BRLAPI_WRITEARGUMENTS_INITIALIZER;
    fprintf(stderr,"Showing dot patterns\n");
    memcpy(text,DOTS_TEXT,DOTS_TEXTLEN);
    memset(text+DOTS_TEXTLEN,' ',sizeof(text)-DOTS_TEXTLEN);
    wa.regionBegin = 1;
    wa.regionSize = sizeof(or);
    wa.text = text;
    memset(or,0,sizeof(or));
    or[DOTS_TEXTLEN+0] = BRL_DOT1;
    or[DOTS_TEXTLEN+1] = BRL_DOT2;
    or[DOTS_TEXTLEN+2] = BRL_DOT3;
    or[DOTS_TEXTLEN+3] = BRL_DOT4;
    or[DOTS_TEXTLEN+4] = BRL_DOT5;
    or[DOTS_TEXTLEN+5] = BRL_DOT6;
    or[DOTS_TEXTLEN+6] = BRL_DOT7;
    or[DOTS_TEXTLEN+7] = BRL_DOT8;
    wa.orMask = or;
    if (brlapi_write(&wa)<0) {
      brlapi_perror("brlapi_write");
      exit(1);
    }
  }
  brlapi_readKey(1, &k);
}

void enterLearnMode(void)
{
  int res;
  brlapi_keyCode_t code;
  int cmd;
  char buf[0X100];

  fprintf(stderr,"Entering command learn mode\n");
  if (brlapi_enterTtyMode(-1, NULL)<0) {
    brlapi_perror("enterTtyMode");
    return;
  }

  if (brlapi_writeText(BRLAPI_CURSOR_OFF, "command learn mode")<0) {
    brlapi_perror("brlapi_writeText");
    exit(1);
  }

  while ((res = brlapi_readKey(1, &code)) != -1) {
    fprintf(stderr, "got key %016"BRLAPI_PRIxKEYCODE"\n",code);
    cmd = cmdBrlapiToBrltty(code);
    describeCommand(cmd, buf, sizeof(buf), 1);
    brlapi_writeText(BRLAPI_CURSOR_OFF, buf);
    fprintf(stderr, "%s\n", buf);
    if (cmd==BRL_CMD_LEARN) return;
  }
  brlapi_perror("brlapi_readKey");
}

void showKeyCodes(void)
{
  int res;
  brlapi_keyCode_t cmd;
  char buf[0X100];

  fprintf(stderr,"Entering keycode learn mode\n");
  if (brlapi_getDriverName(buf, sizeof(buf))==-1) {
    brlapi_perror("getDriverName");
    return;
  }
  if (brlapi_enterTtyMode(-1, buf)<0) {
    brlapi_perror("enterTtyMode");
    return;
  }

  if (brlapi_acceptAllKeys()==-1) {
    brlapi_perror("acceptAllKeys");
    return;
  }

  if (brlapi_writeText(BRLAPI_CURSOR_OFF, "show key codes")<0) {
    brlapi_perror("brlapi_writeText");
    exit(1);
  }

  while ((res = brlapi_readKey(1, &cmd)) != -1) {
    sprintf(buf, "0X%" BRLAPI_PRIxKEYCODE " (%" BRLAPI_PRIuKEYCODE ")",cmd, cmd);
    brlapi_writeText(BRLAPI_CURSOR_OFF, buf);
    fprintf(stderr, "%s\n", buf);
  }
  brlapi_perror("brlapi_readKey");
}

#ifdef SIGUSR1
void emptySignalHandler(int sig) { }
#endif /* SIGUSR1 */

void suspendDriver(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
  fprintf(stderr, "Suspending\n");
  if (brlapi_suspendDriver(name)) {
    brlapi_perror("suspend");
  } else {
#ifdef SIGUSR1
    signal(SIGUSR1,emptySignalHandler);
#endif /* SIGUSR1 */
    fprintf(stderr, "Sleeping\n");
#ifdef HAVE_PAUSE
    pause();
#endif /* HAVE_PAUSE */
    fprintf(stderr, "Resuming\n");
#ifdef SIGUSR1
    signal(SIGUSR1,SIG_DFL);
#endif /* SIGUSR1 */
    if (brlapi_resumeDriver())
      brlapi_perror("resumeDriver");
  }
}

int main(int argc, char *argv[])
{
  int status = 0;
  brlapi_fileDescriptor fd;
  settings.host = NULL; settings.auth = NULL;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "apitest"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  fprintf(stderr, "Connecting to BrlAPI... ");
  if ((fd=brlapi_openConnection(&settings, &settings)) != (brlapi_fileDescriptor)(-1)) {
    fprintf(stderr, "done (fd=%"PRIFD")\n", fd);
    fprintf(stderr,"Connected to %s using auth %s\n", settings.host, settings.auth);

    if (opt_showName) {
      showDriverName();
    }

    if (opt_showSize) {
      showDisplaySize();
    }

    if (opt_showDots) {
      showDots();
    }

    if (opt_learnMode) {
      enterLearnMode();
    }

    if (opt_showKeyCodes) {
      showKeyCodes();
    }

    if (opt_suspendMode) {
      suspendDriver();
    }

    brlapi_closeConnection();
    fprintf(stderr, "Disconnected\n"); 
  } else {
    fprintf(stderr, "failed to connect to %s using auth %s",settings.host, settings.auth);
    brlapi_perror("");
    status = 1;
  }
  return status;
}
