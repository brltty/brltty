/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
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
char* speech_libname = NULL;	/* name of library */
char* speech_drvparm = NULL;	/* arbitrary init parameter */
static void* library = NULL;	/* handle to driver */
#define SPK_SYMBOL "spk_driver"

/* load driver from library */
/* return true (nonzero) on success */
int load_speech_driver(void)
{
  const char* error;

  #ifdef SPK_BUILTIN
    extern speech_driver spk_driver;
    if (speech_libname != NULL)
      if (strcmp(speech_libname, spk_driver.identifier) == 0)
        speech_libname = NULL;
    if (speech_libname == NULL)
      {
	speech = &spk_driver;
	speech_libname = "built-in";
	return 1;
      }
  #else
    if (speech_libname == NULL)
      return 0;
  #endif

  /* allow shortcuts */
  if (strlen(speech_libname) == 2)
    {
      static char name[] = "libbrlttys??.so.1"; /* two ? for shortcut */
      char * pos = strchr(name, '?');
      pos[0] = speech_libname[0];
      pos[1] = speech_libname[1];
      speech_libname = name;
    }

  library = dlopen(speech_libname, RTLD_NOW);
  if (library == NULL) 
    {
      LogAndStderr(LOG_ERR, "%s", dlerror());
      LogAndStderr(LOG_ERR, "Cannot open speech driver library: %s", speech_libname);
      return 0;
    }

  speech = dlsym(library, SPK_SYMBOL); /* locate struct driver - filled with all the data */
  error = dlerror();
  if (error)
    {
      LogAndStderr(LOG_ERR, "%s", error);
      LogAndStderr(LOG_ERR, "Speech driver symbol not found: %s", SPK_SYMBOL);
      exit(10);
    }

  return 1;
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


