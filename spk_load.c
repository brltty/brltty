/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * spk_load.c - handling of dynamic speech drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#include "brl.h"
#include "misc.h"
#include "spk.h"

speech_driver *speech = NULL;	/* filled by dynamic libs */
char *speech_libraryName = NULL;	/* name of library */
static void *library = NULL;	/* handle to driver */
#define SPK_SYMBOL "spk_driver"

/* load driver from library */
/* return true (nonzero) on success */
int load_speech_driver(void)
{
  const char* error;

  #ifdef SPK_BUILTIN
    extern speech_driver spk_driver;
    if (speech_libraryName != NULL)
      if (strcmp(speech_libraryName, spk_driver.identifier) == 0)
        speech_libraryName = NULL;
    if (speech_libraryName == NULL)
      {
	speech = &spk_driver;
	speech_libraryName = "built-in";
	return 1;
      }
  #else
#error "No built-in speech driver: please provide at least NoSpeech driver!"
  #endif

  /* allow shortcuts */
  if (strlen(speech_libraryName) == 2)
    {
      static char name[] = "libbrlttys??.so.1"; /* two ? for shortcut */
      char * pos = strchr(name, '?');
      pos[0] = speech_libraryName[0];
      pos[1] = speech_libraryName[1];
      speech_libraryName = name;
    }

  library = dlopen(speech_libraryName, RTLD_NOW);
  if (library == NULL) 
    {
      LogPrint(LOG_ERR, "%s", dlerror());
      LogPrint(LOG_ERR, "Cannot open speech driver library: %s", speech_libraryName);
      goto liberror;
    }

  speech = dlsym(library, SPK_SYMBOL); /* locate struct driver - filled with all the data */
  error = dlerror();
  if (error)
    {
      LogPrint(LOG_ERR, "%s", error);
      LogPrint(LOG_ERR, "Speech driver symbol not found: %s", SPK_SYMBOL);
      goto liberror;
    }

  return 1;

 liberror:
  /* fall back to built-in */
  speech = &spk_driver;
  speech_libraryName = "built-in";
  return 0;
}


int list_speech_drivers(void)
{
	char buf[64];
	static const char *list_file = LIB_PATH "/brltty-spk.lst";
	int cnt, fd = open( list_file, O_RDONLY );
	if (fd < 0) {
		fprintf( stderr, "Error: can't access speech driver list file\n" );
		perror( list_file );
		return 0;
	}
	fprintf( stderr, "Available Speech Drivers:\n\n" );
	fprintf( stderr, "XX\tDescription\n" );
	fprintf( stderr, "--\t-----------\n" );
	while( (cnt = read( fd, buf, sizeof(buf) )) )
		fwrite( buf, cnt, 1, stderr );
	close(fd);
	return 1;
}


