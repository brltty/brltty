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
 * brl_load.c - handling of dynamic drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#include "brl.h"
#include "misc.h"

braille_driver *braille = NULL;	/* filled by dynamic libs */
char* braille_libname = NULL;	/* name of library */
static void* library = NULL;	/* handle to driver */
#define BRL_SYMBOL "brl_driver"

/* load driver from library */
/* return true (nonzero) on success */
int load_braille_driver(void)
{
  const char* error;

  if (braille_libname == NULL)
    {
      #ifdef BRL_BUILTIN
        extern braille_driver brl_driver;
	braille = &brl_driver;
	braille_libname = "built-in";
	return 1;
      #else
        return 0;
      #endif
    }

  /* allow shortcuts */
  if (strlen(braille_libname) == 2)
    {
      static char name[] = "libbrlttyb??.so.1"; /* two ? for shortcut */
      char * pos = strchr(name, '?');
      pos[0] = braille_libname[0];
      pos[1] = braille_libname[1];
      braille_libname = name;
    }

  library = dlopen(braille_libname, RTLD_NOW);
  if (library == NULL) 
    {
      LogAndStderr(LOG_ERR, "could not open library %s", braille_libname);
      LogAndStderr(LOG_ERR, dlerror()); 
      return 0;
    }

  braille = dlsym(library, BRL_SYMBOL); /* locate struct driver - filled with all the data */
  error = dlerror();
  if (error)
    {
      LogAndStderr(LOG_ERR, "symbol not found: %s: %s", BRL_SYMBOL, error);
      exit(10);
    }

  return 1;
}


int list_braille_driver(void)
{
	char buf[64];
	static const char *list_file = LIB_PATH "/brl_drivers.lst";
	int cnt, fd = open( list_file, O_RDONLY );
	if (fd < 0) {
		fprintf( stderr, "Error: can't access driver list file\n" );
		perror( list_file );
		return 0;
	}
	fprintf( stderr, "Available Braille Drivers:\n\n" );
	fprintf( stderr, "XX\tDescription\n" );
	fprintf( stderr, "--\t-----------\n" );
	while( (cnt = read( fd, buf, sizeof(buf) )) )
		fwrite( buf, cnt, 1, stderr );
	close(fd);
	return 1;
}


