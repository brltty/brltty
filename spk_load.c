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
 * spk_load.c - handling of dynamic drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "brl.h"
#include "misc.h"
#include "spk.h"

speech_driver *speech = NULL;	/* filled by dynamic libs */
char* speech_libname = NULL;	/* name of library */
static void* library = NULL;	/* handle to driver */
#define SPK_SYMBOL "spk_driver"

/* load driver from libray */
/* return true (nonzero) on success */
int load_speech_driver(void)
{
  const char* error;

  if (speech_libname == NULL)
    {
      #ifdef SPK_BUILTIN
        extern speech_driver spk_driver;
	speech = &spk_driver;
	speech_libname = "built-in";
	return 1;
      #else
        return 0;
      #endif
    }

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
      LogAndStderr(LOG_ERR, "could not open library %s: %s",
		   speech_libname, dlerror()); 
      exit(10);
    }

  speech = dlsym(library, SPK_SYMBOL); /* locate struct driver - filled with all the data */
  error = dlerror();
  if (error)
    {
      LogAndStderr(LOG_ERR, "symbol not found: %s: %s", SPK_SYMBOL, error);
      exit(10);
    }

  return 1;
}
